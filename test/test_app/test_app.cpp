#include <unity.h>

#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

#include "fakes.h"
#include "input_service.h"
#include "net_service.h"
#include "rsvp_service.h"
#include "text.h"
#include "ui_service.h"
#include "view_model.h"

using namespace leeloo::app;
using leeloo::rtos::kWaitForever;
using leeloo::rtos::Queue;
using leeloo::rtos::fakes::FakeQueue;
using leeloo::settings::WifiMode;
using leeloo::ui::Event;
namespace reader = leeloo::reader;

void setUp() {}
void tearDown() {}

static_assert(std::is_trivially_copyable<RawEdge>::value, "queued as bytes");
static_assert(std::is_trivially_copyable<RsvpMsg>::value, "queued as bytes");
static_assert(std::is_trivially_copyable<NetMsg>::value, "queued as bytes");
static_assert(std::is_trivially_copyable<ReadingFrame>::value, "published as bytes");

namespace {

// --- Fakes for the ports ------------------------------------------------------

struct Log : ILog {
  std::vector<std::string> lines;
  void log(const char* line) override { lines.emplace_back(line); }
  bool has(const std::string& part) const {
    for (const auto& l : lines) {
      if (l.find(part) != std::string::npos) return true;
    }
    return false;
  }
};

struct Pins : IPins {
  bool level[kButtonCount] = {true, true, true};
  bool read_high(std::uint8_t button) override { return level[button]; }
};

std::string book(const std::string& body) { return "TITLE: Libro\n###START###\n" + body; }

struct Library : ILibrary {
  std::size_t count = 2;
  std::string title = "libro";
  std::string text = book("uno dos tres");
  bool open_ok = true;
  bool remove_ok = true;
  bool save_ok = true;
  bool has_saved = false;
  std::size_t saved_offset = 0;
  std::vector<std::size_t> saves;
  int opens = 0;
  int closes = 0;
  long removed = -1;

  std::size_t book_count() override { return count; }
  bool remove_at(std::size_t index) override {
    if (!remove_ok) return false;
    removed = static_cast<long>(index);
    if (count > 0) --count;
    return true;
  }
  bool open(std::size_t, char* out_title, std::size_t cap, BookBuffer* out) override {
    ++opens;
    if (!open_ok) return false;
    std::snprintf(out_title, cap, "%s", title.c_str());
    out->data = text.c_str();
    out->size = text.size();
    return true;
  }
  void close() override { ++closes; }
  bool load_position(const char*, std::size_t* offset) override {
    if (!has_saved) return false;
    *offset = saved_offset;
    return true;
  }
  bool save_position(const char*, std::size_t offset) override {
    if (!save_ok) return false;
    saves.push_back(offset);
    return true;
  }
};

struct SettingsStore : ISettingsStore {
  int saves = 0;
  bool ok = true;
  leeloo::settings::Settings last;
  bool save(const leeloo::settings::Settings& s) override {
    ++saves;
    last = s;
    return ok;
  }
};

struct DisplayControl : IDisplayControl {
  int calls = 0;
  std::uint8_t last = 0;
  void set_contrast(std::uint8_t reg) override {
    ++calls;
    last = reg;
  }
};

struct Net : INet {
  bool is_running = false;
  bool start_ok = true;
  int starts = 0;
  int stops = 0;
  int marks = 0;
  std::uint32_t last_activity = 0;
  bool running() override { return is_running; }
  bool start() override {
    ++starts;
    if (!start_ok) return false;
    is_running = true;
    return true;
  }
  void stop() override {
    ++stops;
    is_running = false;
  }
  void mark_activity() override { ++marks; }
  std::uint32_t last_activity_ms() override { return last_activity; }
};

// A queue pair: the producer side as the service sees it, plus a way to read what arrived.
template <typename T>
struct Chan {
  FakeQueue port;
  Queue<T> q{&port};
  explicit Chan(std::size_t capacity = 8) : port(capacity, sizeof(T)) {}
  bool take(T* out) { return q.receive(out, 0); }
  bool empty() { return port.size() == 0; }
};

}  // namespace

// --- InputService ---------------------------------------------------------------

namespace {

struct InputFixture {
  Chan<Event> ui;
  Pins pins;
  Log log;
  InputService svc;
  explicit InputFixture(std::size_t capacity = 8) : ui(capacity), svc(&pins, &ui.q, &log) {}
};

constexpr std::uint32_t kDebounce = leeloo::buttons::kDebounceMs;

}  // namespace

void test_up_press_posts_up_once_the_level_is_stable() {
  InputFixture fx;
  fx.pins.level[kUp] = false;
  fx.svc.on_edge({kUp, false, 0});
  TEST_ASSERT_EQUAL_UINT32(kDebounce, fx.svc.wait_ms());
  Event e;
  TEST_ASSERT_FALSE(fx.ui.take(&e));  // not yet: the bounce window is open
  fx.svc.on_timeout(kDebounce);
  TEST_ASSERT_TRUE(fx.ui.take(&e));
  TEST_ASSERT_EQUAL((int)Event::Up, (int)e);
  TEST_ASSERT_EQUAL_UINT32(kWaitForever, fx.svc.wait_ms());
}

