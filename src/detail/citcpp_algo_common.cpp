#include <thread>
#include <ranges>
#include <taskflow/taskflow.hpp>
#include <taskflow/algorithm/for_each.hpp>
#include "citcpp_algo_common.hpp"
#include "index_combinator.hpp"

namespace
{
  // Function to calculate nCr (n choose r)
  unsigned long long
  n_choose_r (int n, int r)
  {
    if (r < 0 || r > n)
      {
	return 0;
      }
    if (r == 0 || r == n)
      {
	return 1;
      }
    if (r > n / 2)
      {
	r = n - r;
      }
    long long res = 1;
    for (int i = 1; i <= r; ++i)
      {
	res = res * (n - i + 1) / i;
      }
    return res;
  }

  unsigned long long
  number_of_combinations_of_chunk (
      const citcpp::detail::model &model,
      citcpp::detail::index_combinator &idx_combinator,
      unsigned int chunk_index)
  {
    unsigned long long num_combinations = 0;

    while (idx_combinator.has_next (chunk_index))
      {
	const std::vector<unsigned int> &next_param_index_combination =
	    idx_combinator.next (chunk_index);

	unsigned long long value_combos = 1;
	for (int param_index : next_param_index_combination)
	  {
	    unsigned int num_param_values = model.get_parameters ()[param_index];
	    value_combos = value_combos * num_param_values;
	  }

	num_combinations += value_combos;
      }

    return num_combinations;
  }

  unsigned long long
  number_of_combinations_to_cover_iterative (const citcpp::detail::model &model,
					     unsigned int t)
  {
    // Handle edge cases for t.
    if ((std::vector<int>::size_type) t > model.get_parameters ().size ())
      {
	// Malformed input.
	return 0;
      }

    if (t == 0)
      {
	// Trivial case.
	return 0;
      }

    if ((std::vector<int>::size_type) t == model.get_parameters ().size ())
      {
	// Trivial case.
	return 1;
      }

    // Determine the number of threads to use. Using hardware_concurrency() is a common default.
    unsigned int num_threads = std::thread::hardware_concurrency ();
    if (num_threads == 0)
      {
	num_threads = 1; // Fallback if hardware_concurrency returns 0
      }

    // Early exit for trivial cases or very small number of combinations
    unsigned long long num_combinations_of_factors = n_choose_r (
	model.get_parameters ().size (), t);
    if (num_combinations_of_factors < num_threads * 2)
      {
	citcpp::detail::index_combinator idx_combinator (
	    model.get_parameters ().size (), t, 1);

	unsigned long long num_combinations = number_of_combinations_of_chunk (
	    model, idx_combinator, 0);

	return num_combinations;
      }

    citcpp::detail::index_combinator idx_combinator (
	model.get_parameters ().size (), t, num_threads);

    // Distribute the initial choices among threads
    // Each thread will compute a partial sum which we aggregate.
    std::atomic_ullong num_combinations = 0;

    tf::Taskflow taskflow;
    taskflow.for_each_index (
	0u,
	num_threads,
	1u,
	[&model, &idx_combinator, &num_combinations]
	(unsigned int i)
	  {
	    unsigned long long chunk_num_combos = number_of_combinations_of_chunk (model, idx_combinator, i);
	    num_combinations.fetch_add(chunk_num_combos, std::memory_order_acq_rel);
	  });

    tf::Executor executor;
    executor.run (taskflow);
    executor.wait_for_all ();

    return num_combinations;
  }

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

  unsigned long long
  number_of_combinations_to_cover_recursive (const citcpp::detail::model &model,
					     unsigned int t)
  {
    unsigned int numFactors = model.get_parameters ().size ();
    if (t > numFactors)
      {
	// Malformed input.
	return 0;
      }

    // Determine the number of threads to use.
    // std::thread::hardware_concurrency() provides a hint for the number of concurrent threads.
    unsigned int num_threads = std::thread::hardware_concurrency ();
    if (num_threads == 0)
      { // Fallback if hardware_concurrency is not well-defined
	num_threads = 1;
      }

    // Early exit for trivial cases or very small number of combinations
    unsigned long long num_combinations_of_factors = n_choose_r (numFactors, t);
    if (num_combinations_of_factors == 0)
      return 0;
    if (num_combinations_of_factors < num_threads * 2)
      { // Example threshold
	// Fallback to serial for very small cases to avoid async overhead
	return recursive_combine_and_sum (0, 0, 1, t, numFactors,
					  model.get_parameters ());
      }

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

    tf::Executor executor;
    executor.run (taskflow);
    executor.wait_for_all ();

    return num_combinations;
  }
}

namespace citcpp
{
  namespace detail
  {
    unsigned long long
    number_of_combinations_to_cover (const model &model, unsigned int t)
    {
      return number_of_combinations_to_cover_recursive (model, t);
    }
  }
}
