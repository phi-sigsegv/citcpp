#ifndef CITCPP_HPP_
#define CITCPP_HPP_

#include <memory>

#include "citcpp_config.hpp"
#include "exec_handle_ipog.hpp"
#include "input_model.hpp"

namespace citcpp {

/**
 * Triggers execution of the calculation of a covering test set.
 * This returns immediately to the caller with a handle object,
 * which can then be used to monitor the progress of the execution
 * or to terminate it, as well as to obtain the final results.
 */
std::unique_ptr<exec_handle_ipog> compute_covering_array_ipog(
    input_model input_model, unsigned int t,
    const covering_array_computation_config &config);

/**
 * Triggers execution of the calculation of a covering test set.
 * This returns immediately to the caller with a handle object,
 * which can then be used to monitor the progress of the execution
 * or to terminate it, as well as to obtain the final results.
 */
std::unique_ptr<exec_handle_ipog> compute_covering_array_ipog(
    input_model input_model, unsigned int t);

}  // namespace citcpp

#endif /* CITCPP_HPP_ */
