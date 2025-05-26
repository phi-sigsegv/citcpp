#ifndef DETAIL_CITCPP_IPOG_HPP_
#define DETAIL_CITCPP_IPOG_HPP_

#include <thread>
#include "citcpp_algo_if.hpp"
#include "exec_handle_impl.hpp"
#include "internal_model.hpp"
#include "set_of_covered_combinations.hpp"

namespace citcpp
{
  namespace detail
  {
    /**
     * This class provides an implementation of the IPOG algorithm.
     */
    class CitCppIpog : public ICitCppAlgo
    {
    public:
      CitCppIpog (const InputModel &input_model) :
	  ICitCppAlgo (), m_model (input_model), m_covered_combinations ()
      {
      }

      /**
       * Too lazy to implement/ensuring that it is well-defined.
       */
      CitCppIpog (CitCppIpog&&) = delete;
      CitCppIpog (const CitCppIpog&) = delete;

      /**
       * Too lazy to implement/ensuring that it is well-defined.
       */
      CitCppIpog&
      operator= (CitCppIpog&&) = delete;
      CitCppIpog&
      operator= (const CitCppIpog&) = delete;

      /**
       * This is the entry point to be called by a thread.
       */
      void
      entryPoint (ExecHandleImpl &exec_handle)
      {
	exec_handle.setTestSet (::citcpp::TestSet ());
      }

    private:
      const Model m_model;
      SetOfCoveredCombinations m_covered_combinations;
    };
  }
}

#endif /* DETAIL_CITCPP_IPOG_HPP_ */
