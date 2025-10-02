#ifndef IPOG_HORIZONTAL_EXTENSION_HPP_
#define IPOG_HORIZONTAL_EXTENSION_HPP_

#include "coverage_map.hpp"
#include "internal_model.hpp"
#include "internal_test_set.hpp"
#include "ipog_horizontal_extension_common.hpp"

namespace citcpp {
namespace detail {

ipog_horizontal_extension_result ipog_horizontal_extension(
    const unsigned int current_param_idx,
    const unsigned long long num_missing_combinations_to_cover,
    internal_test_set &test_set, coverage_map &cov_map);

ipog_horizontal_extension_result ipog_horizontal_extension(
    const unsigned int current_param_idx,
    const unsigned long long num_missing_combinations_to_cover,
    internal_test_set &test_set, coverage_map &cov_map, thread_pool &tp);

}  // namespace detail
}  // namespace citcpp

#endif /* IPOG_HORIZONTAL_EXTENSION_HPP_ */
