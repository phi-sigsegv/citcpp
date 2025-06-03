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
      class set_of_covered_pv_combinations_tmpl
      {
      public:
	typedef typename T_IMPL::size_type size_type;

      public:
	void
	insert (const pv_combination &combination)
	{
	  impl_.insert (combination);
	}

	size_type
	size () const noexcept
	{
	  return impl_.size ();
	}

	bool
	contains (const pv_combination &combination)
	{
	  return impl_.contains (combination);
	}

      private:
	T_IMPL impl_;
      };

    using hashset_of_combinations = std::unordered_set<pv_combination, combination_hash, combination_equal>;
    using set_of_covered_pv_combinations = set_of_covered_pv_combinations_tmpl<hashset_of_combinations>;
  }
}

#endif /* DETAIL_SET_OF_COVERED_COMBINATIONS_HPP_ */
