#include <citcpp/test_set.hpp>

#include "detail/citcpp_utils.hpp"

namespace citcpp {

test_set::test_set(std::string_view value_separator)
    : value_separator_(value_separator), parameters_(), test_set_() {}

test_set::test_set() : test_set(detail::DEFAULT_VALUE_SEPARATOR) {}

std::ostream& operator<<(std::ostream& os, const test_set& test_set) {
  const std::string* sep = &detail::EMPTY_VALUE_SEPARATOR;
  for (const auto& param : test_set.get_parameters()) {
    os << *sep << param.get_name();
    sep = &test_set.value_separator_;
  }
  os << "\n";

  for (const auto& test : test_set.get_list_of_tests()) {
    sep = &detail::EMPTY_VALUE_SEPARATOR;
    for (std::vector<int>::size_type p = 0; p < test.size(); ++p) {
      int pv = test.at(p);

      const parameter& param = test_set.get_parameters()[p];
      if (pv >= 0 && (std::vector<parameter_value>::size_type)pv <
                         param.get_values().size()) {
        os << *sep << param.get_values()[pv];
      } else {
        os << *sep << DONT_CARE_PARAMETER_VALUE;
      }
      sep = &test_set.value_separator_;
    }
    os << "\n";
  }

  return os;
}

}  // namespace citcpp
