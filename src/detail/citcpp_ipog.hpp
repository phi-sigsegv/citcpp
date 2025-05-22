#ifndef DETAIL_CITCPP_IPOG_HPP_
#define DETAIL_CITCPP_IPOG_HPP_

#include <thread>
#include "citcpp_algo_if.hpp"
#include "exec_handle_impl.hpp"

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
      /**
       * This is the entry point to be called by a thread.
       */
      void
      entryPoint (ExecHandleImpl &exec_handle)
      {
      }
    };
  }
}

#endif /* DETAIL_CITCPP_IPOG_HPP_ */
