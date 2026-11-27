#include <unity.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include "fakes.h"
#include "recording_service.h"

using namespace leeloo::app;
using leeloo::rtos::kWaitForever;
using leeloo::rtos::Queue;
using leeloo::rtos::fakes::FakeQueue;
using leeloo::ui::Event;
namespace rec = leeloo::recording;

void setUp() {}
void tearDown() {}

namespace {

struct Log : ILog {
  std::vector<std::string> lines;
  void log(const char* line) override { lines.emplace_back(line); }
};

struct Mic : IMic {
  bool start_ok = true;
  bool pause_ok = true;
  bool resume_ok = true;
  bool read_fails = false;
  int starts = 0, stops = 0, pauses = 0, resumes = 0, reads = 0;
  std::uint32_t rate = 0;
  std::size_t chunk_samples = 100;  // every read returns this many samples of `value`
  std::int16_t value = 0x0102;
  std::size_t reads_left = 1000000;
  bool start(std::uint32_t r) override {
    ++starts;
    rate = r;
    return start_ok;
  }
  void stop() override { ++stops; }
  bool pause() override { ++pauses; return pause_ok; }
  bool resume() override { ++resumes; return resume_ok; }
  int read(std::int16_t* out, std::size_t max) override {
    ++reads;
    if (read_fails) return -1;
    if (reads_left == 0) return 0;
    --reads_left;
    const std::size_t n = std::min(max, chunk_samples);
    std::fill(out, out + n, value);
    return static_cast<int>(n);
  }
};

struct Store : IRecordingStore {
  bool is_ready = true;
  bool has_file = false;
  bool lock_ok = true;
  bool open_ok = true;
  bool write_ok = true;
  bool finish_ok = true;
  bool remove_ok = true;
  std::size_t free = 8u * 1024 * 1024;  // 8 MiB free: ~190 s after the reserve
  bool locked = false;
  int cleanups = 0, aborts = 0, finishes = 0, removes = 0, opens = 0;
  std::uint32_t open_rate = 0, finish_rate = 0, finish_pcm = 0;
  std::vector<std::uint8_t> data;

  bool ready() override { return is_ready; }
  bool exists() override { return has_file; }
  std::size_t free_bytes() override { return free; }
  bool lock() override {
    if (!lock_ok || locked) return false;
    locked = true;
    return true;
  }
  void unlock() override { locked = false; }
  bool open_temp(std::uint32_t rate) override {
    ++opens;
    open_rate = rate;
    data.clear();
    return open_ok;
  }
  bool write(const std::uint8_t* d, std::size_t n) override {
    if (!write_ok) return false;
    data.insert(data.end(), d, d + n);
    return true;
  }
  bool finish(std::uint32_t rate, std::uint32_t pcm) override {
    ++finishes;
    finish_rate = rate;
    finish_pcm = pcm;
    return finish_ok;
  }
  void abort() override { ++aborts; }
  bool remove() override {
    ++removes;
    if (remove_ok) has_file = false;
    return remove_ok;
  }
  void cleanup_temp() override { ++cleanups; }
};

struct Fixture {
  FakeQueue port{16, sizeof(Event)};
  Queue<Event> ui{&port};
  Mic mic;
  Store store;
  Log log;
  RecordingService svc{&mic, &store, &ui, &log};

  bool start(std::uint32_t now = 0) { return svc.handle({RecordingCmd::Start}, now); }
  bool stop(std::uint32_t now = 0) { return svc.handle({RecordingCmd::Stop}, now); }
  bool toggle(std::uint32_t now = 0) { return svc.handle({RecordingCmd::TogglePause}, now); }
  std::vector<Event> events() {
    std::vector<Event> out;
    Event e;
    while (ui.receive(&e, 0)) out.push_back(e);
    return out;
  }
  Event only_event() {
    const auto all = events();
    TEST_ASSERT_EQUAL_UINT32(1, all.size());
    return all.empty() ? Event::Up : all[0];
  }
  bool no_events() { return port.size() == 0; }
};

}  // namespace

// --- boot ------------------------------------------------------------------------

void test_begin_removes_temp_leftovers_and_reports_a_stored_wav() {
  Fixture fx;
  fx.store.has_file = true;
  fx.svc.begin();
  TEST_ASSERT_EQUAL(1, fx.store.cleanups);
  TEST_ASSERT_EQUAL((int)Event::AudioAvailable, (int)fx.only_event());
}

