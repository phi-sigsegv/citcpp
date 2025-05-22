#ifndef TESTSET_HPP_
#define TESTSET_HPP_

#include <vector>
#include <list>
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
