#include "constraint_handler_void.hpp"

namespace citcpp {
namespace detail {

constraint_handler_void::constraint_handler_void(const internal_model& model)
    : model_(model) {}

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

}  // namespace detail
}  // namespace citcpp
