#include <unity.h>

#include "ui.h"

using namespace leeloo::ui;
using leeloo::settings::BrightnessMode;
using leeloo::settings::Settings;
using leeloo::settings::WifiMode;

void setUp() {}
void tearDown() {}

namespace {

Ui make_ui(std::size_t books = 0, Settings s = Settings{}) {
  Ui u(s);
  u.set_book_count(books);
  return u;
}

Outcome press(Ui& u, Event e) { return u.handle(e); }

// Walks from the root menu to `target` (0 Leer, 1 Biblioteca, 2 Ajustes, 3 Info).
void open_root_item(Ui& u, std::size_t target) {
  while (u.cursor() != target) press(u, Event::Down);
  press(u, Event::Select);
}

// Biblioteca > Borrar: the second row of the Manage screen.
void open_delete_list(Ui& u) {
  open_root_item(u, 1);
  press(u, Event::Down);
  press(u, Event::Select);
}

}  // namespace

// --- root menu ---

void test_starts_on_root_with_first_item_focused() {
  Ui u = make_ui();
  TEST_ASSERT_EQUAL(Screen::Root, u.screen());
  TEST_ASSERT_EQUAL_UINT(0, u.cursor());
}

void test_root_cursor_wraps_at_both_ends() {
  Ui u = make_ui();
  press(u, Event::Up);
  TEST_ASSERT_EQUAL_UINT(3, u.cursor());
  press(u, Event::Down);
  TEST_ASSERT_EQUAL_UINT(0, u.cursor());
}

void test_root_back_is_a_no_op() {
  Ui u = make_ui();
  press(u, Event::Back);
  TEST_ASSERT_EQUAL(Screen::Root, u.screen());
}

// --- Leer ---

void test_read_with_no_books_stays_on_empty_list() {
  Ui u = make_ui(0);
  open_root_item(u, 0);
  TEST_ASSERT_EQUAL(Screen::Read, u.screen());
  const Outcome o = press(u, Event::Select);
  TEST_ASSERT_EQUAL(Command::None, o.command);
  TEST_ASSERT_EQUAL(Screen::Read, u.screen());
}

void test_selecting_a_book_opens_the_reading_screen() {
  Ui u = make_ui(3);
  open_root_item(u, 0);
  press(u, Event::Down);
  const Outcome o = press(u, Event::Select);
  TEST_ASSERT_EQUAL(Command::OpenBook, o.command);
  TEST_ASSERT_EQUAL_UINT(1, o.index);
  TEST_ASSERT_EQUAL(Screen::Reading, u.screen());
  TEST_ASSERT_EQUAL_UINT(1, u.reading_book());
}

void test_message_returns_to_the_screen_it_came_from() {
  Ui u = make_ui(1);
  open_delete_list(u);
  press(u, Event::DeleteFailed);
  TEST_ASSERT_EQUAL(Screen::Message, u.screen());
  press(u, Event::Select);
  TEST_ASSERT_EQUAL(Screen::DeleteList, u.screen());
}

void test_reading_screen_forwards_buttons_to_the_adapter() {
  Ui u = make_ui(2);
  open_root_item(u, 0);
  press(u, Event::Select);
  TEST_ASSERT_EQUAL(Command::ReadingUp, press(u, Event::Up).command);
  TEST_ASSERT_EQUAL(Command::ReadingDown, press(u, Event::Down).command);
  TEST_ASSERT_EQUAL(Command::ReadingSelect, press(u, Event::Select).command);
  TEST_ASSERT_EQUAL(Screen::Reading, u.screen());
}

void test_back_while_reading_returns_to_list_and_closes_book() {
  Ui u = make_ui(2);
  open_root_item(u, 0);
  press(u, Event::Select);
  const Outcome o = press(u, Event::Back);
  TEST_ASSERT_EQUAL(Command::CloseBook, o.command);
  TEST_ASSERT_EQUAL(Screen::Read, u.screen());
}

void test_finished_book_shows_end_menu_with_releer_first() {
  Ui u = make_ui(2);
  open_root_item(u, 0);
  press(u, Event::Down);
  press(u, Event::Select);
  press(u, Event::ReaderFinished);
  TEST_ASSERT_EQUAL(Screen::ReadEnd, u.screen());
  TEST_ASSERT_EQUAL_UINT(0, u.cursor());
}

