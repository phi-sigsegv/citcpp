#ifndef TESTSET_HPP_
#define TESTSET_HPP_

#include <vector>
#include <list>
#include <iostream>
#include "input_model.hpp"

namespace citcpp
{
  /**
   * This class represents a produced testset.
   */
  class TestSet
  {
  public:
    const std::list<std::vector<ParameterValue>>&
    getListOfTests () const
    {
      return testset_;
    }

    std::list<std::vector<ParameterValue>>&
    getListOfTests ()
    {
      return testset_;
    }

    friend std::ostream&
    operator<< (std::ostream &os, const TestSet &test_set)
    {
      for (const auto &test : test_set.getListOfTests ())
	{
	  const char *sep = "";
	  for (const auto &pv : test)
	    {
	      os << sep << pv.getValue ();
	      sep = ", ";
	    }
	  os << "\n";
	}

      return os;
    }

  private:
    std::list<std::vector<ParameterValue>> testset_;
  };
}

#endif /* TESTSET_HPP_ */
