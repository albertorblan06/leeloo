#include <unity.h>

#include "ui.h"

using namespace leeloo::ui;
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

// Walks from the root menu to `target` (0 Leer, 1 Biblioteca, 2 Grabadora, 3 Ajustes, 4 Info).
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
  TEST_ASSERT_EQUAL_UINT(4, u.cursor());
  press(u, Event::Down);
  TEST_ASSERT_EQUAL_UINT(0, u.cursor());
}

// --- Apagar ---

void test_a_long_press_on_the_root_asks_before_switching_off() {
  Ui u = make_ui();
  const Outcome o = press(u, Event::Back);
  TEST_ASSERT_EQUAL(Command::None, o.command);
  TEST_ASSERT_EQUAL(Screen::PowerConfirm, u.screen());
  TEST_ASSERT_EQUAL_UINT(0, u.cursor());  // No is focused: a stray long press never switches off
  TEST_ASSERT_FALSE(u.back_focused());
}

void test_choosing_no_returns_to_the_root_menu() {
  Ui u = make_ui();
  press(u, Event::Back);
  press(u, Event::Select);  // No
  TEST_ASSERT_EQUAL(Screen::Root, u.screen());
  TEST_ASSERT_TRUE(u.back_navigation());
  TEST_ASSERT_EQUAL_UINT(0, u.cursor());
}

void test_a_long_press_on_the_confirmation_cancels_it() {
  Ui u = make_ui();
  press(u, Event::Back);
  press(u, Event::Back);
  TEST_ASSERT_EQUAL(Screen::Root, u.screen());
}

void test_choosing_si_apagar_switches_off() {
  Ui u = make_ui();
  press(u, Event::Back);
  press(u, Event::Down);  // Sí, apagar
  TEST_ASSERT_EQUAL_UINT(1, u.cursor());
  press(u, Event::Select);
  TEST_ASSERT_EQUAL(Screen::PoweringOff, u.screen());
}

void test_the_two_confirmation_rows_wrap() {
  Ui u = make_ui();
  press(u, Event::Back);
  press(u, Event::Up);
  TEST_ASSERT_EQUAL_UINT(1, u.cursor());
  press(u, Event::Down);
  TEST_ASSERT_EQUAL_UINT(0, u.cursor());
}

void test_held_buttons_do_nothing_on_the_confirmation() {
  Ui u = make_ui();
  press(u, Event::Back);
  press(u, Event::DownRepeat);
  TEST_ASSERT_EQUAL_UINT(0, u.cursor());
  TEST_ASSERT_EQUAL(Screen::PowerConfirm, u.screen());
}

void test_nothing_interrupts_the_power_off_screen() {
  Ui u = make_ui();
  press(u, Event::Back);
  press(u, Event::Down);
  press(u, Event::Select);
  for (const Event e : {Event::Up, Event::Down, Event::Select, Event::Back, Event::UpRepeat,
                        Event::AudioAvailable, Event::UploadTimedOut, Event::AudioTransferred}) {
    const Outcome o = press(u, e);
    TEST_ASSERT_EQUAL(Command::None, o.command);
    TEST_ASSERT_EQUAL(Screen::PoweringOff, u.screen());
  }
}

void test_a_long_press_elsewhere_still_just_goes_back() {
  Ui u = make_ui(1);
  open_root_item(u, 0);  // Leer
  press(u, Event::Back);
  TEST_ASSERT_EQUAL(Screen::Root, u.screen());  // one level, not the power question
  press(u, Event::Back);
  TEST_ASSERT_EQUAL(Screen::PowerConfirm, u.screen());  // only on the root
}

// --- Leer ---