void test_releer_reopens_the_same_book_from_the_start() {
  Ui u = make_ui(2);
  open_root_item(u, 0);
  press(u, Event::Down);
  press(u, Event::Select);
  press(u, Event::ReaderFinished);
  const Outcome o = press(u, Event::Select);
  TEST_ASSERT_EQUAL(Command::OpenBook, o.command);
  TEST_ASSERT_EQUAL_UINT(1, o.index);
  TEST_ASSERT_EQUAL(Screen::Reading, u.screen());
}

void test_volver_al_menu_closes_book_and_returns_to_list() {
  Ui u = make_ui(2);
  open_root_item(u, 0);
  press(u, Event::Select);
  press(u, Event::ReaderFinished);
  press(u, Event::Down);
  const Outcome o = press(u, Event::Select);
  TEST_ASSERT_EQUAL(Command::CloseBook, o.command);
  TEST_ASSERT_EQUAL(Screen::Read, u.screen());
}

void test_failed_open_returns_to_list_with_notice() {
  Ui u = make_ui(1);
  open_root_item(u, 0);
  press(u, Event::Select);
  press(u, Event::BookOpenFailed);
  TEST_ASSERT_EQUAL(Screen::Message, u.screen());
  TEST_ASSERT_EQUAL(Message::BookOpenFailed, u.message());
  press(u, Event::Select);
  TEST_ASSERT_EQUAL(Screen::Read, u.screen());
}

void test_back_from_end_menu_closes_book() {
  Ui u = make_ui(1);
  open_root_item(u, 0);
  press(u, Event::Select);
  press(u, Event::ReaderFinished);
  TEST_ASSERT_EQUAL(Command::CloseBook, press(u, Event::Back).command);
  TEST_ASSERT_EQUAL(Screen::Read, u.screen());
}

void test_back_from_read_returns_to_root() {
  Ui u = make_ui(2);
  open_root_item(u, 0);
  press(u, Event::Back);
  TEST_ASSERT_EQUAL(Screen::Root, u.screen());
}

// --- Biblioteca and Wi-Fi upload ---

void test_upload_with_wifi_off_does_not_start_and_says_so() {
  Settings s;
  s.wifi = WifiMode::Off;
  Ui u = make_ui(0, s);
  open_root_item(u, 1);
  const Outcome o = press(u, Event::Select);
  TEST_ASSERT_EQUAL(Command::None, o.command);
  TEST_ASSERT_EQUAL(Screen::Message, u.screen());
  TEST_ASSERT_EQUAL(Message::WifiOff, u.message());
}

void test_upload_with_wifi_auto_starts_the_session() {
  Ui u = make_ui();
  open_root_item(u, 1);
  const Outcome o = press(u, Event::Select);
  TEST_ASSERT_EQUAL(Command::StartUpload, o.command);
  TEST_ASSERT_EQUAL(Screen::Upload, u.screen());
}

void test_upload_with_wifi_on_starts_the_session() {
  Settings s;
  s.wifi = WifiMode::On;
  Ui u = make_ui(0, s);
  open_root_item(u, 1);
  TEST_ASSERT_EQUAL(Command::StartUpload, press(u, Event::Select).command);
}

void test_failed_ap_start_shows_error_and_returns_to_manage() {
  Ui u = make_ui();
  open_root_item(u, 1);
  press(u, Event::Select);
  press(u, Event::UploadStartFailed);
  TEST_ASSERT_EQUAL(Message::ApStartFailed, u.message());
  press(u, Event::Select);
  TEST_ASSERT_EQUAL(Screen::Manage, u.screen());
}

void test_back_during_upload_stops_session_and_returns_to_manage() {
  Ui u = make_ui();
  open_root_item(u, 1);
  press(u, Event::Select);
  const Outcome o = press(u, Event::Back);
  TEST_ASSERT_EQUAL(Command::StopUpload, o.command);
  TEST_ASSERT_EQUAL(Screen::Manage, u.screen());
}

void test_saved_upload_shows_success_message() {
  Ui u = make_ui();
  open_root_item(u, 1);
  press(u, Event::Select);
  press(u, Event::UploadSaved);
  TEST_ASSERT_EQUAL(Message::UploadSaved, u.message());
}

void test_timed_out_upload_shows_timeout_message() {
  Ui u = make_ui();
  open_root_item(u, 1);
  press(u, Event::Select);
  press(u, Event::UploadTimedOut);
  TEST_ASSERT_EQUAL(Message::UploadTimedOut, u.message());
}

// --- Borrar ---

