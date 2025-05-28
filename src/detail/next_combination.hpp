#ifndef DETAIL_NEXT_COMBINATION_HPP_
#define DETAIL_NEXT_COMBINATION_HPP_

#include <vector>
#include <algorithm>

namespace citcpp
{
  namespace detail
  {
    template<typename T>
      class NextCombination
      {
      public:
	NextCombination (const std::vector<T> &elements, int k) :
	    m_elements (elements), m_k (k), m_selector (elements.size ()), m_combination (
		elements.begin (), elements.begin () + k)
	{
	  std::fill (m_selector.begin () + k, m_selector.end (), true);
	}

	NextCombination (NextCombination&&) = default;
	NextCombination (const NextCombination&) = default;

	NextCombination&
	operator= (NextCombination&&) = default;

	NextCombination&
	operator= (const NextCombination&) = default;

	const std::vector<T>&
	getCombination () const
	{
	  return m_combination;
	}

	std::vector<T>&
	getCombination ()
	{
	  return m_combination;
	}

	bool
	next_combination ()
	{
	  if (m_k < 0 || m_k > m_elements.size ())
	    {
	      // invalid k value.
	      return false;
	    }
	  if (m_k == 0)
	    {
	      // Special case: only one combination, the empty set
	      return false;
	    }
	  if (m_k == m_elements.size ())
	    { // Special case: only one combination, all elements
	      return false;
	    }

	  bool ret = std::next_permutation (m_selector.begin (),
					    m_selector.end ());

	  int num_selected = 0;
	  for (std::vector<bool>::size_type i = 0; i < m_elements.size (); ++i)
	    {
	      if (!m_selector[i])
		{
		  m_combination[num_selected++] = m_elements[i];
		}
	    }

	  return ret;
	}

      private:
	const std::vector<T> &m_elements;
	const typename std::vector<T>::size_type m_k;
	std::vector<bool> m_selector;
	std::vector<T> m_combination;
      };

    template<typename T>
      NextCombination<T>
      makeNextCombination (const std::vector<T> &elements, int k)
      {
	return NextCombination<T> (elements, k);
      }
  }
}

#endif /* DETAIL_NEXT_COMBINATION_HPP_ */