void test_begin_with_nothing_stored_is_silent() {
  Fixture fx;
  fx.svc.begin();
  TEST_ASSERT_TRUE(fx.no_events());
}

void test_begin_reports_unavailable_storage() {
  Fixture fx;
  fx.store.is_ready = false;
  fx.store.has_file = true;
  fx.svc.begin();
  TEST_ASSERT_EQUAL((int)Event::AudioStorageUnavailable, (int)fx.only_event());
}

// --- start -----------------------------------------------------------------------

// Free space that leaves exactly `seconds` of 16 kHz recording after the reserve.
std::size_t free_for(std::uint32_t seconds) {
  return rec::kReserveBytes + rec::kWriteSlackBytes + rec::kWavHeaderBytes +
         static_cast<std::size_t>(seconds) * rec::kBytesPerSecond;
}

void test_start_records_at_16_khz_for_as_long_as_the_free_space_allows() {
  Fixture fx;
  fx.store.free = free_for(100);
  fx.start();
  TEST_ASSERT_EQUAL_UINT32(16000, fx.store.open_rate);
  TEST_ASSERT_EQUAL_UINT32(16000, fx.mic.rate);
  TEST_ASSERT_TRUE(fx.store.locked);
  const RecordingFrame f = fx.svc.frame();
  TEST_ASSERT_TRUE(f.active);
  TEST_ASSERT_FALSE(f.paused);
  TEST_ASSERT_EQUAL_UINT32(100, f.max_seconds);
  TEST_ASSERT_EQUAL_UINT32(16000, f.sample_rate);
  TEST_ASSERT_TRUE(fx.no_events());
}

void test_a_different_amount_of_free_space_gives_a_different_limit() {
  Fixture a;
  a.store.free = free_for(30);
  a.start();
  Fixture b;
  b.store.free = free_for(200);
  b.start();
  TEST_ASSERT_EQUAL_UINT32(30, a.svc.frame().max_seconds);
  TEST_ASSERT_EQUAL_UINT32(200, b.svc.frame().max_seconds);
}

void test_start_without_space_reports_it_and_keeps_nothing_locked() {
  Fixture fx;
  fx.store.free = free_for(4);  // under the five-second minimum
  fx.start();
  TEST_ASSERT_EQUAL((int)Event::AudioNoSpace, (int)fx.only_event());
  TEST_ASSERT_FALSE(fx.store.locked);
  TEST_ASSERT_EQUAL(0, fx.store.opens);
  TEST_ASSERT_FALSE(fx.svc.frame().active);
}

void test_start_while_storage_is_busy_reports_busy() {
  Fixture fx;
  fx.store.lock_ok = false;
  fx.start();
  TEST_ASSERT_EQUAL((int)Event::AudioBusy, (int)fx.only_event());
  TEST_ASSERT_EQUAL(0, fx.store.opens);
}

void test_start_with_a_wav_already_pending_fails_and_releases_the_lock() {
  Fixture fx;
  fx.store.has_file = true;
  fx.start();
  TEST_ASSERT_EQUAL((int)Event::AudioStartFailed, (int)fx.only_event());
  TEST_ASSERT_FALSE(fx.store.locked);
  TEST_ASSERT_EQUAL(0, fx.store.opens);
}

void test_start_fails_cleanly_when_the_temp_file_cannot_be_created() {
  Fixture fx;
  fx.store.open_ok = false;
  fx.start();
  TEST_ASSERT_EQUAL((int)Event::AudioStartFailed, (int)fx.only_event());
  TEST_ASSERT_FALSE(fx.store.locked);
  TEST_ASSERT_EQUAL(0, fx.mic.starts);
}

void test_start_fails_cleanly_when_the_microphone_cannot_start() {
  Fixture fx;
  fx.mic.start_ok = false;
  fx.start();
  TEST_ASSERT_EQUAL((int)Event::AudioStartFailed, (int)fx.only_event());
  TEST_ASSERT_EQUAL(1, fx.store.aborts);  // the temp file is removed
  TEST_ASSERT_FALSE(fx.store.locked);
  TEST_ASSERT_FALSE(fx.svc.frame().active);
}

void test_a_second_start_while_recording_changes_nothing() {
  Fixture fx;
  fx.start();
  fx.start();
  TEST_ASSERT_EQUAL(1, fx.mic.starts);
  TEST_ASSERT_TRUE(fx.no_events());
}

// --- capture ---------------------------------------------------------------------

