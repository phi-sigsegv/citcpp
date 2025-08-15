#ifndef COVM_EXEC_HANDLE_HPP_
#define COVM_EXEC_HANDLE_HPP_

#include <future>

#include "coverage_measurement.hpp"

namespace citcpp {

/**
 * This class provides means to monitor the progress of the execution,
 * as well as to terminate it, if coverage measurement of a test set
 * shall be aborted and the intermediate results shall be returned.
 * All methods can be safely called from an arbitrary thread.
 */
class covm_exec_handle {
  public:
    /**
     * The real destructor of this handle calls abort() and joins with
     * the executing thread, in order to ensure a clean termination
     * if this handle is destroyed without the client explicitly waiting
     * for the execution.
     */
    virtual ~covm_exec_handle() {}

    /**
     * Returns the number of combinations to cover based on the given
     * input model and desired interaction strength.
     */
    virtual unsigned long long get_number_of_combinations_to_cover() const = 0;

    /**
     * Returns the currently covered number of combinations. This number
     * is frequently updated during the execution. So for instance
     * this method can be used for showing the progress of the execution.
     */
    virtual unsigned long long get_number_of_covered_combinations() const = 0;

    /**
     * Returns the current number of tests whose coverage has been measured
     * so far. This number is frequently updated during the execution.
     * So for instance this method can be used for showing the progress of
     * the execution.
     */
    virtual unsigned int get_number_of_measured_tests() const = 0;

    /**
     * Calling this method aborts the current execution.
     */
    virtual void abort() = 0;

    /**
     * This method returns a future for the coverage measurement result produced
     * by the execution.
     * The measurement result may either be complete with respect to the given
     * test set or not. This depends on whether the execution has
     * been aborted.
     */
    virtual std::future<coverage_measurement> get_coverage_measurement() = 0;

    /**
     * Once the coverage measurement has terminated, either because the
     * complete test set has been measured or because is has been aborted,
     * this method returns the duration of the execution in milli seconds.
     * If the execution has not terminated yet, then calling this method
     * returns 0.0.
     */
    virtual unsigned int get_duration_in_milli_seconds() const = 0;
};

}  // namespace citcpp

#endif /* COVM_EXEC_HANDLE_HPP_ */
