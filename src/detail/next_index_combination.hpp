#ifndef DETAIL_NEXT_PARAM_COMBINATION_HPP_
#define DETAIL_NEXT_PARAM_COMBINATION_HPP_

#include <vector>
#include <memory>

namespace citcpp
{
  namespace detail
  {
    class NextIndexCombination
    {
    public:
      /**
       * Constructs an instance for iterating over all combinations of size k
       * chosen from a range of [0, ... , n]. The iteration is split into
       * num_chunk more or less equally sized number of combinations.
       */
      NextIndexCombination (unsigned int n, unsigned int k,
			    unsigned int num_chunks);

      ~NextIndexCombination ();

      /**
       * Too lazy to implement/ensuring that it is well-defined.
       */
      NextIndexCombination (NextIndexCombination&&) = delete;
      NextIndexCombination (const NextIndexCombination&) = delete;

      /**
       * Too lazy to implement/ensuring that it is well-defined.
       */
      NextIndexCombination&
      operator= (NextIndexCombination&&) = delete;
      NextIndexCombination&
      operator= (const NextIndexCombination&) = delete;

      /**
       * @brief Returns the total number of combinations.
       *
       * @return The total number of combinations.
       */
      unsigned long long
      getCombinationsCount () const;

      /**
       * @brief Returns whether more combinations are left.
       *
       * @param chunk_index The index of the chunk to query
       * @return True if more combinations are left, false otherwise.
       */
      bool
      hasNext (unsigned int chunk_index) const;

      /**
       * @brief Generates the next combination in lexicographical order.
       *
       * @param chunk_index The index of the chunk to query
       * @return A const reference to the std::vector<int> representing the combination.
       */
      const std::vector<unsigned int>&
      next (unsigned int chunk_index);

    private:
      class impl;
      std::unique_ptr<impl> impl_;
    };
  }
}

#endif /* DETAIL_NEXT_PARAM_COMBINATION_HPP_ */
