#include <taskflow/algorithm/for_each.hpp>
#include "for_each_combination.hpp"

namespace
{
  // Recursive helper function for combination generation.
  void
  recursive_combine (unsigned int start_idx_for_next,
		     unsigned int current_count, unsigned int n, unsigned int k,
		     std::vector<unsigned int> &callback_data,
		     const std::function<void
		     (const std::vector<unsigned int>&)> &callback)
  {
    if (current_count == k)
      {
	callback (callback_data);
	return;
      }

    for (unsigned int j = start_idx_for_next; j < n; ++j)
      {
	callback_data[current_count] = j;
	recursive_combine (j + 1, current_count + 1, n, k, callback_data,
			   callback);
      }
  }
}

namespace citcpp
{
  namespace detail
  {
    void
    for_each_combination (unsigned int n, unsigned int k,
			  const std::function<void
			  (const std::vector<unsigned int>&)> &callback)
    {
      std::vector<unsigned int> callback_data (k);
      recursive_combine (0, 0, n, k, callback_data, callback);
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
    for_each_combination (unsigned int n, unsigned int k,
			  tf::Executor &executor, const std::function<void
			  (const std::vector<unsigned int>&)> &callback)
    {
      tf::Taskflow taskflow;
      taskflow.for_each_index (0u, n, 1u, [n, k, &callback]
      (unsigned int i)
	{
	  std::vector<unsigned int> callback_data (k);
	  callback_data[0] = i;
	  recursive_combine (i+1, 1, n, k, callback_data, callback);
	});

      executor.run (taskflow).wait ();
    }
  }
}
