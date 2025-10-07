#include <citcpp/citcpp.hpp>

#include "detail/cagen_exec_handle_ipog_impl.hpp"
#include "detail/citcpp_covm.hpp"
#include "detail/citcpp_ipog.hpp"
#include "detail/citcpp_ipog_otf.hpp"
#include "detail/covm_exec_handle_impl.hpp"

namespace citcpp {

std::unique_ptr<cagen_exec_handle_ipog> compute_covering_array_ipog(
    model input_model, unsigned int t,
    const covering_array_computation_config &config) {

  detail::cagen_exec_handle_ipog_impl *handle =
      new detail::cagen_exec_handle_ipog_impl();

  switch (config.algorithm()) {
    case covering_array_computation_algorithm::IPOG: {
      auto ipog_algo =
          std::make_unique<detail::citcpp_ipog>(std::move(input_model), config);
      ipog_algo->set_interaction_strength(t);
      handle->set_runnable(std::move(ipog_algo));
      break;
    }
    case covering_array_computation_algorithm::IPOG_OTF: {
      auto ipog_algo = std::make_unique<detail::citcpp_ipog_otf>(
          std::move(input_model), config);
      ipog_algo->set_interaction_strength(t);
      handle->set_runnable(std::move(ipog_algo));
      break;
    }
  }

  return std::unique_ptr<cagen_exec_handle_ipog>(handle);
}

std::unique_ptr<cagen_exec_handle_ipog> compute_covering_array_ipog(
    model input_model, unsigned int t) {
  return compute_covering_array_ipog(input_model, t,
                                     covering_array_computation_config());
}

std::unique_ptr<cagen_exec_handle_ipog> compute_covering_array_ipog(
    model input_model, test_set tests, unsigned int t,
    const covering_array_computation_config &config) {

  detail::cagen_exec_handle_ipog_impl *handle =
      new detail::cagen_exec_handle_ipog_impl();

  switch (config.algorithm()) {
    case covering_array_computation_algorithm::IPOG: {
      auto ipog_algo = std::make_unique<detail::citcpp_ipog>(
          std::move(input_model), std::move(tests), config);
      ipog_algo->set_interaction_strength(t);
      handle->set_runnable(std::move(ipog_algo));
      break;
    }
    case covering_array_computation_algorithm::IPOG_OTF: {
      auto ipog_algo = std::make_unique<detail::citcpp_ipog_otf>(
          std::move(input_model), std::move(tests), config);
      ipog_algo->set_interaction_strength(t);
      handle->set_runnable(std::move(ipog_algo));
      break;
    }
  }

  return std::unique_ptr<cagen_exec_handle_ipog>(handle);
}

std::unique_ptr<cagen_exec_handle_ipog> compute_covering_array_ipog(
    model input_model, test_set tests, unsigned int t) {
  return compute_covering_array_ipog(input_model, tests, t,
                                     covering_array_computation_config());
}

std::unique_ptr<covm_exec_handle> measure_coverage(
    model input_model, test_set tests, unsigned int t,
    const coverage_measurement_config &config) {

  auto covm_algo = std::make_unique<detail::citcpp_covm>(
      std::move(input_model), std::move(tests), config);

  covm_algo->set_interaction_strength(t);

  detail::covm_exec_handle_impl *handle = new detail::covm_exec_handle_impl();
  handle->set_runnable(std::move(covm_algo));
  return std::unique_ptr<covm_exec_handle>(handle);
}

std::unique_ptr<covm_exec_handle> measure_coverage(model input_model,
                                                   test_set tests,
                                                   unsigned int t) {
  return measure_coverage(input_model, tests, t, coverage_measurement_config());
}

}  // namespace citcpp
