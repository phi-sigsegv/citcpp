#include "citcpp_utils.hpp"

#include <queue>
#include <unordered_map>
#include <vector>

namespace citcpp {
namespace detail {

const std::string EMPTY_VALUE_SEPARATOR = "";
const std::string DEFAULT_VALUE_SEPARATOR = ", ";

internal_test_set create_internal_test_set(const model& input_model,
                                           const test_set& tests) {

  struct ParamValueHash {
      std::size_t operator()(const parameter_value& v) const {
        std::size_t h = std::hash<std::variant<bool, std::string, int>>{}(
            v.get_variant_value());
        return h;
      }
  };

  std::vector<std::unordered_map<parameter_value, int, ParamValueHash>>
      all_param_value_mappings(
          input_model.get_parameters().size(),
          std::unordered_map<parameter_value, int, ParamValueHash>());

  {
    int model_param_index = 0;
    for (const parameter& param : input_model.get_parameters()) {
      auto& param_value_mappings = all_param_value_mappings[model_param_index];

      int param_value_index = 0;
      for (const parameter_value& param_value : param.get_values()) {
        param_value_mappings[param_value] = param_value_index;

        ++param_value_index;
      }

      ++model_param_index;
    }
  }

  std::vector<int> param_mapping(tests.get_parameters().size(), -1);

  {
    int test_param_index = 0;
    for (const parameter& param_in_test : tests.get_parameters()) {
      int model_param_index = 0;
      for (const parameter& param_in_model : input_model.get_parameters()) {
        if (param_in_model.get_name() == param_in_test.get_name() &&
            param_in_model.get_type() == param_in_test.get_type()) {

          param_mapping[test_param_index] = model_param_index;
          break;
        }

        ++model_param_index;
      }

      ++test_param_index;
    }
  }

  internal_test_set internal_test_set;

  {
    for (const std::vector<int>& values : tests.get_list_of_tests()) {
      test internal_test(all_param_value_mappings.size(), -2);
      int test_param_index = 0;
      for (const int test_param_value_idx : values) {
        if (param_mapping[test_param_index] >= 0) {
          int model_param_index = param_mapping[test_param_index];
          if (test_param_value_idx < 0) {
            internal_test.get_values()[model_param_index] = -1;
          } else {
            const auto& param_value_mappings =
                all_param_value_mappings[model_param_index];

            auto param_value_idx_it = param_value_mappings.find(
                tests.get_parameters()[test_param_index]
                    .get_values()[test_param_value_idx]);
            if (param_value_idx_it != param_value_mappings.end()) {
              internal_test.get_values()[model_param_index] =
                  param_value_idx_it->second;
            }
          }
        }

        ++test_param_index;
      }

      internal_test_set.get_list_of_tests().push_back(std::move(internal_test));
    }
  }

  return internal_test_set;
}

unsigned int get_product_of_max_n_parameter_sizes(
    const unsigned int num_parameters, const unsigned int n,
    const citcpp::detail::internal_model& model,
    const std::vector<unsigned int>& parameter_index_map) {

  // Define a min-heap.
  // std::priority_queue is a max-heap by default.
  // We use std::greater to make it a min-heap.
  // The smallest element is at the top.
  std::priority_queue<unsigned int, std::vector<unsigned int>,
                      std::greater<unsigned int>>
      min_heap;

  for (unsigned int i = 0; i < num_parameters; ++i) {
    const unsigned int num_param_values =
        model.get_parameter_num_values()[parameter_index_map[i]];

    if (min_heap.size() < n) {
      // Case 1: Heap is not full, just insert the element.
      min_heap.push(num_param_values);
    } else {
      // Case 2: Heap is full (size == n).
      // Check if the current number is greater than the smallest element in
      // the heap (the top).
      if (num_param_values > min_heap.top()) {
        // Remove the smallest element
        min_heap.pop();
        // Insert the larger current number
        min_heap.push(num_param_values);
      }
      // If current_num is <= min_heap.top(), it's not one of the n largest,
      // so we ignore it.
    }
  }

  unsigned int product = 1;
  while (!min_heap.empty()) {
    product *= min_heap.top();
    min_heap.pop();
  }

  return product;
}

}  // namespace detail
}  // namespace citcpp