void test_select_released_before_long_press_posts_select() {
  InputFixture fx;
  fx.pins.level[kSelect] = false;
  fx.svc.on_edge({kSelect, false, 0});
  fx.svc.on_timeout(kDebounce);
  Event e;
  TEST_ASSERT_FALSE(fx.ui.take(&e));  // SELECT acts on release
  fx.pins.level[kSelect] = true;
  fx.svc.on_edge({kSelect, true, 100});
  fx.svc.on_timeout(100 + kDebounce);
  TEST_ASSERT_TRUE(fx.ui.take(&e));
  TEST_ASSERT_EQUAL((int)Event::Select, (int)e);
}

void test_select_held_past_long_press_posts_back() {
  InputFixture fx;
  fx.pins.level[kSelect] = false;
  fx.svc.on_edge({kSelect, false, 0});
  fx.svc.on_timeout(kDebounce);
  fx.pins.level[kSelect] = true;
  fx.svc.on_edge({kSelect, true, 1000});
  fx.svc.on_timeout(1000 + kDebounce);
  Event e;
  TEST_ASSERT_TRUE(fx.ui.take(&e));
  TEST_ASSERT_EQUAL((int)Event::Back, (int)e);
}

void test_bounce_that_returns_to_idle_posts_nothing() {
  InputFixture fx;
  fx.svc.on_edge({kDown, false, 0});
  fx.pins.level[kDown] = true;  // the pin went back before the window ended
  fx.svc.on_timeout(kDebounce);
  TEST_ASSERT_TRUE(fx.ui.empty());
  TEST_ASSERT_EQUAL_UINT32(kWaitForever, fx.svc.wait_ms());
}

void test_ignores_edge_with_button_index_out_of_range() {
  InputFixture fx;
  fx.svc.on_edge({7, false, 0});
  TEST_ASSERT_EQUAL_UINT32(kWaitForever, fx.svc.wait_ms());
  TEST_ASSERT_TRUE(fx.ui.empty());
}

void test_logs_the_drop_when_the_ui_queue_is_full() {
  InputFixture fx(1);
  // Two confirmed UP presses; the first fills the queue, the second finds it full.
  fx.pins.level[kUp] = false;
  fx.svc.on_edge({kUp, false, 0});
  fx.svc.on_timeout(kDebounce);
  fx.pins.level[kUp] = true;
  fx.svc.on_edge({kUp, true, 100});
  fx.svc.on_timeout(100 + kDebounce);
  fx.pins.level[kUp] = false;
  fx.svc.on_edge({kUp, false, 200});
  fx.svc.on_timeout(200 + kDebounce);
  TEST_ASSERT_TRUE(fx.log.has("Cola de UI llena"));
}

// --- RsvpService ----------------------------------------------------------------

namespace {

struct RsvpFixture {
  Chan<Event> ui;
  Library lib;
  Log log;
  RsvpService svc;
  RsvpFixture() : svc(&lib, &ui.q, nullptr, &log) {}
  bool open(std::uint32_t now = 0) { return svc.handle({RsvpCmd::Open, 0}, now); }
  std::string word() { return svc.frame().word; }
};

}  // namespace

void test_open_shows_the_first_word_at_default_speed() {
  RsvpFixture fx;
  TEST_ASSERT_TRUE(fx.open());
  const ReadingFrame f = fx.svc.frame();
  TEST_ASSERT_TRUE(f.active);
  TEST_ASSERT_EQUAL((int)reader::State::Reading, (int)f.state);
  TEST_ASSERT_EQUAL_UINT32(reader::kDefaultWpm, f.wpm);
  TEST_ASSERT_EQUAL_STRING("uno", f.word);
}

void test_open_resumes_at_the_saved_word() {
  RsvpFixture fx;
  fx.lib.has_saved = true;
  fx.lib.saved_offset = 4;  // "dos" in "uno dos tres"
  fx.open();
  TEST_ASSERT_EQUAL_STRING("dos", fx.word().c_str());
}

void test_open_failure_reports_book_open_failed() {
  RsvpFixture fx;
  fx.lib.open_ok = false;
  fx.open();
  Event e;
  TEST_ASSERT_TRUE(fx.ui.take(&e));
  TEST_ASSERT_EQUAL((int)Event::BookOpenFailed, (int)e);
  TEST_ASSERT_FALSE(fx.svc.frame().active);
}

void test_book_without_start_marker_fails_and_is_released() {
  RsvpFixture fx;
  fx.lib.text = "sin cabecera";
  fx.open();
  Event e;
  TEST_ASSERT_TRUE(fx.ui.take(&e));
  TEST_ASSERT_EQUAL((int)Event::BookOpenFailed, (int)e);
  TEST_ASSERT_TRUE(fx.lib.closes >= 1);
  TEST_ASSERT_FALSE(fx.svc.frame().active);
}

void test_book_with_empty_body_finishes_at_once() {
  RsvpFixture fx;
  fx.lib.text = book("");
  fx.open();
  Event e;
  TEST_ASSERT_TRUE(fx.ui.take(&e));
  TEST_ASSERT_EQUAL((int)Event::ReaderFinished, (int)e);
  TEST_ASSERT_FALSE(fx.svc.frame().active);
}

void test_select_pauses_and_saves_then_resumes() {
  RsvpFixture fx;
  fx.open();
  TEST_ASSERT_TRUE(fx.svc.handle({RsvpCmd::Select, 0}, 50));
  TEST_ASSERT_EQUAL((int)reader::State::Paused, (int)fx.svc.frame().state);
  TEST_ASSERT_EQUAL_UINT32(1, fx.lib.saves.size());
  TEST_ASSERT_EQUAL_UINT32(0, fx.lib.saves.back());
  TEST_ASSERT_TRUE(fx.svc.handle({RsvpCmd::Select, 0}, 100));
  TEST_ASSERT_EQUAL((int)reader::State::Reading, (int)fx.svc.frame().state);
}