void test_poll_writes_samples_as_little_endian_pcm() {
  Fixture fx;
  fx.mic.chunk_samples = 3;
  fx.mic.reads_left = 1;
  fx.start();
  fx.svc.poll(10);
  const std::vector<std::uint8_t> expected = {0x02, 0x01, 0x02, 0x01, 0x02, 0x01};
  TEST_ASSERT_TRUE(expected == fx.store.data);
}

void test_elapsed_time_follows_the_bytes_written_and_updates_once_per_second() {
  Fixture fx;
  fx.mic.chunk_samples = 250;  // 500 bytes per read; 16 kHz mono 16-bit = 32000 bytes per second
  fx.mic.reads_left = 63;      // 31 500 bytes buffered: not yet 1 s
  fx.start();
  TEST_ASSERT_FALSE(fx.svc.poll(10));  // 32 reads: a poll stops at its read limit...
  TEST_ASSERT_FALSE(fx.svc.poll(11));  // ...and the next one drains the rest
  TEST_ASSERT_EQUAL_UINT32(31500, fx.store.data.size());
  TEST_ASSERT_EQUAL_UINT32(0, fx.svc.frame().elapsed_seconds);
  fx.mic.reads_left = 1;  // 32 000 bytes
  TEST_ASSERT_TRUE(fx.svc.poll(40));
  TEST_ASSERT_EQUAL_UINT32(1, fx.svc.frame().elapsed_seconds);
}

// 16 kHz mono arrives at 16 000 samples/s but one read returns at most 256 and the task wakes
// every 20 ms, so a single read per wake-up would fall behind and the DMA would overflow.
void test_one_poll_drains_everything_the_microphone_has_buffered() {
  Fixture fx;
  fx.mic.chunk_samples = 256;
  fx.mic.reads_left = 5;
  fx.start();
  fx.svc.poll(10);
  TEST_ASSERT_EQUAL_UINT32(5 * 256 * 2, fx.store.data.size());
  TEST_ASSERT_EQUAL_UINT32(20, fx.svc.wait_ms(10));  // drained: nothing is waiting
}

void test_a_backlog_longer_than_one_poll_is_continued_without_waiting() {
  Fixture fx;
  fx.mic.chunk_samples = 256;  // never runs dry
  fx.start();
  fx.svc.poll(10);
  const std::size_t first = fx.store.data.size();
  TEST_ASSERT_TRUE(first > 256 * 2);  // more than one read per poll
  TEST_ASSERT_EQUAL_UINT32(0, fx.svc.wait_ms(10));  // there is still data: poll again at once
  fx.svc.poll(11);
  TEST_ASSERT_TRUE(fx.store.data.size() > first);
}

void test_an_empty_read_changes_nothing() {
  Fixture fx;
  fx.mic.reads_left = 0;
  fx.start();
  TEST_ASSERT_FALSE(fx.svc.poll(10));
  TEST_ASSERT_EQUAL_UINT32(0, fx.store.data.size());
}

void test_the_limit_ends_the_capture_at_the_exact_byte() {
  Fixture fx;
  fx.store.free = free_for(30);  // 960 000 bytes of audio
  fx.mic.chunk_samples = 255;  // 510 bytes per read does not divide 960 000: the last read is cut
  fx.start();
  for (int i = 0; i < 4000 && fx.svc.frame().active; ++i) fx.svc.poll(i);
  TEST_ASSERT_FALSE(fx.svc.frame().active);
  TEST_ASSERT_EQUAL_UINT32(960000, fx.store.data.size());
  TEST_ASSERT_EQUAL_UINT32(960000, fx.store.finish_pcm);
  TEST_ASSERT_EQUAL_UINT32(16000, fx.store.finish_rate);
  TEST_ASSERT_EQUAL((int)Event::AudioSaved, (int)fx.only_event());
  TEST_ASSERT_FALSE(fx.store.locked);
}

// Nothing captured yet: an error is a failure and the temp file is dropped.
void test_a_write_failure_before_any_audio_aborts_and_reports() {
  Fixture fx;
  fx.start();
  fx.store.write_ok = false;
  TEST_ASSERT_TRUE(fx.svc.poll(10));
  TEST_ASSERT_EQUAL((int)Event::AudioSaveFailed, (int)fx.only_event());
  TEST_ASSERT_EQUAL(1, fx.store.aborts);
  TEST_ASSERT_EQUAL(0, fx.store.finishes);
  TEST_ASSERT_EQUAL(1, fx.mic.stops);
  TEST_ASSERT_FALSE(fx.store.locked);
  TEST_ASSERT_FALSE(fx.svc.frame().active);
}

