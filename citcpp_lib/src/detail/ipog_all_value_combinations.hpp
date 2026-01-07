#ifndef IPOG_ALL_VALUE_COMBINATIONS_HPP_
#define IPOG_ALL_VALUE_COMBINATIONS_HPP_

#include "constraint_handler.hpp"
#include "datatypes_config.hpp"
#include "internal_model.hpp"
#include "internal_test_set.hpp"

namespace citcpp {
namespace detail {

/**
 * Create the full cartesian product of the values of t parameters from
 * the model and write them to the given test set.
 * The given parameter_index_map specifies the parameters whose values to
 * combine.
 */
void create_all_value_combinations(
    unsigned int strength, const internal_model& model,
    const std::vector<unsigned int>& parameter_index_map,
    const constraint_handler& constr_handler, internal_test_set& test_set);

/**
 * Create the full cartesian product of the values of t parameters from
 * the model and write them to the given test set.
 * The given parameter_index_map specifies the parameters whose values to
 * combine.
 */
void create_all_value_combinations(
    unsigned int strength, const internal_model& model,
    const std::vector<unsigned int>& parameter_index_map,
    const constraint_handler& constr_handler, internal_test_set& test_set,
    thread_pool& tp);

}  // namespace detail
}  // namespace citcpp

#endif /* IPOG_ALL_VALUE_COMBINATIONS_HPP_ */
