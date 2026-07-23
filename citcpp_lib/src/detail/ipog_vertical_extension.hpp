#ifndef IPOG_VERTICAL_EXTENSION_HPP_
#define IPOG_VERTICAL_EXTENSION_HPP_

#include <utility>
#include <vector>

#include "constraint_handler.hpp"
#include "coverage_map.hpp"
#include "internal_model.hpp"
#include "internal_test_set.hpp"
#include "ipog_horizontal_extension_common.hpp"
#include "ipog_vertical_extension_common.hpp"

namespace citcpp {
namespace detail {

ipog_vertical_extension_result ipog_vertical_extension(
    unsigned long long num_missing_combinations_to_cover,
    constraint_handler& constr_handler,
    ipog_horizontal_extension_result&
        partitioning_of_tests_according_to_current_values,
    internal_test_set& test_set,
    std::vector<std::pair<const internal_relation*, ipog_coverage_map>>&
        relations);

}  // namespace detail
}  // namespace citcpp

#include "ipog_vertical_extension.tpp"

#endif /* IPOG_VERTICAL_EXTENSION_HPP_ */
