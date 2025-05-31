#ifndef DETAIL_CITCPP_ALGO_IF_HPP_
#define DETAIL_CITCPP_ALGO_IF_HPP_

namespace citcpp
{
  namespace detail
  {
    // Forward declaration of ExecHandleImpl due to usage of
    // ICitCppAlgo by ExecHandleImpl definition.
    class ExecHandleImpl;

    /**
     * This class provides an entry point for a thread to call.
     */
    class ICitCppAlgo
    {
    public:
      virtual
      ~ICitCppAlgo ()
      {
      }

      /**
       * Sets the desired global interaction strength.
       */
      void
      setInteractionStrength (unsigned int t);

      /**
       * This is the entry point to be called by a thread.
       */
      virtual void
      entryPoint (ExecHandleImpl &exec_handle) = 0;
    };
  }
}

#endif /* DETAIL_CITCPP_ALGO_IF_HPP_ */