void test_read_with_no_books_offers_only_the_back_row() {
  Ui u = make_ui(0);
  open_root_item(u, 0);
  TEST_ASSERT_EQUAL(Screen::Read, u.screen());
  TEST_ASSERT_TRUE(u.back_focused());
  const Outcome o = press(u, Event::Select);
  TEST_ASSERT_EQUAL(Command::None, o.command);  // nothing to open
  TEST_ASSERT_EQUAL(Screen::Root, u.screen());
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

void test_delete_list_with_no_books_offers_only_the_back_row() {
  Ui u = make_ui(0);
  open_delete_list(u);
  TEST_ASSERT_TRUE(u.back_focused());
  const Outcome o = press(u, Event::Select);
  TEST_ASSERT_EQUAL(Command::None, o.command);  // nothing to delete
  TEST_ASSERT_EQUAL(Screen::Manage, u.screen());
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

void test_brightness_steps_by_five_within_range() {
  Ui u = make_ui();
  open_root_item(u, 3);
  open_root_item(u, 0);
  press(u, Event::Left);
  TEST_ASSERT_EQUAL_UINT8(95, u.settings().brightness_percent);
  for (int i = 0; i < 30; ++i) press(u, Event::Left);
  TEST_ASSERT_EQUAL_UINT8(10, u.settings().brightness_percent);
}

void test_brightness_saves_and_returns_with_the_opposite_step() {
  Ui u = make_ui();
  open_root_item(u, 3);
  open_root_item(u, 0);
  const Outcome o = press(u, Event::Left);  // 100 -> 95
  TEST_ASSERT_EQUAL(Command::SaveSettings, o.command);
  TEST_ASSERT_EQUAL_UINT8(95, u.settings().brightness_percent);
  press(u, Event::Right);
  TEST_ASSERT_EQUAL_UINT8(100, u.settings().brightness_percent);
}

void test_pressing_past_the_limit_changes_nothing_and_saves_nothing() {
  Ui u = make_ui();
  open_root_item(u, 3);
  open_root_item(u, 0);  // Brillo at 100
  const Outcome o = press(u, Event::Right);
  TEST_ASSERT_EQUAL(Command::None, o.command);
  TEST_ASSERT_EQUAL_UINT8(100, u.settings().brightness_percent);
}

void test_wifi_setting_cycles_through_three_modes() {
  Ui u = make_ui();
  open_root_item(u, 3);  // Ajustes
  press(u, Event::Down);
  press(u, Event::Down);
  open_root_item(u, 2);  // Wi-Fi
  TEST_ASSERT_EQUAL(WifiMode::Auto, u.settings().wifi);
  press(u, Event::Right);
  TEST_ASSERT_EQUAL(WifiMode::On, u.settings().wifi);
  press(u, Event::Right);
  TEST_ASSERT_EQUAL(WifiMode::Off, u.settings().wifi);
}

// --- three-button editing: SELECT toggles edit mode, UP/DOWN adjust ---

// Ajustes > the given row (0 Brillo, 1 Salto por pulsación, 2 Wi-Fi), without entering edit mode.
void open_setting(Ui& u, std::size_t row) {
  open_root_item(u, 3);
  open_root_item(u, row);
}

void test_edit_mode_is_off_when_a_setting_screen_opens() {
  Ui u = make_ui();
  open_setting(u, 0);
  TEST_ASSERT_FALSE(u.editing());
}

void test_up_and_down_do_not_change_a_value_until_editing() {
  Ui u = make_ui();
  open_setting(u, 0);
  TEST_ASSERT_EQUAL(Command::None, press(u, Event::Down).command);
  TEST_ASSERT_EQUAL_UINT8(100, u.settings().brightness_percent);
}

void test_select_starts_editing_and_down_lowers_the_value() {
  Ui u = make_ui();
  open_setting(u, 0);
  press(u, Event::Select);
  TEST_ASSERT_TRUE(u.editing());
  const Outcome o = press(u, Event::Down);
  TEST_ASSERT_EQUAL(Command::SaveSettings, o.command);
  TEST_ASSERT_EQUAL_UINT8(95, u.settings().brightness_percent);
  press(u, Event::Up);
  TEST_ASSERT_EQUAL_UINT8(100, u.settings().brightness_percent);
}

void test_select_again_stops_editing() {
  Ui u = make_ui();
  open_setting(u, 0);
  press(u, Event::Select);
  press(u, Event::Select);
  TEST_ASSERT_FALSE(u.editing());
  TEST_ASSERT_EQUAL(Command::None, press(u, Event::Down).command);
}

void test_back_while_editing_only_stops_editing() {
  Ui u = make_ui();
  open_setting(u, 0);
  press(u, Event::Select);
  press(u, Event::Back);
  TEST_ASSERT_EQUAL(Screen::Brightness, u.screen());
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

void test_menus_still_move_with_up_and_down() {
  Ui u = make_ui();
  press(u, Event::Down);
  TEST_ASSERT_EQUAL_UINT(1, u.cursor());
}

// --- Info and navigation ---

void test_about_is_read_only() {
  Ui u = make_ui();
  open_root_item(u, 4);
  TEST_ASSERT_EQUAL(Command::None, press(u, Event::Left).command);
  TEST_ASSERT_EQUAL(Screen::About, u.screen());
}

void test_long_back_climbs_one_level_at_a_time() {
  Ui u = make_ui(1);
  open_root_item(u, 1);
  press(u, Event::Select);  // Añadir libro: enters Upload
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

// --- reading speed step and held-button repeats ---

void test_speed_step_screen_edits_the_step_and_saves() {
  Ui u = make_ui();
  open_setting(u, 1);
  TEST_ASSERT_EQUAL(Screen::SpeedStep, u.screen());
  TEST_ASSERT_EQUAL_UINT16(25, u.settings().reading_step_ppm);
  press(u, Event::Select);  // start editing
  Outcome out = press(u, Event::Up);
  TEST_ASSERT_EQUAL(Command::SaveSettings, out.command);
  TEST_ASSERT_EQUAL_UINT16(50, u.settings().reading_step_ppm);
  press(u, Event::Up);
  TEST_ASSERT_EQUAL_UINT16(100, u.settings().reading_step_ppm);
  out = press(u, Event::Up);  // already at the top: nothing to save
  TEST_ASSERT_EQUAL(Command::None, out.command);
  press(u, Event::Down);
  TEST_ASSERT_EQUAL_UINT16(50, u.settings().reading_step_ppm);
}

void test_speed_step_does_not_change_until_editing() {
  Ui u = make_ui();
  open_setting(u, 1);
  const Outcome out = press(u, Event::Up);
  TEST_ASSERT_EQUAL(Command::None, out.command);
  TEST_ASSERT_EQUAL_UINT16(25, u.settings().reading_step_ppm);
}

void test_back_from_speed_step_returns_to_ajustes() {
  Ui u = make_ui();
  open_setting(u, 1);
  press(u, Event::Back);
  TEST_ASSERT_EQUAL(Screen::Settings, u.screen());
}

void test_ajustes_lists_three_settings_and_back_and_wraps() {
  Ui u = make_ui();
  open_root_item(u, 3);
  press(u, Event::Up);
  TEST_ASSERT_EQUAL_UINT(3, u.cursor());
  TEST_ASSERT_TRUE(u.back_focused());
  press(u, Event::Down);
  TEST_ASSERT_EQUAL_UINT(0, u.cursor());
}

void test_constructor_sanitizes_an_unsupported_reading_step() {
  Settings s;
  s.reading_step_ppm = 33;
  Ui u = make_ui(0, s);
  TEST_ASSERT_EQUAL_UINT16(25, u.settings().reading_step_ppm);
}

void test_held_buttons_do_not_move_menu_cursors() {
  Ui u = make_ui(3);
  press(u, Event::DownRepeat);
  press(u, Event::UpRepeat);
  press(u, Event::DownRepeat);
  TEST_ASSERT_EQUAL_UINT(0, u.cursor());
  open_root_item(u, 0);  // Leer: a list of books
  press(u, Event::DownRepeat);
  TEST_ASSERT_EQUAL_UINT(0, u.cursor());
}

void test_held_buttons_repeat_a_number_only_while_editing() {
  Ui u = make_ui();
  open_setting(u, 0);  // Brillo
  Outcome out = press(u, Event::DownRepeat);
  TEST_ASSERT_EQUAL(Command::None, out.command);
  TEST_ASSERT_EQUAL_UINT8(100, u.settings().brightness_percent);
  press(u, Event::Select);  // edit
  out = press(u, Event::DownRepeat);
  TEST_ASSERT_EQUAL(Command::SaveSettings, out.command);
  TEST_ASSERT_EQUAL_UINT8(95, u.settings().brightness_percent);
}

void test_held_buttons_do_not_repeat_a_choice() {
  Settings s;
  s.wifi = leeloo::settings::WifiMode::Auto;
  Ui u = make_ui(0, s);
  open_setting(u, 2);  // Wi-Fi
  press(u, Event::Select);
  const Outcome out = press(u, Event::UpRepeat);
  TEST_ASSERT_EQUAL(Command::None, out.command);
  TEST_ASSERT_EQUAL(leeloo::settings::WifiMode::Auto, u.settings().wifi);
}

void test_held_buttons_repeat_the_brightness_level() {
  Ui u = make_ui();
  open_setting(u, 0);
  press(u, Event::Select);  // edit
  const Outcome out = press(u, Event::DownRepeat);
  TEST_ASSERT_EQUAL(Command::SaveSettings, out.command);
  TEST_ASSERT_EQUAL_UINT8(95, u.settings().brightness_percent);
}

void test_held_buttons_while_reading_are_forwarded() {
  Ui u = make_ui(1);
  open_root_item(u, 0);
  press(u, Event::Select);  // book 0: Reading
  TEST_ASSERT_EQUAL(Command::ReadingUpRepeat, press(u, Event::UpRepeat).command);
  TEST_ASSERT_EQUAL(Command::ReadingDownRepeat, press(u, Event::DownRepeat).command);
}

// --- fixed `< Atrás` row and slide direction ---

// Moves the cursor onto the last row (Up wraps to it), selects it and checks where it leads.
void expect_back_row_returns_to(Ui& u, Screen parent) {
  press(u, Event::Up);
  TEST_ASSERT_TRUE(u.back_focused());
  press(u, Event::Select);
  TEST_ASSERT_EQUAL(parent, u.screen());
  TEST_ASSERT_TRUE(u.back_navigation());
}

void test_every_secondary_screen_ends_with_a_back_row_to_its_parent() {
  {
    Ui u = make_ui(2);
    open_root_item(u, 0);
    expect_back_row_returns_to(u, Screen::Root);
  }
  {
    Ui u = make_ui();
    open_root_item(u, 1);
    expect_back_row_returns_to(u, Screen::Root);
  }
  {
    Ui u = make_ui();
    open_root_item(u, 3);
    expect_back_row_returns_to(u, Screen::Root);
  }
  {
    Ui u = make_ui();
    open_root_item(u, 4);
    expect_back_row_returns_to(u, Screen::Root);
  }
  {
    Ui u = make_ui(2);
    open_delete_list(u);
    expect_back_row_returns_to(u, Screen::Manage);
  }
  for (std::size_t row = 0; row < 3; ++row) {
    Ui u = make_ui();
    open_setting(u, row);
    expect_back_row_returns_to(u, Screen::Settings);
  }
}

void test_book_list_back_row_comes_after_the_last_book() {
  Ui u = make_ui(2);
  open_root_item(u, 0);
  press(u, Event::Down);
  TEST_ASSERT_FALSE(u.back_focused());  // the second book
  press(u, Event::Down);
  TEST_ASSERT_TRUE(u.back_focused());
  TEST_ASSERT_EQUAL_UINT(2, u.cursor());
  const Outcome o = press(u, Event::Select);
  TEST_ASSERT_EQUAL(Command::None, o.command);  // Atrás never opens a book
  TEST_ASSERT_EQUAL(Screen::Root, u.screen());
}

void test_select_on_the_upload_screen_cancels_the_session() {
  Ui u = make_ui();
  open_root_item(u, 1);
  press(u, Event::Select);  // Añadir libro
  TEST_ASSERT_EQUAL(Screen::Upload, u.screen());
  TEST_ASSERT_TRUE(u.back_focused());
  const Outcome o = press(u, Event::Select);
  TEST_ASSERT_EQUAL(Command::StopUpload, o.command);
  TEST_ASSERT_EQUAL(Screen::Manage, u.screen());
  TEST_ASSERT_TRUE(u.back_navigation());
}

void test_the_back_row_has_no_value_to_edit_or_change() {
  Ui u = make_ui();
  open_setting(u, 0);
  press(u, Event::Up);  // wraps to Atrás
  TEST_ASSERT_TRUE(u.back_focused());
  TEST_ASSERT_EQUAL(Command::None, press(u, Event::Right).command);
  TEST_ASSERT_EQUAL(Command::None, press(u, Event::Left).command);
  TEST_ASSERT_EQUAL_UINT8(100, u.settings().brightness_percent);
}

void test_forward_moves_are_not_back_navigation_and_back_moves_are() {
  Ui u = make_ui(1);
  open_root_item(u, 0);
  TEST_ASSERT_FALSE(u.back_navigation());
  press(u, Event::Back);
  TEST_ASSERT_TRUE(u.back_navigation());
  open_root_item(u, 1);
  TEST_ASSERT_FALSE(u.back_navigation());
}

void test_dismissing_a_message_slides_back() {
  Settings s;
  s.wifi = WifiMode::Off;
  Ui u = make_ui(0, s);
  open_root_item(u, 1);
  press(u, Event::Select);  // Añadir libro with Wi-Fi off: message
  TEST_ASSERT_EQUAL(Screen::Message, u.screen());
  TEST_ASSERT_FALSE(u.back_navigation());
  press(u, Event::Select);
  TEST_ASSERT_EQUAL(Screen::Manage, u.screen());
  TEST_ASSERT_TRUE(u.back_navigation());
}

// --- Grabadora ---

// Root > Grabadora (third row).
void open_record_menu(Ui& u) { open_root_item(u, 2); }

// Grabadora > Nueva grabación: the screen opens at once and the recorder is asked to start.
Outcome start_recording(Ui& u) {
  open_record_menu(u);
  return press(u, Event::Select);
}

void test_grabar_is_the_third_root_item_and_opens_the_record_menu() {
  Ui u = make_ui();
  open_record_menu(u);
  TEST_ASSERT_EQUAL(Screen::RecordMenu, u.screen());
}

void test_record_menu_without_a_wav_offers_grabar_then_back() {
  Ui u = make_ui();
  open_record_menu(u);
  press(u, Event::Up);
  TEST_ASSERT_TRUE(u.back_focused());
  TEST_ASSERT_EQUAL_UINT(1, u.cursor());  // Nueva grabación, Atrás
  press(u, Event::Select);
  TEST_ASSERT_EQUAL(Screen::Root, u.screen());
}

void test_grabar_enters_the_recording_screen_and_starts_the_recorder() {
  Ui u = make_ui();
  const Outcome o = start_recording(u);
  TEST_ASSERT_EQUAL(Command::StartRecording, o.command);
  TEST_ASSERT_EQUAL(Screen::Recording, u.screen());
}

void test_recording_select_toggles_pause_and_detener_stops() {
  Ui u = make_ui();
  start_recording(u);
  TEST_ASSERT_EQUAL(Command::ToggleRecordingPause, press(u, Event::Select).command);
  press(u, Event::Down);
  TEST_ASSERT_EQUAL(Command::StopRecording, press(u, Event::Select).command);
  TEST_ASSERT_EQUAL(Screen::Recording, u.screen());  // it leaves when the recorder answers
}

void test_a_long_press_while_recording_asks_to_stop() {
  Ui u = make_ui();
  start_recording(u);
  TEST_ASSERT_EQUAL(Command::StopRecording, press(u, Event::Back).command);
  TEST_ASSERT_EQUAL(Screen::Recording, u.screen());
}

void test_held_buttons_do_nothing_while_recording() {
  Ui u = make_ui();
  start_recording(u);
  TEST_ASSERT_EQUAL(Command::None, press(u, Event::UpRepeat).command);
  TEST_ASSERT_EQUAL(Screen::Recording, u.screen());
}

void test_saved_audio_returns_to_the_menu_with_the_wav_available() {
  Ui u = make_ui();
  start_recording(u);
  press(u, Event::AudioSaved);
  TEST_ASSERT_EQUAL(Screen::Message, u.screen());
  TEST_ASSERT_EQUAL(Message::AudioSaved, u.message());
  TEST_ASSERT_TRUE(u.recording_available());
  press(u, Event::Select);  // dismiss
  TEST_ASSERT_EQUAL(Screen::RecordMenu, u.screen());
  TEST_ASSERT_EQUAL_UINT(0, u.cursor());  // Enviar al móvil, Borrar audio, Atrás
  press(u, Event::Up);
  TEST_ASSERT_EQUAL_UINT(2, u.cursor());
  TEST_ASSERT_TRUE(u.back_focused());
}

void test_every_start_or_save_failure_leaves_the_recording_screen_with_its_message() {
  const struct {
    Event event;
    Message message;
  } cases[] = {{Event::AudioStartFailed, Message::AudioStartFailed},
               {Event::AudioBusy, Message::AudioBusy},
               {Event::AudioNoSpace, Message::AudioNoSpace},
               {Event::AudioSaveFailed, Message::AudioSaveFailed}};
  for (const auto& c : cases) {
    Ui u = make_ui();
    start_recording(u);
    press(u, c.event);
    TEST_ASSERT_EQUAL(Screen::Message, u.screen());
    TEST_ASSERT_EQUAL((int)c.message, (int)u.message());
    TEST_ASSERT_FALSE(u.recording_available());
    press(u, Event::Select);
    TEST_ASSERT_EQUAL(Screen::RecordMenu, u.screen());
    TEST_ASSERT_TRUE(u.back_navigation());
  }
}

void test_recording_ended_frees_a_screen_waiting_for_a_stop() {
  Ui u = make_ui();
  start_recording(u);
  press(u, Event::RecordingEnded);
  TEST_ASSERT_EQUAL(Screen::RecordMenu, u.screen());
}

// A pending WAV, as reported by the recorder at boot.
Ui ui_with_wav() {
  Ui u = make_ui();
  press(u, Event::AudioAvailable);
  return u;
}

void test_boot_report_of_a_wav_changes_the_menu_rows() {
  Ui u = ui_with_wav();
  open_record_menu(u);
  press(u, Event::Down);
  TEST_ASSERT_EQUAL_UINT(1, u.cursor());  // Borrar grabación is the second row
  press(u, Event::Down);
  TEST_ASSERT_TRUE(u.back_focused());
}

void test_subir_audio_starts_the_wifi_session_and_atras_cancels_it() {
  Ui u = ui_with_wav();
  open_record_menu(u);
  TEST_ASSERT_EQUAL(Command::StartRecordingUpload, press(u, Event::Select).command);
  TEST_ASSERT_EQUAL(Screen::RecordingUpload, u.screen());
  TEST_ASSERT_TRUE(u.back_focused());
  TEST_ASSERT_EQUAL(Command::StopRecordingUpload, press(u, Event::Select).command);
  TEST_ASSERT_EQUAL(Screen::RecordMenu, u.screen());
  TEST_ASSERT_TRUE(u.back_navigation());
}

void test_subir_audio_with_wifi_off_says_so_and_does_not_start() {
  Settings s;
  s.wifi = WifiMode::Off;
  Ui u = make_ui(0, s);
  press(u, Event::AudioAvailable);
  open_record_menu(u);
  const Outcome o = press(u, Event::Select);
  TEST_ASSERT_EQUAL(Command::None, o.command);
  TEST_ASSERT_EQUAL(Screen::Message, u.screen());
  TEST_ASSERT_EQUAL(Message::WifiOff, u.message());
}

void test_a_transfer_on_the_upload_screen_returns_to_the_menu_and_stops_the_ap() {
  Ui u = ui_with_wav();
  open_record_menu(u);
  press(u, Event::Select);  // Enviar al móvil
  const Outcome o = press(u, Event::AudioTransferred);
  TEST_ASSERT_EQUAL(Command::RecordingTransferred, o.command);
  TEST_ASSERT_EQUAL(Screen::Message, u.screen());
  TEST_ASSERT_EQUAL(Message::AudioTransferred, u.message());
  TEST_ASSERT_FALSE(u.recording_available());
}

void test_a_transfer_confirmed_on_another_screen_only_updates_availability() {
  Ui u = ui_with_wav();
  open_root_item(u, 0);  // Leer
  const Outcome o = press(u, Event::AudioTransferred);
  TEST_ASSERT_EQUAL(Command::None, o.command);  // the net service hears of it once, not here
  TEST_ASSERT_EQUAL(Screen::Read, u.screen());
  TEST_ASSERT_FALSE(u.recording_available());
  press(u, Event::Back);
  open_record_menu(u);
  TEST_ASSERT_EQUAL_UINT(0, u.cursor());  // Nueva grabación is the first row again
  press(u, Event::Down);
  TEST_ASSERT_TRUE(u.back_focused());      // and Atrás the second: there is no Borrar grabación
}

void test_audio_events_reach_the_ui_while_reading() {
  Ui u = make_ui(1);
  open_root_item(u, 0);
  press(u, Event::Select);  // Reading
  press(u, Event::AudioAvailable);
  TEST_ASSERT_EQUAL(Screen::Reading, u.screen());
  TEST_ASSERT_TRUE(u.recording_available());
}

void test_upload_start_failure_and_timeout_return_to_the_record_menu() {
  for (const Event e : {Event::AudioUploadStartFailed, Event::AudioUploadTimedOut}) {
    Ui u = ui_with_wav();
    open_record_menu(u);
    press(u, Event::Select);
    press(u, e);
    TEST_ASSERT_EQUAL(Screen::Message, u.screen());
    press(u, Event::Select);
    TEST_ASSERT_EQUAL(Screen::RecordMenu, u.screen());
    TEST_ASSERT_TRUE(u.recording_available());  // the WAV is still on the device
  }
}

void test_borrar_audio_asks_for_confirmation_and_focuses_cancelar() {
  Ui u = ui_with_wav();
  open_record_menu(u);
  press(u, Event::Down);
  TEST_ASSERT_EQUAL(Command::None, press(u, Event::Select).command);
  TEST_ASSERT_EQUAL(Screen::RecordingDeleteConfirm, u.screen());
  TEST_ASSERT_EQUAL_UINT(0, u.cursor());
  press(u, Event::Select);  // Cancelar
  TEST_ASSERT_EQUAL(Screen::RecordMenu, u.screen());
  TEST_ASSERT_TRUE(u.recording_available());
}

void test_confirmed_delete_waits_for_the_recorder_then_reports() {
  Ui u = ui_with_wav();
  open_record_menu(u);
  press(u, Event::Down);
  press(u, Event::Select);
  press(u, Event::Down);  // Borrar
  TEST_ASSERT_EQUAL(Command::DeleteRecording, press(u, Event::Select).command);
  TEST_ASSERT_EQUAL(Screen::RecordingDeleteConfirm, u.screen());
  press(u, Event::AudioDeleted);
  TEST_ASSERT_EQUAL(Message::AudioDeleted, u.message());
  TEST_ASSERT_FALSE(u.recording_available());
}

void test_a_failed_delete_keeps_the_wav_available() {
  Ui u = ui_with_wav();
  open_record_menu(u);
  press(u, Event::Down);
  press(u, Event::Select);
  press(u, Event::Down);
  press(u, Event::Select);
  press(u, Event::AudioDeleteFailed);
  TEST_ASSERT_EQUAL(Message::AudioDeleteFailed, u.message());
  TEST_ASSERT_TRUE(u.recording_available());
}

void test_unavailable_audio_storage_leaves_only_the_back_row() {
  Ui u = make_ui();
  press(u, Event::AudioStorageUnavailable);
  open_record_menu(u);
  TEST_ASSERT_TRUE(u.back_focused());
  TEST_ASSERT_EQUAL(Command::None, press(u, Event::Select).command);  // Atrás, never a recording
  TEST_ASSERT_EQUAL(Screen::Root, u.screen());
}

void test_back_from_the_delete_confirmation_returns_to_the_record_menu() {
  Ui u = ui_with_wav();
  open_record_menu(u);
  press(u, Event::Down);
  press(u, Event::Select);
  press(u, Event::Back);
  TEST_ASSERT_EQUAL(Screen::RecordMenu, u.screen());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_starts_on_root_with_first_item_focused);
  RUN_TEST(test_grabar_is_the_third_root_item_and_opens_the_record_menu);
  RUN_TEST(test_record_menu_without_a_wav_offers_grabar_then_back);
  RUN_TEST(test_grabar_enters_the_recording_screen_and_starts_the_recorder);
  RUN_TEST(test_recording_select_toggles_pause_and_detener_stops);
  RUN_TEST(test_a_long_press_while_recording_asks_to_stop);
  RUN_TEST(test_held_buttons_do_nothing_while_recording);
  RUN_TEST(test_saved_audio_returns_to_the_menu_with_the_wav_available);
  RUN_TEST(test_every_start_or_save_failure_leaves_the_recording_screen_with_its_message);
  RUN_TEST(test_recording_ended_frees_a_screen_waiting_for_a_stop);
  RUN_TEST(test_boot_report_of_a_wav_changes_the_menu_rows);
  RUN_TEST(test_subir_audio_starts_the_wifi_session_and_atras_cancels_it);
  RUN_TEST(test_subir_audio_with_wifi_off_says_so_and_does_not_start);
  RUN_TEST(test_a_transfer_on_the_upload_screen_returns_to_the_menu_and_stops_the_ap);
  RUN_TEST(test_a_transfer_confirmed_on_another_screen_only_updates_availability);
  RUN_TEST(test_audio_events_reach_the_ui_while_reading);
  RUN_TEST(test_upload_start_failure_and_timeout_return_to_the_record_menu);
  RUN_TEST(test_borrar_audio_asks_for_confirmation_and_focuses_cancelar);
  RUN_TEST(test_confirmed_delete_waits_for_the_recorder_then_reports);
  RUN_TEST(test_a_failed_delete_keeps_the_wav_available);
  RUN_TEST(test_unavailable_audio_storage_leaves_only_the_back_row);
  RUN_TEST(test_back_from_the_delete_confirmation_returns_to_the_record_menu);
  RUN_TEST(test_every_secondary_screen_ends_with_a_back_row_to_its_parent);
  RUN_TEST(test_book_list_back_row_comes_after_the_last_book);
  RUN_TEST(test_select_on_the_upload_screen_cancels_the_session);
  RUN_TEST(test_the_back_row_has_no_value_to_edit_or_change);
  RUN_TEST(test_forward_moves_are_not_back_navigation_and_back_moves_are);
  RUN_TEST(test_dismissing_a_message_slides_back);
  RUN_TEST(test_speed_step_screen_edits_the_step_and_saves);
  RUN_TEST(test_speed_step_does_not_change_until_editing);
  RUN_TEST(test_back_from_speed_step_returns_to_ajustes);
  RUN_TEST(test_ajustes_lists_three_settings_and_back_and_wraps);
  RUN_TEST(test_constructor_sanitizes_an_unsupported_reading_step);
  RUN_TEST(test_held_buttons_do_not_move_menu_cursors);
  RUN_TEST(test_held_buttons_repeat_a_number_only_while_editing);
  RUN_TEST(test_held_buttons_do_not_repeat_a_choice);
  RUN_TEST(test_held_buttons_repeat_the_brightness_level);
  RUN_TEST(test_held_buttons_while_reading_are_forwarded);
  RUN_TEST(test_root_cursor_wraps_at_both_ends);
  RUN_TEST(test_a_long_press_on_the_root_asks_before_switching_off);
  RUN_TEST(test_choosing_no_returns_to_the_root_menu);
  RUN_TEST(test_a_long_press_on_the_confirmation_cancels_it);
  RUN_TEST(test_choosing_si_apagar_switches_off);
  RUN_TEST(test_the_two_confirmation_rows_wrap);
  RUN_TEST(test_held_buttons_do_nothing_on_the_confirmation);
  RUN_TEST(test_nothing_interrupts_the_power_off_screen);
  RUN_TEST(test_a_long_press_elsewhere_still_just_goes_back);
  RUN_TEST(test_read_with_no_books_offers_only_the_back_row);
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
  RUN_TEST(test_delete_list_with_no_books_offers_only_the_back_row);
  RUN_TEST(test_delete_confirm_focuses_cancel_by_default);
  RUN_TEST(test_cancel_in_delete_confirm_deletes_nothing);
  RUN_TEST(test_confirming_delete_asks_to_remove_the_selected_book);
  RUN_TEST(test_failed_delete_keeps_list_and_shows_error);
  RUN_TEST(test_brightness_steps_by_five_within_range);
  RUN_TEST(test_brightness_saves_and_returns_with_the_opposite_step);
  RUN_TEST(test_pressing_past_the_limit_changes_nothing_and_saves_nothing);
  RUN_TEST(test_wifi_setting_cycles_through_three_modes);
  RUN_TEST(test_edit_mode_is_off_when_a_setting_screen_opens);
  RUN_TEST(test_up_and_down_do_not_change_a_value_until_editing);
  RUN_TEST(test_select_starts_editing_and_down_lowers_the_value);
  RUN_TEST(test_select_again_stops_editing);
  RUN_TEST(test_back_while_editing_only_stops_editing);
  RUN_TEST(test_wifi_mode_changes_with_up_while_editing);
  RUN_TEST(test_menus_still_move_with_up_and_down);
  RUN_TEST(test_about_is_read_only);
  RUN_TEST(test_long_back_climbs_one_level_at_a_time);
  RUN_TEST(test_library_change_cancels_an_open_delete_confirm);
  RUN_TEST(test_book_count_change_resets_list_cursor);
  return UNITY_END();
}