void test_delete_list_with_no_books_does_nothing() {
  Ui u = make_ui(0);
  open_delete_list(u);
  const Outcome o = press(u, Event::Select);
  TEST_ASSERT_EQUAL(Command::None, o.command);
  TEST_ASSERT_EQUAL(Screen::DeleteList, u.screen());
}

void test_delete_confirm_focuses_cancel_by_default() {
  Ui u = make_ui(2);
  open_delete_list(u);
  press(u, Event::Down);  // book 1
  press(u, Event::Select);
  TEST_ASSERT_EQUAL(Screen::DeleteConfirm, u.screen());
  TEST_ASSERT_EQUAL_UINT(0, u.cursor());
}

void test_cancel_in_delete_confirm_deletes_nothing() {
  Ui u = make_ui(2);
  open_delete_list(u);
  press(u, Event::Select);  // confirm for book 0
  const Outcome o = press(u, Event::Select);  // Cancelar is focused
  TEST_ASSERT_EQUAL(Command::None, o.command);
  TEST_ASSERT_EQUAL(Screen::DeleteList, u.screen());
}

void test_confirming_delete_asks_to_remove_the_selected_book() {
  Ui u = make_ui(3);
  open_delete_list(u);
  press(u, Event::Down);  // book 1
  press(u, Event::Select);
  press(u, Event::Down);  // focus Borrar
  const Outcome o = press(u, Event::Select);
  TEST_ASSERT_EQUAL(Command::DeleteBook, o.command);
  TEST_ASSERT_EQUAL_UINT(1, o.index);
}

void test_failed_delete_keeps_list_and_shows_error() {
  Ui u = make_ui(2);
  open_delete_list(u);
  press(u, Event::Select);  // confirm for book 0
  press(u, Event::Down);    // focus Borrar
  press(u, Event::Select);
  press(u, Event::DeleteFailed);
  TEST_ASSERT_EQUAL(Message::DeleteFailed, u.message());
  press(u, Event::Select);
  TEST_ASSERT_EQUAL(Screen::DeleteList, u.screen());
}

// --- Ajustes ---

void test_brightness_mode_toggles_and_saves() {
  Ui u = make_ui();
  open_root_item(u, 2);
  open_root_item(u, 0);
  const Outcome o = press(u, Event::Left);
  TEST_ASSERT_EQUAL(Command::SaveSettings, o.command);
  TEST_ASSERT_EQUAL(BrightnessMode::Auto, u.settings().brightness_mode);
}

void test_manual_level_is_hidden_in_auto_mode() {
  Settings s;
  s.brightness_mode = BrightnessMode::Auto;
  Ui u = make_ui(0, s);
  open_root_item(u, 2);
  open_root_item(u, 0);
  press(u, Event::Down);  // only one row exists in Auto
  TEST_ASSERT_EQUAL_UINT(0, u.cursor());
}

void test_auto_brightness_reports_missing_sensor() {
  Settings s;
  s.brightness_mode = BrightnessMode::Auto;
  Ui u = make_ui(0, s);
  TEST_ASSERT_TRUE(u.sensor_unavailable());
}

void test_manual_level_steps_by_five_within_range() {
  Ui u = make_ui();
  open_root_item(u, 2);
  open_root_item(u, 0);
  press(u, Event::Down);
  press(u, Event::Left);
  TEST_ASSERT_EQUAL_UINT8(95, u.settings().brightness_percent);
  for (int i = 0; i < 30; ++i) press(u, Event::Left);
  TEST_ASSERT_EQUAL_UINT8(10, u.settings().brightness_percent);
}

void test_contrast_steps_by_five_and_saves() {
  Ui u = make_ui();
  open_root_item(u, 2);
  press(u, Event::Down);
  open_root_item(u, 1);
  const Outcome o = press(u, Event::Left);  // 100 -> 95
  TEST_ASSERT_EQUAL(Command::SaveSettings, o.command);
  TEST_ASSERT_EQUAL_UINT8(95, u.settings().contrast_percent);
  press(u, Event::Right);
  TEST_ASSERT_EQUAL_UINT8(100, u.settings().contrast_percent);
}

void test_pressing_past_the_limit_changes_nothing_and_saves_nothing() {
  Ui u = make_ui();
  open_root_item(u, 2);
  press(u, Event::Down);
  open_root_item(u, 1);  // Contraste at 100
  const Outcome o = press(u, Event::Right);
  TEST_ASSERT_EQUAL(Command::None, o.command);
  TEST_ASSERT_EQUAL_UINT8(100, u.settings().contrast_percent);
}

