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
    class citcpp_ipog : public citcpp_algo_if
    {
    public:
      citcpp_ipog (const input_model &input_model) :
	  citcpp_algo_if (), model_ (input_model), strength_ (1), covered_combinations_ ()
      {
      }

      /**
       * Too lazy to implement/ensuring that it is well-defined.
       */
      citcpp_ipog (citcpp_ipog&&) = delete;
      citcpp_ipog (const citcpp_ipog&) = delete;

      /**
       * Too lazy to implement/ensuring that it is well-defined.
       */
      citcpp_ipog&
      operator= (citcpp_ipog&&) = delete;
      citcpp_ipog&
      operator= (const citcpp_ipog&) = delete;

      void
      set_interaction_strength (unsigned int t)
      {
	strength_ = t;
      }

      /**
       * This is the entry point to be called by a thread.
       */
      void
      entry_point (exec_handle_impl &exec_handle)
      {
	// First we compute the number of combination we have to cover.
	unsigned long long number_of_combination_to_cover =
	    number_of_combinations_to_cover (model_, strength_);
	exec_handle.set_number_of_combinations_to_cover (
	    number_of_combination_to_cover);

	// Set the generated test set.
	// This will also signal to the client that we are done.
	exec_handle.set_test_set (::citcpp::test_set ());
      }

    private:
      const model model_;
      unsigned int strength_;
      set_of_covered_pv_combinations covered_combinations_;
    };
  }
}

#endif /* DETAIL_CITCPP_IPOG_HPP_ */
