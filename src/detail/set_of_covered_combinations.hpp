#ifndef DETAIL_SET_OF_COVERED_COMBINATIONS_HPP_
#define DETAIL_SET_OF_COVERED_COMBINATIONS_HPP_

#include <unordered_set>
#include "combination.hpp"

namespace citcpp
{
  namespace detail
  {
    /**
     * This template documents the methods needed from the container tracking
     * the covered combinations.
     */
    template<typename T_IMPL>
      class SetOfCoveredCombinationsTmpl
      {
      public:
	typedef typename T_IMPL::size_type size_type;

      public:
	void
	insert (const Combination &combination)
	{
	  m_impl.insert (combination);
	}

	size_type
	size () const noexcept
	{
	  return m_impl.size ();
	}

	bool
	contains (const Combination &combination)
	{
	  return m_impl.contains (combination);
	}

      private:
	T_IMPL m_impl;
      };

    using HashsetOfCombinations = std::unordered_set<Combination, CombinationHash, CombinationEqual>;
    using SetOfCoveredCombinations = SetOfCoveredCombinationsTmpl<HashsetOfCombinations>;
  }
}

#endif /* DETAIL_SET_OF_COVERED_COMBINATIONS_HPP_ */
