#ifndef COVM_EXEC_HANDLE_HPP_
#define COVM_EXEC_HANDLE_HPP_

#include <future>
#include <string>
#include <string_view>
#include <unordered_map>

#include "coverage_measurement.hpp"

namespace citcpp {

/**
 * This class models the result of a coverage measurement job.
 */
class covm_exec_result {
  public:
    /**
     * This enumeration defines the possible results of a coverage measurement
     * job.
     */
    enum class covm_result_code {
      /**
       * The coverage measurement completed succesfully and the entire
       * test set has been checked against the coverage base.
       */
      COVERAGE_MEASUREMENT_COMPLETED,
      /**
       * The coverage measurement has been aborted, meaning the test set has
       * only been measured against a fraction of the coverage base.
       */
      COVERAGE_MEASUREMENT_ABORTED,
      /**
       * The coverage measurement has terminated with an error.
       * Use covm_exec_result#get_error_message in order to retrieve
       * the corresponding error message.
       */
      COVERAGE_MEASUREMENT_ERROR
    };

    covm_exec_result()
        : result_(),
          invalid_test_indices_(),
          result_code_(covm_result_code::COVERAGE_MEASUREMENT_COMPLETED),
          error_message_() {}

    /**
     * Returns a map from relation IDs to objects collecting the results
     * of the coverage measurement.
     */
    const std::unordered_map<std::string, coverage_measurement>& get_result()
        const {

      return result_;
    }

    /**
     * Returns a list of indices of tests which are invalid, i.e. violate
     * at least one constraint. Invalid tests are ignored during
     * the coverage measurement.
     */
    const std::vector<std::size_t>& get_invalid_test_indices() const {
      return invalid_test_indices_;
    }

    /**
     * Returns the status code defining the result of the coverage measurement
     * execution.
     */
    covm_result_code get_result_code() const { return result_code_; }

    /**
     * Returns the error message if an error occurred, or otherwise an empty
     * string.
     */
    std::string_view get_error_message() const { return error_message_; }

  protected:
    std::unordered_map<std::string, coverage_measurement> result_;
    std::vector<std::size_t> invalid_test_indices_;
    covm_result_code result_code_;
    std::string error_message_;
};

/**
 * This class provides means to monitor the progress of the execution,
 * as well as to terminate it, if coverage measurement of a test set
 * shall be aborted and the intermediate results shall be returned.
 * All methods can be safely called from an arbitrary thread.
 */
class covm_exec_handle {
  public:
    /**
     * This enumeration defines the different execution phases of the algorithm
     * for measuring coverage.
     */
    enum class phase {
      /**
       * This is a phase where the constraint handler is being initalized.
       */
      CONSTRAINT_HANDLER_INIT = 0,
      /**
       * This is a phase where the coverage is being measured.
       */
      COVERAGE_MEASUREMENT = 1
    };

    /**
     * The real destructor of this handle calls abort() and joins with
     * the executing thread, in order to ensure a clean termination
     * if this handle is destroyed without the client explicitly waiting
     * for the execution.
     */
    virtual ~covm_exec_handle() {}

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
     * Returns the current number of combinations whose coverage has been
     * checked. This number is frequently updated during the execution. So for
     * instance this method can be used for showing the progress of the
     * execution when compared again the number of combinations to process.
     */
    virtual unsigned long long get_number_of_processed_combinations() const = 0;

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
    virtual std::future<covm_exec_result> get_coverage_measurement() = 0;

    /**
     * Once the coverage measurement has terminated, either because the
     * complete test set has been measured or because is has been aborted,
     * this method returns the duration of the execution in milli seconds.
     * If the execution has not terminated yet, then calling this method
     * returns 0.0.
     */
    virtual std::size_t get_duration_in_milli_seconds() const = 0;
};

}  // namespace citcpp

#endif /* COVM_EXEC_HANDLE_HPP_ */
