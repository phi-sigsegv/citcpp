#ifndef EXEC_HANDLE_HPP_
#define EXEC_HANDLE_HPP_

#include <future>

#include "test_set.hpp"

namespace citcpp
{
  /**
   * This class provides means to monitor the progress of the execution,
   * as well as to terminate it, if construction of a complete
   * covering array shall be aborted and the intermediate results
   * shall be returned.
   * All methods can be safely called from an arbitrary thread.
   */
  class exec_handle
  {
  public:
    /**
     * The real destructor of this handle calls abort() and joins with
     * the executing thread, in order to ensure a clean termination
     * if this handle is destroyed without the client explicitly waiting
     * for the execution.
     */
    virtual
    ~exec_handle ()
    {
    }

    /**
     * Returns the number of combinations to cover based on the given
     * input model and desired interaction strength.
     */
    virtual unsigned long long
    get_number_of_combinations_to_cover () const = 0;

    /**
     * Returns the currently covered number of combinations. This number
     * is frequently updated during the execution. So for instance
     * this method can be used for showing the progress of the execution.
     */
    virtual unsigned long long
    get_number_of_covered_combinations () const = 0;

    /**
     * Returns the current size of the testset. This number
     * is frequently updated during the execution. So for instance
     * this method can be used for showing the progress of the execution.
     */
    virtual unsigned int
    get_testset_size () const = 0;

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
    virtual std::future<test_set>
    get_test_set () = 0;

    /**
     * Once the computation of the test set has terminated, either because the
     * complete test set has been generated or because is has been aborted,
     * this method returns the duration of the computation in milli seconds.
     * If the computation has not terminated yet, then calling this method
     * returns 0.0.
     */
    virtual unsigned int
    get_duration_in_milli_seconds () const = 0;
  };
}

#endif /* EXEC_HANDLE_HPP_ */
