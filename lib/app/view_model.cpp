#include "view_model.h"

namespace leeloo::app {

ViewModel::ViewModel(rtos::IMutex* mutex, rtos::INotifier* display)
    : mutex_(mutex), display_(display) {}

void ViewModel::publish_ui(const ui::Ui& ui) {
  update([&ui](Snapshot& s) { s.ui = ui; });
}

void ViewModel::publish_frame(const ReadingFrame& frame) {
  update([&frame](Snapshot& s) { s.frame = frame; });
}

void ViewModel::publish_net(const NetStatus& net) {
  update([&net](Snapshot& s) { s.net = net; });
}

void ViewModel::request_contrast(std::uint8_t register_value) {
  update([register_value](Snapshot& s) {
    s.contrast_pending = true;
    s.contrast = register_value;
  });
}

bool ViewModel::read_if_new(Snapshot* out, std::uint32_t* seen) {
  rtos::ScopedLock lock(mutex_);
  if (version_ == *seen) return false;
  *seen = version_;
  *out = state_;
  state_.contrast_pending = false;
  return true;
}

}  // namespace leeloo::app
