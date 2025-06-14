#ifndef DETAIL_CITCPP_ALGO_COMMON_HPP_
#define DETAIL_CITCPP_ALGO_COMMON_HPP_

#include <taskflow/taskflow.hpp>
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
    number_of_combinations_to_cover (tf::Executor &executor, const model &model,
				     unsigned int t);
  }
}

#endif /* DETAIL_CITCPP_ALGO_COMMON_HPP_ */
