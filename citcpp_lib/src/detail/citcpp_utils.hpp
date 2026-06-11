#ifndef CITCPP_UTILS_HPP_
#define CITCPP_UTILS_HPP_

#include <citcpp/model.hpp>
#include <citcpp/test_set.hpp>
#include <new>
#include <string>
#include <utility>

#include "bitset.hpp"
#include "datatypes_config.hpp"
#include "internal_model.hpp"
#include "internal_test_set.hpp"

namespace {

template <class T_VISITOR, typename... T_ADDITIONAL_VISITOR_ARGS>
bool recursively_visit_all_value_combos_of_param_combo(
    const citcpp::detail::internal_model& model,
    const citcpp::detail::param_vector& param_indices,
    citcpp::detail::value_vector& value_indices, int current_index,
    citcpp::detail::bitset_uint64::size_type partial_bit_pos,
    T_VISITOR& visitor,
    T_ADDITIONAL_VISITOR_ARGS&&... additional_visitor_args) {
  using namespace citcpp::detail;

  // The current range goes from 0 to max_value[current_index]
  const unsigned int max_val =
      model.get_parameter_num_values()[param_indices[current_index]];

  bitset_uint64::size_type bit_pos_value_factor = 1;
  for (std::vector<unsigned int>::size_type j = current_index + 1;
       j < param_indices.size(); ++j) {
    bit_pos_value_factor *= model.get_parameter_num_values()[param_indices[j]];
  }

  for (int i = max_val - 1; i >= 0; --i) {
    value_indices[current_index] = i;

    bool ret = true;

    if (current_index == 0) {
      bitset_uint64::size_type bit_pos =
          partial_bit_pos + i * bit_pos_value_factor;
      // Call the visitor.
      ret = visitor(
          value_indices, bit_pos,
          std::forward<T_ADDITIONAL_VISITOR_ARGS>(additional_visitor_args)...);
    } else {
      ret = recursively_visit_all_value_combos_of_param_combo(
          model, param_indices, value_indices, current_index - 1,
          partial_bit_pos + i * bit_pos_value_factor, visitor,
          std::forward<T_ADDITIONAL_VISITOR_ARGS>(additional_visitor_args)...);
    }

    if (!ret) {
      return false;
    }
  }

  return true;
}

}  // namespace

namespace citcpp {
namespace detail {

extern const std::string EMPTY_VALUE_SEPARATOR;
extern const std::string DEFAULT_VALUE_SEPARATOR;

internal_test_set create_internal_test_set(const model& input_model,
                                           const citcpp::test_set& tests);

unsigned int get_product_of_max_n_parameter_sizes(
    const unsigned int num_parameters, const unsigned int n,
    const citcpp::detail::internal_model& model,
    const std::vector<unsigned int>& parameter_index_map);

template <class T_VISITOR, typename... T_ADDITIONAL_VISITOR_ARGS>
void visit_all_value_combos_of_param_combo(
    const citcpp::detail::internal_model& model,
    const citcpp::detail::param_vector& param_indices,
    citcpp::detail::value_vector& value_indices, T_VISITOR& visitor,
    T_ADDITIONAL_VISITOR_ARGS&&... additional_visitor_args) {
  using namespace citcpp::detail;

  recursively_visit_all_value_combos_of_param_combo(
      model, param_indices, value_indices, value_indices.size() - 1, 0, visitor,
      std::forward<T_ADDITIONAL_VISITOR_ARGS>(additional_visitor_args)...);
}

}  // namespace detail
}  // namespace citcpp

#endif /* CITCPP_UTILS_HPP_ */
