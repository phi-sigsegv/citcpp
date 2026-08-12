#include <citcpp/test_set.hpp>

#include "detail/citcpp_utils.hpp"

namespace citcpp {

test_set::test_set(std::string_view value_separator)
    : value_separator_(value_separator), parameters_(), test_set_() {}

test_set::test_set() : test_set(detail::DEFAULT_VALUE_SEPARATOR) {}

const std::string& test_set::get_value_separator() const {
  return value_separator_;
}

const std::vector<parameter>& test_set::get_parameters() const {
  return parameters_;
}

std::vector<parameter>& test_set::get_parameters() { return parameters_; }

void test_set::add_parameter(parameter parameter) {
  parameters_.push_back(std::move(parameter));
}

const std::list<std::vector<int>>& test_set::get_list_of_tests() const {
  return test_set_;
}

std::list<std::vector<int>>& test_set::get_list_of_tests() { return test_set_; }

bool test_set::operator==(const test_set& other) const {
  return parameters_ == other.parameters_ && test_set_ == other.test_set_;
}

std::ostream& operator<<(std::ostream& os, const test_set& test_set) {
  const std::string* sep = &detail::EMPTY_VALUE_SEPARATOR;
  for (const auto& param : test_set.get_parameters()) {
    os << *sep << param.get_name();
    sep = &test_set.value_separator_;
  }
  os << "\n";

  for (const auto& test : test_set.get_list_of_tests()) {
    sep = &detail::EMPTY_VALUE_SEPARATOR;
    for (std::size_t p = 0; p < test.size(); ++p) {
      int pv = test.at(p);

      const parameter& param = test_set.get_parameters()[p];
      if (pv >= 0 && static_cast<std::size_t>(pv) < param.get_values().size()) {
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
