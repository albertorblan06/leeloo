#pragma once

namespace leeloo::storage_recording {

inline constexpr char kRecordingPath[] = "/recording.wav";
inline constexpr char kRecordingTempPath[] = "/recording.tmp";

bool exists();
bool remove();
void cleanup_temp();

}  // namespace leeloo::storage_recording
