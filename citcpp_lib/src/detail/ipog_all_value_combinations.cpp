#include "ipog_all_value_combinations.hpp"

#include <citcpp/function_ref.hpp>

#include "constraint_handler_concurrent.hpp"

namespace {

void recursively_add_test_for_each_combination(
    const citcpp::detail::internal_model& model,
    const std::vector<unsigned int>& parameter_index_map,
    unsigned int current_index, std::vector<unsigned int>& values,
    citcpp::detail::internal_test_set& test_set,
    citcpp::function_ref<bool(const citcpp::detail::test&)> predicate) {
  using namespace citcpp::detail;

  if (current_index == values.size()) {
    // Initialize all values of the test with don't care.
    test t(model.get_parameter_num_values().size(), -1);

    // Replace the first t elements with the cross product element.
    for (unsigned int index = 0; index < values.size(); ++index) {
      t.get_values()[parameter_index_map[index]] = values[index];
    }

    if (predicate(t)) {
      test_set.get_list_of_tests().push_back(std::move(t));
    }

    return;
  }

  const unsigned int max_val =
      model.get_parameter_num_values()[parameter_index_map[current_index]];

  for (unsigned int i = 0; i < max_val; ++i) {
    values[current_index] = i;
    recursively_add_test_for_each_combination(model, parameter_index_map,
                                              current_index + 1, values,
                                              test_set, predicate);
  }
}

}  // namespace

namespace citcpp {
namespace detail {

void create_all_value_combinations(
    unsigned int strength, const internal_model& model,
    const std::vector<unsigned int>& parameter_index_map,
    const constraint_handler& constr_handler, internal_test_set& test_set) {

  std::vector<unsigned int> values(strength);
  recursively_add_test_for_each_combination(
      model, parameter_index_map, 0, values, test_set,
      [&strength, &constr_handler](const test& t) {
        return constr_handler.is_valid_partial_test(t);
      });
}

void create_all_value_combinations(
    unsigned int strength, const internal_model& model,
    const std::vector<unsigned int>& parameter_index_map,
    const constraint_handler& constr_handler, internal_test_set& test_set,
    thread_pool& tp) {

  if (!constr_handler.is_thread_safe()) {
    // We fall back to the sequential algorithm.
    create_all_value_combinations(strength, model, parameter_index_map,
                                  constr_handler, test_set);

    return;
  }

  // If we reach this point, then we know that we can use the parallelized
  // version, since the constraint handler is thread safe.
  std::vector<unsigned int> values(strength);
  recursively_add_test_for_each_combination(
      model, parameter_index_map, 0, values, test_set,
      // We consider each test to be valid here, and filter out the invalid ones
      // afterwards. This is to exploit parallelization potential
      // in the expensive validity checks.
      [](const test& t) { return true; });

  concurrent_constraint_handler concurrent_handler(constr_handler);
  bitset_uint64 test_validity_info(
      concurrent_handler.check_validity_of_partial_tests(test_set, tp));

  // Now that we have the info which of the partial tests is valid,
  // we simply remove the invalid ones.
  auto test_it = test_set.get_list_of_tests().begin();
  unsigned int test_index = 0;
  while (test_it != test_set.get_list_of_tests().end()) {
    const auto& t = *test_it;
    if (!test_validity_info.test(test_index)) {
      test_it = test_set.get_list_of_tests().erase(test_it);
    } else {
      ++test_it;
    }
    ++test_index;
  }
}

}  // namespace detail
}  // namespace citcpp
