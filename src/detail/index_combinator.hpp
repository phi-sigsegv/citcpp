#ifndef DETAIL_NEXT_PARAM_COMBINATION_HPP_
#define DETAIL_NEXT_PARAM_COMBINATION_HPP_

#include <vector>
#include <memory>

namespace citcpp
{
  namespace detail
  {
    class index_combinator
    {
    public:
      /**
       * Constructs an instance for iterating over all combinations of size k
       * chosen from a range of [0, ... , n]. The iteration is split into
       * num_chunk more or less equally sized number of combinations.
       */
      index_combinator (unsigned int n, unsigned int k,
			     unsigned int num_chunks);

      ~index_combinator ();

      /**
       * Too lazy to implement/ensuring that it is well-defined.
       */
      index_combinator (index_combinator&&) = delete;
      index_combinator (const index_combinator&) = delete;

      /**
       * Too lazy to implement/ensuring that it is well-defined.
       */
      index_combinator&
      operator= (index_combinator&&) = delete;
      index_combinator&
      operator= (const index_combinator&) = delete;

      /**
       * @brief Returns the total number of combinations.
       *
       * @return The total number of combinations.
       */
      unsigned long long
      get_combinations_count () const;

      /**
       * @brief Returns whether more combinations are left.
       *
       * @param chunk_index The index of the chunk to query
       * @return True if more combinations are left, false otherwise.
       */
      bool
      has_next (unsigned int chunk_index) const;

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
