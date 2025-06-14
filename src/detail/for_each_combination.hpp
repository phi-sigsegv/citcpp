#ifndef DETAIL_FOR_EACH_COMBINATION_HPP_
#define DETAIL_FOR_EACH_COMBINATION_HPP_

#include <functional>
#include <vector>
#include <taskflow/taskflow.hpp>

namespace citcpp
{
  namespace detail
  {
    /**
     * Iterates over all combinations of size k
     * chosen from a range of [0, ... , n - 1] in lexicographical order.
     * For each combination the given callback is executed.
     */
    void
    for_each_combination (unsigned int n, unsigned int k,
			  const std::function<void
			  (const std::vector<unsigned int>&)> &callback);

    /**
     * Iterates over all combinations of size k
     * chosen from a range of [0, ... , n - 1] in lexicographical order.
     * For each combination the given callback is executed.
     * The iteration may be parallelized if using this overload. The parallelization works by
     * partitioning the number of combinations according to the first element in lexicographical order.
     * Combinations starting with [0, ... , n - 1] are each executed as a separated
     * task, meaning the first element in the combination passed to the callback can be used
     * in order to avoid data races.
     */
    void
    for_each_combination (unsigned int n, unsigned int k,
			  tf::Executor &executor, const std::function<void
			  (const std::vector<unsigned int>&)> &callback);
  }
}

#endif /* DETAIL_FOR_EACH_COMBINATION_HPP_ */
