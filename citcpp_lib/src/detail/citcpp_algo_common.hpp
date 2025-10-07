#ifndef DETAIL_CITCPP_ALGO_COMMON_HPP_
#define DETAIL_CITCPP_ALGO_COMMON_HPP_

#include "datatypes_config.hpp"
#include "internal_model.hpp"

namespace citcpp {
namespace detail {

/**
 * Returns the number of t-tuples to cover for n parameters
 * (indices [0, ... ,n-1]) from the given model and
 * interaction strength t.
 * Depending on the value of the parameter \a fixed_last_parameter, the last
 * parameter is fixed. Or in other words: We count tuples of length t-1
 * from the parameters [0, ... ,n-2], and extend those by always
 * appending a value from parameter n-1 to them.
 */
unsigned long long number_of_combinations_to_cover(
    unsigned int n, const internal_model &model,
    const std::vector<unsigned int> &parameter_index_map, unsigned int t,
    bool fixed_last_parameter);

/**
 * Returns the number of t-tuples to cover for n parameters
 * (indices [0, ... ,n-1]) from the given model and
 * interaction strength t.
 * Depending on the value of the parameter \a fixed_last_parameter, the last
 * parameter is fixed. Or in other words: We count tuples of length t-1
 * from the parameters [0, ... ,n-2], and extend those by always
 * appending a value from parameter n-1 to them.
 */
unsigned long long number_of_combinations_to_cover(
    unsigned int n, const internal_model &model,
    const std::vector<unsigned int> &parameter_index_map, unsigned int t,
    bool fixed_last_parameter, thread_pool &tp);

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_CITCPP_ALGO_COMMON_HPP_ */
