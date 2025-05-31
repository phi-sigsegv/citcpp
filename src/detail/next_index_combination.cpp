#include <functional>
#include "next_index_combination.hpp"

namespace
{
  /**
   * @brief Initializes and returns a 2D vector containing precomputed binomial coefficients C(n, k).
   *
   * This function builds Pascal's triangle up to 'max_n' to allow for O(1) lookups
   * of binomial coefficients later. It's designed to be called once.
   *
   * @param max_n The maximum 'n' value for which binomial coefficients should be precomputed.
   * @return A std::vector<std::vector<long long>> where coeffs[n][k] stores C(n, k).
   */
  std::vector<std::vector<long long>>
  initialize_binomial_coefficients (unsigned int max_n)
  {
    // Resize the outer vector to hold up to max_n + 1 rows (for 0 to max_n)
    std::vector<std::vector<long long>> coeffs (max_n + 1);
    for (unsigned int i = 0; i <= max_n; ++i)
      {
	// Resize the inner vector for row 'i' to hold 'i + 1' elements (for 0 to i)
	coeffs[i].resize (i + 1);
	coeffs[i][0] = 1; // C(i, 0) is always 1
	if (i > 0)
	  {
	    coeffs[i][i] = 1; // C(i, i) is always 1
	  }
	// Calculate intermediate coefficients using the Pascal's identity: C(n, k) = C(n-1, k-1) + C(n-1, k)
	for (unsigned int j = 1; j < i; ++j)
	  {
	    coeffs[i][j] = coeffs[i - 1][j - 1] + coeffs[i - 1][j];
	    // Basic overflow check: if the sum overflows, it might become negative.
	    // Mark it as -1 to indicate overflow, though for typical N, K in combinatorial testing,
	    // long long is usually sufficient.
	    if (coeffs[i][j] < 0)
	      {
		coeffs[i][j] = -1; // Indicate overflow
	      }
	  }
      }
    return coeffs;
  }

  /**
   * @brief Generates the next combination in lexicographical order.
   *
   * This is an iterative algorithm to find the next combination from a given one.
   * It modifies the input 'p' vector in place.
   *
   * @param p The current combination (vector of indices), which will be updated to the next one.
   * @param n_elements The total number of elements available (0 to n_elements - 1).
   * @return True if a next combination was found, false if 'p' was already the last combination.
   */
  bool
  next_combination_lexicographical (std::vector<unsigned int> &p,
				    unsigned int n_elements)
  {
    int k_select = p.size (); // Number of elements in the combination
    int i = k_select - 1;    // Start from the rightmost element

    // Find the rightmost element that can be incremented
    // An element p[i] can be incremented if it's not yet at its maximum possible value
    // for its position (n_elements - (k_select - i)).
    while (i >= 0 && p[i] == n_elements - k_select + i)
      {
	--i;
      }

    if (i < 0)
      {
	// If i is less than 0, it means all elements are at their maximum values (e.g., [n-3, n-2, n-1])
	// and there are no more combinations.
	return false;
      }

    // Increment the found element
    ++p[i];
    // Reset all elements to the right of p[i] to their minimum possible values
    // relative to the incremented p[i].
    for (int j = i + 1; j < k_select; ++j)
      {
	p[j] = p[j - 1] + 1;
      }
    return true;
  }

  /**
   * @brief Retrieves the binomial coefficient C(n, k) from a precomputed table.
   *
   * This function provides O(1) access to C(n, k) values once the table is initialized.
   *
   * @param n Total number of items.
   * @param k Number of items to choose.
   * @param binomial_coeffs A const reference to the precomputed binomial coefficients table.
   * @return The value of C(n, k), or 0 if n < k or k < 0, or an error if coeffs table is not sufficient.
   */
  unsigned long long
  combinations_count (
      std::vector<int>::size_type n, std::vector<int>::size_type k,
      const std::vector<std::vector<long long>> &binomial_coeffs)
  {
    if (k < 0 || k > n)
      return 0; // Invalid combination parameters
    if (k == 0 || k == n)
      return 1; // Base cases: C(n, 0) = 1, C(n, n) = 1
    // Defensive check: Ensure the requested n,k are within the precomputed table's bounds.
    // This indicates an error if initialize_binomial_coefficients wasn't called with a sufficiently large max_n.
    if (n >= binomial_coeffs.size () || k >= binomial_coeffs[n].size ())
      {
	return 0; // Return 0 or throw an exception to indicate an error
      }
    return binomial_coeffs[n][k]; // O(1) lookup
  }

