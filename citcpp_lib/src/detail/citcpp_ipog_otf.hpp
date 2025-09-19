#ifndef DETAIL_CITCPP_IPOG_OTF_HPP_
#define DETAIL_CITCPP_IPOG_OTF_HPP_

#include <citcpp/citcpp_config.hpp>
#include <citcpp/input_model.hpp>
#include <citcpp/test_set.hpp>

#include "internal_model.hpp"
#include "internal_test_set.hpp"

namespace citcpp {
namespace detail {

// Forward declaration of cagen_exec_handle_ipog_impl due to usage of
// citcpp_ipog by cagen_exec_handle_ipog_impl definition.
class cagen_exec_handle_ipog_impl;

/**
 * This class provides an implementation of the IPOG algorithm.
 */
class citcpp_ipog_otf {
  public:
    citcpp_ipog_otf(const input_model &input_model,
                    const covering_array_computation_config &config);
    citcpp_ipog_otf(input_model &&input_model,
                    const covering_array_computation_config &config);
    citcpp_ipog_otf(const input_model &input_model,
                    const citcpp::test_set &tests,
                    const covering_array_computation_config &config);
    citcpp_ipog_otf(input_model &&input_model, test_set &&tests,
                    const covering_array_computation_config &config);

    /**
     * Too lazy to implement/ensuring that it is well-defined.
     */
    citcpp_ipog_otf(citcpp_ipog_otf &&) = delete;
    citcpp_ipog_otf(const citcpp_ipog_otf &) = delete;

    /**
     * Too lazy to implement/ensuring that it is well-defined.
     */
    citcpp_ipog_otf &operator=(citcpp_ipog_otf &&) = delete;
    citcpp_ipog_otf &operator=(const citcpp_ipog_otf &) = delete;

    void set_interaction_strength(unsigned int t);

    /**
     * This is the entry point to be called by a thread.
     */
    void entry_point(cagen_exec_handle_ipog_impl &exec_handle);

  private:
    const citcpp::covering_array_computation_config config_;
    const citcpp::input_model input_model_;
    const model model_;
    const internal_test_set input_tests_;
    unsigned int strength_;
};

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_CITCPP_IPOG_OTF_HPP_ */