void test_wifi_setting_cycles_through_three_modes() {
  Ui u = make_ui();
  open_root_item(u, 2);
  press(u, Event::Down);
  press(u, Event::Down);
  open_root_item(u, 2);
  TEST_ASSERT_EQUAL(WifiMode::Auto, u.settings().wifi);
  press(u, Event::Right);
  TEST_ASSERT_EQUAL(WifiMode::On, u.settings().wifi);
  press(u, Event::Right);
  TEST_ASSERT_EQUAL(WifiMode::Off, u.settings().wifi);
}

// --- three-button editing: SELECT toggles edit mode, UP/DOWN adjust ---

// Ajustes > the given row (0 Brillo, 1 Contraste, 2 Wi-Fi), without entering edit mode.
void open_setting(Ui& u, std::size_t row) {
  open_root_item(u, 2);
  open_root_item(u, row);
}

void test_edit_mode_is_off_when_a_setting_screen_opens() {
  Ui u = make_ui();
  open_setting(u, 1);
  TEST_ASSERT_FALSE(u.editing());
}

void test_up_and_down_do_not_change_a_value_until_editing() {
  Ui u = make_ui();
  open_setting(u, 1);
  TEST_ASSERT_EQUAL(Command::None, press(u, Event::Down).command);
  TEST_ASSERT_EQUAL_UINT8(100, u.settings().contrast_percent);
}

void test_select_starts_editing_and_down_lowers_the_value() {
  Ui u = make_ui();
  open_setting(u, 1);
  press(u, Event::Select);
  TEST_ASSERT_TRUE(u.editing());
  const Outcome o = press(u, Event::Down);
  TEST_ASSERT_EQUAL(Command::SaveSettings, o.command);
  TEST_ASSERT_EQUAL_UINT8(95, u.settings().contrast_percent);
  press(u, Event::Up);
  TEST_ASSERT_EQUAL_UINT8(100, u.settings().contrast_percent);
}

void test_select_again_stops_editing() {
  Ui u = make_ui();
  open_setting(u, 1);
  press(u, Event::Select);
  press(u, Event::Select);
  TEST_ASSERT_FALSE(u.editing());
  TEST_ASSERT_EQUAL(Command::None, press(u, Event::Down).command);
}

void test_back_while_editing_only_stops_editing() {
  Ui u = make_ui();
  open_setting(u, 1);
  press(u, Event::Select);
  press(u, Event::Back);
  TEST_ASSERT_EQUAL(Screen::Contrast, u.screen());
  TEST_ASSERT_FALSE(u.editing());
  press(u, Event::Back);
  TEST_ASSERT_EQUAL(Screen::Settings, u.screen());
}

void test_wifi_mode_changes_with_up_while_editing() {
  Ui u = make_ui();
  open_setting(u, 2);
  press(u, Event::Select);
  press(u, Event::Up);
  TEST_ASSERT_EQUAL(WifiMode::On, u.settings().wifi);
}

void test_brightness_rows_are_chosen_before_editing() {
  Ui u = make_ui();
  open_setting(u, 0);
  press(u, Event::Down);  // moves to the Nivel row; no editing yet
  TEST_ASSERT_EQUAL_UINT(1, u.cursor());
  press(u, Event::Select);
  press(u, Event::Down);
  TEST_ASSERT_EQUAL_UINT8(95, u.settings().brightness_percent);
}

void test_changing_brightness_mode_keeps_editing_on() {
  Ui u = make_ui();
  open_setting(u, 0);
  press(u, Event::Select);  // edit the Modo row
  press(u, Event::Up);      // Manual -> Auto
  TEST_ASSERT_EQUAL(BrightnessMode::Auto, u.settings().brightness_mode);
  TEST_ASSERT_TRUE(u.editing());
}

void test_menus_still_move_with_up_and_down() {
  Ui u = make_ui();
  press(u, Event::Down);
  TEST_ASSERT_EQUAL_UINT(1, u.cursor());
}

// --- Info and navigation ---

void test_about_is_read_only() {
  Ui u = make_ui();
  open_root_item(u, 3);
  TEST_ASSERT_EQUAL(Command::None, press(u, Event::Left).command);
  TEST_ASSERT_EQUAL(Screen::About, u.screen());
}

