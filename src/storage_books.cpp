#include "storage_books.h"

#include <Arduino.h>
#include <LittleFS.h>
#include <esp_heap_caps.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "upload.h"

namespace leeloo::storage {
namespace {

constexpr const char* kBooksDir = "/books";
constexpr const char* kExtension = ".txt";
constexpr std::size_t kExtensionLen = 4;
constexpr const char* kPositionExtension = ".pos";
constexpr std::size_t kNameCap = 64;
constexpr std::size_t kPathCap = 80;  // "/books/" + a 63-byte name + ".pos" fits

bool g_mounted = false;
SemaphoreHandle_t g_lock = nullptr;

// Recursive, so public calls can nest; created on first use.
SemaphoreHandle_t lock_handle() {
  if (g_lock == nullptr) g_lock = xSemaphoreCreateRecursiveMutex();
  return g_lock;
}

// File::name() may come with or without the directory, depending on the core.
const char* base_name(const char* name) {
  const char* slash = std::strrchr(name, '/');
  return slash != nullptr ? slash + 1 : name;
}

bool is_book(const char* name) {
  const std::size_t len = std::strlen(name);
  return len > kExtensionLen && std::strcmp(name + len - kExtensionLen, kExtension) == 0;
}

// Walks /books and reports the book at `index` through `name_out`. The order is
// the filesystem's own, which stays stable while the library does not change.
bool find_book(std::size_t index, char* name_out, std::size_t cap) {
  File dir = LittleFS.open(kBooksDir);
  if (!dir || !dir.isDirectory()) return false;
  std::size_t seen = 0;
  for (File f = dir.openNextFile(); f; f = dir.openNextFile()) {
    const char* name = base_name(f.name());
    if (f.isDirectory() || !is_book(name)) continue;
    if (seen++ != index) continue;
    const std::size_t len = std::strlen(name);
    if (len + 1 > cap) return false;
    std::memcpy(name_out, name, len + 1);
    return true;
  }
  return false;
}

// Full path of book `index`, e.g. "/books/Libro.txt".
bool book_path_for(std::size_t index, char* out, std::size_t cap) {
  char name[kNameCap];
  if (!find_book(index, name, sizeof name)) return false;
  const int n = std::snprintf(out, cap, "%s/%s", kBooksDir, name);
  return n > 0 && static_cast<std::size_t>(n) < cap;
}

// Position file of the book titled `title`, e.g. "/books/Libro.pos".
bool position_path_for(const char* title, char* out, std::size_t cap) {
  const int n = std::snprintf(out, cap, "%s/%s%s", kBooksDir, title, kPositionExtension);
  return n > 0 && static_cast<std::size_t>(n) < cap;
}

}  // namespace

bool mount() {
  lock_handle();  // create the lock before the HTTP task can ask for it
  g_mounted = LittleFS.begin(false);  // formatOnFail stays false: never wipe books
  // A freshly flashed image has no books directory, and uploads need it to exist.
  if (g_mounted && !LittleFS.exists(kBooksDir)) LittleFS.mkdir(kBooksDir);
  return g_mounted;
}

bool mounted() { return g_mounted; }

Guard::Guard() { xSemaphoreTakeRecursive(lock_handle(), portMAX_DELAY); }
Guard::~Guard() { xSemaphoreGiveRecursive(lock_handle()); }

std::size_t book_count() {
  Guard guard;
  if (!g_mounted) return 0;
  std::size_t count = 0;
  File dir = LittleFS.open(kBooksDir);
  if (!dir || !dir.isDirectory()) return 0;
  for (File f = dir.openNextFile(); f; f = dir.openNextFile()) {
    if (!f.isDirectory() && is_book(base_name(f.name()))) ++count;
  }
  return count;
}

bool title_at(std::size_t index, char* out, std::size_t cap) {
  Guard guard;
  if (!g_mounted) return false;
  char name[64];
  if (!find_book(index, name, sizeof name)) return false;
  const std::size_t title_len = std::strlen(name) - kExtensionLen;
  if (title_len + 1 > cap) return false;
  std::memcpy(out, name, title_len);
  out[title_len] = '\0';
  return true;
}

// Position file of book `index`; the title comes from the same library snapshot.
bool position_path_at(std::size_t index, char* out, std::size_t cap) {
  char title[kNameCap];
  return title_at(index, title, sizeof title) && position_path_for(title, out, cap);
}

bool remove_at(std::size_t index) {
  Guard guard;
  if (!g_mounted) return false;
  char path[kPathCap];
  if (!book_path_for(index, path, sizeof path)) return false;
  // Found before the book goes, because the position file is named after it.
  char position[kPathCap];
  const bool has_position = position_path_at(index, position, sizeof position);
  if (!LittleFS.remove(path)) return false;
  // shortcut: a position file that fails to delete is left behind; a re-uploaded
  // book of the same name would resume at a stale offset. Upgrade to a retry if seen.
  if (has_position) LittleFS.remove(position);
  return true;
}

bool load_text(std::size_t index, BookText* out) {
  Guard guard;
  out->data = nullptr;
  out->size = 0;
  if (!g_mounted) return false;
  char path[kPathCap];
  if (!book_path_for(index, path, sizeof path)) return false;
  File file = LittleFS.open(path, "r");
  if (!file) return false;
  const std::size_t size = file.size();
  if (size == 0 || size > leeloo::upload::kMaxBookBytes) {
    file.close();
    return false;
  }
  // Books live in PSRAM: the N16R8 has 8 MB of it, and the 1 MiB limit leaves room.
  char* data = static_cast<char*>(heap_caps_malloc(size, MALLOC_CAP_SPIRAM));
  if (data == nullptr) {
    file.close();
    return false;
  }
  const std::size_t read = file.read(reinterpret_cast<std::uint8_t*>(data), size);
  file.close();
  if (read != size) {
    heap_caps_free(data);
    return false;
  }
  out->data = data;
  out->size = size;
  return true;
}

void free_text(BookText* text) {
  heap_caps_free(text->data);
  text->data = nullptr;
  text->size = 0;
}

bool load_position(const char* title, std::size_t* offset_out) {
  Guard guard;
  if (!g_mounted) return false;
  char path[kPathCap];
  if (!position_path_for(title, path, sizeof path)) return false;
  File file = LittleFS.open(path, "r");
  if (!file) return false;
  char text[24] = {};
  file.readBytes(text, sizeof text - 1);
  file.close();
  char* end = nullptr;
  const unsigned long value = std::strtoul(text, &end, 10);
  if (end == text || (*end != '\0' && *end != '\n')) return false;
  *offset_out = value;
  return true;
}

bool save_position(const char* title, std::size_t offset) {
  Guard guard;
  if (!g_mounted) return false;
  char path[kPathCap];
  if (!position_path_for(title, path, sizeof path)) return false;
  char temp[kPathCap + 4];
  std::snprintf(temp, sizeof temp, "%s.tmp", path);
  char text[24];
  const int n = std::snprintf(text, sizeof text, "%lu\n", static_cast<unsigned long>(offset));
  File file = LittleFS.open(temp, "w");
  if (!file) return false;
  const std::size_t written = file.write(reinterpret_cast<const std::uint8_t*>(text), static_cast<std::size_t>(n));
  file.close();
  if (written != static_cast<std::size_t>(n) || !LittleFS.rename(temp, path)) {
    LittleFS.remove(temp);
    return false;
  }
  return true;
}

std::size_t free_bytes() {
  Guard guard;
  if (!g_mounted) return 0;
  const std::size_t total = LittleFS.totalBytes();
  const std::size_t used = LittleFS.usedBytes();
  return used < total ? total - used : 0;
}

}  // namespace leeloo::storage
