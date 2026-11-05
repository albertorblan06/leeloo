#include "rsvp.h"

#include <algorithm>
#include <limits>

#include "leeloo_core.h"

namespace leeloo::rsvp {

namespace {

bool is_continuation(char byte) {
  return (static_cast<unsigned char>(byte) & 0xC0) == 0x80;
}

bool is_space(char byte) {
  return byte == ' ' || byte == '\t' || byte == '\n' || byte == '\r';
}

// Classic RSVP ORP rule, keyed on the word's length in code points.
std::size_t orp_index(std::size_t codepoints) {
  if (codepoints <= 1) return 0;
  if (codepoints <= 5) return 1;
  if (codepoints <= 9) return 2;
  if (codepoints <= 13) return 3;
  return 4;
}

}  // namespace

std::optional<WordSpan> next_word(const char* text, std::size_t size, std::size_t offset) {
  if (text == nullptr || offset > size) return std::nullopt;
  if (offset < size && is_continuation(text[offset])) return std::nullopt;

  std::size_t start = offset;
  while (start < size && is_space(text[start])) ++start;
  if (start == size) return std::nullopt;

  std::size_t end = start;
  while (end < size && !is_space(text[end])) ++end;
  return WordSpan{start, end - start};
}

FocalSplit focal_split(const char* word, std::size_t length) {
  if (word == nullptr || length == 0) return {0, 0};

  std::size_t codepoints = 0;
  for (std::size_t i = 0; i < length; ++i) {
    if (!is_continuation(word[i])) ++codepoints;
  }
  if (codepoints == 0) return {length, 0};  // only stray continuation bytes

  // Walk to the lead byte of the ORP code point.
  const std::size_t target = orp_index(codepoints);
  std::size_t seen = 0;
  std::size_t focal = 0;
  for (; focal < length; ++focal) {
    if (is_continuation(word[focal])) continue;
    if (seen == target) break;
    ++seen;
  }

  // A code point is at most 4 bytes; the cap bounds malformed input too.
  constexpr std::size_t kMaxCodepointBytes = 4;
  std::size_t next = focal + 1;
  while (next < length && next - focal < kMaxCodepointBytes && is_continuation(word[next])) {
    ++next;
  }
  return {focal, next - focal};
}

namespace {

// Decodes the code point at byte `i`. Malformed bytes decode as themselves
// with width 1, so the scan always advances.
std::uint32_t decode_at(const char* text, std::size_t size, std::size_t i, std::size_t* width) {
  const auto lead = static_cast<unsigned char>(text[i]);
  const int len = leeloo::utf8_sequence_length(lead);
  if (len <= 1 || i + static_cast<std::size_t>(len) > size) {
    *width = 1;
    return lead;
  }
  std::uint32_t cp = lead & (0xFF >> (len + 1));
  for (int k = 1; k < len; ++k) {
    const auto cont = static_cast<unsigned char>(text[i + k]);
    if ((cont & 0xC0) != 0x80) {
      *width = 1;
      return lead;
    }
    cp = (cp << 6) | (cont & 0x3F);
  }
  *width = static_cast<std::size_t>(len);
  return cp;
}

// Latin-1 letters only (ASCII plus U+00C0..U+00FF except × and ÷); enough for Spanish.
bool is_letter(std::uint32_t cp) {
  if ((cp >= 'a' && cp <= 'z') || (cp >= 'A' && cp <= 'Z')) return true;
  return cp >= 0xC0 && cp <= 0xFF && cp != 0xD7 && cp != 0xF7;
}

std::uint32_t to_lower(std::uint32_t cp) {
  if (cp >= 'A' && cp <= 'Z') return cp + 32;
  if (cp >= 0xC0 && cp <= 0xDE && cp != 0xD7) return cp + 32;
  return cp;
}

enum class Kind { kOther, kConsonant, kStrongVowel, kWeakVowel };

Kind classify(std::uint32_t cp, bool is_last_letter) {
  if (!is_letter(cp)) return Kind::kOther;
  switch (to_lower(cp)) {
    case 'a': case 'e': case 'o':
      return Kind::kStrongVowel;
    case 0xE1: case 0xE9: case 0xF3:  // á é ó
    case 0xED: case 0xFA:             // í ú: the accent forces a hiatus
      return Kind::kStrongVowel;
    case 'i': case 'u': case 0xFC:    // ü
      return Kind::kWeakVowel;
    case 'y':                         // "y" at the end of a word is a vowel ("Paraguay")
      return is_last_letter ? Kind::kWeakVowel : Kind::kConsonant;
    default:
      return Kind::kConsonant;
  }
}

// Consonant pairs that stay together: digraphs ch/ll/rr and the stop+liquid
// clusters (br cr dr fr gr pr tr; bl cl fl gl pl).
bool is_inseparable(std::uint32_t first, std::uint32_t second) {
  if ((first == 'c' && second == 'h') || (first == 'l' && second == 'l') ||
      (first == 'r' && second == 'r')) {
    return true;
  }
  const bool stop_r = (first == 'b' || first == 'c' || first == 'd' || first == 'f' ||
                       first == 'g' || first == 'p' || first == 't');
  if (second == 'r') return stop_r;
  if (second == 'l') return first == 'b' || first == 'c' || first == 'f' || first == 'g' || first == 'p';
  return false;
}

struct Letter {
  std::size_t byte;
  std::size_t index;
  std::uint32_t cp;  // lowercase
};

// Keeps the boundary closest to the middle; ties keep the earlier candidate
// because candidates arrive in increasing order.
struct BestSplit {
  std::size_t letters;
  std::size_t byte = 0;
  std::size_t cost = std::numeric_limits<std::size_t>::max();

