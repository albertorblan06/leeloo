#pragma once

#include <cstddef>
#include <cstdint>

#include "rtos.h"

// FreeRTOS adapters for the ports in lib/rtos. Hardware-bound: covered by the build
// (pio run -e esp32s3) and docs/hardware-checklists/rtos.md, not by host tests.
namespace leeloo::rtos::freertos {

class Queue : public IQueue {
 public:
  // Returns false from `ok()` when the RTOS cannot allocate the queue.
  Queue(std::size_t length, std::size_t item_size);
  ~Queue() override;
  Queue(const Queue&) = delete;
  Queue& operator=(const Queue&) = delete;

  bool ok() const { return handle_ != nullptr; }

  // QueueHandle_t for the one caller that cannot use the port: the button ISR,
  // which needs xQueueSendFromISR.
  void* native_handle() const { return handle_; }

  bool send_bytes(const void* item, std::size_t size, std::uint32_t timeout_ms) override;
  bool receive_bytes(void* item, std::size_t size, std::uint32_t timeout_ms) override;

 private:
  void* handle_ = nullptr;  // QueueHandle_t, kept opaque so this header needs no FreeRTOS include
  std::size_t item_size_ = 0;
};

class Clock : public IClock {
 public:
  std::uint32_t now_ms() const override;
};

// Non-recursive mutex with priority inheritance (xSemaphoreCreateMutex).
class Mutex : public IMutex {
 public:
  Mutex();
  ~Mutex() override;
  Mutex(const Mutex&) = delete;
  Mutex& operator=(const Mutex&) = delete;

  bool ok() const { return handle_ != nullptr; }
  void lock() override;
  void unlock() override;

 private:
  void* handle_ = nullptr;  // SemaphoreHandle_t
};

// Wakes one task with xTaskNotifyGive. Set the task once, before any producer runs.
class TaskNotifier : public INotifier {
 public:
  void set_task(void* task_handle) { task_ = task_handle; }
  void notify() override;

 private:
  void* task_ = nullptr;  // TaskHandle_t
};

}  // namespace leeloo::rtos::freertos
