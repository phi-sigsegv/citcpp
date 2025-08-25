#ifndef IPOG_HORIZONTAL_EXTENSION_HPP_
#define IPOG_HORIZONTAL_EXTENSION_HPP_

#include "coverage_map.hpp"
#include "internal_model.hpp"
#include "internal_test_set.hpp"

namespace citcpp {
namespace detail {

struct ipog_horizontal_extension_result {
    std::vector<list_intrusive<test>> value_to_row_mapping;
    list_intrusive<test> rows_with_current_parameter_dont_care_value;
    unsigned long long num_new_covered_tuples;
};

ipog_horizontal_extension_result ipog_horizontal_extension(
    const unsigned int current_param_idx, const unsigned int strength,
    const model &model, const std::vector<unsigned int> &parameter_index_map,
    const unsigned long long num_missing_combinations_to_cover,
    internal_test_set &test_set, coverage_map &cov_map);

ipog_horizontal_extension_result ipog_horizontal_extension(
    const unsigned int current_param_idx, const unsigned int strength,
    const model &model, const std::vector<unsigned int> &parameter_index_map,
    const unsigned long long num_missing_combinations_to_cover,
    internal_test_set &test_set, coverage_map &cov_map, thread_pool &tp);

}  // namespace detail
}  // namespace citcpp

#endif /* IPOG_HORIZONTAL_EXTENSION_HPP_ */
