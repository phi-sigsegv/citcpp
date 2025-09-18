#ifndef IPOG_OTF_VERTICAL_EXTENSION_HPP_
#define IPOG_OTF_VERTICAL_EXTENSION_HPP_

#include "coverage_map.hpp"
#include "internal_model.hpp"
#include "internal_test_set.hpp"
#include "ipog_horizontal_extension_common.hpp"
#include "ipog_vertical_extension_common.hpp"

namespace citcpp {
namespace detail {

ipog_vertical_extension_result ipog_vertical_extension(
    const unsigned int current_param_idx, const unsigned int strength,
    const model &model, const std::vector<unsigned int> &parameter_index_map,
    const unsigned long long num_missing_combinations_to_cover,
    ipog_horizontal_extension_result
        &partitioning_of_tests_according_to_current_values,
    internal_test_set &test_set);

}  // namespace detail
}  // namespace citcpp

#endif /* IPOG_OTF_VERTICAL_EXTENSION_HPP_ */
