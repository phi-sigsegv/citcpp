#ifndef DETAIL_CITCPP_COVM_HPP_
#define DETAIL_CITCPP_COVM_HPP_

#include <citcpp/citcpp_config.hpp>
#include <citcpp/input_model.hpp>
#include <citcpp/test_set.hpp>

#include "internal_model.hpp"
#include "internal_test_set.hpp"

namespace citcpp {
namespace detail {

// Forward declaration of covm_exec_handle_impl due to usage of
// citcpp_covm by covm_exec_handle_impl definition.
class covm_exec_handle_impl;

/**
 * This class provides an implementation of the IPOG algorithm.
 */
class citcpp_covm {
  public:
    citcpp_covm(const model &input_model, const test_set &tests,
                const coverage_measurement_config &config);
    citcpp_covm(model &&input_model, test_set &&tests,
                const coverage_measurement_config &config);

    /**
     * Too lazy to implement/ensuring that it is well-defined.
     */
    citcpp_covm(citcpp_covm &&) = delete;
    citcpp_covm(const citcpp_covm &) = delete;

    /**
     * Too lazy to implement/ensuring that it is well-defined.
     */
    citcpp_covm &operator=(citcpp_covm &&) = delete;
    citcpp_covm &operator=(const citcpp_covm &) = delete;

    void set_interaction_strength(unsigned int t);

    /**
     * This is the entry point to be called by a thread.
     */
    void entry_point(covm_exec_handle_impl &exec_handle);

  private:
    const coverage_measurement_config config_;
    const model input_model_;
    const internal_model model_;
    const test_set input_tests_;
    const internal_test_set tests_;
    unsigned int strength_;
};

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_CITCPP_COVM_HPP_ */
