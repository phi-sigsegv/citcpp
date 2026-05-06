#include "constraint_handler_void.hpp"

namespace citcpp {
namespace detail {

constraint_handler_void::constraint_handler_void(const internal_model& model)
    : base_type(), model_(model) {}

bool constraint_handler_void::is_thread_safe() const { return true; }

bool constraint_handler_void::is_valid_partial_test(const test& t) const {
  return true;
}

bitset_uint64 constraint_handler_void::get_valid_parameter_assignments(
    const test& t, unsigned int param_idx) const {

  const int num_param_values = model_.get_parameter_num_values()[param_idx];
  bitset_uint64 values(num_param_values);
  values.set();

  return values;
}

void constraint_handler_void::replace_dont_care_values(test& t) const {
  for (unsigned int i = 0; i < t.get_values().size(); ++i) {
    int& value = t.get_values()[i];
    if (value < 0) {
      // Found don't care value. We simply replace it with the
      // first value of the respective parameter.
      value = 0;
    }
  }
}

void constraint_handler_void::cache_partial_test(const test* t) {}

void constraint_handler_void::update_cached_partial_test(const test* t) {}

void constraint_handler_void::update_cached_partial_test(const test* t,
                                                         unsigned int param_idx,
                                                         int value) {}

}  // namespace detail
}  // namespace citcpp