void test_commands_without_an_open_book_change_nothing() {
  RsvpFixture fx;
  TEST_ASSERT_FALSE(fx.svc.handle({RsvpCmd::Select, 0}, 0));
  TEST_ASSERT_FALSE(fx.svc.handle({RsvpCmd::Up, 0}, 0));
  TEST_ASSERT_FALSE(fx.svc.handle({RsvpCmd::Down, 0}, 0));
}

void test_up_and_down_change_speed_by_one_step_and_show_the_label() {
  RsvpFixture fx;
  fx.lib.text = book("uno dos tres cuatro cinco seis siete ocho nueve diez");
  fx.open();
  fx.svc.handle({RsvpCmd::Up, 0}, 10);
  ReadingFrame f = fx.svc.frame();
  TEST_ASSERT_EQUAL_UINT32(reader::kDefaultWpm + leeloo::rsvp::kWpmStep, f.wpm);
  TEST_ASSERT_TRUE(f.show_wpm);
  fx.svc.handle({RsvpCmd::Down, 0}, 20);
  fx.svc.handle({RsvpCmd::Down, 0}, 30);
  TEST_ASSERT_EQUAL_UINT32(reader::kDefaultWpm - leeloo::rsvp::kWpmStep, fx.svc.frame().wpm);
}

void test_wpm_label_hides_when_its_time_is_up() {
  RsvpFixture fx;
  std::string body;
  for (int i = 0; i < 30; ++i) body += "palabra ";
  fx.lib.text = book(body);
  fx.open();
  fx.svc.handle({RsvpCmd::Up, 0}, 10);
  TEST_ASSERT_TRUE(fx.svc.frame().show_wpm);
  TEST_ASSERT_TRUE(fx.svc.on_timeout(10 + RsvpService::kWpmLabelMs));
  TEST_ASSERT_FALSE(fx.svc.frame().show_wpm);
}

void test_paused_down_jumps_to_next_sentence_and_saves() {
  RsvpFixture fx;
  fx.lib.text = book("Uno dos. Tres cuatro.");
  fx.open();
  fx.svc.handle({RsvpCmd::Select, 0}, 10);  // pause
  fx.svc.handle({RsvpCmd::Down, 0}, 20);
  TEST_ASSERT_EQUAL_STRING("Tres", fx.word().c_str());
  TEST_ASSERT_EQUAL_UINT32(9, fx.lib.saves.back());  // a seek is kept even if power is lost
}

void test_close_saves_the_position_and_releases_the_book() {
  RsvpFixture fx;
  fx.open();
  const int closes_before = fx.lib.closes;
  fx.svc.handle({RsvpCmd::Close, 0}, 10);
  TEST_ASSERT_FALSE(fx.svc.frame().active);
  TEST_ASSERT_EQUAL_UINT32(1, fx.lib.saves.size());
  TEST_ASSERT_TRUE(fx.lib.closes > closes_before);
}

void test_save_failure_is_logged_not_ignored() {
  RsvpFixture fx;
  fx.lib.save_ok = false;
  fx.open();
  fx.svc.handle({RsvpCmd::Select, 0}, 10);
  TEST_ASSERT_TRUE(fx.log.has("no se pudo guardar"));
}

void test_last_word_held_finishes_the_book_and_resets_the_position() {
  RsvpFixture fx;
  fx.lib.text = book("uno");
  fx.open();
  TEST_ASSERT_TRUE(fx.svc.on_timeout(1000));
  Event e;
  TEST_ASSERT_TRUE(fx.ui.take(&e));
  TEST_ASSERT_EQUAL((int)Event::ReaderFinished, (int)e);
  TEST_ASSERT_FALSE(fx.svc.frame().active);
  TEST_ASSERT_EQUAL_UINT32(0, fx.lib.saves.back());  // a finished book restarts from its first word
}

void test_waits_forever_without_a_book_and_until_the_next_word_while_reading() {
  RsvpFixture fx;
  TEST_ASSERT_EQUAL_UINT32(kWaitForever, fx.svc.wait_ms(0));
  fx.lib.text = book("uno dos tres cuatro");
  fx.open(0);
  TEST_ASSERT_EQUAL_UINT32(200, fx.svc.wait_ms(0));  // 300 WPM is 200 ms per word
  TEST_ASSERT_EQUAL_UINT32(150, fx.svc.wait_ms(50));
  TEST_ASSERT_EQUAL_UINT32(0, fx.svc.wait_ms(500));  // overdue: wake at once
}

void test_paused_reader_has_nothing_to_wake_for() {
  RsvpFixture fx;
  fx.open();
  fx.svc.handle({RsvpCmd::Select, 0}, 10);
  TEST_ASSERT_EQUAL_UINT32(kWaitForever, fx.svc.wait_ms(20));
}

void test_frame_cuts_a_long_word_with_an_ellipsis() {
  RsvpFixture fx;
  fx.lib.text = book("anticonstitucionalmente");
  fx.open();
  TEST_ASSERT_EQUAL_STRING("anticonstitucio...", fx.word().c_str());
}

