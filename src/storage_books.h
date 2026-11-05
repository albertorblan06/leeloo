#pragma once

#include <cstddef>

// LittleFS book library shared by the OLED menu and the upload server. Every
// public call takes one recursive lock, so the two tasks never see a half-done
// replace. Hardware-bound: covered by docs/hardware-checklists/menu.md.
namespace leeloo::storage {

// Mounts LittleFS without formatting. A failed mount leaves existing data alone.
bool mount();
bool mounted();

// Holds the library lock for its lifetime; calls below may nest inside it.
class Guard {
 public:
  Guard();
  ~Guard();
  Guard(const Guard&) = delete;
  Guard& operator=(const Guard&) = delete;
};

std::size_t book_count();

// Writes the title (file name without ".txt") of book `index` into `out`.
// Returns false when `index` is out of range or `cap` is too small.
bool title_at(std::size_t index, char* out, std::size_t cap);

// Removes book `index`. Returns false when it does not exist or cannot be removed.
bool remove_at(std::size_t index);

std::size_t free_bytes();

// A whole book file held in PSRAM. Release with free_text().
struct BookText {
  char* data = nullptr;
  std::size_t size = 0;
};

// Loads book `index`. Returns false when it is missing, larger than the upload
// limit, or PSRAM cannot hold it. Nothing is loaded partially.
bool load_text(std::size_t index, BookText* out);
void free_text(BookText* text);

// Saved reading position (a word offset) of the book titled `title`. Keyed by
// title, not by index: indexes shift when another book is added or removed.
// Stored as "<title>.pos", so it is not listed as a book.
bool load_position(const char* title, std::size_t* offset_out);
// Replaces the saved position atomically. Returns false when the write fails.
bool save_position(const char* title, std::size_t offset);

}  // namespace leeloo::storage
