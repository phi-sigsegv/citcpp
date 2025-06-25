#include <taskflow/taskflow.hpp>
#include <taskflow/algorithm/for_each.hpp>
#include "citcpp_algo_common.hpp"

namespace
{
  // Recursive helper function for combination generation and sum calculation
  // This function will be called by each async task.
  unsigned long long
  recursive_combine_and_sum (unsigned int start_idx_for_next,
			     unsigned int current_count,
			     unsigned long long current_prod_val,
			     unsigned int t, unsigned int numFactors,
			     const std::vector<unsigned int> &factorLevels)
  {
    if (current_count == t)
      {
	return current_prod_val;
      }

    unsigned long long partial_sum = 0;
    for (unsigned int j = start_idx_for_next; j < numFactors; ++j)
      {
	partial_sum += recursive_combine_and_sum (
	    j + 1, current_count + 1, current_prod_val * factorLevels[j], t,
	    numFactors, factorLevels);
      }

    return partial_sum;
  }
}

namespace citcpp
{
  namespace detail
  {
    unsigned long long
    number_of_combinations_to_cover (const model &model, unsigned int t)
    {
      return recursive_combine_and_sum (0, 0, 1, t,
					model.get_parameters ().size (),
					model.get_parameters ());
    }

    unsigned long long
    number_of_combinations_to_cover (tf::Executor &executor, const model &model,
				     unsigned int t)
    {
      const unsigned int numFactors = model.get_parameters ().size ();

      // Distribute the initial choices among threads
      // Each thread will compute a partial sum which we aggregate.
      std::atomic_ullong num_combinations = 0;

      tf::Taskflow taskflow;
      taskflow.for_each_index (
	  0u,
	  numFactors,
	  1u,
	  [&model, t, numFactors, &num_combinations]
	  (unsigned int i)
	    {
	      unsigned long long chunk_num_combos = recursive_combine_and_sum (i+1, 1, model.get_parameters ()[i], t, numFactors,
		  model.get_parameters ());
	      num_combinations.fetch_add(chunk_num_combos, std::memory_order_acq_rel);
	    });

      executor.run (taskflow).wait ();

      return num_combinations;
    }
  }
}