void test_frame_cut_never_splits_a_multibyte_letter() {
  RsvpFixture fx;
  std::string word;
  for (int i = 0; i < 20; ++i) word += "Á";  // 2 bytes each
  fx.lib.text = book(word);
  fx.open();
  const std::string out = fx.word();
  TEST_ASSERT_EQUAL_UINT32(15 * 2 + 3, out.size());
  TEST_ASSERT_TRUE(leeloo::text::valid_utf8(out.data(), out.size()));
  TEST_ASSERT_EQUAL_STRING("...", out.c_str() + out.size() - 3);
}

void test_frame_word_is_empty_for_invalid_utf8() {
  RsvpFixture fx;
  fx.lib.text = book("\xff\xfe");
  fx.open();
  TEST_ASSERT_EQUAL_UINT32(0, fx.word().size());
}

void test_frame_keeps_spanish_accents_and_inverted_marks() {
  RsvpFixture fx;
  fx.lib.text = book("¿Qué ñandú?");
  fx.open();
  TEST_ASSERT_EQUAL_STRING("¿Qué", fx.word().c_str());
}

// --- NetService -----------------------------------------------------------------

namespace {

struct NetFixture {
  Chan<Event> ui;
  Net net;
  Log log;
  NetService svc;
  NetFixture() : svc(&net, &ui.q, &log) {}
  void cmd(NetCmd c, std::uint32_t now, WifiMode wifi = WifiMode::Auto, bool screen = false) {
    svc.handle({c, wifi, screen}, now);
  }
};

constexpr std::uint32_t kLimit = leeloo::upload::kUploadInactivityMs;

}  // namespace

void test_boot_with_wifi_on_starts_the_ap() {
  NetFixture fx;
  fx.cmd(NetCmd::Boot, 0, WifiMode::On);
  TEST_ASSERT_EQUAL(1, fx.net.starts);
  TEST_ASSERT_TRUE(fx.net.is_running);
}

void test_boot_with_automatico_keeps_the_ap_off() {
  NetFixture fx;
  fx.cmd(NetCmd::Boot, 0, WifiMode::Auto);
  TEST_ASSERT_EQUAL(0, fx.net.starts);
}

void test_boot_failure_with_wifi_on_is_logged() {
  NetFixture fx;
  fx.net.start_ok = false;
  fx.cmd(NetCmd::Boot, 0, WifiMode::On);
  TEST_ASSERT_TRUE(fx.log.has("al arrancar"));
}

void test_start_upload_in_automatico_starts_the_ap_and_polls_activity() {
  NetFixture fx;
  fx.cmd(NetCmd::Boot, 0, WifiMode::Auto);
  fx.cmd(NetCmd::StartUpload, 1000);
  TEST_ASSERT_EQUAL(1, fx.net.starts);
  TEST_ASSERT_EQUAL(1, fx.net.marks);  // an old activity time must not close the new session
  TEST_ASSERT_EQUAL_UINT32(NetService::kInactivityPollMs, fx.svc.wait_ms(1000));
}

void test_start_failure_reports_upload_start_failed() {
  NetFixture fx;
  fx.net.start_ok = false;
  fx.cmd(NetCmd::StartUpload, 0);
  Event e;
  TEST_ASSERT_TRUE(fx.ui.take(&e));
  TEST_ASSERT_EQUAL((int)Event::UploadStartFailed, (int)e);
  TEST_ASSERT_EQUAL_UINT32(kWaitForever, fx.svc.wait_ms(0));
}

void test_start_upload_reuses_an_ap_that_is_already_up() {
  NetFixture fx;
  fx.cmd(NetCmd::Boot, 0, WifiMode::On);
  fx.cmd(NetCmd::StartUpload, 10, WifiMode::On);
  TEST_ASSERT_EQUAL(1, fx.net.starts);
}

void test_stop_upload_stops_the_ap_in_automatico() {
  NetFixture fx;
  fx.cmd(NetCmd::Boot, 0, WifiMode::Auto);
  fx.cmd(NetCmd::StartUpload, 0);
  fx.cmd(NetCmd::StopUpload, 10);
  TEST_ASSERT_FALSE(fx.net.is_running);
  TEST_ASSERT_EQUAL_UINT32(kWaitForever, fx.svc.wait_ms(10));
}

void test_stop_upload_keeps_the_ap_when_wifi_is_on() {
  NetFixture fx;
  fx.cmd(NetCmd::Boot, 0, WifiMode::On);
  fx.cmd(NetCmd::StartUpload, 0, WifiMode::On);
  fx.cmd(NetCmd::StopUpload, 10, WifiMode::On);
  TEST_ASSERT_TRUE(fx.net.is_running);
}

void test_inactivity_ends_an_automatico_session_and_tells_the_ui() {
  NetFixture fx;
  fx.cmd(NetCmd::Boot, 0, WifiMode::Auto);
  fx.cmd(NetCmd::StartUpload, 0);
  fx.svc.on_timeout(kLimit - 1);
  TEST_ASSERT_TRUE(fx.net.is_running);
  fx.svc.on_timeout(kLimit);
  TEST_ASSERT_FALSE(fx.net.is_running);
  Event e;
  TEST_ASSERT_TRUE(fx.ui.take(&e));
  TEST_ASSERT_EQUAL((int)Event::UploadTimedOut, (int)e);
}

void test_phone_activity_keeps_the_session_alive() {
  NetFixture fx;
  fx.cmd(NetCmd::Boot, 0, WifiMode::Auto);
  fx.cmd(NetCmd::StartUpload, 0);
  fx.net.last_activity = kLimit - 100;  // the HTTP task saw a request just before the limit
  fx.svc.on_timeout(kLimit);
  TEST_ASSERT_TRUE(fx.net.is_running);
  TEST_ASSERT_TRUE(fx.ui.empty());
}

