#include <taskflow/taskflow.hpp>
#include <taskflow/algorithm/for_each.hpp>
#include "citcpp_algo_common.hpp"

namespace
{
  // Recursive helper function for combination generation and sum calculation
  // This function will be called by each async task.
  unsigned long long
  recursive_combine_and_sum (int start_idx_for_next, int current_level,
			     unsigned long long current_prod_val,
			     const std::vector<unsigned int> &factorLevels)
  {
    unsigned long long partial_sum = 0;
    for (int j = start_idx_for_next; j >= current_level; --j)
      {
	if (current_level == 0)
	  {
	    partial_sum += current_prod_val * factorLevels[j];
	  }
	else
	  {
	    partial_sum += recursive_combine_and_sum (
		j - 1, current_level - 1, current_prod_val * factorLevels[j],
		factorLevels);
	  }
      }

    return partial_sum;
  }

  unsigned long long
  recursive_combine_and_sum (
      int start_idx_for_next, int current_level,
      unsigned long long current_prod_val,
      const std::vector<unsigned int> &factorLevels,
      const std::vector<unsigned int> &parameter_index_map)
  {
    unsigned long long partial_sum = 0;
    for (int j = start_idx_for_next; j >= current_level; --j)
      {
	if (current_level == 0)
	  {
	    partial_sum += current_prod_val
		* factorLevels[parameter_index_map[j]];
	  }
	else
	  {
	    partial_sum += recursive_combine_and_sum (
		j - 1, current_level - 1,
		current_prod_val * factorLevels[parameter_index_map[j]],
		factorLevels, parameter_index_map);
	  }
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
      return recursive_combine_and_sum (model.get_parameters ().size () - 1,
					t - 1, 1, model.get_parameters ());
    }

    unsigned long long
    number_of_combinations_to_cover (tf::Executor &executor, const model &model,
				     unsigned int t)
    {
      if (t < 2)
	{
	  return number_of_combinations_to_cover (model, t);
	}

      const unsigned int numFactors = model.get_parameters ().size ();

      // Distribute the initial choices among threads
      // Each thread will compute a partial sum which we aggregate.
      std::atomic_ullong num_combinations = 0;

      tf::Taskflow taskflow;
      taskflow.for_each_index (
	  t - 1,
	  numFactors,
	  1,
	  [&model, t, &num_combinations]
	  (int i)
	    {
	      unsigned long long chunk_num_combos = recursive_combine_and_sum (i - 1, t - 2, model.get_parameters ()[i], model.get_parameters ());
	      num_combinations.fetch_add(chunk_num_combos, std::memory_order_acq_rel);
	    });

      executor.run (taskflow).wait ();

      return num_combinations;
    }

    unsigned long long
    number_of_combinations_to_cover (
	unsigned int current_param_idx, const model &model,
	const std::vector<unsigned int> &parameter_index_map, unsigned int t)
    {
      return recursive_combine_and_sum (current_param_idx - 1, t - 1, 1,
					model.get_parameters (),
					parameter_index_map);
    }

    unsigned long long
    number_of_combinations_to_cover (
	tf::Executor &executor, unsigned int current_param_idx,
	const model &model,
	const std::vector<unsigned int> &parameter_index_map, unsigned int t)
    {
      if (t < 2)
	{
	  return number_of_combinations_to_cover (current_param_idx, model,
						  parameter_index_map, t);
	}

      // Distribute the initial choices among threads
      // Each thread will compute a partial sum which we aggregate.
      std::atomic_ullong num_combinations = 0;

      tf::Taskflow taskflow;
      taskflow.for_each_index (
	  t - 1,
	  current_param_idx,
	  1,
	  [&model, &parameter_index_map, t, &num_combinations]
	  (int i)
	    {
	      unsigned long long chunk_num_combos = recursive_combine_and_sum (i - 1, t - 2, model.get_parameters ()[parameter_index_map[i]], model.get_parameters (), parameter_index_map);
	      num_combinations.fetch_add(chunk_num_combos, std::memory_order_acq_rel);
	    });

      executor.run (taskflow).wait ();

      return num_combinations;
    }
  }
}
