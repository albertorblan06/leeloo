#include <unity.h>

#include <cstdint>

#include "fakes.h"
#include "rtos.h"

void setUp() {}
void tearDown() {}

using leeloo::rtos::Queue;
using leeloo::rtos::fakes::FakeClock;
using leeloo::rtos::fakes::FakeQueue;

struct Message {
  std::uint8_t kind;
  std::uint32_t at_ms;
};

void test_delivers_items_in_fifo_order() {
  FakeQueue port(4, sizeof(std::uint8_t));
  Queue<std::uint8_t> queue(&port);
  TEST_ASSERT_TRUE(queue.send(1, 0));
  TEST_ASSERT_TRUE(queue.send(2, 0));
  std::uint8_t first = 0;
  std::uint8_t second = 0;
  TEST_ASSERT_TRUE(queue.receive(&first, 0));
  TEST_ASSERT_TRUE(queue.receive(&second, 0));
  TEST_ASSERT_EQUAL_UINT8(1, first);
  TEST_ASSERT_EQUAL_UINT8(2, second);
}

void test_rejects_send_when_queue_is_full() {
  FakeQueue port(2, sizeof(std::uint8_t));
  Queue<std::uint8_t> queue(&port);
  TEST_ASSERT_TRUE(queue.send(1, 0));
  TEST_ASSERT_TRUE(queue.send(2, 0));
  TEST_ASSERT_FALSE(queue.send(3, 0));  // dropped, the producer must see it
  TEST_ASSERT_EQUAL_UINT32(2, port.size());
}

void test_receive_on_empty_queue_returns_false() {
  FakeQueue port(2, sizeof(std::uint8_t));
  Queue<std::uint8_t> queue(&port);
  std::uint8_t item = 9;
  TEST_ASSERT_FALSE(queue.receive(&item, leeloo::rtos::kWaitForever));
  TEST_ASSERT_EQUAL_UINT8(9, item);  // untouched on failure
}

void test_round_trips_a_struct_without_changing_fields() {
  FakeQueue port(2, sizeof(Message));
  Queue<Message> queue(&port);
  TEST_ASSERT_TRUE(queue.send(Message{7, 123456u}, 0));
  Message out{};
  TEST_ASSERT_TRUE(queue.receive(&out, 0));
  TEST_ASSERT_EQUAL_UINT8(7, out.kind);
  TEST_ASSERT_EQUAL_UINT32(123456u, out.at_ms);
}

void test_rejects_item_whose_size_differs_from_queue_item_size() {
  FakeQueue port(2, sizeof(Message));
  std::uint8_t wrong = 1;
  TEST_ASSERT_FALSE(port.send_bytes(&wrong, sizeof wrong, 0));
  TEST_ASSERT_EQUAL_UINT32(0, port.size());
}

void test_fake_clock_reports_set_and_advanced_time() {
  FakeClock clock;
  clock.set(1000);
  clock.advance(250);
  TEST_ASSERT_EQUAL_UINT32(1250, clock.now_ms());
}

void test_elapsed_time_survives_32_bit_wrap() {
  FakeClock clock;
  clock.set(UINT32_MAX - 4);
  const std::uint32_t start = clock.now_ms();
  clock.advance(10);  // wraps past zero
  TEST_ASSERT_EQUAL_UINT32(10u, clock.now_ms() - start);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_delivers_items_in_fifo_order);
  RUN_TEST(test_rejects_send_when_queue_is_full);
  RUN_TEST(test_receive_on_empty_queue_returns_false);
  RUN_TEST(test_round_trips_a_struct_without_changing_fields);
  RUN_TEST(test_rejects_item_whose_size_differs_from_queue_item_size);
  RUN_TEST(test_fake_clock_reports_set_and_advanced_time);
  RUN_TEST(test_elapsed_time_survives_32_bit_wrap);
  return UNITY_END();
}
