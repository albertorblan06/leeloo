#include "rtos_freertos.h"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

namespace leeloo::rtos::freertos {

namespace {

TickType_t to_ticks(std::uint32_t timeout_ms) {
  return timeout_ms == kWaitForever ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
}

}  // namespace

Queue::Queue(std::size_t length, std::size_t item_size)
    : handle_(xQueueCreate(length, item_size)), item_size_(item_size) {}

Queue::~Queue() {
  if (handle_ != nullptr) vQueueDelete(static_cast<QueueHandle_t>(handle_));
}

bool Queue::send_bytes(const void* item, std::size_t size, std::uint32_t timeout_ms) {
  if (handle_ == nullptr || size != item_size_) return false;
  return xQueueSend(static_cast<QueueHandle_t>(handle_), item, to_ticks(timeout_ms)) == pdTRUE;
}

bool Queue::receive_bytes(void* item, std::size_t size, std::uint32_t timeout_ms) {
  if (handle_ == nullptr || size != item_size_) return false;
  return xQueueReceive(static_cast<QueueHandle_t>(handle_), item, to_ticks(timeout_ms)) == pdTRUE;
}

std::uint32_t Clock::now_ms() const { return static_cast<std::uint32_t>(millis()); }

Mutex::Mutex() : handle_(xSemaphoreCreateMutex()) {}

Mutex::~Mutex() {
  if (handle_ != nullptr) vSemaphoreDelete(static_cast<SemaphoreHandle_t>(handle_));
}

void Mutex::lock() {
  if (handle_ != nullptr) xSemaphoreTake(static_cast<SemaphoreHandle_t>(handle_), portMAX_DELAY);
}

void Mutex::unlock() {
  if (handle_ != nullptr) xSemaphoreGive(static_cast<SemaphoreHandle_t>(handle_));
}

void TaskNotifier::notify() {
  if (task_ != nullptr) xTaskNotifyGive(static_cast<TaskHandle_t>(task_));
}

}  // namespace leeloo::rtos::freertos
