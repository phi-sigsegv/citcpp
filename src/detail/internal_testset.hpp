#ifndef DETAIL_INTERNAL_TESTSET_HPP_
#define DETAIL_INTERNAL_TESTSET_HPP_

#include <vector>
#include <list>

namespace citcpp
{
  namespace detail
  {
    /**
     * This class represents a produced test set.
     */
    class TestSet
    {
    public:
      const std::list<std::vector<int>>&
      getListOfTests () const
      {
	return m_testset;
      }

      std::list<std::vector<int>>&
      getListOfTests ()
      {
	return m_testset;
      }

    private:
      std::list<std::vector<int>> m_testset;
    };
  }
}

#endif /* DETAIL_INTERNAL_TESTSET_HPP_ */
