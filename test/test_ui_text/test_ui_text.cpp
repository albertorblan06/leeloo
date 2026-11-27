#include <unity.h>

#include <cstring>

#include "ui_text.h"

using namespace leeloo::ui;
using leeloo::settings::WifiMode;

void setUp() {}
void tearDown() {}

namespace {

// Characters, not bytes: á, ñ, ¿ take two bytes in UTF-8 but one OLED cell.
int chars(const char* s) {
  int n = 0;
  for (; *s != '\0'; ++s) {
    if ((static_cast<unsigned char>(*s) & 0xC0) != 0x80) ++n;
  }
  return n;
}

constexpr Message kAll[] = {
    Message::WifiOff,        Message::ApStartFailed,       Message::UploadSaved,
    Message::UploadTimedOut, Message::DeleteFailed,        Message::BookOpenFailed,
    Message::AudioStartFailed, Message::AudioBusy,         Message::AudioNoSpace,
    Message::AudioSaveFailed,  Message::AudioSaved,        Message::AudioDeleted,
    Message::AudioDeleteFailed, Message::AudioTransferred, Message::AudioUploadStartFailed,
    Message::AudioUploadTimedOut};

}  // namespace

void test_every_message_has_a_first_line() {
  for (const Message m : kAll) TEST_ASSERT_TRUE(chars(message_text(m).line1) > 0);
}

void test_every_message_line_fits_the_oled_row() {
  for (const Message m : kAll) {
    const MessageText t = message_text(m);
    TEST_ASSERT_TRUE(chars(t.line1) <= kMaxRowChars);
    TEST_ASSERT_TRUE(chars(t.line2) <= kMaxRowChars);
  }
}

void test_no_message_still_calls_the_recording_audio_or_the_device_equipo() {
  for (const Message m : kAll) {
    const MessageText t = message_text(m);
    TEST_ASSERT_NULL(std::strstr(t.line1, "audio"));
    TEST_ASSERT_NULL(std::strstr(t.line2, "audio"));
    TEST_ASSERT_NULL(std::strstr(t.line2, "equipo"));
  }
}

void test_none_has_no_text() {
  TEST_ASSERT_EQUAL_STRING("", message_text(Message::None).line1);
  TEST_ASSERT_EQUAL_STRING("", message_text(Message::None).line2);
}

void test_wifi_help_exists_for_every_mode_and_fits() {
  for (const WifiMode mode : {WifiMode::Off, WifiMode::Auto, WifiMode::On}) {
    const MessageText t = wifi_help(mode);
    TEST_ASSERT_TRUE(chars(t.line1) > 0);
    TEST_ASSERT_TRUE(chars(t.line1) <= kMaxRowChars);
    TEST_ASSERT_TRUE(chars(t.line2) <= kMaxRowChars);
  }
}

void test_character_counting_counts_accents_once() {
  TEST_ASSERT_EQUAL_INT(7, chars("Atrás ¿"));  // A t r á s space ¿
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_every_message_has_a_first_line);
  RUN_TEST(test_every_message_line_fits_the_oled_row);
  RUN_TEST(test_no_message_still_calls_the_recording_audio_or_the_device_equipo);
  RUN_TEST(test_none_has_no_text);
  RUN_TEST(test_wifi_help_exists_for_every_mode_and_fits);
  RUN_TEST(test_character_counting_counts_accents_once);
  return UNITY_END();
}
