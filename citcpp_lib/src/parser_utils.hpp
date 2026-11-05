#ifndef PARSER_UTILS_HPP_
#define PARSER_UTILS_HPP_

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string_view>

namespace citcpp {
namespace detail {

// Hashes std::string and std::string_view alike, for lookups by a
// std::string_view in a map with std::string keys.
struct string_hash {
    using is_transparent = void;

    std::size_t operator()(std::string_view sv) const noexcept {
      return std::hash<std::string_view>{}(sv);
    }
};

// The character classes are ASCII and do not depend on the locale.

inline bool is_digit(char c) { return c >= '0' && c <= '9'; }

inline bool is_identifier_start(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

inline bool is_identifier_char(char c) {
  return is_identifier_start(c) || is_digit(c);
}

inline char to_lower(char c) {
  return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
}

// Whether a and b are equal, ignoring the case of ASCII letters.
inline bool iequals(std::string_view a, std::string_view b) {
  if (a.size() != b.size()) {
    return false;
  }
  for (std::size_t i = 0; i < a.size(); ++i) {
    if (to_lower(a[i]) != to_lower(b[i])) {
      return false;
    }
  }
  return true;
}

// The number of bytes of the UTF-8 code point at the start of s, or 0 if s
// does not start with a complete one.
inline std::size_t codepoint_length(std::string_view s) {
  if (s.empty()) {
    return 0;
  }
  const auto b = static_cast<std::uint8_t>(s[0]);
  if ((b & 0x80) == 0) {
    return 1;
  } else if ((b & 0xE0) == 0xC0 && s.size() >= 2) {
    return 2;
  } else if ((b & 0xF0) == 0xE0 && s.size() >= 3) {
    return 3;
  } else if ((b & 0xF8) == 0xF0 && s.size() >= 4) {
    return 4;
  }
  return 0;
}

// The number of UTF-8 code points in s, counting each byte that does not
// start a complete one as one.
std::size_t count_codepoints(std::string_view s);

// Parses the whole of s as an integer with an optional sign ('+' or '-').
// Returns false if s is not one or it does not fit into an int.
bool parse_int(std::string_view s, int& value);

// s without its leading and trailing whitespace (' ', '\t', '\n', '\v', '\f'
// and '\r').
std::string_view trimmed(std::string_view s);

}  // namespace detail
}  // namespace citcpp

#endif /* PARSER_UTILS_HPP_ */
