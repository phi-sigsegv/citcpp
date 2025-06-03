#ifndef TEST_SET_HPP_
#define TEST_SET_HPP_

#include <vector>
#include <list>
#include <iostream>
#include "input_model.hpp"

namespace citcpp
{
  /**
   * This class represents a produced test set.
   */
  class test_set
  {
  public:
    const std::list<std::vector<parameter_value>>&
    get_list_of_tests () const
    {
      return test_set_;
    }

    std::list<std::vector<parameter_value>>&
    get_list_of_tests ()
    {
      return test_set_;
    }

    friend std::ostream&
    operator<< (std::ostream &os, const test_set &test_set)
    {
      for (const auto &test : test_set.get_list_of_tests ())
	{
	  const char *sep = "";
	  for (const auto &pv : test)
	    {
	      os << sep << pv.get_value ();
	      sep = ", ";
	    }
	  os << "\n";
	}

      return os;
    }

  private:
    std::list<std::vector<parameter_value>> test_set_;
  };
}

#endif /* TEST_SET_HPP_ */
