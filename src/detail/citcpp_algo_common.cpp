#include <thread>
#include <future>
#include "citcpp_algo_common.hpp"
#include "next_index_combination.hpp"

namespace
{
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
}

namespace citcpp
{
  namespace detail
  {
    unsigned long long
    getNumberOfCombinationsToCover (const Model &model, unsigned int t)
    {
      // Handle edge cases for t.
      if (t < 0
	  || (std::vector<int>::size_type) t > model.getParameters ().size ())
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

      // Adjust the number of threads if there are fewer combinations than threads,
      // to avoid creating unnecessary threads with tiny or empty chunks.
      if (model.getParameters ().size () < num_threads)
	{
	  num_threads = 1;
	}

      detail::NextIndexCombination next_index_combination (
	  model.getParameters ().size (), t, num_threads);

      std::vector<std::future<unsigned long long>> futures;

      // Launch asynchronous tasks (threads) for each chunk.
      for (unsigned int i = 0; i < num_threads; ++i)
	{
	  futures.push_back (
	  // std::async launches the task. std::launch::async ensures it runs on a new thread.
	      std::async (std::launch::async, getNumberOfCombinationsOfChunk,
			  std::cref (model), std::ref (next_index_combination),
			  i));
	}

      unsigned long long num_combinations = 0;

      // Collect results from all futures. f.get() blocks until the associated task completes.
      for (auto &f : futures)
	{
	  unsigned long long chunk_num_combos = f.get ();
	  num_combinations += chunk_num_combos;
	}

      return num_combinations;
    }
  }
}
