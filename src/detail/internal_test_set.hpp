#ifndef DETAIL_INTERNAL_TESTSET_HPP_
#define DETAIL_INTERNAL_TESTSET_HPP_

#include <vector>
#include <list>

namespace citcpp
{
  namespace detail
  {
    typedef std::vector<int> test;

    /**
     * This class represents a produced test set.
     */
    class test_set
    {
    public:
      const std::list<test>&
      get_list_of_tests () const
      {
	return testset_;
      }

      std::list<test>&
      get_list_of_tests ()
      {
	return testset_;
      }

    private:
      std::list<test> testset_;
    };
  }
}

#endif /* DETAIL_INTERNAL_TESTSET_HPP_ */