void test_a_microphone_error_before_any_audio_aborts_and_reports() {
  Fixture fx;
  fx.start();
  fx.mic.read_fails = true;
  TEST_ASSERT_TRUE(fx.svc.poll(10));
  TEST_ASSERT_EQUAL((int)Event::AudioSaveFailed, (int)fx.only_event());
  TEST_ASSERT_EQUAL(1, fx.store.aborts);
  TEST_ASSERT_FALSE(fx.store.locked);
}

void test_a_pause_that_fails_before_any_audio_aborts_the_capture() {
  Fixture fx;
  fx.start();
  fx.mic.pause_ok = false;
  fx.toggle(1);
  TEST_ASSERT_EQUAL((int)Event::AudioSaveFailed, (int)fx.only_event());
  TEST_ASSERT_FALSE(fx.store.locked);
  TEST_ASSERT_FALSE(fx.svc.frame().active);
}

// Audio already captured is kept: a full disk must not throw away the recording so far.
void test_a_full_disk_mid_recording_saves_what_was_captured() {
  Fixture fx;
  fx.mic.chunk_samples = 100;
  fx.start();
  fx.mic.reads_left = 2;
  fx.svc.poll(1);  // 400 bytes written
  fx.mic.reads_left = 1;
  fx.store.write_ok = false;
  TEST_ASSERT_TRUE(fx.svc.poll(2));
  TEST_ASSERT_EQUAL(1, fx.store.finishes);
  TEST_ASSERT_EQUAL_UINT32(400, fx.store.finish_pcm);  // the failed chunk is not counted
  TEST_ASSERT_EQUAL(0, fx.store.aborts);
  TEST_ASSERT_EQUAL((int)Event::AudioSaved, (int)fx.only_event());
  TEST_ASSERT_FALSE(fx.store.locked);
  TEST_ASSERT_FALSE(fx.svc.frame().active);
}

void test_a_microphone_error_mid_recording_saves_what_was_captured() {
  Fixture fx;
  fx.mic.chunk_samples = 100;
  fx.start();
  fx.mic.reads_left = 1;
  fx.svc.poll(1);
  fx.mic.read_fails = true;
  fx.svc.poll(2);
  TEST_ASSERT_EQUAL_UINT32(200, fx.store.finish_pcm);
  TEST_ASSERT_EQUAL((int)Event::AudioSaved, (int)fx.only_event());
}

void test_a_pause_that_fails_mid_recording_saves_what_was_captured() {
  Fixture fx;
  fx.mic.chunk_samples = 100;
  fx.start();
  fx.mic.reads_left = 1;
  fx.svc.poll(1);
  fx.mic.pause_ok = false;
  fx.toggle(2);
  TEST_ASSERT_EQUAL_UINT32(200, fx.store.finish_pcm);
  TEST_ASSERT_EQUAL((int)Event::AudioSaved, (int)fx.only_event());
  TEST_ASSERT_FALSE(fx.store.locked);
}

void test_when_saving_after_an_error_also_fails_the_failure_is_reported() {
  Fixture fx;
  fx.mic.chunk_samples = 100;
  fx.start();
  fx.mic.reads_left = 1;
  fx.svc.poll(1);
  fx.store.write_ok = false;
  fx.store.finish_ok = false;
  fx.mic.reads_left = 1;
  fx.svc.poll(2);
  TEST_ASSERT_EQUAL(1, fx.store.aborts);
  TEST_ASSERT_EQUAL((int)Event::AudioSaveFailed, (int)fx.only_event());
  TEST_ASSERT_FALSE(fx.store.locked);
}

void test_poll_does_nothing_when_idle() {
  Fixture fx;
  TEST_ASSERT_FALSE(fx.svc.poll(10));
  TEST_ASSERT_EQUAL(0, fx.mic.reads);
}

// --- stop ------------------------------------------------------------------------

void test_stop_stores_the_recording_with_its_exact_length() {
  Fixture fx;
  fx.mic.chunk_samples = 100;
  fx.start();
  fx.mic.reads_left = 1;
  fx.svc.poll(1);
  fx.mic.reads_left = 1;
  fx.svc.poll(2);
  TEST_ASSERT_TRUE(fx.stop(3));
  TEST_ASSERT_EQUAL(1, fx.store.finishes);
  TEST_ASSERT_EQUAL_UINT32(400, fx.store.finish_pcm);  // 2 reads x 100 samples x 2 bytes
  TEST_ASSERT_EQUAL_UINT32(16000, fx.store.finish_rate);
  TEST_ASSERT_EQUAL((int)Event::AudioSaved, (int)fx.only_event());
  TEST_ASSERT_EQUAL(1, fx.mic.stops);
  TEST_ASSERT_FALSE(fx.store.locked);
  TEST_ASSERT_FALSE(fx.svc.frame().active);
}

