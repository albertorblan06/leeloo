#pragma once

// Host-side fakes for the ports in rtos.h. Test-only: nothing on the device includes this.
// shortcut: a fake never blocks. An empty receive returns false at once, so a
// timeout test checks the result, not the elapsed time. Upgrade if a test needs real waiting.

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <vector>

#include "rtos.h"

namespace leeloo::rtos::fakes {

class FakeQueue : public IQueue {
 public:
  FakeQueue(std::size_t capacity, std::size_t item_size) : capacity_(capacity), item_size_(item_size) {}

  bool send_bytes(const void* item, std::size_t size, std::uint32_t) override {
    if (size != item_size_ || items_.size() >= capacity_) return false;
    const auto* bytes = static_cast<const std::uint8_t*>(item);
    items_.emplace_back(bytes, bytes + size);
    return true;
  }

  bool receive_bytes(void* item, std::size_t size, std::uint32_t) override {
    if (size != item_size_ || items_.empty()) return false;
    std::memcpy(item, items_.front().data(), size);
    items_.pop_front();
    return true;
  }

  std::size_t size() const { return items_.size(); }

 private:
  std::size_t capacity_;
  std::size_t item_size_;
  std::deque<std::vector<std::uint8_t>> items_;
};

class FakeClock : public IClock {
 public:
  void set(std::uint32_t now) { now_ = now; }
  void advance(std::uint32_t by) { now_ += by; }
  std::uint32_t now_ms() const override { return now_; }

 private:
  std::uint32_t now_ = 0;
};

class FakeMutex : public IMutex {
 public:
  void lock() override {
    ++depth;
    ++locks;
  }
  void unlock() override { --depth; }
  int depth = 0;  // 0 when balanced
  int locks = 0;
};

class FakeNotifier : public INotifier {
 public:
  void notify() override {
    ++count;
    if (mutex != nullptr) depth_at_last = mutex->depth;
  }
  const FakeMutex* mutex = nullptr;  // set to record whether the lock was held at notify time
  int count = 0;
  int depth_at_last = -1;
};

}  // namespace leeloo::rtos::fakes
