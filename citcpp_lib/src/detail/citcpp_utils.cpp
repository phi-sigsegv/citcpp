#include "citcpp_utils.hpp"

#include <queue>
#include <unordered_map>
#include <vector>

namespace citcpp {
namespace detail {

const std::string EMPTY_VALUE_SEPARATOR = "";
const std::string DEFAULT_VALUE_SEPARATOR = ", ";
const citcpp::parameter_value DONT_CARE_PARAMETER_VALUE{"*"};

internal_test_set create_internal_test_set(const input_model &input_model,
                                           const test_set &tests) {

  struct ParamValueHash {
      std::size_t operator()(const parameter_value &v) const {
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
    for (const parameter &param : input_model.get_parameters()) {
      auto &param_value_mappings = all_param_value_mappings[model_param_index];

      int param_value_index = 0;
      for (const parameter_value &param_value : param.get_values()) {
        param_value_mappings[param_value] = param_value_index;

        ++param_value_index;
      }

      ++model_param_index;
    }
  }

  std::vector<int> param_mapping(tests.get_parameters().size(), -1);

  {
    int test_param_index = 0;
    for (const parameter_def &param_def : tests.get_parameters()) {
      int model_param_index = 0;
      for (const parameter &param : input_model.get_parameters()) {
        if (param.get_name() == param_def.get_name() &&
            param.get_type() == param_def.get_type()) {

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
    for (const std::vector<parameter_value> &values :
         tests.get_list_of_tests()) {

      test internal_test(all_param_value_mappings.size(), -2);
      int test_param_index = 0;
      for (const parameter_value &param_value : values) {
        if (param_mapping[test_param_index] >= 0) {
          int model_param_index = param_mapping[test_param_index];
          const auto &param_value_mappings =
              all_param_value_mappings[model_param_index];

          auto param_value_idx_it = param_value_mappings.find(param_value);
          if (param_value_idx_it != param_value_mappings.end()) {
            internal_test.get_values()[model_param_index] =
                param_value_idx_it->second;
          } else if (param_value == DONT_CARE_PARAMETER_VALUE) {
            // Cannot find value. If it is a don't care, then set its index to
            // -1.
            internal_test.get_values()[model_param_index] = -1;
          }
        }

        ++test_param_index;
      }

      internal_test_set.get_list_of_tests().push_back(std::move(internal_test));
    }
  }

  return internal_test_set;
}

void replace_dont_care_values(internal_test_set &test_set, const model &model) {

  for (test &t : test_set.get_list_of_tests()) {
    for (unsigned int i = 0; i < t.get_values().size(); ++i) {
      int &value = t.get_values()[i];
      if (value < 0) {
        // Found don't care value. We simply replace it with the
        // first value of the respective parameter.
        value = 0;
      }
    }
  }
}

unsigned int get_product_of_max_n_parameter_sizes(
    const unsigned int num_parameters, const unsigned int n,
    const citcpp::detail::model &model,
    const std::vector<unsigned int> &parameter_index_map) {

  // Define a min-heap.
  // std::priority_queue is a max-heap by default.
  // We use std::greater to make it a min-heap.
  // The smallest element is at the top.
  std::priority_queue<unsigned int, std::vector<unsigned int>,
                      std::greater<unsigned int>>
      min_heap;

  for (unsigned int i = 0; i < num_parameters; ++i) {
    const unsigned int num_param_values =
        model.get_parameters()[parameter_index_map[i]];

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
