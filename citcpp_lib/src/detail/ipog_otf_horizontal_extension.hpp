#ifndef IPOG_OTF_HORIZONTAL_EXTENSION_HPP_
#define IPOG_OTF_HORIZONTAL_EXTENSION_HPP_

#include "constraint_handler.hpp"
#include "functor_executor.hpp"
#include "internal_model.hpp"
#include "internal_test_set.hpp"
#include "ipog_horizontal_extension_common.hpp"

namespace citcpp {
namespace detail {

ipog_horizontal_extension_result ipog_horizontal_extension(
    const unsigned long long num_missing_combinations_to_cover,
    const constraint_handler& constr_handler, internal_test_set& test_set,
    const internal_model& model,
    const std::vector<internal_relation>& relations, bool is_extend_mode);

ipog_horizontal_extension_result ipog_horizontal_extension(
    const unsigned long long num_missing_combinations_to_cover,
    const constraint_handler& constr_handler, internal_test_set& test_set,
    const internal_model& model,
    const std::vector<internal_relation>& relations, bool is_extend_mode,
    functor_executor& exec);

}  // namespace detail
}  // namespace citcpp

#endif /* IPOG_OTF_HORIZONTAL_EXTENSION_HPP_ */
