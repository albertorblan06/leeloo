#pragma once

#include <cstddef>
#include <optional>

namespace leeloo::menu {

// Cursor over a fixed list of `count` items. Moves wrap around at both ends.
class Menu {
 public:
  explicit Menu(std::size_t count) : count_(count) {}

  std::size_t index() const { return index_; }

  void up();
  void down();

  // Returns the highlighted item, or nullopt when the menu is empty.
  std::optional<std::size_t> select() const;

 private:
  std::size_t count_;
  std::size_t index_ = 0;
};

}  // namespace leeloo::menu
