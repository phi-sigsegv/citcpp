#ifndef IPOG_HORIZONTAL_EXTENSION_HPP_
#define IPOG_HORIZONTAL_EXTENSION_HPP_

#include <utility>
#include <vector>

#include "constraint_handler.hpp"
#include "coverage_map.hpp"
#include "internal_model.hpp"
#include "internal_test_set.hpp"
#include "ipog_horizontal_extension_common.hpp"

namespace citcpp {
namespace detail {

ipog_horizontal_extension_result ipog_horizontal_extension(
    unsigned long long num_missing_combinations_to_cover,
    constraint_handler& constr_handler, internal_test_set& test_set,
    std::vector<std::pair<const internal_relation*, ipog_coverage_map>>&
        relations);

}  // namespace detail
}  // namespace citcpp

#endif /* IPOG_HORIZONTAL_EXTENSION_HPP_ */
