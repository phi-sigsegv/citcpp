#include "constraint_handler.hpp"

namespace citcpp {
namespace detail {

bitset_uint64 constraint_handler::check_validity_of_partial_tests(
    const internal_test_set& test_set,
    unsigned int param_index_range_end) const {

  bitset_uint64 result(test_set.get_list_of_tests().size());

  unsigned int test_index = 0;
  for (const auto& t : test_set.get_list_of_tests()) {
    if (is_valid_partial_test(t, param_index_range_end)) {
      result.set(test_index);
    }
    ++test_index;
  }

  return result;
}

std::vector<bitset_uint64> constraint_handler::get_valid_parameter_assignments(
    const internal_test_set& test_set, unsigned int param_idx) const {

  std::vector<bitset_uint64> result(test_set.get_list_of_tests().size());

  unsigned int test_index = 0;
  for (const auto& t : test_set.get_list_of_tests()) {
    result[test_index] = get_valid_parameter_assignments(t, param_idx);
    ++test_index;
  }

  return result;
}

void constraint_handler::replace_dont_care_values(
    internal_test_set& test_set) const {

  for (auto& t : test_set.get_list_of_tests()) {
    replace_dont_care_values(t);
  }
}

}  // namespace detail
}  // namespace citcpp
