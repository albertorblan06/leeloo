#include "tasks.h"

#include <Arduino.h>
#include <esp_task_wdt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <new>

#include "app_adapters.h"
#include "display_ui.h"
#include "input_service.h"
#include "messages.h"
#include "net_service.h"
#include "pins.h"
#include "rsvp_service.h"
#include "rtos_freertos.h"
#include "storage_books.h"
#include "ui_service.h"
#include "upload_server.h"
#include "view_model.h"

namespace leeloo::tasks {
namespace {

using app::NetMsg;
using app::RawEdge;
using app::RsvpMsg;
using ui::Event;

// --- Task set: priorities and cores (docs/ARCHITECTURE.md §4.1) ----------------
//
// Core 1 runs what the reader sees; core 0 runs the radio. A higher number preempts a lower one.
//   RSVP    5  word timing is the deadline that matters most
//   Input   4  a press must be classified promptly, but it only posts a message
//   UI      3  menu logic, storage calls
//   Display 2  slow (I2C dump), nobody waits for it
//   Net     2  Wi-Fi bring-up blocks for seconds; core 0 also runs the Wi-Fi and HTTP tasks
constexpr UBaseType_t kRsvpPriority = 5;
constexpr UBaseType_t kInputPriority = 4;
constexpr UBaseType_t kUiPriority = 3;
constexpr UBaseType_t kDisplayPriority = 2;
constexpr UBaseType_t kNetPriority = 2;

constexpr BaseType_t kUiCore = 1;
constexpr BaseType_t kNetCore = 0;

// Stack sizes in bytes. Generous on purpose: this work ran on the 8 KiB Arduino loop task before.
// The high-water marks measured on the device (milestone 4) settle them.
constexpr std::uint32_t kInputStack = 4096;
constexpr std::uint32_t kRsvpStack = 6144;
constexpr std::uint32_t kUiStack = 6144;
constexpr std::uint32_t kDisplayStack = 6144;
constexpr std::uint32_t kNetStack = 8192;

// Every task wakes at least this often to feed the watchdog, even with nothing to do.
constexpr std::uint32_t kHeartbeatMs = 1000;
// Longer than the 5 s default: starting the soft AP can block the net task for seconds.
constexpr std::uint32_t kWatchdogTimeoutS = 10;

constexpr std::size_t kEdgeQueueLength = 16;
constexpr std::size_t kUiQueueLength = 16;
constexpr std::size_t kRsvpQueueLength = 8;
constexpr std::size_t kNetQueueLength = 8;

static_assert(pins::kButtonCount == app::kButtonCount, "one pin per button");

// Everything the tasks share. Built once in start() and never freed.
struct App {
  // Queues: edges (ISR -> input), UI events (everyone -> UI), commands to RSVP and net.
  rtos::freertos::Queue edge_port{kEdgeQueueLength, sizeof(RawEdge)};
  rtos::freertos::Queue ui_port{kUiQueueLength, sizeof(Event)};
  rtos::freertos::Queue rsvp_port{kRsvpQueueLength, sizeof(RsvpMsg)};
  rtos::freertos::Queue net_port{kNetQueueLength, sizeof(NetMsg)};
  rtos::Queue<RawEdge> edges{&edge_port};
  rtos::Queue<Event> ui_events{&ui_port};
  rtos::Queue<RsvpMsg> rsvp_cmds{&rsvp_port};
  rtos::Queue<NetMsg> net_cmds{&net_port};

  // What the display draws: written by UI, RSVP and net, read by the display task.
  rtos::freertos::Mutex view_mutex;
  rtos::freertos::TaskNotifier display_wake;
  app::ViewModel view{&view_mutex, &display_wake};

  adapters::SerialLog log;
  adapters::GpioPins pin_reader;
  adapters::StorageLibrary library;
  adapters::NvsSettings settings_store;
  adapters::DisplayContrast contrast{&view};
  adapters::SoftAp ap;

  app::InputService input{&pin_reader, &ui_events, &log};
  app::RsvpService rsvp{&library, &ui_events, display::piece_count, &log};
  app::NetService net{&ap, &ui_events, &log};
  app::UiService ui;

  explicit App(const settings::Settings& boot)
      : ui(boot, &library, &settings_store, &contrast, &rsvp_cmds, &net_cmds, &log) {}

