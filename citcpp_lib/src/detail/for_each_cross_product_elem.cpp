#include "for_each_cross_product_elem.hpp"

namespace
{
  /**
   * @brief Recursive function to generate the cartesian product of multiple zero-based integer ranges.
   *
   * @param max_values A vector where each element is the maximum value for a range (min is implicitly 0).
   * E.g., {2, 1, 5} means ranges [0,2], [0,1], [0,5].
   * @param current_index The index of the range currently being processed.
   * @param callback_data The vector storing the cross product element being built.
   * @param callback The callback to be called for each cross product element.
   */
  void
  recursive_cross_product (const std::vector<unsigned int> &max_values,
			   unsigned int current_index,
			   std::vector<unsigned int> &callback_data,
			   const std::function<void
			   (const std::vector<unsigned int>&)> &callback)
  {
    if (current_index == max_values.size ())
      {
	callback (callback_data);
	return;
      }

    // The current range goes from 0 to max_values[current_index]
    unsigned int max_val = max_values[current_index];

    for (unsigned int i = 0; i < max_val; ++i)
      {
	callback_data[current_index] = i;
	recursive_cross_product (max_values, current_index + 1, callback_data,
				 callback);
      }
  }
}

namespace citcpp
{
  namespace detail
  {
    void
    for_each_cross_product_elem (const std::vector<unsigned int> &max_values,
				 const std::function<void
				 (const std::vector<unsigned int>&)> &callback)
    {
      std::vector<unsigned int> callback_data (max_values.size ());
      recursive_cross_product (max_values, 0, callback_data, callback);
    }
  }
}
