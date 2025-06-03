#ifndef DETAIL_CITCPP_ALGO_IF_HPP_
#define DETAIL_CITCPP_ALGO_IF_HPP_

namespace citcpp
{
  namespace detail
  {
    // Forward declaration of exec_handle_impl due to usage of
    // citcpp_algo_if by exec_handle_impl definition.
    class exec_handle_impl;

    /**
     * This class provides an entry point for a thread to call.
     */
    class citcpp_algo_if
    {
    public:
      virtual
      ~citcpp_algo_if ()
      {
      }

      /**
       * Sets the desired global interaction strength.
       */
      void
      set_interaction_strength (unsigned int t);

      /**
       * This is the entry point to be called by a thread.
       */
      virtual void
      entry_point (exec_handle_impl &exec_handle) = 0;
    };
  }
}

#endif /* DETAIL_CITCPP_ALGO_IF_HPP_ */
