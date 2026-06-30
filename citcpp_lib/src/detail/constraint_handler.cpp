#include "constraint_handler.hpp"

#include "constraint_handler_sylvan_ldd.hpp"
#include "constraint_handler_void.hpp"
#include "idd_variable_ordering.hpp"

namespace citcpp {
namespace detail {

constraint_handler::~constraint_handler() {}

bitset_uint64 constraint_handler::check_validity_of_partial_tests(
    const internal_test_set& test_set) const {

  bitset_uint64 result(test_set.get_list_of_tests().size());

  unsigned int test_index = 0;
  for (const auto& t : test_set.get_list_of_tests()) {
    if (is_valid_partial_test(t)) {
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

std::unique_ptr<constraint_handler>
constraint_handler::create_constraint_handler(
    const internal_model& model, int num_worker_threads,
    constraint_handler_init_progress& exec_handle) {

  if (model.get_input_model().get_constraints().empty()) {
    exec_handle.set_constraint_handler_init_progress_target(0);
    exec_handle.set_constraint_handler_init_progress_current(0);
    return std::make_unique<constraint_handler_void>(model);
  } else {
    exec_handle.set_constraint_handler_init_progress_target(
        model.get_input_model().get_constraints().size());
    exec_handle.set_constraint_handler_init_progress_current(0);

    std::vector<unsigned int> variable_order =
        compute_mcmf_variable_order(model);

    constraint_handler_sylvan_idd* handler = new constraint_handler_sylvan_idd(
        model, variable_order, num_worker_threads, exec_handle);
    handler->use_per_test_idd(false);
    return std::unique_ptr<constraint_handler_sylvan_idd>(handler);
  }
}

}  // namespace detail
}  // namespace citcpp
