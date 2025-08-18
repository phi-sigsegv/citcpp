#ifndef CITCPP_HPP_
#define CITCPP_HPP_

#include <memory>

#include "cagen_exec_handle_ipog.hpp"
#include "citcpp_config.hpp"
#include "covm_exec_handle.hpp"
#include "input_model.hpp"
#include "test_set.hpp"

namespace citcpp {

/**
 * Triggers execution of the calculation of a covering test set.
 * This returns immediately to the caller with a handle object,
 * which can then be used to monitor the progress of the execution
 * or to terminate it, as well as to obtain the final results.
 */
std::unique_ptr<cagen_exec_handle_ipog> compute_covering_array_ipog(
    input_model input_model, unsigned int t,
    const covering_array_computation_config &config);

/**
 * Triggers execution of the calculation of a covering test set.
 * This returns immediately to the caller with a handle object,
 * which can then be used to monitor the progress of the execution
 * or to terminate it, as well as to obtain the final results.
 */
std::unique_ptr<cagen_exec_handle_ipog> compute_covering_array_ipog(
    input_model input_model, unsigned int t);

/**
 * Triggers execution of the coverage measurement of a given test set.
 * This returns immediately to the caller with a handle object,
 * which can then be used to monitor the progress of the execution
 * or to terminate it, as well as to obtain the final results.
 */
std::unique_ptr<covm_exec_handle> measure_coverage(
    input_model input_model, test_set tests, unsigned int t,
    const coverage_measurement_config &config);

/**
 * Triggers execution of the coverage measurement of a given test set.
 * This returns immediately to the caller with a handle object,
 * which can then be used to monitor the progress of the execution
 * or to terminate it, as well as to obtain the final results.
 */
std::unique_ptr<covm_exec_handle> measure_coverage(input_model input_model,
                                                   test_set tests,
                                                   unsigned int t);

}  // namespace citcpp

#endif /* CITCPP_HPP_ */
