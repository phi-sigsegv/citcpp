#ifndef DETAIL_FOR_EACH_CROSS_PRODUCT_ELEM_HPP_
#define DETAIL_FOR_EACH_CROSS_PRODUCT_ELEM_HPP_

#include <functional>
#include <vector>
#include <taskflow/taskflow.hpp>

namespace citcpp
{
  namespace detail
  {
    /**
     * Iterates over all elements of the cross product of the given
     * zero-based integer ranges. The given vector specifies the maximum values
     * for the ranges (min is implicitly 0).
     * Example: max_values = {2,2},
     * This function would generate: {0,0}, {0,1}, {1,0}, {1,1}.
     * For each cross product element, the given callback is executed.
     */
    void
    for_each_cross_product_elem (const std::vector<unsigned int> &max_values,
				 const std::function<void
				 (const std::vector<unsigned int>&)> &callback);

    /**
     * Iterates over all elements of the cross product of the given
     * zero-based integer ranges. The given vector specifies the maximum values
     * for the ranges (min is implicitly 0).
     * Example: max_values = {2,2},
     * This function would generate: {0,0}, {0,1}, {1,0}, {1,1}.
     * For each cross product element, the given callback is executed.
     * The iteration may be parallelized if using this overload. The parallelization works by
     * partitioning the number of cross product elements according to the first element
     * in lexicographical order. Elements starting with [0, ... , n - 1], where n is the first
     * integer in ordered_int_sets, are each executed as a separated
     * task, meaning the first value in the cross product element passed to the callback can be used
     * in order to avoid data races.
     */
    void
    for_each_cross_product_elem (const std::vector<unsigned int> &max_values,
				 tf::Executor &executor,
				 const std::function<void
				 (const std::vector<unsigned int>&)> &callback);
  }
}

#endif /* DETAIL_FOR_EACH_CROSS_PRODUCT_ELEM_HPP_ */
