#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include <cstddef>
#include <cstdint>

#include "buttons.h"
#include "joystick.h"
#include "menu.h"

// Temporary joystick test (replaces the EC11). Hardware-bound: covered by
// docs/hardware-checklists/joystick-input.md. Pure logic lives in lib/.

namespace {

using leeloo::buttons::Debouncer;
using leeloo::buttons::Edge;
using leeloo::buttons::Press;
using leeloo::joystick::Axis;
using leeloo::joystick::Direction;

constexpr int kSdaPin = 8;
constexpr int kSclPin = 9;
// VRy on GPIO4 and VRx on GPIO5 (ADC1, so they keep working with Wi-Fi on);
// SW on GPIO7 to GND with the internal pull-up.
constexpr std::uint8_t kAxisYPin = 4;
constexpr std::uint8_t kAxisXPin = 5;
constexpr std::uint8_t kSwitchPin = 7;
constexpr std::uint32_t kAxisSamplePeriodMs = 10;
constexpr int kCalibrationSamples = 16;

enum Button : std::uint8_t { kUp = 0, kDown = 1, kLeft = 2, kRight = 3, kSelect = 4 };
constexpr std::size_t kButtonCount = 5;
constexpr const char* kButtonNames[kButtonCount] = {"ARRIBA", "ABAJO", "IZQUIERDA", "DERECHA", "SELECT"};

constexpr std::size_t kEdgeQueueLength = 16;
constexpr std::size_t kUiQueueLength = 16;

struct RawEdge {
  bool high;
  std::uint32_t ms;
};

struct ButtonEvent {
  Button button;
  Press kind;
};

enum class Screen : std::uint8_t { Menu, Buttons, Counter, About };

QueueHandle_t g_edge_queue = nullptr;
QueueHandle_t g_ui_queue = nullptr;

// U8g2 is the SH1106 likely controller; see docs/ARCHITECTURE.md.
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
bool g_display_ready = false;

// --- Interrupts: only queue the raw edge, never debounce here -----------------

void IRAM_ATTR on_switch_edge() {
  RawEdge edge{digitalRead(kSwitchPin) == HIGH, millis()};
  BaseType_t woken = pdFALSE;
  xQueueSendFromISR(g_edge_queue, &edge, &woken);
  if (woken == pdTRUE) portYIELD_FROM_ISR();
}

// --- Switch task: debounce, classify, forward ---------------------------------

void handle_switch_level(bool high, std::uint32_t now, Debouncer* debouncer, std::uint32_t* pressed_at) {
  const Edge edge = debouncer->update(high, now);
  if (edge == Edge::Pressed) {
    *pressed_at = now;
  } else if (edge == Edge::Released) {
    const Press kind = leeloo::buttons::classify(leeloo::buttons::elapsed_ms(*pressed_at, now));
    const ButtonEvent event{kSelect, kind};
    xQueueSend(g_ui_queue, &event, 0);
  }
}

// Blocks until an edge arrives. After an edge it waits one debounce window; if
// no new edge came, it samples the pin so the debouncer can confirm the level,
// repeating while the change is pending. Idle = no timer, no polling.
void switch_task(void*) {
  Debouncer debouncer;
  std::uint32_t pressed_at = 0;
  bool settling = false;
  for (;;) {
    RawEdge edge;
    const TickType_t wait = settling ? pdMS_TO_TICKS(leeloo::buttons::kDebounceMs) : portMAX_DELAY;
    if (xQueueReceive(g_edge_queue, &edge, wait) == pdTRUE) {
      settling = true;
      handle_switch_level(edge.high, edge.ms, &debouncer, &pressed_at);
      continue;
    }
    handle_switch_level(digitalRead(kSwitchPin) == HIGH, millis(), &debouncer, &pressed_at);
    // The timeout can fire a tick before the debounce window is over; keep
    // sampling until the change is confirmed or rejected, otherwise the press is lost.
    settling = debouncer.pending();
  }
}

// --- Axis task: the ADC has no interrupt, so this one samples ------------------
// shortcut: fixed 10 ms sampling and no auto-repeat while held, upgrade when
// the menu needs scrolling long lists.

int read_axis_center(std::uint8_t pin) {
  long sum = 0;
  for (int i = 0; i < kCalibrationSamples; ++i) {
    sum += analogRead(pin);
    delay(5);
  }
  return static_cast<int>(sum / kCalibrationSamples);
}

void send_direction(Direction direction, Button low, Button high) {
  if (direction == Direction::None) return;
  const ButtonEvent event{direction == Direction::Low ? low : high, Press::Short};
  xQueueSend(g_ui_queue, &event, 0);
}

void axis_task(void*) {
  // The stick is assumed untouched at boot; its rest position is the center.
  const int center_y = read_axis_center(kAxisYPin);
  const int center_x = read_axis_center(kAxisXPin);
  Serial.printf("Joystick centers: Y=%d X=%d\n", center_y, center_x);
  Axis axis_y(center_y);
  Axis axis_x(center_x);
  for (;;) {
    send_direction(axis_y.update(analogRead(kAxisYPin)), kUp, kDown);
    send_direction(axis_x.update(analogRead(kAxisXPin)), kLeft, kRight);
    vTaskDelay(pdMS_TO_TICKS(kAxisSamplePeriodMs));
  }
}

// --- UI state -----------------------------------------------------------------

struct Ui {
  Screen screen = Screen::Menu;
  leeloo::menu::Menu menu{3};
  std::uint32_t presses[kButtonCount] = {};
  bool has_last = false;
  ButtonEvent last{kUp, Press::Short};
  int counter = 0;
};

void open_screen(Ui* ui, Screen screen) {
  ui->screen = screen;
  if (screen == Screen::Buttons) ui->has_last = false;
}

// SELECT long press always returns to the main menu from a sub-screen.
void apply_event(Ui* ui, const ButtonEvent& event) {
  const bool long_select = event.button == kSelect && event.kind == Press::Long;
  switch (ui->screen) {
    case Screen::Menu:
      if (event.button == kUp) ui->menu.up();
      if (event.button == kDown) ui->menu.down();
      if (event.button == kSelect && event.kind == Press::Short) {
        const auto item = ui->menu.select();
        if (item) open_screen(ui, static_cast<Screen>(*item + 1));
      }
      break;
    case Screen::Buttons:
      if (long_select) {
        open_screen(ui, Screen::Menu);
        break;
      }
      ui->last = event;
      ui->has_last = true;
      ++ui->presses[event.button];
      break;
    case Screen::Counter:
      if (long_select) {
        open_screen(ui, Screen::Menu);
        break;
      }
      if (event.button == kSelect) ++ui->counter;
      if (event.button == kUp) ++ui->counter;
      if (event.button == kDown) --ui->counter;
      break;
    case Screen::About:
      open_screen(ui, Screen::Menu);
      break;
  }
}

// --- Drawing -----------------------------------------------------------------

void draw_header(const char* title) {
  u8g2.setFont(u8g2_font_helvB10_tf);
  u8g2.drawUTF8(0, 11, title);
  u8g2.drawHLine(0, 14, 128);
}

void draw_menu(const Ui& ui) {
  static const char* const kItems[] = {"Probar joystick", "Contador", "Acerca de"};
  draw_header("Menu de prueba");
  u8g2.setFont(u8g2_font_helvR10_tf);
  for (std::size_t i = 0; i < 3; ++i) {
    const int y = 28 + static_cast<int>(i) * 13;
    const bool selected = i == ui.menu.index();
    if (selected) u8g2.drawBox(0, y - 10, 128, 12);
    u8g2.setDrawColor(selected ? 0 : 1);
    u8g2.drawUTF8(4, y, kItems[i]);
    u8g2.setDrawColor(1);
  }
}

void draw_buttons(const Ui& ui) {
  draw_header("Probar joystick");
  u8g2.setFont(u8g2_font_helvR10_tf);
  char line[32];
  if (ui.has_last) {
    snprintf(line, sizeof(line), "%s %s", kButtonNames[ui.last.button],
             ui.last.kind == Press::Long ? "largo" : "corto");
  } else {
    snprintf(line, sizeof(line), "Mueve o pulsa");
  }
  u8g2.drawUTF8(0, 27, line);
  const auto n = [&ui](Button b) { return static_cast<unsigned long>(ui.presses[b]); };
  snprintf(line, sizeof(line), "A:%lu B:%lu S:%lu", n(kUp), n(kDown), n(kSelect));
  u8g2.drawUTF8(0, 40, line);
  snprintf(line, sizeof(line), "I:%lu D:%lu", n(kLeft), n(kRight));
  u8g2.drawUTF8(0, 53, line);
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawUTF8(0, 63, "SELECT largo: volver");
}

void draw_counter(const Ui& ui) {
  draw_header("Contador");
  char value[16];
  snprintf(value, sizeof(value), "%d", ui.counter);
  u8g2.setFont(u8g2_font_helvB14_tf);
  u8g2.drawUTF8(0, 40, value);
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawUTF8(0, 60, "UP+ DOWN- SEL+ (largo: volver)");
}

void draw_about() {
  draw_header("Acerca de");
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawUTF8(0, 28, "Leeloo: prueba joystick");
  u8g2.drawUTF8(0, 40, "Y:4 X:5 SW:7");
  u8g2.drawUTF8(0, 60, "Build " __DATE__);
}

void draw(const Ui& ui) {
  if (!g_display_ready) return;
  u8g2.clearBuffer();
  switch (ui.screen) {
    case Screen::Menu: draw_menu(ui); break;
    case Screen::Buttons: draw_buttons(ui); break;
    case Screen::Counter: draw_counter(ui); break;
    case Screen::About: draw_about(); break;
  }
  u8g2.sendBuffer();
}

bool i2c_device_present(std::uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

// Returns 0x3C or 0x3D when the OLED answers, otherwise 0.
std::uint8_t find_oled() {
  for (std::uint8_t address : {std::uint8_t{0x3C}, std::uint8_t{0x3D}}) {
    if (i2c_device_present(address)) return address;
  }
  return 0;
}

void init_display() {
  Wire.begin(kSdaPin, kSclPin);
  const std::uint8_t address = find_oled();
  if (address == 0) {
    Serial.println("No OLED at 0x3C/0x3D: input still reports on serial.");
    return;
  }
  u8g2.setI2CAddress(address << 1);  // U8g2 wants the 8-bit form
  u8g2.begin();
  u8g2.setBusClock(400000);
  u8g2.enableUTF8Print();
  g_display_ready = true;
}

}  // namespace

void setup() {
  Serial.begin(115200);
  init_display();

  g_edge_queue = xQueueCreate(kEdgeQueueLength, sizeof(RawEdge));
  g_ui_queue = xQueueCreate(kUiQueueLength, sizeof(ButtonEvent));
  if (g_edge_queue == nullptr || g_ui_queue == nullptr) {
    Serial.println("Queue allocation failed; halting.");
    for (;;) delay(1000);
  }
  pinMode(kSwitchPin, INPUT_PULLUP);
  analogReadResolution(12);
  xTaskCreate(switch_task, "switch", 4096, nullptr, 2, nullptr);
  xTaskCreate(axis_task, "axis", 4096, nullptr, 2, nullptr);
  attachInterrupt(digitalPinToInterrupt(kSwitchPin), on_switch_edge, CHANGE);
}

void loop() {
  static Ui ui;
  static bool drawn = false;
  if (!drawn) {
    draw(ui);
    drawn = true;
  }
  ButtonEvent event;
  if (xQueueReceive(g_ui_queue, &event, portMAX_DELAY) != pdTRUE) return;
  Serial.printf("%s %s\n", kButtonNames[event.button], event.kind == Press::Long ? "largo" : "corto");
  apply_event(&ui, event);
  draw(ui);
}
