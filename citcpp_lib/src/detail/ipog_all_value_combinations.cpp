#include "ipog_all_value_combinations.hpp"

namespace {

void recursively_add_test_for_each_combination(
    const citcpp::detail::internal_model& model,
    const std::vector<unsigned int>& parameter_index_map,
    unsigned int current_index, std::vector<unsigned int>& values,
    citcpp::detail::internal_test_set& test_set) {
  using namespace citcpp::detail;

  if (current_index == values.size()) {
    // Initialize all values of the test with don't care.
    test t(model.get_parameter_num_values().size(), -1);

    // Replace the first t elements with the cross product element.
    for (unsigned int index = 0; index < values.size(); ++index) {
      t.get_values()[parameter_index_map[index]] = values[index];
    }

    test_set.get_list_of_tests().push_back(std::move(t));

    return;
  }

  const unsigned int max_val =
      model.get_parameter_num_values()[parameter_index_map[current_index]];

  for (unsigned int i = 0; i < max_val; ++i) {
    values[current_index] = i;
    recursively_add_test_for_each_combination(
        model, parameter_index_map, current_index + 1, values, test_set);
  }
}

}  // namespace

namespace citcpp {
namespace detail {

create_all_value_combinations_result create_all_value_combinations(
    unsigned int strength, const internal_model& model,
    const std::vector<unsigned int>& parameter_index_map,
    citcpp::detail::internal_test_set& test_set) {

  create_all_value_combinations_result result{0};

  auto previous_test_set_size = test_set.get_list_of_tests().size();
  std::vector<unsigned int> values(strength);
  recursively_add_test_for_each_combination(model, parameter_index_map, 0,
                                            values, test_set);

  result.num_created_combinations =
      test_set.get_list_of_tests().size() - previous_test_set_size;

  return result;
}

}  // namespace detail
}  // namespace citcpp