void test_stop_without_any_audio_discards_the_file() {
  Fixture fx;
  fx.start();
  fx.stop();
  TEST_ASSERT_EQUAL(0, fx.store.finishes);
  TEST_ASSERT_EQUAL(1, fx.store.aborts);
  TEST_ASSERT_EQUAL((int)Event::AudioSaveFailed, (int)fx.only_event());
  TEST_ASSERT_FALSE(fx.store.locked);
}

void test_a_failed_finalize_removes_the_temp_file_and_reports() {
  Fixture fx;
  fx.start();
  fx.svc.poll(1);
  fx.store.finish_ok = false;
  fx.stop();
  TEST_ASSERT_EQUAL(1, fx.store.aborts);
  TEST_ASSERT_EQUAL((int)Event::AudioSaveFailed, (int)fx.only_event());
  TEST_ASSERT_FALSE(fx.store.locked);
}

void test_stop_with_nothing_recording_is_answered_so_the_screen_can_leave() {
  Fixture fx;
  TEST_ASSERT_FALSE(fx.stop());
  TEST_ASSERT_EQUAL((int)Event::RecordingEnded, (int)fx.only_event());
}

void test_stop_after_a_failed_start_is_answered_too() {
  Fixture fx;
  fx.store.lock_ok = false;
  fx.start();
  fx.events();  // AudioBusy, which the UI may have missed
  fx.stop();
  TEST_ASSERT_EQUAL((int)Event::RecordingEnded, (int)fx.only_event());
}

// --- pause -----------------------------------------------------------------------

void test_pause_stops_reading_and_resume_continues() {
  Fixture fx;
  fx.start();
  fx.svc.poll(1);
  TEST_ASSERT_TRUE(fx.toggle(2));
  TEST_ASSERT_TRUE(fx.svc.frame().paused);
  TEST_ASSERT_EQUAL(1, fx.mic.pauses);
  const int reads = fx.mic.reads;
  fx.svc.poll(3);
  TEST_ASSERT_EQUAL(reads, fx.mic.reads);  // nothing is read while paused
  TEST_ASSERT_TRUE(fx.toggle(4));
  TEST_ASSERT_FALSE(fx.svc.frame().paused);
  TEST_ASSERT_EQUAL(1, fx.mic.resumes);
  fx.svc.poll(5);
  TEST_ASSERT_TRUE(fx.mic.reads > reads);
}

void test_toggle_without_a_recording_does_nothing() {
  Fixture fx;
  TEST_ASSERT_FALSE(fx.toggle());
  TEST_ASSERT_EQUAL(0, fx.mic.pauses);
}

void test_a_long_pause_saves_the_recording_and_frees_the_storage() {
  Fixture fx;
  fx.start();
  fx.svc.poll(1);
  fx.toggle(1000);
  TEST_ASSERT_FALSE(fx.svc.poll(1000 + rec::kMaxPauseMs - 1));
  TEST_ASSERT_TRUE(fx.store.locked);
  TEST_ASSERT_TRUE(fx.svc.poll(1000 + rec::kMaxPauseMs));
  TEST_ASSERT_EQUAL(1, fx.store.finishes);
  TEST_ASSERT_EQUAL((int)Event::AudioSaved, (int)fx.only_event());
  TEST_ASSERT_FALSE(fx.store.locked);
}

// --- delete ----------------------------------------------------------------------

void test_delete_removes_the_stored_wav() {
  Fixture fx;
  fx.store.has_file = true;
  fx.svc.handle({RecordingCmd::Delete}, 0);
  TEST_ASSERT_EQUAL((int)Event::AudioDeleted, (int)fx.only_event());
  TEST_ASSERT_FALSE(fx.store.has_file);
}

void test_a_failed_delete_is_reported_and_keeps_the_file() {
  Fixture fx;
  fx.store.has_file = true;
  fx.store.remove_ok = false;
  fx.svc.handle({RecordingCmd::Delete}, 0);
  TEST_ASSERT_EQUAL((int)Event::AudioDeleteFailed, (int)fx.only_event());
  TEST_ASSERT_TRUE(fx.store.has_file);
}