  /**
   * @brief Computes the Nth (0-indexed) combination in lexicographical order.
   *
   * This function determines the exact combination corresponding to a given rank.
   * It's a critical component for lexicographical chunking in parallel generation.
   *
   * @param n Total number of elements (0 to n-1).
   * @param k Number of elements to choose for the combination.
   * @param rank_to_find The 0-indexed rank of the desired combination.
   * @param binomial_coeffs A const reference to the precomputed binomial coefficients table.
   * @return A std::vector<int> representing the combination.
   */
  std::vector<unsigned int>
  get_nth_combination (
      unsigned int n, unsigned int k, unsigned long long rank_to_find,
      const std::vector<std::vector<long long>> &binomial_coeffs)
  {
    std::vector<unsigned int> combination (k);
    int last_chosen = -1; // The value of the last element chosen (initially -1 as elements are 0-indexed)

    // Iterate 'k' times to find each of the 'k' elements in the combination
    for (unsigned int i = 0; i < k; ++i)
      {
	// 'current_element_candidate' starts searching from the element immediately after the last chosen one.
	int current_element_candidate = last_chosen + 1;

	// This while loop determines the (i)th element of the combination.
	// It iterates by trying successive candidate values.
	// For each candidate, it calculates how many combinations can be formed
	// if that candidate is chosen.
	// The loop continues until a candidate is found such that the count of
	// combinations starting with it (and higher) is greater than or equal to
	// the remaining 'rank_to_find'.
	//
	// In the worst case, this while loop can execute roughly O(n) times for each of the 'k' elements.
	// Therefore, a single call to get_nth_combination can involve up to O(k * n) calls to combinations_count.
	while (true)
	  {
	    // C(remaining_n_elements, remaining_k_elements) if 'current_element_candidate' is chosen.
	    // (n - current_element_candidate - 1) is the number of elements available AFTER current_element_candidate.
	    // (k - i - 1) is the number of elements remaining to choose.
	    unsigned long long count_if_chosen = combinations_count (
		n - current_element_candidate - 1, k - i - 1, binomial_coeffs);

	    if (count_if_chosen > rank_to_find)
	      {
		// If this candidate can form enough combinations to cover 'rank_to_find',
		// then 'current_element_candidate' is our chosen element for this position 'i'.
		combination[i] = current_element_candidate;
		last_chosen = current_element_candidate; // Update last_chosen for the next iteration
		break; // Exit the inner while loop, move to finding the next element (increment 'i')
	      }
	    else
	      {
		// This 'current_element_candidate' is too small; its contribution is not enough.
		// Subtract its contribution from 'rank_to_find' and try the next higher candidate.
		rank_to_find -= count_if_chosen;
		current_element_candidate++;
	      }
	  }
      }
    return combination;
  }
}

namespace citcpp
{
  namespace detail
  {
    class NextIndexCombinationPerChunkData
    {
    public:
      NextIndexCombinationPerChunkData (
	  int n, int k, long long start_rank, long long end_rank,
	  const std::vector<std::vector<long long>> &binomial_coeffs) :
	  n_ (n), start_rank_ (start_rank), end_rank_ (end_rank), cur_rank_ (
	      start_rank), cur_combination_ (
	      get_nth_combination (n, k, start_rank, binomial_coeffs))
      {
	// First, find the exact combination corresponding to the 'start_rank'.
	// This is the starting point for this thread's generation.
      }

      /**
       * @brief Returns whether more combinations are left.
       *
       * @return True if more combinations are left, false otherwise.
       */
      bool
      hasNext () const
      {
	return cur_rank_ < end_rank_;
      }

