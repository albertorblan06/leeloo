#include <unity.h>

#include "menu.h"

void setUp() {}
void tearDown() {}

void test_down_wraps_from_last_to_first() {
  leeloo::menu::Menu menu(3);
  menu.down();
  menu.down();
  menu.down();
  TEST_ASSERT_EQUAL(0, menu.index());
}

void test_up_wraps_from_first_to_last() {
  leeloo::menu::Menu menu(3);
  menu.up();
  TEST_ASSERT_EQUAL(2, menu.index());
}

void test_select_returns_current_index() {
  leeloo::menu::Menu menu(3);
  menu.down();
  const auto selected = menu.select();
  TEST_ASSERT_TRUE(selected.has_value());
  TEST_ASSERT_EQUAL(1, *selected);
}

void test_empty_menu_ignores_input() {
  leeloo::menu::Menu menu(0);
  menu.up();
  menu.down();
  TEST_ASSERT_EQUAL(0, menu.index());
  TEST_ASSERT_FALSE(menu.select().has_value());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_down_wraps_from_last_to_first);
  RUN_TEST(test_up_wraps_from_first_to_last);
  RUN_TEST(test_select_returns_current_index);
  RUN_TEST(test_empty_menu_ignores_input);
  return UNITY_END();
}