void test_long_back_climbs_one_level_at_a_time() {
  Ui u = make_ui(1);
  open_root_item(u, 1);
  press(u, Event::Select);  // Subir: enters Upload
  press(u, Event::Back);    // Upload back to Manage
  press(u, Event::Back);    // Manage back to Root
  TEST_ASSERT_EQUAL(Screen::Root, u.screen());
}

void test_library_change_cancels_an_open_delete_confirm() {
  Ui u = make_ui(3);
  open_delete_list(u);
  press(u, Event::Down);
  press(u, Event::Select);  // confirm for book 1
  u.set_book_count(2);      // an upload changed the library underneath
  TEST_ASSERT_EQUAL(Screen::DeleteList, u.screen());
}

void test_book_count_change_resets_list_cursor() {
  Ui u = make_ui(3);
  open_root_item(u, 0);
  press(u, Event::Down);
  press(u, Event::Down);
  u.set_book_count(1);
  TEST_ASSERT_EQUAL_UINT(0, u.cursor());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_starts_on_root_with_first_item_focused);
  RUN_TEST(test_root_cursor_wraps_at_both_ends);
  RUN_TEST(test_root_back_is_a_no_op);
  RUN_TEST(test_read_with_no_books_stays_on_empty_list);
  RUN_TEST(test_selecting_a_book_opens_the_reading_screen);
  RUN_TEST(test_message_returns_to_the_screen_it_came_from);
  RUN_TEST(test_reading_screen_forwards_buttons_to_the_adapter);
  RUN_TEST(test_back_while_reading_returns_to_list_and_closes_book);
  RUN_TEST(test_finished_book_shows_end_menu_with_releer_first);
  RUN_TEST(test_releer_reopens_the_same_book_from_the_start);
  RUN_TEST(test_volver_al_menu_closes_book_and_returns_to_list);
  RUN_TEST(test_failed_open_returns_to_list_with_notice);
  RUN_TEST(test_back_from_end_menu_closes_book);
  RUN_TEST(test_back_from_read_returns_to_root);
  RUN_TEST(test_upload_with_wifi_off_does_not_start_and_says_so);
  RUN_TEST(test_upload_with_wifi_auto_starts_the_session);
  RUN_TEST(test_upload_with_wifi_on_starts_the_session);
  RUN_TEST(test_failed_ap_start_shows_error_and_returns_to_manage);
  RUN_TEST(test_back_during_upload_stops_session_and_returns_to_manage);
  RUN_TEST(test_saved_upload_shows_success_message);
  RUN_TEST(test_timed_out_upload_shows_timeout_message);
  RUN_TEST(test_delete_list_with_no_books_does_nothing);
  RUN_TEST(test_delete_confirm_focuses_cancel_by_default);
  RUN_TEST(test_cancel_in_delete_confirm_deletes_nothing);
  RUN_TEST(test_confirming_delete_asks_to_remove_the_selected_book);
  RUN_TEST(test_failed_delete_keeps_list_and_shows_error);
  RUN_TEST(test_brightness_mode_toggles_and_saves);
  RUN_TEST(test_manual_level_is_hidden_in_auto_mode);
  RUN_TEST(test_auto_brightness_reports_missing_sensor);
  RUN_TEST(test_manual_level_steps_by_five_within_range);
  RUN_TEST(test_contrast_steps_by_five_and_saves);
  RUN_TEST(test_pressing_past_the_limit_changes_nothing_and_saves_nothing);
  RUN_TEST(test_wifi_setting_cycles_through_three_modes);
  RUN_TEST(test_edit_mode_is_off_when_a_setting_screen_opens);
  RUN_TEST(test_up_and_down_do_not_change_a_value_until_editing);
  RUN_TEST(test_select_starts_editing_and_down_lowers_the_value);
  RUN_TEST(test_select_again_stops_editing);
  RUN_TEST(test_back_while_editing_only_stops_editing);
  RUN_TEST(test_wifi_mode_changes_with_up_while_editing);
  RUN_TEST(test_brightness_rows_are_chosen_before_editing);
  RUN_TEST(test_changing_brightness_mode_keeps_editing_on);
  RUN_TEST(test_menus_still_move_with_up_and_down);
  RUN_TEST(test_about_is_read_only);
  RUN_TEST(test_long_back_climbs_one_level_at_a_time);
  RUN_TEST(test_library_change_cancels_an_open_delete_confirm);
  RUN_TEST(test_book_count_change_resets_list_cursor);
  return UNITY_END();
}