  void consider(const Letter& boundary) {
    const std::size_t twice = 2 * boundary.index;
    const std::size_t dist = twice > letters ? twice - letters : letters - twice;
    if (dist < cost) {
      cost = dist;
      byte = boundary.byte;
    }
  }
};

std::size_t count_letters(const char* word, std::size_t length) {
  std::size_t count = 0;
  for (std::size_t i = 0; i < length;) {
    std::size_t width = 1;
    if (is_letter(decode_at(word, length, i, &width))) ++count;
    i += width;
  }
  return count;
}

}  // namespace

std::size_t split_point(const char* word, std::size_t length) {
  if (word == nullptr) return 0;
  const std::size_t letters = count_letters(word, length);
  if (letters < 2) return 0;

  BestSplit best{letters};
  Kind last_vowel = Kind::kOther;  // kOther = no vowel seen yet
  std::size_t run = 0;             // consonants since the last vowel
  Letter prev_cons{};              // last two consonants of the run
  Letter last_cons{};
  std::size_t index = 0;

  for (std::size_t i = 0; i < length;) {
    std::size_t width = 1;
    const std::uint32_t cp = decode_at(word, length, i, &width);
    const Letter letter{i, index, to_lower(cp)};
    const Kind kind = classify(cp, index == letters - 1);
    i += width;
    if (kind == Kind::kOther) continue;
    ++index;

    if (kind == Kind::kConsonant) {
      prev_cons = last_cons;
      last_cons = letter;
      ++run;
      continue;
    }

    if (last_vowel != Kind::kOther) {
      if (run == 0 && last_vowel == Kind::kStrongVowel && kind == Kind::kStrongVowel) {
        best.consider(letter);  // hiatus: two strong vowels never share a syllable
      } else if (run == 1) {
        best.consider(last_cons);  // V-CV
      } else if (run >= 2) {
        best.consider(is_inseparable(prev_cons.cp, last_cons.cp) ? prev_cons : last_cons);
      }
    }
    run = 0;
    last_vowel = kind;
  }
  return best.byte;
}

namespace {

constexpr std::uint32_t kNormalHold = 100;
constexpr std::uint32_t kCommaHold = 200;
constexpr std::uint32_t kSentenceHold = 300;
constexpr std::uint32_t kParagraphHold = 500;
constexpr std::uint32_t kLongWordHold = 125;
constexpr std::size_t kLongWordLetters = 8;

bool is_digit(std::uint32_t cp) { return cp >= '0' && cp <= '9'; }

// Hold for a punctuation mark that ends a word.
std::uint32_t mark_hold(std::uint32_t cp) {
  switch (cp) {
    case '.': case '?': case '!': case 0x2026:  // . ? ! …
      return kSentenceHold;
    case ',': case ';': case ':': case ')': case ']': case '"': case '\'':
    case 0xBB: case 0x201D: case 0x2019:        // » ” ’
    case 0x2014: case 0x2013:                   // — –
      return kCommaHold;
    default:
      return kNormalHold;
  }
}

std::size_t count_newlines_after(const char* text, std::size_t size, std::size_t from) {
  std::size_t newlines = 0;
  for (std::size_t i = from; i < size; ++i) {
    const char byte = text[i];
    if (byte == '\n') {
      ++newlines;
    } else if (byte != ' ' && byte != '\t' && byte != '\r') {
      break;
    }
  }
  return newlines;
}

}  // namespace

// shortcut: abbreviations such as "Sr." count as sentence ends, upgrade with an abbreviation list.
std::uint32_t hold_percent(const char* text, std::size_t size, const WordSpan& span) {
  if (text == nullptr || span.start > size || span.length > size - span.start) return kNormalHold;

  const std::size_t end = span.start + span.length;
  std::uint32_t trailing = kNormalHold;  // strongest mark after the last letter or digit
  std::size_t letters = 0;
  for (std::size_t i = span.start; i < end;) {
    std::size_t width = 1;
    const std::uint32_t cp = decode_at(text, end, i, &width);
    i += width;
    if (is_letter(cp) || is_digit(cp)) {
      trailing = kNormalHold;
      if (is_letter(cp)) ++letters;
    } else {
      trailing = std::max(trailing, mark_hold(cp));
    }
  }

  std::uint32_t hold = trailing;
  if (letters > kLongWordLetters) hold = std::max(hold, kLongWordHold);
  if (count_newlines_after(text, size, end) >= 2) hold = std::max(hold, kParagraphHold);
  return hold;
}

bool ends_sentence(const char* text, std::size_t size, const WordSpan& span) {
  return hold_percent(text, size, span) >= kSentenceHold;
}

std::uint32_t word_delay_ms(std::uint32_t wpm) {
  const std::uint32_t clamped = std::clamp(wpm, kMinWpm, kMaxWpm);
  // Round to the nearest step; both bounds are already on a step.
  const std::uint32_t steps = (clamped - kMinWpm + kWpmStep / 2) / kWpmStep;
  const std::uint32_t snapped = kMinWpm + steps * kWpmStep;
  return 60000 / snapped;
}

}  // namespace leeloo::rsvp
