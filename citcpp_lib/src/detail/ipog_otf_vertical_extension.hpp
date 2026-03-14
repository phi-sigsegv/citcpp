#ifndef IPOG_OTF_VERTICAL_EXTENSION_HPP_
#define IPOG_OTF_VERTICAL_EXTENSION_HPP_

#include <functional>

#include "constraint_handler.hpp"
#include "coverage_map.hpp"
#include "internal_model.hpp"
#include "internal_test_set.hpp"
#include "ipog_horizontal_extension_common.hpp"
#include "ipog_vertical_extension_common.hpp"

namespace citcpp {
namespace detail {

ipog_vertical_extension_result ipog_otf_vertical_extension(
    const unsigned long long num_missing_combinations_to_cover,
    const constraint_handler& constr_handler,
    ipog_horizontal_extension_result&
        partitioning_of_tests_according_to_current_values,
    internal_test_set& test_set, const internal_model& model,
    const std::vector<internal_relation>& relations);

}  // namespace detail
}  // namespace citcpp

#include "ipog_otf_vertical_extension.tpp"

#endif /* IPOG_OTF_VERTICAL_EXTENSION_HPP_ */