  bool ok() const {
    return edge_port.ok() && ui_port.ok() && rsvp_port.ok() && net_port.ok() && view_mutex.ok();
  }
};

App* g_app = nullptr;

[[noreturn]] void halt(const char* reason) {
  Serial.println(reason);
  for (;;) delay(1000);
}

std::uint32_t capped(std::uint32_t wait_ms) { return std::min(wait_ms, kHeartbeatMs); }

// Called first thing in each task: from then on it must call feed() at least every few seconds.
void supervise() {
  if (esp_task_wdt_add(xTaskGetCurrentTaskHandle()) != ESP_OK) g_app->log.log("WDT: no se pudo vigilar la tarea");
}
void feed() { esp_task_wdt_reset(); }

// --- Interrupt: only queue the raw edge, never debounce here -------------------

// One ISR serves all three buttons; the argument is the button index.
void IRAM_ATTR on_button_edge(void* arg) {
  const auto button = static_cast<std::uint8_t>(reinterpret_cast<std::uintptr_t>(arg));
  RawEdge edge{button, digitalRead(pins::kButtonPins[button]) == HIGH, millis()};
  BaseType_t woken = pdFALSE;
  xQueueSendFromISR(static_cast<QueueHandle_t>(g_app->edge_port.native_handle()), &edge, &woken);
  if (woken == pdTRUE) portYIELD_FROM_ISR();
}

// Called from the HTTP task (core 0) after a book is stored.
void on_book_saved() {
  if (!g_app->ui_events.send(Event::UploadSaved, app::kPostTimeoutMs)) {
    g_app->log.log("Cola de UI llena: se pierde UploadSaved");
  }
}

// --- Task bodies: wait for a message or a deadline, hand it to the service ------

void input_task(void*) {
  supervise();
  for (;;) {
    feed();
    RawEdge edge;
    if (g_app->edges.receive(&edge, capped(g_app->input.wait_ms()))) {
      g_app->input.on_edge(edge);
    } else {
      g_app->input.on_timeout(millis());
    }
  }
}

void ui_task(void*) {
  supervise();
  g_app->ui.sync_books();  // the library may already hold books from an earlier session
  g_app->view.publish_ui(g_app->ui.model());
  for (;;) {
    feed();
    Event event;
    if (!g_app->ui_events.receive(&event, kHeartbeatMs)) continue;
    g_app->ui.handle(event);
    g_app->view.publish_ui(g_app->ui.model());
  }
}

void rsvp_task(void*) {
  supervise();
  for (;;) {
    feed();
    RsvpMsg msg;
    bool changed;
    if (g_app->rsvp_cmds.receive(&msg, capped(g_app->rsvp.wait_ms(millis())))) {
      changed = g_app->rsvp.handle(msg, millis());
    } else {
      changed = g_app->rsvp.on_timeout(millis());
    }
    if (changed) g_app->view.publish_frame(g_app->rsvp.frame());
  }
}

void net_task(void*) {
  supervise();
  app::NetStatus shown;
  bool published = false;
  for (;;) {
    feed();
    NetMsg msg;
    if (g_app->net_cmds.receive(&msg, capped(g_app->net.wait_ms(millis())))) {
      g_app->net.handle(msg, millis());  // may block for seconds while the AP comes up
    } else {
      g_app->net.on_timeout(millis());
    }
    const app::NetStatus now = g_app->ap.status();
    if (!published || now.running != shown.running || std::strcmp(now.pin, shown.pin) != 0) {
      g_app->view.publish_net(now);
      shown = now;
      published = true;
    }
  }
}

// The only task that touches the I2C bus and the frame buffer.
void display_task(void*) {
  supervise();
  app::Snapshot snap;
  std::uint32_t seen = 0;
  for (;;) {
    feed();
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(kHeartbeatMs));
    if (!g_app->view.read_if_new(&snap, &seen)) continue;
    if (snap.contrast_pending) display::set_contrast(snap.contrast);
    display::Status status;
    status.storage_ok = storage::mounted();
    if (snap.ui.screen() == ui::Screen::About) status.free_bytes = storage::free_bytes();
    status.ap_running = snap.net.running;
    status.pin = snap.net.pin;
    display::render(snap.ui, status, snap.frame);
  }
}

TaskHandle_t spawn(TaskFunction_t body, const char* name, std::uint32_t stack, UBaseType_t priority,
                   BaseType_t core) {
  TaskHandle_t handle = nullptr;
  if (xTaskCreatePinnedToCore(body, name, stack, nullptr, priority, &handle, core) != pdPASS) {
    Serial.printf("No se pudo crear la tarea %s\n", name);
    halt("Sin memoria para las tareas; detenido.");
  }
  return handle;
}

}  // namespace

void start(const settings::Settings& boot) {
  g_app = new (std::nothrow) App(boot);
  if (g_app == nullptr || !g_app->ok()) halt("Sin memoria para colas y servicios; detenido.");

  esp_task_wdt_init(kWatchdogTimeoutS, true);

  // The display task first, so every producer finds a task to wake.
  g_app->display_wake.set_task(spawn(display_task, "display", kDisplayStack, kDisplayPriority, kUiCore));
  spawn(ui_task, "ui", kUiStack, kUiPriority, kUiCore);
  spawn(rsvp_task, "rsvp", kRsvpStack, kRsvpPriority, kUiCore);
  spawn(net_task, "net", kNetStack, kNetPriority, kNetCore);
  spawn(input_task, "input", kInputStack, kInputPriority, kUiCore);

  upload_server::set_on_saved(on_book_saved);
  g_app->net_cmds.send(NetMsg{app::NetCmd::Boot, boot.wifi, false}, app::kPostTimeoutMs);

  for (const std::uint8_t pin : pins::kButtonPins) pinMode(pin, INPUT_PULLUP);
  for (std::uint8_t b = 0; b < pins::kButtonCount; ++b) {
    attachInterruptArg(digitalPinToInterrupt(pins::kButtonPins[b]), on_button_edge,
                       reinterpret_cast<void*>(static_cast<std::uintptr_t>(b)), CHANGE);
  }
}

}  // namespace leeloo::tasks
