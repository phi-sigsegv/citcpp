#ifndef CAGEN_EXEC_HANDLE_HPP_
#define CAGEN_EXEC_HANDLE_HPP_

#include <future>
#include <string>
#include <string_view>

#include "test_set.hpp"

namespace citcpp {

/**
 * This class models the result of a covering array generation job.
 */
class cagen_exec_result {
  public:
    /**
     * This enumeration defines the possible results of a covering array
     * generation job.
     */
    enum class cagen_result_code {
      /**
       * The covering array generation completed succesfully and the test set
       * covers the entire specified coverage base.
       */
      COVERING_ARRAY_GENERATION_COMPLETED,
      /**
       * The covering array generation has been aborted, meaning the test set
       * may only cover a fraction of the coverage base.
       */
      COVERING_ARRAY_GENERATION_ABORTED,
      /**
       * The overing array generation has terminated with an error.
       * Use cagen_exec_result#get_error_message in order to retrieve
       * the corresponding error message.
       */
      COVERING_ARRAY_GENERATION_ERROR
    };

    /**
     * Returns the created test set.
     */
    const test_set &get_result() const { return test_set_; }

    /**
     * Returns the status code defining the result of the covering array
     * generation.
     */
    cagen_result_code get_result_code() const { return result_code_; }

    /**
     * Returns the error message if an error occurred, or otherwise an empty
     * string.
     */
    std::string_view get_error_message() const { return error_message_; }

  protected:
    test_set test_set_;
    cagen_result_code result_code_;
    std::string error_message_;
};

/**
 * This class provides means to monitor the progress of the execution,
 * as well as to terminate it, if construction of a complete
 * covering array shall be aborted and the intermediate results
 * shall be returned.
 * All methods can be safely called from an arbitrary thread.
 */
class cagen_exec_handle {
  public:
    /**
     * The real destructor of this handle calls abort() and joins with
     * the executing thread, in order to ensure a clean termination
     * if this handle is destroyed without the client explicitly waiting
     * for the execution.
     */
    virtual ~cagen_exec_handle() {}

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
     * Returns the current size of the testset. This number
     * is frequently updated during the execution. So for instance
     * this method can be used for showing the progress of the execution.
     */
    virtual unsigned int get_testset_size() const = 0;

    /**
     * Calling this method aborts the current execution.
     */
    virtual void abort() = 0;

    /**
     * This method returns a future for the test set produced by the execution.
     * The test set may either be complete with respect to the desired
     * interaction coverage or not. This depends on whether the execution has
     * been aborted.
     */
    virtual std::future<cagen_exec_result> get_test_set() = 0;

    /**
     * Once the computation of the test set has terminated, either because the
     * complete test set has been generated or because is has been aborted,
     * this method returns the duration of the computation in milli seconds.
     * If the computation has not terminated yet, then calling this method
     * returns 0.0.
     */
    virtual unsigned int get_duration_in_milli_seconds() const = 0;
};

}  // namespace citcpp

#endif /* CAGEN_EXEC_HANDLE_HPP_ */
