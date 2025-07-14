#ifndef DETAIL_CITCPP_ALGO_COMMON_HPP_
#define DETAIL_CITCPP_ALGO_COMMON_HPP_

#include "datatypes_config.hpp"
#include "internal_model.hpp"

namespace citcpp
{
  namespace detail
  {
    /**
     * Returns the number of tuple combinations to cover for the given model and
     * interaction strength.
     */
    unsigned long long
    number_of_combinations_to_cover (const model &model, unsigned int t);

    /**
     * Returns the number of tuple combinations to cover for the given model and
     * interaction strength.
     */
    unsigned long long
    number_of_combinations_to_cover (thread_pool &tp, const model &model,
				     unsigned int t);

    /**
     * Returns the number of tuple combinations to cover for the given model and
     * interaction strength.
     */
    unsigned long long
    number_of_combinations_to_cover (
	unsigned int current_param_idx, const model &model,
	const std::vector<unsigned int> &parameter_index_map, unsigned int t);

    /**
     * Returns the number of tuple combinations to cover for the given model and
     * interaction strength.
     */
    unsigned long long
    number_of_combinations_to_cover (
	thread_pool &tp, unsigned int current_param_idx, const model &model,
	const std::vector<unsigned int> &parameter_index_map, unsigned int t);
  }
}

#endif /* DETAIL_CITCPP_ALGO_COMMON_HPP_ */
