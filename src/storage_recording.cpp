#include "storage_recording.h"

#include <LittleFS.h>

#include "storage_books.h"

namespace leeloo::storage_recording {

bool exists() {
  storage::Guard guard;
  return storage::mounted() && LittleFS.exists(kRecordingPath);
}

bool remove() {
  if (!storage::try_begin_mutation()) return false;
  bool removed = false;
  {
    storage::Guard guard;
    removed = !LittleFS.exists(kRecordingPath) || LittleFS.remove(kRecordingPath);
  }
  storage::end_mutation();
  return removed;
}

void cleanup_temp() {
  storage::Guard guard;
  if (storage::mounted()) LittleFS.remove(kRecordingTempPath);
}

}  // namespace leeloo::storage_recording
