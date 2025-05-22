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
    TestSet ()
    {
      std::cout << "Default ctr" << std::endl;
    }

    TestSet (const TestSet &other)
    {
      std::cout << "copy ctr" << std::endl;
    }

    TestSet (TestSet &&other)
    {
      std::cout << "move ctr" << std::endl;
    }

    TestSet&
    operator= (const TestSet &other)
    {
      std::cout << "copy assign" << std::endl;
      return *this;
    }

    TestSet&
    operator= (TestSet &&other)
    {
      std::cout << "move assign" << std::endl;
      return *this;
    }

    const std::list<std::vector<ParameterValue>>&
    getListOfTests () const
    {
      return m_testset;
    }

    std::list<std::vector<ParameterValue>>&
    getListOfTests ()
    {
      return m_testset;
    }

  private:
    std::list<std::vector<ParameterValue>> m_testset;
  };
}

#endif /* TESTSET_HPP_ */
