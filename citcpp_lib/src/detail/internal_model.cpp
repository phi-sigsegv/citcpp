#include "internal_model.hpp"

#include <unordered_map>

#include "citcpp_utils.hpp"

namespace citcpp {
namespace detail {

internal_model::internal_model(const model& input_model)
    : input_model_(input_model), parameters_() {
  for (const parameter& p : input_model.get_parameters()) {
    parameters_.push_back(static_cast<unsigned int>(p.get_values().size()));
  }
}

const model& internal_model::get_input_model() const { return input_model_; }

test_set internal_model::create_from_internal_test_set(
    const internal_test_set& int_test_set) const {

  test_set ret(DEFAULT_VALUE_SEPARATOR);

  convert_test_set(int_test_set, ret);

  return ret;
}

test_set internal_model::create_from_internal_test_set(
    const internal_test_set& int_test_set,
    std::string_view value_separator) const {

  test_set ret(value_separator);

  convert_test_set(int_test_set, ret);

  return ret;
}

void internal_model::convert_test_set(const internal_test_set& src,
                                      test_set& tgt) const {

  for (const parameter& param : input_model_.get_parameters()) {
    tgt.add_parameter(param);
  }

  for (const test& test : src.get_list_of_tests()) {
    tgt.get_list_of_tests().emplace_back(test.get_values());
  }
}

internal_relation::internal_relation(
    const std::vector<unsigned int>& parameter_index_map,
    unsigned int specified_interaction_strength)
    : parameter_index_map_(parameter_index_map),
      specified_interaction_strength_(specified_interaction_strength),
      current_param_idx_(0) {}

internal_relation::internal_relation(
    std::vector<unsigned int>&& parameter_index_map,
    unsigned int specified_interaction_strength)
    : parameter_index_map_(std::move(parameter_index_map)),
      specified_interaction_strength_(specified_interaction_strength),
      current_param_idx_(0) {}

void internal_relation::sort_parameters(
    const std::vector<unsigned int>& parameter_index_map) {

  std::unordered_map<unsigned int, std::size_t> param_to_order_map;
  for (std::size_t i = 0; i < parameter_index_map.size(); ++i) {
    param_to_order_map[parameter_index_map[i]] = i;
  }

  std::sort(parameter_index_map_.begin(), parameter_index_map_.end(),
            [&param_to_order_map](const unsigned int& index1,
                                  const unsigned int& index2) {
              return param_to_order_map[index1] < param_to_order_map[index2];
            });
}

}  // namespace detail
}  // namespace citcpp
