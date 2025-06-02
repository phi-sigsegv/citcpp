#include <algorithm>
#include <execution>
#include <ranges>
#include <atomic>
#include "citcpp_algo_common.hpp"
#include "next_index_combination.hpp"

namespace
{
  // Function to calculate nCr (n choose r)
  unsigned long long
  nCr (int n, int r)
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
  getNumberOfCombinationsOfChunk (
      const citcpp::detail::Model &model,
      citcpp::detail::NextIndexCombination &next_index_combination,
      unsigned int chunk_index)
  {
    unsigned long long num_combinations = 0;

    while (next_index_combination.hasNext (chunk_index))
      {
	const std::vector<unsigned int> &next_param_index_combination =
	    next_index_combination.next (chunk_index);

	unsigned long long value_combos = 1;
	for (int param_index : next_param_index_combination)
	  {
	    unsigned int num_param_values = model.getParameters ()[param_index];
	    value_combos = value_combos * num_param_values;
	  }

	num_combinations += value_combos;
      }

    return num_combinations;
  }

  unsigned long long
  getNumberOfCombinationsToCover_iterative (const citcpp::detail::Model &model,
					    unsigned int t)
  {
    // Handle edge cases for t.
    if ((std::vector<int>::size_type) t > model.getParameters ().size ())
      {
	// Malformed input.
	return 0;
      }

    if (t == 0)
      {
	// Trivial case.
	return 0;
      }

    if ((std::vector<int>::size_type) t == model.getParameters ().size ())
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
    unsigned long long numCombinationsOfFactors = nCr (
	model.getParameters ().size (), t);
    if (numCombinationsOfFactors < num_threads * 2)
      {
	citcpp::detail::NextIndexCombination next_index_combination (
	    model.getParameters ().size (), t, 1);

	unsigned long long num_combinations = getNumberOfCombinationsOfChunk (
	    model, next_index_combination, 0);

	return num_combinations;
      }

    citcpp::detail::NextIndexCombination next_index_combination (
	model.getParameters ().size (), t, num_threads);

    std::atomic_ullong num_combinations = 0;

    // Distribute the initial choices among threads
    // Each thread will compute a partial sum and return it via a future.
    auto range = std::views::iota ((unsigned int) 0, num_threads);
    std::for_each (
	std::execution::par_unseq,
	range.begin (),
	range.end (),
	[&model, &next_index_combination, &num_combinations]
	(unsigned int i)
	  {
	    unsigned long long chunk_num_combos = getNumberOfCombinationsOfChunk (model, next_index_combination, i);
	    num_combinations.fetch_add(chunk_num_combos, std::memory_order_acq_rel);
	  }
	);

    return num_combinations;

//    std::vector<std::future<unsigned long long>> futures;
//
//// Launch asynchronous tasks (threads) for each chunk.
//    for (unsigned int i = 0; i < num_threads; ++i)
//      {
//	futures.push_back (
//	// std::async launches the task. std::launch::async ensures it runs on a new thread.
//	    std::async (std::launch::async, getNumberOfCombinationsOfChunk,
//			std::cref (model), std::ref (next_index_combination),
//			i));
//      }
//
//    unsigned long long num_combinations = 0;
//
//// Collect results from all futures. f.get() blocks until the associated task completes.
//    for (auto &f : futures)
//      {
//	unsigned long long chunk_num_combos = f.get ();
//	num_combinations += chunk_num_combos;
//      }
//
//    return num_combinations;
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
  getNumberOfCombinationsToCover_recursive (const citcpp::detail::Model &model,
					    unsigned int t)
  {
    unsigned int numFactors = model.getParameters ().size ();
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
    unsigned long long numCombinationsOfFactors = nCr (numFactors, t);
    if (numCombinationsOfFactors == 0)
      return 0;
    if (numCombinationsOfFactors < num_threads * 2)
      { // Example threshold
	// Fallback to serial for very small cases to avoid async overhead
	return recursive_combine_and_sum (0, 0, 1, t, numFactors,
					  model.getParameters ());
      }

    std::atomic_ullong num_combinations = 0;

    // Distribute the initial choices among threads
    // Each thread will compute a partial sum and return it via a future.
    auto range = std::views::iota ((unsigned int) 0, numFactors);
    std::for_each (
	std::execution::par_unseq,
	range.begin (),
	range.end (),
	[&model, t, numFactors, &num_combinations]
	(unsigned int i)
	  {
	    unsigned long long chunk_num_combos = recursive_combine_and_sum (i+1, 1, model.getParameters ()[i], t, numFactors,
		model.getParameters ());
	    num_combinations.fetch_add(chunk_num_combos, std::memory_order_acq_rel);
	  }
	);

    return num_combinations;
  }
}

namespace citcpp
{
  namespace detail
  {
    unsigned long long
    getNumberOfCombinationsToCover (const Model &model, unsigned int t)
    {
      return getNumberOfCombinationsToCover_recursive (model, t);
    }
  }
}