void test_inactivity_check_survives_a_32_bit_clock_wrap() {
  NetFixture fx;
  fx.cmd(NetCmd::Boot, 0, WifiMode::Auto);
  fx.cmd(NetCmd::StartUpload, UINT32_MAX - 9);
  fx.net.last_activity = UINT32_MAX - 9;
  fx.svc.on_timeout(kLimit - 11);  // now - start wraps but is one tick short of the limit
  TEST_ASSERT_TRUE(fx.net.is_running);
  fx.svc.on_timeout(kLimit - 10);
  TEST_ASSERT_FALSE(fx.net.is_running);
}

void test_save_on_upload_screen_stops_the_ap_after_the_grace_time() {
  NetFixture fx;
  fx.cmd(NetCmd::Boot, 0, WifiMode::Auto);
  fx.cmd(NetCmd::StartUpload, 0);
  fx.cmd(NetCmd::SavedOnUploadScreen, 100);
  fx.svc.on_timeout(100 + NetService::kStopAfterSaveMs - 1);
  TEST_ASSERT_TRUE(fx.net.is_running);  // the phone's 201 is still on its way
  fx.svc.on_timeout(100 + NetService::kStopAfterSaveMs);
  TEST_ASSERT_FALSE(fx.net.is_running);
  TEST_ASSERT_EQUAL_UINT32(kWaitForever, fx.svc.wait_ms(2000));
}

void test_save_with_wifi_on_schedules_no_stop() {
  NetFixture fx;
  fx.cmd(NetCmd::Boot, 0, WifiMode::On);
  fx.cmd(NetCmd::StartUpload, 0, WifiMode::On);
  fx.cmd(NetCmd::SavedOnUploadScreen, 100, WifiMode::On);
  fx.svc.on_timeout(100 + NetService::kStopAfterSaveMs);
  TEST_ASSERT_TRUE(fx.net.is_running);
}

void test_new_upload_cancels_a_deferred_stop() {
  NetFixture fx;
  fx.cmd(NetCmd::Boot, 0, WifiMode::Auto);
  fx.cmd(NetCmd::StartUpload, 0);
  fx.cmd(NetCmd::SavedOnUploadScreen, 100);
  fx.cmd(NetCmd::StartUpload, 200);
  fx.svc.on_timeout(100 + NetService::kStopAfterSaveMs);
  TEST_ASSERT_TRUE(fx.net.is_running);
}

void test_deferred_stop_sets_the_wait_to_the_remaining_time() {
  NetFixture fx;
  fx.cmd(NetCmd::Boot, 0, WifiMode::Auto);
  fx.cmd(NetCmd::StartUpload, 0);
  fx.cmd(NetCmd::SavedOnUploadScreen, 1000);
  // The inactivity poll (500 ms) is shorter than the 1500 ms grace time.
  TEST_ASSERT_EQUAL_UINT32(NetService::kInactivityPollMs, fx.svc.wait_ms(1000));
  fx.svc.on_timeout(1000 + NetService::kStopAfterSaveMs - 100);
  TEST_ASSERT_EQUAL_UINT32(100, fx.svc.wait_ms(1000 + NetService::kStopAfterSaveMs - 100));
}

void test_wifi_set_to_on_starts_the_ap_at_once() {
  NetFixture fx;
  fx.cmd(NetCmd::ApplyWifi, 0, WifiMode::On);
  TEST_ASSERT_TRUE(fx.net.is_running);
}

void test_leaving_wifi_on_stops_an_ap_no_session_owns() {
  NetFixture fx;
  fx.cmd(NetCmd::ApplyWifi, 0, WifiMode::On);
  fx.cmd(NetCmd::ApplyWifi, 10, WifiMode::Off);
  TEST_ASSERT_FALSE(fx.net.is_running);
}

void test_leaving_wifi_on_keeps_the_ap_while_the_upload_screen_owns_it() {
  NetFixture fx;
  fx.cmd(NetCmd::ApplyWifi, 0, WifiMode::On);
  fx.cmd(NetCmd::ApplyWifi, 10, WifiMode::Auto, true);
  TEST_ASSERT_TRUE(fx.net.is_running);
}

// --- UiService ------------------------------------------------------------------

namespace {

struct UiFixture {
  Chan<RsvpMsg> rsvp;
  Chan<NetMsg> net;
  Library lib;
  SettingsStore store;
  DisplayControl display;
  Log log;
  UiService svc;
  explicit UiFixture(std::size_t net_capacity = 8)
      : net(net_capacity),
        svc(leeloo::settings::Settings{}, &lib, &store, &display, &rsvp.q, &net.q, &log) {
    svc.sync_books();
  }
  void press(Event e) { svc.handle(e); }
  void presses(std::initializer_list<Event> events) {
    for (Event e : events) svc.handle(e);
  }
  leeloo::ui::Screen screen() const { return svc.model().screen(); }
};

}  // namespace

void test_sync_books_follows_the_library_without_resetting_the_cursor() {
  UiFixture fx;
  TEST_ASSERT_EQUAL_UINT32(2, fx.svc.model().book_count());
  fx.presses({Event::Select, Event::Down});  // Leer, cursor on the second book
  fx.svc.sync_books();                       // same count: nothing changes
  TEST_ASSERT_EQUAL_UINT32(1, fx.svc.model().cursor());
  fx.lib.count = 3;
  fx.svc.sync_books();
  TEST_ASSERT_EQUAL_UINT32(3, fx.svc.model().book_count());
}

