#ifndef IPOG_HORIZONTAL_EXTENSION_COMMON_HPP_
#define IPOG_HORIZONTAL_EXTENSION_COMMON_HPP_

#include <unordered_map>

#include "internal_test_set.hpp"

namespace citcpp {
namespace detail {

inline std::vector<int> get_value_to_valid_options(
    int num_current_param_values,
    const std::vector<citcpp::detail::bitset_uint64>& valid_values) {

  std::vector<int> valid_value_options(num_current_param_values, 0);

  for (const auto& test_valid_values : valid_values) {
    for (int v = 0; v < num_current_param_values; ++v) {
      if (test_valid_values.test(v)) {
        valid_value_options[v] += 1;
      }
    }
  }

  return valid_value_options;
}

struct new_covered_tuples_and_selected_value {
    unsigned long long num_new_covered_tuples_;
    int selected_value_;
};

struct ipog_horizontal_extension_result {
    std::vector<list_intrusive<test_list_intrusive_integ>> value_to_row_mapping;
    list_intrusive<test_list_intrusive_integ>
        rows_with_current_parameter_dont_care_value;
    std::unordered_map<const internal_relation*, unsigned long long>
        num_new_covered_tuples;
};

}  // namespace detail
}  // namespace citcpp

#endif /* IPOG_HORIZONTAL_EXTENSION_COMMON_HPP_ */
