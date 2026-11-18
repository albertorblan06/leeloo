#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

namespace leeloo::rsvp {

struct WordSpan {
  std::size_t start;   // byte offset of the first byte of the word
  std::size_t length;  // bytes; never splits a UTF-8 code point
};

struct FocalSplit {
  std::size_t prefix_bytes;  // bytes before the focal letter
  std::size_t focal_bytes;   // bytes of the focal code point
};

// Finds the next word at or after `offset`, separated by ASCII whitespace.
// Returns nullopt when no word remains, when `offset` is past the end, or
// when `offset` lands inside a UTF-8 sequence.
std::optional<WordSpan> next_word(const char* text, std::size_t size, std::size_t offset);

// Splits a word around its Optimal Recognition Point (focal letter).
// Counts code points, not bytes, so accented Spanish letters are one letter.
FocalSplit focal_split(const char* word, std::size_t length);

// Byte offset where a long word should be split into two chunks: the approximate
// Spanish syllable boundary closest to the middle (ties go to the earlier one).
// Returns 0 when there is no boundary, e.g. one-syllable words, so the caller
// can clip instead of cutting inside a syllable. Approximate: it ignores
// prefixes, loanwords and the rarer hiatus/diphthong exceptions.
std::size_t split_point(const char* word, std::size_t length);

// How long to hold the word at `span`, as a percentage of one word period
// (100 = normal). Takes the largest of: a trailing mark (comma level 200,
// sentence level 300), a paragraph break after the word (500, two or more
// newlines), and a long word (more than 8 letters, 125). Opening marks such as
// ¿ ¡ ( « — never add a pause.
std::uint32_t hold_percent(const char* text, std::size_t size, const WordSpan& span);

// True when the word at `span` ends a sentence: a sentence mark (. ? ! …) or a
// paragraph break after it. Same threshold as the sentence hold in hold_percent.
bool ends_sentence(const char* text, std::size_t size, const WordSpan& span);

constexpr std::uint32_t kMinWpm = 100;
constexpr std::uint32_t kMaxWpm = 500;
constexpr std::uint32_t kDefaultWpm = 300;
constexpr std::uint32_t kDefaultWpmStep = 25;
constexpr std::uint32_t kWpmSteps[] = {5, 10, 25, 50, 100};

// The step is selected from kWpmSteps; an invalid value falls back to the default.
std::uint32_t normalize_wpm_step(std::uint32_t step);

// WPM is clamped to [kMinWpm, kMaxWpm] and snapped from kMinWpm using `step`.
std::uint32_t normalize_wpm(std::uint32_t wpm, std::uint32_t step = kDefaultWpmStep);

// Milliseconds to show one word at the normalized WPM and selected step.
std::uint32_t word_delay_ms(std::uint32_t wpm, std::uint32_t step = kDefaultWpmStep);

}  // namespace leeloo::rsvp
