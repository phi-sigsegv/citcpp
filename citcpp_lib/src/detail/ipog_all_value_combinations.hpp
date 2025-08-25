#ifndef IPOG_ALL_VALUE_COMBINATIONS_HPP_
#define IPOG_ALL_VALUE_COMBINATIONS_HPP_

#include "internal_model.hpp"
#include "internal_test_set.hpp"

namespace citcpp {
namespace detail {

/**
 * This represents the result of the execution of function
 * create_all_value_combinations.
 */
struct create_all_value_combinations_result {
    /**
     * The number of elements of the created cartesian product.
     */
    unsigned long long num_created_combinations;
};

/**
 * Create the full cartesian product of the values of t parameters from
 * the model and write them to the given test set.
 * The given parameter_indices specify the parameters whose values to combine.
 * The algorithm returns the number of elements of the created cartesian
 * product.
 */
create_all_value_combinations_result create_all_value_combinations(
    unsigned int strength, const model &model,
    const std::vector<unsigned int> &parameter_index_map,
    internal_test_set &test_set);

}  // namespace detail
}  // namespace citcpp

#endif /* IPOG_ALL_VALUE_COMBINATIONS_HPP_ */
