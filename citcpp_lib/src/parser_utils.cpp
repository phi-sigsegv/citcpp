#include "parser_utils.hpp"

#include <charconv>
#include <system_error>

namespace citcpp {
namespace detail {

std::size_t count_codepoints(std::string_view s) {
  std::size_t count = 0;
  for (std::size_t i = 0; i < s.size(); ++count) {
    const std::size_t len = codepoint_length(s.substr(i));
    i += len == 0 ? 1 : len;
  }
  return count;
}

bool parse_int(std::string_view s, int& value) {
  // std::from_chars does not accept a '+' sign.
  if (s.size() > 1 && s[0] == '+' && s[1] != '-') {
    s.remove_prefix(1);
  }
  const char* end = s.data() + s.size();
  const auto [ptr, ec] = std::from_chars(s.data(), end, value);
  return ec == std::errc() && ptr == end;
}

std::string_view trimmed(std::string_view s) {
  constexpr std::string_view whitespace = " \t\n\v\f\r";
  const std::size_t first = s.find_first_not_of(whitespace);
  if (first == s.npos) {
    return {};
  }
  return s.substr(first, s.find_last_not_of(whitespace) - first + 1);
}

}  // namespace detail
}  // namespace citcpp
