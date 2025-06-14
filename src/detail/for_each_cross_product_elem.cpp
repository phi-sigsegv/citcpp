#include <taskflow/algorithm/for_each.hpp>
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
    for_each_cross_product_elem (const std::vector<unsigned int> &max_values,
				 tf::Executor &executor,
				 const std::function<void
				 (const std::vector<unsigned int>&)> &callback)
    {
      tf::Taskflow taskflow;
      taskflow.for_each_index (
	  0u, max_values[max_values.size () - 1], 1u, [&max_values, &callback]
	  (unsigned int i)
	    {
	      std::vector<unsigned int> callback_data (max_values.size ());
	      callback_data[0] = i;
	      recursive_cross_product (max_values, 1, callback_data, callback);
	    });

      executor.run (taskflow).wait ();
    }
  }
}
