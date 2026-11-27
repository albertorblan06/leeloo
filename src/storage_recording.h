#pragma once

#include <cstddef>

// Where the one pending WAV lives. Callers hold storage::Guard around each call and use
// storage::try_begin_mutation() around the sequences that change files.
namespace leeloo::storage_recording {

constexpr const char* kRecordingPath = "/recording.wav";
constexpr const char* kRecordingTempPath = "/recording.tmp";

bool exists();
// Size of the stored WAV, 0 when there is none.
std::size_t size();
// True when no stored WAV is left afterwards (including when there was none).
bool remove();
void cleanup_temp();

}  // namespace leeloo::storage_recording