      /**
       * @brief Generates the next combination in lexicographical order.
       *
       * @return A const reference to the std::vector<int> representing the combination.
       */
      const std::vector<unsigned int>&
      next ()
      {
	if (cur_rank_ == start_rank_)
	  {
	    // The first combination returned is the starting combination.
	    cur_rank_++;
	    return cur_combination_;
	  }

	next_combination_lexicographical (cur_combination_, n_);
	cur_rank_++;

	return cur_combination_;
      }

    private:
      const int n_;
      const long long start_rank_;
      const long long end_rank_;
      long long cur_rank_;
      std::vector<unsigned int> cur_combination_;
    };

    class NextIndexCombination::impl
    {
    public:
      impl (unsigned int n, unsigned int k, unsigned int num_chunks) :
	  n_ (n), k_ (k), binomial_coeffs_ (
	      initialize_binomial_coefficients (n)), per_chunk_data_ ()
      {
	// Calculate the total number of combinations using the initialized table.
	const long long total_combinations = combinations_count (
	    n, k, binomial_coeffs_);

	// Calculate the approximate size of each chunk. The last chunk will take any remainder.
	long long chunk_size = total_combinations / num_chunks;

	// Create the per-chunk data.
	for (unsigned int i = 0; i < num_chunks; ++i)
	  {
	    long long start_rank = i * chunk_size;
	    // The last chunk takes all remaining combinations to ensure all are covered.
	    long long end_rank =
		(i == num_chunks - 1) ?
		    total_combinations : (i + 1) * chunk_size;

	    per_chunk_data_.emplace_back (n, k, start_rank, end_rank,
					  std::cref (binomial_coeffs_));
	  }
      }

      /**
       * @brief Returns the total number of combinations.
       *
       * @return The total number of combinations.
       */
      unsigned long long
      getCombinationsCount () const
      {
	return combinations_count (n_, k_, binomial_coeffs_);
      }

      /**
       * @brief Returns whether more combinations are left.
       *
       * @param chunk_index The index of the chunk to query
       * @return True if more combinations are left, false otherwise.
       */
      bool
      hasNext (unsigned int chunk_index) const
      {
	if (chunk_index < 0)
	  {
	    chunk_index = 0;
	  }
	if (chunk_index >= per_chunk_data_.size ())
	  {
	    chunk_index = per_chunk_data_.size () - 1;
	  }

	return per_chunk_data_[chunk_index].hasNext ();
      }

      /**
       * @brief Generates the next combination in lexicographical order.
       *
       * @param chunk_index The index of the chunk to query
       * @return A const reference to the std::vector<int> representing the combination.
       */
      const std::vector<unsigned int>&
      next (unsigned int chunk_index)
      {
	if (chunk_index < 0)
	  {
	    chunk_index = 0;
	  }
	if (chunk_index >= per_chunk_data_.size ())
	  {
	    chunk_index = per_chunk_data_.size () - 1;
	  }

	return per_chunk_data_[chunk_index].next ();
      }

    private:
      const unsigned int n_;
      const unsigned int k_;
      // --- Binomial Coefficients Initialization ---
      const std::vector<std::vector<long long>> binomial_coeffs_;
      std::vector<NextIndexCombinationPerChunkData> per_chunk_data_;
    };

    NextIndexCombination::NextIndexCombination (unsigned int n, unsigned int k,
						unsigned int num_chunks) :
	impl_ (new impl (n, k, num_chunks))
    {
    }

    NextIndexCombination::~NextIndexCombination () = default;

    unsigned long long
    NextIndexCombination::getCombinationsCount () const
    {
      return impl_->getCombinationsCount ();
    }

    bool
    NextIndexCombination::hasNext (unsigned int chunk_index) const
    {
      return impl_->hasNext (chunk_index);
    }

    const std::vector<unsigned int>&
    NextIndexCombination::next (unsigned int chunk_index)
    {
      return impl_->next (chunk_index);
    }
  }
}
