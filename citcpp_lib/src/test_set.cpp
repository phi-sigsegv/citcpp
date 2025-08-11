#include <citcpp/test_set.hpp>

#include "detail/citcpp_utils.hpp"

namespace citcpp {

test_set::test_set(std::string_view value_separator)
    : value_separator_(value_separator), parameters_(), test_set_() {}

test_set::test_set() : test_set(detail::DEFAULT_VALUE_SEPARATOR) {}

std::ostream &operator<<(std::ostream &os, const test_set &test_set) {
  const std::string *sep = &detail::EMPTY_VALUE_SEPARATOR;
  for (const auto &param : test_set.get_parameters()) {
    os << *sep << param;
    sep = &test_set.value_separator_;
  }
  os << "\n";

  for (const auto &test : test_set.get_list_of_tests()) {
    sep = &detail::EMPTY_VALUE_SEPARATOR;
    for (const auto &pv : test) {
      os << *sep << pv;
      sep = &test_set.value_separator_;
    }
    os << "\n";
  }

  return os;
}

}  // namespace citcpp