void test_choosing_a_book_sends_open_with_its_index() {
  UiFixture fx;
  fx.presses({Event::Select, Event::Down, Event::Select});  // Leer, second book
  RsvpMsg m;
  TEST_ASSERT_TRUE(fx.rsvp.take(&m));
  TEST_ASSERT_EQUAL((int)RsvpCmd::Open, (int)m.cmd);
  TEST_ASSERT_EQUAL_UINT32(1, m.index);
  TEST_ASSERT_EQUAL((int)leeloo::ui::Screen::Reading, (int)fx.screen());
}

void test_reading_buttons_are_forwarded_and_back_closes_the_book() {
  UiFixture fx;
  fx.presses({Event::Select, Event::Select});  // open book 0
  RsvpMsg m;
  fx.rsvp.take(&m);
  fx.press(Event::Up);
  fx.press(Event::Down);
  fx.press(Event::Select);
  fx.press(Event::Back);
  const RsvpCmd expected[] = {RsvpCmd::Up, RsvpCmd::Down, RsvpCmd::Select, RsvpCmd::Close};
  for (RsvpCmd cmd : expected) {
    TEST_ASSERT_TRUE(fx.rsvp.take(&m));
    TEST_ASSERT_EQUAL((int)cmd, (int)m.cmd);
  }
  TEST_ASSERT_EQUAL((int)leeloo::ui::Screen::Read, (int)fx.screen());
}

void test_book_open_failure_from_rsvp_shows_a_message() {
  UiFixture fx;
  fx.presses({Event::Select, Event::Select, Event::BookOpenFailed});
  TEST_ASSERT_EQUAL((int)leeloo::ui::Screen::Message, (int)fx.screen());
  TEST_ASSERT_EQUAL((int)leeloo::ui::Message::BookOpenFailed, (int)fx.svc.model().message());
}

void test_upload_screen_sends_start_and_back_sends_stop() {
  UiFixture fx;
  fx.presses({Event::Down, Event::Select, Event::Select});  // Biblioteca > Subir
  NetMsg m;
  TEST_ASSERT_TRUE(fx.net.take(&m));
  TEST_ASSERT_EQUAL((int)NetCmd::StartUpload, (int)m.cmd);
  TEST_ASSERT_EQUAL((int)leeloo::ui::Screen::Upload, (int)fx.screen());
  fx.press(Event::Back);
  TEST_ASSERT_TRUE(fx.net.take(&m));
  TEST_ASSERT_EQUAL((int)NetCmd::StopUpload, (int)m.cmd);
}

void test_save_on_the_upload_screen_notifies_net_and_refreshes_the_count() {
  UiFixture fx;
  fx.presses({Event::Down, Event::Select, Event::Select});
  NetMsg m;
  fx.net.take(&m);  // StartUpload
  fx.lib.count = 3;
  fx.press(Event::UploadSaved);
  TEST_ASSERT_TRUE(fx.net.take(&m));
  TEST_ASSERT_EQUAL((int)NetCmd::SavedOnUploadScreen, (int)m.cmd);
  TEST_ASSERT_EQUAL_UINT32(3, fx.svc.model().book_count());
  TEST_ASSERT_EQUAL((int)leeloo::ui::Message::UploadSaved, (int)fx.svc.model().message());
}

void test_save_elsewhere_does_not_touch_net_but_refreshes_the_count() {
  UiFixture fx;
  fx.lib.count = 5;
  fx.press(Event::UploadSaved);  // the phone saved while the root menu was showing
  TEST_ASSERT_TRUE(fx.net.empty());
  TEST_ASSERT_EQUAL_UINT32(5, fx.svc.model().book_count());
}

void test_confirmed_delete_removes_the_book_and_resyncs() {
  UiFixture fx;
  // Biblioteca > Borrar > first book > Borrar
  fx.presses({Event::Down, Event::Select, Event::Down, Event::Select, Event::Select, Event::Down,
              Event::Select});
  TEST_ASSERT_EQUAL(0, fx.lib.removed);
  TEST_ASSERT_EQUAL_UINT32(1, fx.svc.model().book_count());
}

void test_cancelled_delete_keeps_the_book() {
  UiFixture fx;
  fx.presses({Event::Down, Event::Select, Event::Down, Event::Select, Event::Select, Event::Select});
  TEST_ASSERT_EQUAL(-1, fx.lib.removed);
  TEST_ASSERT_EQUAL_UINT32(2, fx.svc.model().book_count());
}

void test_failed_delete_shows_the_error_and_keeps_the_count() {
  UiFixture fx;
  fx.lib.remove_ok = false;
  fx.presses({Event::Down, Event::Select, Event::Down, Event::Select, Event::Select, Event::Down,
              Event::Select});
  TEST_ASSERT_EQUAL((int)leeloo::ui::Screen::Message, (int)fx.screen());
  TEST_ASSERT_EQUAL((int)leeloo::ui::Message::DeleteFailed, (int)fx.svc.model().message());
  TEST_ASSERT_EQUAL_UINT32(2, fx.svc.model().book_count());
}

