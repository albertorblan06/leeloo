#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

// Ports the task bodies depend on. The FreeRTOS adapters live in src/ and the
// host fakes in fakes.h, so the logic above them builds and tests on the dev machine.
namespace leeloo::rtos {

// Passed as a timeout to mean "wait without a deadline".
constexpr std::uint32_t kWaitForever = UINT32_MAX;

// Untyped bounded queue. Items are copied by value; `size` must match the
// size the queue was created with, otherwise the call fails.
class IQueue {
 public:
  virtual ~IQueue() = default;
  // False when the queue is still full after waiting up to timeout_ms.
  virtual bool send_bytes(const void* item, std::size_t size, std::uint32_t timeout_ms) = 0;
  // False when no item arrives within timeout_ms.
  virtual bool receive_bytes(void* item, std::size_t size, std::uint32_t timeout_ms) = 0;
};

// Typed view over an IQueue. Items cross the RTOS as raw bytes, so only
// trivially copyable types are accepted.
template <typename T>
class Queue {
  static_assert(std::is_trivially_copyable<T>::value, "queue items are copied as raw bytes");

 public:
  explicit Queue(IQueue* port) : port_(port) {}

  bool send(const T& item, std::uint32_t timeout_ms) {
    return port_->send_bytes(&item, sizeof item, timeout_ms);
  }
  bool receive(T* item, std::uint32_t timeout_ms) {
    return port_->receive_bytes(item, sizeof *item, timeout_ms);
  }

 private:
  IQueue* port_;
};

// Monotonic milliseconds. Callers compare times with unsigned subtraction so a 32-bit wrap is harmless.
class IClock {
 public:
  virtual ~IClock() = default;
  virtual std::uint32_t now_ms() const = 0;
};

// Mutual exclusion between tasks. The FreeRTOS adapter uses a mutex with priority
// inheritance, so a low-priority holder is boosted while a high-priority task waits.
class IMutex {
 public:
  virtual ~IMutex() = default;
  virtual void lock() = 0;
  virtual void unlock() = 0;
};

class ScopedLock {
 public:
  explicit ScopedLock(IMutex* mutex) : mutex_(mutex) { mutex_->lock(); }
  ~ScopedLock() { mutex_->unlock(); }
  ScopedLock(const ScopedLock&) = delete;
  ScopedLock& operator=(const ScopedLock&) = delete;

 private:
  IMutex* mutex_;
};

// Wakes one waiting task. Several notifications before it runs may merge into one.
class INotifier {
 public:
  virtual ~INotifier() = default;
  virtual void notify() = 0;
};

}  // namespace leeloo::rtos
