#ifndef DETAIL_CITCPP_IPOG_HPP_
#define DETAIL_CITCPP_IPOG_HPP_

#include <thread>
#include "citcpp_algo_if.hpp"
#include "exec_handle_impl.hpp"
#include "internal_model.hpp"
#include "set_of_covered_pv_combinations.hpp"
#include "citcpp_algo_common.hpp"

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
	  ICitCppAlgo (), model_ (input_model), strength_ (1), covered_combinations_ ()
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

      void
      setInteractionStrength (unsigned int t)
      {
	strength_ = t;
      }

      /**
       * This is the entry point to be called by a thread.
       */
      void
      entryPoint (ExecHandleImpl &exec_handle)
      {
	// First we compute the number of combination we have to cover.
	unsigned long long number_of_combination_to_cover =
	    getNumberOfCombinationsToCover (model_, strength_);
	exec_handle.setNumberOfCombinationsToCover (
	    number_of_combination_to_cover);

	// Set the generated test set.
	// This will also signal to the client that we are done.
	exec_handle.setTestSet (::citcpp::TestSet ());
      }

    private:
      const Model model_;
      unsigned int strength_;
      SetOfCoveredPVCombinations covered_combinations_;
    };
  }
}

#endif /* DETAIL_CITCPP_IPOG_HPP_ */
