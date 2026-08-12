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
    const test_set& get_result() const;

    /**
     * Sets the test set.
     */
    void set_result(const test_set& tests);

    /**
     * Sets the test set.
     */
    void set_result(test_set&& tests);

    /**
     * Returns the status code defining the result of the covering array
     * generation.
     */
    cagen_result_code get_result_code() const;

    /**
     * Sets the status code defining the result of a covering array
     * generation.
     */
    void set_result_code(cagen_result_code result_code);

    /**
     * Returns the error message if an error occurred, or otherwise an empty
     * string.
     */
    std::string_view get_error_message() const;

    /**
     * Sets the error message if an error occured. Shall be set to an empty
     * string otherwise.
     */
    void set_error_message(std::string_view error_message);

  private:
    test_set test_set_{};
    cagen_result_code result_code_{
        cagen_result_code::COVERING_ARRAY_GENERATION_COMPLETED};
    std::string error_message_{};
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
     * This enumeration defines the different execution phases of the algorithm
     * for constructing a covering array.
     */
    enum class phase {
      /**
       * This is a phase where the constraint handler is being initalized.
       */
      CONSTRAINT_HANDLER_INIT = 0,
      /**
       * This is a phase where the covering array is being created.
       */
      COVERING_ARRAY_CONSTRUCTION = 1
    };

    /**
     * The real destructor of this handle calls abort() and joins with
     * the executing thread, in order to ensure a clean termination
     * if this handle is destroyed without the client explicitly waiting
     * for the execution.
     */
    virtual ~cagen_exec_handle() = default;

    /**
     * Returns which phase is active.
     */
    virtual phase get_execution_phase() const = 0;

    /**
     * Returns a progress value, which represents the state where the
     * constraint handler is fully initialized.
     */
    virtual unsigned int get_constraint_handler_init_progress_target()
        const = 0;

    /**
     * Returns the current progress value with regard to the initialization
     * of the constraint handler.
     */
    virtual unsigned int get_constraint_handler_init_progress_current()
        const = 0;

    /**
     * Returns the number of combinations to process based on the given
     * input model and desired interaction strength.
     * Constraints of the model are ignored for the computation of this
     * number, meaning it will NOT reflect the number of all valid
     * combinations of the desired interaction strength, but may be a number
     * greater than that.
     */
    virtual unsigned long long get_number_of_combinations_to_process()
        const = 0;

    /**
     * Returns the number of combinations processed so far. This number
     * is frequently updated during the execution. So for instance
     * this method can be used for showing the progress of the execution
     * when compared again the number of combinations to process.
     */
    virtual unsigned long long get_number_of_processed_combinations() const = 0;

    /**
     * Returns the currently covered number of combinations. This number
     * is frequently updated during the execution.
     */
    virtual unsigned long long get_number_of_covered_combinations() const = 0;

    /**
     * Returns the current size of the testset. This number
     * is frequently updated during the execution. So for instance
     * this method can be used for showing the progress of the execution.
     */
    virtual std::size_t get_testset_size() const = 0;

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
    virtual std::size_t get_duration_in_milli_seconds() const = 0;
};

}  // namespace citcpp

#endif /* CAGEN_EXEC_HANDLE_HPP_ */
