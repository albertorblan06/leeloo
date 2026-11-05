// Mirrors docs/WEB-API.md §4. Keep in sync with the firmware constants.
export const kMaxBookBytes = 1_048_576;
export const kMaxNameBytes = 48;
// Header values are cut to this so a TITLE/AUTHOR line always fits the device's line buffer.
export const kMaxHeaderValueBytes = 200;