void test_changing_contrast_saves_applies_and_tells_net() {
  UiFixture fx;
  // Ajustes > Contraste > edit > one step down
  fx.presses({Event::Down, Event::Down, Event::Select, Event::Down, Event::Select, Event::Select,
              Event::Down});
  TEST_ASSERT_EQUAL(1, fx.store.saves);
  TEST_ASSERT_EQUAL_UINT8(95, fx.store.last.contrast_percent);
  TEST_ASSERT_EQUAL_UINT8(leeloo::settings::display_register(100, 95), fx.display.last);
  NetMsg m;
  TEST_ASSERT_TRUE(fx.net.take(&m));
  TEST_ASSERT_EQUAL((int)NetCmd::ApplyWifi, (int)m.cmd);
  TEST_ASSERT_FALSE(m.upload_screen_open);
}

void test_changing_wifi_mode_sends_the_new_mode_to_net() {
  UiFixture fx;
  // Ajustes > Wi-Fi > edit > next mode
  fx.presses({Event::Down, Event::Down, Event::Select, Event::Down, Event::Down, Event::Select,
              Event::Select, Event::Up});
  NetMsg m;
  TEST_ASSERT_TRUE(fx.net.take(&m));
  TEST_ASSERT_EQUAL((int)NetCmd::ApplyWifi, (int)m.cmd);
  TEST_ASSERT_EQUAL((int)leeloo::settings::cycle_wifi(WifiMode::Auto, +1), (int)m.wifi);
}

void test_settings_save_failure_is_logged_and_still_applied() {
  UiFixture fx;
  fx.store.ok = false;
  fx.presses({Event::Down, Event::Down, Event::Select, Event::Down, Event::Select, Event::Select,
              Event::Down});
  TEST_ASSERT_TRUE(fx.log.has("Ajustes"));
  TEST_ASSERT_EQUAL(1, fx.display.calls);
}

void test_full_net_queue_is_logged_instead_of_dropped_silently() {
  UiFixture fx(1);
  fx.presses({Event::Down, Event::Select, Event::Select});  // StartUpload fills the queue
  fx.press(Event::Back);                                    // StopUpload finds it full
  TEST_ASSERT_TRUE(fx.log.has("Cola de red llena"));
}

// --- ViewModel ------------------------------------------------------------------

namespace {

struct ViewFixture {
  leeloo::rtos::fakes::FakeMutex mutex;
  leeloo::rtos::fakes::FakeNotifier display;
  ViewModel view;
  Snapshot snap;
  std::uint32_t seen = 0;
  ViewFixture() : view(&mutex, &display) { display.mutex = &mutex; }
  bool read() { return view.read_if_new(&snap, &seen); }
  static ReadingFrame frame_with(const char* word) {
    ReadingFrame f;
    f.active = true;
    std::snprintf(f.word, sizeof f.word, "%s", word);
    return f;
  }
};

}  // namespace

void test_view_has_nothing_to_read_before_the_first_publish() {
  ViewFixture fx;
  TEST_ASSERT_FALSE(fx.read());
}

void test_published_state_is_delivered_once() {
  ViewFixture fx;
  fx.view.publish_frame(ViewFixture::frame_with("hola"));
  TEST_ASSERT_TRUE(fx.read());
  TEST_ASSERT_EQUAL_STRING("hola", fx.snap.frame.word);
  TEST_ASSERT_FALSE(fx.read());  // nothing new since the last read
}

void test_latest_publish_wins_when_the_display_is_slow() {
  ViewFixture fx;
  fx.view.publish_frame(ViewFixture::frame_with("uno"));
  fx.view.publish_frame(ViewFixture::frame_with("dos"));
  TEST_ASSERT_TRUE(fx.read());
  TEST_ASSERT_EQUAL_STRING("dos", fx.snap.frame.word);  // "uno" is skipped, not queued
  TEST_ASSERT_FALSE(fx.read());
}

void test_each_publish_wakes_the_display_outside_the_lock() {
  ViewFixture fx;
  fx.view.publish_frame(ViewFixture::frame_with("uno"));
  fx.view.publish_net(NetStatus{});
  TEST_ASSERT_EQUAL(2, fx.display.count);
  TEST_ASSERT_EQUAL(0, fx.display.depth_at_last);  // the woken task must not run into a held lock
  TEST_ASSERT_EQUAL(0, fx.mutex.depth);
}

void test_producers_do_not_overwrite_each_others_part() {
  ViewFixture fx;
  leeloo::settings::Settings s;
  leeloo::ui::Ui ui(s);
  ui.set_book_count(3);
  NetStatus net;
  net.running = true;
  std::snprintf(net.pin, sizeof net.pin, "%s", "1234");
  fx.view.publish_ui(ui);
  fx.view.publish_net(net);
  fx.view.publish_frame(ViewFixture::frame_with("hola"));
  TEST_ASSERT_TRUE(fx.read());
  TEST_ASSERT_EQUAL_UINT32(3, fx.snap.ui.book_count());
  TEST_ASSERT_TRUE(fx.snap.net.running);
  TEST_ASSERT_EQUAL_STRING("1234", fx.snap.net.pin);
  TEST_ASSERT_EQUAL_STRING("hola", fx.snap.frame.word);
}

void test_contrast_request_is_handed_over_once() {
  ViewFixture fx;
  fx.view.request_contrast(200);
  TEST_ASSERT_TRUE(fx.read());
  TEST_ASSERT_TRUE(fx.snap.contrast_pending);
  TEST_ASSERT_EQUAL_UINT8(200, fx.snap.contrast);
  fx.view.publish_frame(ViewFixture::frame_with("x"));  // a later frame must not re-apply it
  TEST_ASSERT_TRUE(fx.read());
  TEST_ASSERT_FALSE(fx.snap.contrast_pending);
}