void test_delete_is_ignored_while_recording() {
  Fixture fx;
  fx.start();
  TEST_ASSERT_FALSE(fx.svc.handle({RecordingCmd::Delete}, 0));
  TEST_ASSERT_EQUAL(0, fx.store.removes);
}

// --- waiting ---------------------------------------------------------------------

void test_idle_waits_forever_and_recording_polls_every_20_ms() {
  Fixture fx;
  TEST_ASSERT_EQUAL_UINT32(kWaitForever, fx.svc.wait_ms(0));
  fx.start();
  TEST_ASSERT_EQUAL_UINT32(20, fx.svc.wait_ms(0));
}

void test_a_paused_capture_wakes_at_least_every_second_and_at_its_expiry() {
  Fixture fx;
  fx.start();
  fx.toggle(0);
  TEST_ASSERT_EQUAL_UINT32(1000, fx.svc.wait_ms(10));
  TEST_ASSERT_EQUAL_UINT32(300, fx.svc.wait_ms(rec::kMaxPauseMs - 300));
  TEST_ASSERT_EQUAL_UINT32(0, fx.svc.wait_ms(rec::kMaxPauseMs + 5));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_begin_removes_temp_leftovers_and_reports_a_stored_wav);
  RUN_TEST(test_begin_with_nothing_stored_is_silent);
  RUN_TEST(test_begin_reports_unavailable_storage);
  RUN_TEST(test_start_records_at_16_khz_for_as_long_as_the_free_space_allows);
  RUN_TEST(test_a_different_amount_of_free_space_gives_a_different_limit);
  RUN_TEST(test_start_without_space_reports_it_and_keeps_nothing_locked);
  RUN_TEST(test_start_while_storage_is_busy_reports_busy);
  RUN_TEST(test_start_with_a_wav_already_pending_fails_and_releases_the_lock);
  RUN_TEST(test_start_fails_cleanly_when_the_temp_file_cannot_be_created);
  RUN_TEST(test_start_fails_cleanly_when_the_microphone_cannot_start);
  RUN_TEST(test_a_second_start_while_recording_changes_nothing);
  RUN_TEST(test_poll_writes_samples_as_little_endian_pcm);
  RUN_TEST(test_elapsed_time_follows_the_bytes_written_and_updates_once_per_second);
  RUN_TEST(test_one_poll_drains_everything_the_microphone_has_buffered);
  RUN_TEST(test_a_backlog_longer_than_one_poll_is_continued_without_waiting);
  RUN_TEST(test_an_empty_read_changes_nothing);
  RUN_TEST(test_the_limit_ends_the_capture_at_the_exact_byte);
  RUN_TEST(test_a_write_failure_before_any_audio_aborts_and_reports);
  RUN_TEST(test_a_microphone_error_before_any_audio_aborts_and_reports);
  RUN_TEST(test_a_pause_that_fails_before_any_audio_aborts_the_capture);
  RUN_TEST(test_a_full_disk_mid_recording_saves_what_was_captured);
  RUN_TEST(test_a_microphone_error_mid_recording_saves_what_was_captured);
  RUN_TEST(test_a_pause_that_fails_mid_recording_saves_what_was_captured);
  RUN_TEST(test_when_saving_after_an_error_also_fails_the_failure_is_reported);
  RUN_TEST(test_poll_does_nothing_when_idle);
  RUN_TEST(test_stop_stores_the_recording_with_its_exact_length);
  RUN_TEST(test_stop_without_any_audio_discards_the_file);
  RUN_TEST(test_a_failed_finalize_removes_the_temp_file_and_reports);
  RUN_TEST(test_stop_with_nothing_recording_is_answered_so_the_screen_can_leave);
  RUN_TEST(test_stop_after_a_failed_start_is_answered_too);
  RUN_TEST(test_pause_stops_reading_and_resume_continues);
  RUN_TEST(test_toggle_without_a_recording_does_nothing);
  RUN_TEST(test_a_long_pause_saves_the_recording_and_frees_the_storage);
  RUN_TEST(test_delete_removes_the_stored_wav);
  RUN_TEST(test_a_failed_delete_is_reported_and_keeps_the_file);
  RUN_TEST(test_delete_is_ignored_while_recording);
  RUN_TEST(test_idle_waits_forever_and_recording_polls_every_20_ms);
  RUN_TEST(test_a_paused_capture_wakes_at_least_every_second_and_at_its_expiry);
  return UNITY_END();
}
