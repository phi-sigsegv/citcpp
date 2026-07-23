#include "constraint_handler_void.hpp"

namespace citcpp {
namespace detail {

constraint_handler_void::constraint_handler_void(const internal_model& model)
    : base_type(), model_(model) {}

bool constraint_handler_void::is_thread_safe() const { return true; }

bool constraint_handler_void::is_valid_partial_test(const test&) const {
  return true;
}

void constraint_handler_void::mark_valid_tuples(
    coverage_bitset& value_combinations, const param_vector&) const {

  value_combinations.set_all_valid();
}

bitset_uint64 constraint_handler_void::get_valid_parameter_assignments(
    const test&, unsigned int param_idx) const {

  const unsigned int num_param_values =
      model_.get_parameter_num_values()[param_idx];
  bitset_uint64 values(static_cast<bitset_uint64::size_type>(num_param_values));
  values.set();

  return values;
}

void constraint_handler_void::replace_dont_care_values(test& t) const {
  for (std::size_t i = 0; i < t.get_values().size(); ++i) {
    int& value = t.get_values()[i];
    if (value < 0) {
      // Found don't care value. We simply replace it with the
      // first value of the respective parameter.
      value = 0;
    }
  }
}

test_list_intrusive_integ*
constraint_handler_void::get_first_test_valid_for_assignment(
    list_intrusive<test_list_intrusive_integ>& test_list,
    const param_vector& param_indices,
    const value_vector& value_indices) const {

  if (test_list.empty()) {
    return nullptr;
  }

  for (test_list_intrusive_integ& list_node : test_list) {
    test& t = list_node.get_test();

    bool covers_combo = true;
    for (std::size_t i = 0; i < param_indices.size(); ++i) {
      const int param_value_to_assign = value_indices[i];
      const int param_value_in_test = t.get_values()[param_indices[i]];

      if (param_value_in_test >= 0 &&
          param_value_to_assign != param_value_in_test) {
        // Cannot inject value combination in this test, moving on to the next
        // one.
        covers_combo = false;
        break;
      }
    }

    if (covers_combo) {
      return &list_node;
    }
  }

  return nullptr;
}

void constraint_handler_void::cache_partial_test(const test*) {}

void constraint_handler_void::update_cached_partial_test(const test*) {}

void constraint_handler_void::update_cached_partial_test(const test*,
                                                         unsigned int, int) {}

}  // namespace detail
}  // namespace citcpp
