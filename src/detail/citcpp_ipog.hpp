#ifndef DETAIL_CITCPP_IPOG_HPP_
#define DETAIL_CITCPP_IPOG_HPP_

#include <thread>
#include <algorithm>
#include "citcpp_algo_if.hpp"
#include "exec_handle_impl.hpp"
#include "internal_model.hpp"
#include "set_of_covered_pv_combinations.hpp"
#include "citcpp_algo_common.hpp"
#include "index_combinator.hpp"

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
      citcpp_ipog (const input_model &input_model);
      citcpp_ipog (input_model &&input_model);

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
      entry_point (exec_handle_impl &exec_handle);

    private:
      const citcpp::input_model input_model_;
      const model model_;
      unsigned int strength_;
      test_set test_set_;
    };
  }
}

#endif /* DETAIL_CITCPP_IPOG_HPP_ */
