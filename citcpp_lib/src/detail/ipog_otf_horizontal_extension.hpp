#ifndef IPOG_OTF_HORIZONTAL_EXTENSION_HPP_
#define IPOG_OTF_HORIZONTAL_EXTENSION_HPP_

#include "datatypes_config.hpp"
#include "internal_model.hpp"
#include "internal_test_set.hpp"
#include "ipog_horizontal_extension_common.hpp"

namespace citcpp {
namespace detail {

ipog_horizontal_extension_result ipog_horizontal_extension(
    const unsigned int current_param_idx, const unsigned int strength,
    const model &model, const std::vector<unsigned int> &parameter_index_map,
    const unsigned long long num_missing_combinations_to_cover,
    internal_test_set &test_set, bool is_extend_mode);

ipog_horizontal_extension_result ipog_horizontal_extension(
    const unsigned int current_param_idx, const unsigned int strength,
    const model &model, const std::vector<unsigned int> &parameter_index_map,
    const unsigned long long num_missing_combinations_to_cover,
    internal_test_set &test_set, bool is_extend_mode, thread_pool &tp);

}  // namespace detail
}  // namespace citcpp

#endif /* IPOG_OTF_HORIZONTAL_EXTENSION_HPP_ */
