#ifndef EXEC_HANDLE_HPP_
#define EXEC_HANDLE_HPP_

#include <future>
#include "testset.hpp"

namespace citcpp
{
  /**
   * This class provides means to monitor the progress of the execution,
   * as well as to terminate it, if construction of a complete
   * covering array shall be aborted and the intermediate results
   * shall be returned.
   * All methods can be safely called from an arbitrary thread.
   */
  class IExecHandle
  {
  public:
    /**
     * The real destructor of this handle calls abort() and joins with
     * the executing thread, in order to ensure a clean termination
     * if this handle is destroyed without the client explicitly waiting
     * for the execution.
     */
    virtual
    ~IExecHandle ()
    {
    }

    /**
     * Returns the number of combinations to cover based on the given
     * input model and desired interaction strength. The returned number
     * is constant throughout the whole execution.
     */
    virtual unsigned long
    getNumberOfCombinationsToCover () const = 0;

    /**
     * Returns the currently covered number of combinations. This number
     * is frequently updated during the execution. So for instance
     * this method can be used for showing the progress of the execution.
     */
    virtual unsigned long
    getNumberOfCoveredCombinations () const = 0;

    /**
     * Calling this method aborts the current execution. The call returns
     * once the partial test set has been constructed.
     */
    virtual void
    abort () = 0;

    /**
     * This method returns a future for the test set produced by the execution. The
     * wrapped test set may either be complete with respect to the desired interaction
     * coverage or not. This depends on whether the execution has been aborted.
     */
    virtual std::future<TestSet>
    getTestSet () = 0;
  };
}

#endif /* EXEC_HANDLE_HPP_ */
