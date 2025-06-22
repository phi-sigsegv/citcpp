#ifndef DETAIL_CITCPP_IPOG_HPP_
#define DETAIL_CITCPP_IPOG_HPP_

#include "internal_model.hpp"
#include "../input_model.hpp"
#include "../citcpp_config.hpp"
#include "internal_test_set.hpp"

namespace citcpp
{
  namespace detail
  {
    // Forward declaration of exec_handle_ipog_impl due to usage of
    // citcpp_ipog by exec_handle_ipog_impl definition.
    class exec_handle_ipog_impl;

    /**
     * This class provides an implementation of the IPOG algorithm.
     */
    class citcpp_ipog
    {
    public:
      citcpp_ipog (const input_model &input_model,
		   const covering_array_computation_config &config);
      citcpp_ipog (input_model &&input_model,
		   const covering_array_computation_config &config);

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
      set_interaction_strength (unsigned int t);

      /**
       * This is the entry point to be called by a thread.
       */
      void
      entry_point (exec_handle_ipog_impl &exec_handle);

    private:
      const citcpp::covering_array_computation_config config_;
      const citcpp::input_model input_model_;
      const model model_;
      unsigned int strength_;
      test_set test_set_;
    };
  }
}

#endif /* DETAIL_CITCPP_IPOG_HPP_ */
