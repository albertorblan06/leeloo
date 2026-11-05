#include "menu.h"

namespace leeloo::menu {

void Menu::up() {
  if (count_ == 0) return;
  index_ = (index_ + count_ - 1) % count_;
}

void Menu::down() {
  if (count_ == 0) return;
  index_ = (index_ + 1) % count_;
}

std::optional<std::size_t> Menu::select() const {
  if (count_ == 0) return std::nullopt;
  return index_;
}

}  // namespace leeloo::menu
