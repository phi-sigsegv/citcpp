#ifndef DETAIL_FOR_EACH_CROSS_PRODUCT_ELEM_HPP_
#define DETAIL_FOR_EACH_CROSS_PRODUCT_ELEM_HPP_

#include <functional>
#include <vector>

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
  }
}

#endif /* DETAIL_FOR_EACH_CROSS_PRODUCT_ELEM_HPP_ */
