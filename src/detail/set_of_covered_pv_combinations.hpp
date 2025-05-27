#ifndef DETAIL_SET_OF_COVERED_COMBINATIONS_HPP_
#define DETAIL_SET_OF_COVERED_COMBINATIONS_HPP_

#include <unordered_set>
#include "pv_combination.hpp"

namespace citcpp
{
  namespace detail
  {
    /**
     * This template documents the methods needed from the container tracking
     * the covered combinations.
     */
    template<typename T_IMPL>
      class SetOfCoveredPVCombinationsTmpl
      {
      public:
	typedef typename T_IMPL::size_type size_type;

      public:
	void
	insert (const PVCombination &combination)
	{
	  m_impl.insert (combination);
	}

	size_type
	size () const noexcept
	{
	  return m_impl.size ();
	}

	bool
	contains (const PVCombination &combination)
	{
	  return m_impl.contains (combination);
	}

      private:
	T_IMPL m_impl;
      };

    using HashsetOfCombinations = std::unordered_set<PVCombination, CombinationHash, CombinationEqual>;
    using SetOfCoveredPVCombinations = SetOfCoveredPVCombinationsTmpl<HashsetOfCombinations>;
  }
}

#endif /* DETAIL_SET_OF_COVERED_COMBINATIONS_HPP_ */