void test_read_balances_the_lock() {
  ViewFixture fx;
  fx.view.publish_frame(ViewFixture::frame_with("x"));
  fx.read();
  fx.read();
  TEST_ASSERT_EQUAL(0, fx.mutex.depth);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_view_has_nothing_to_read_before_the_first_publish);
  RUN_TEST(test_published_state_is_delivered_once);
  RUN_TEST(test_latest_publish_wins_when_the_display_is_slow);
  RUN_TEST(test_each_publish_wakes_the_display_outside_the_lock);
  RUN_TEST(test_producers_do_not_overwrite_each_others_part);
  RUN_TEST(test_contrast_request_is_handed_over_once);
  RUN_TEST(test_read_balances_the_lock);
  RUN_TEST(test_up_press_posts_up_once_the_level_is_stable);
  RUN_TEST(test_select_released_before_long_press_posts_select);
  RUN_TEST(test_select_held_past_long_press_posts_back);
  RUN_TEST(test_bounce_that_returns_to_idle_posts_nothing);
  RUN_TEST(test_ignores_edge_with_button_index_out_of_range);
  RUN_TEST(test_logs_the_drop_when_the_ui_queue_is_full);

  RUN_TEST(test_open_shows_the_first_word_at_default_speed);
  RUN_TEST(test_open_resumes_at_the_saved_word);
  RUN_TEST(test_open_failure_reports_book_open_failed);
  RUN_TEST(test_book_without_start_marker_fails_and_is_released);
  RUN_TEST(test_book_with_empty_body_finishes_at_once);
  RUN_TEST(test_select_pauses_and_saves_then_resumes);
  RUN_TEST(test_commands_without_an_open_book_change_nothing);
  RUN_TEST(test_up_and_down_change_speed_by_one_step_and_show_the_label);
  RUN_TEST(test_wpm_label_hides_when_its_time_is_up);
  RUN_TEST(test_paused_down_jumps_to_next_sentence_and_saves);
  RUN_TEST(test_close_saves_the_position_and_releases_the_book);
  RUN_TEST(test_save_failure_is_logged_not_ignored);
  RUN_TEST(test_last_word_held_finishes_the_book_and_resets_the_position);
  RUN_TEST(test_waits_forever_without_a_book_and_until_the_next_word_while_reading);
  RUN_TEST(test_paused_reader_has_nothing_to_wake_for);
  RUN_TEST(test_frame_cuts_a_long_word_with_an_ellipsis);
  RUN_TEST(test_frame_cut_never_splits_a_multibyte_letter);
  RUN_TEST(test_frame_word_is_empty_for_invalid_utf8);
  RUN_TEST(test_frame_keeps_spanish_accents_and_inverted_marks);

  RUN_TEST(test_boot_with_wifi_on_starts_the_ap);
  RUN_TEST(test_boot_with_automatico_keeps_the_ap_off);
  RUN_TEST(test_boot_failure_with_wifi_on_is_logged);
  RUN_TEST(test_start_upload_in_automatico_starts_the_ap_and_polls_activity);
  RUN_TEST(test_start_failure_reports_upload_start_failed);
  RUN_TEST(test_start_upload_reuses_an_ap_that_is_already_up);
  RUN_TEST(test_stop_upload_stops_the_ap_in_automatico);
  RUN_TEST(test_stop_upload_keeps_the_ap_when_wifi_is_on);
  RUN_TEST(test_inactivity_ends_an_automatico_session_and_tells_the_ui);
  RUN_TEST(test_phone_activity_keeps_the_session_alive);
  RUN_TEST(test_inactivity_check_survives_a_32_bit_clock_wrap);
  RUN_TEST(test_save_on_upload_screen_stops_the_ap_after_the_grace_time);
  RUN_TEST(test_save_with_wifi_on_schedules_no_stop);
  RUN_TEST(test_new_upload_cancels_a_deferred_stop);
  RUN_TEST(test_deferred_stop_sets_the_wait_to_the_remaining_time);
  RUN_TEST(test_wifi_set_to_on_starts_the_ap_at_once);
  RUN_TEST(test_leaving_wifi_on_stops_an_ap_no_session_owns);
  RUN_TEST(test_leaving_wifi_on_keeps_the_ap_while_the_upload_screen_owns_it);

  RUN_TEST(test_sync_books_follows_the_library_without_resetting_the_cursor);
  RUN_TEST(test_choosing_a_book_sends_open_with_its_index);
  RUN_TEST(test_reading_buttons_are_forwarded_and_back_closes_the_book);
  RUN_TEST(test_book_open_failure_from_rsvp_shows_a_message);
  RUN_TEST(test_upload_screen_sends_start_and_back_sends_stop);
  RUN_TEST(test_save_on_the_upload_screen_notifies_net_and_refreshes_the_count);
  RUN_TEST(test_save_elsewhere_does_not_touch_net_but_refreshes_the_count);
  RUN_TEST(test_confirmed_delete_removes_the_book_and_resyncs);
  RUN_TEST(test_cancelled_delete_keeps_the_book);
  RUN_TEST(test_failed_delete_shows_the_error_and_keeps_the_count);
  RUN_TEST(test_changing_contrast_saves_applies_and_tells_net);
  RUN_TEST(test_changing_wifi_mode_sends_the_new_mode_to_net);
  RUN_TEST(test_settings_save_failure_is_logged_and_still_applied);
  RUN_TEST(test_full_net_queue_is_logged_instead_of_dropped_silently);
  return UNITY_END();
}
