#include "storage_recording.h"

#include <LittleFS.h>

#include "storage_books.h"

namespace leeloo::storage_recording {

bool exists() {
  storage::Guard guard;
  return storage::mounted() && LittleFS.exists(kRecordingPath);
}

std::size_t size() {
  storage::Guard guard;
  if (!storage::mounted()) return 0;
  File f = LittleFS.open(kRecordingPath, "r");
  if (!f) return 0;
  const std::size_t bytes = f.size();
  f.close();
  return bytes;
}

bool remove() {
  storage::Guard guard;
  if (!storage::mounted()) return false;
  return !LittleFS.exists(kRecordingPath) || LittleFS.remove(kRecordingPath);
}

void cleanup_temp() {
  storage::Guard guard;
  if (storage::mounted()) LittleFS.remove(kRecordingTempPath);
}

}  // namespace leeloo::storage_recording
