#include <algorithm>
#include <citcpp/citcpp.hpp>

#include "detail/cagen_exec_handle_ipog_impl.hpp"
#include "detail/citcpp_covm.hpp"
#include "detail/citcpp_ipog.hpp"
#include "detail/citcpp_ipog_otf.hpp"
#include "detail/covm_exec_handle_impl.hpp"

namespace {

// Sorts the list of values of integer parameters in ascending order.
// This is required, since some constraint handlers might rely on this
// to make the respective implementation easier.
void sort_int_parameter_values(citcpp::model& input_model) {
  using namespace citcpp;

  for (auto& param : input_model.get_parameters()) {
    if (param.get_type() == parameter_type::INTEGER) {
      std::sort(param.get_values().begin(), param.get_values().end(),
                [](const parameter_value& a, const parameter_value& b) {
                  const int a_int = a;
                  const int b_int = b;

                  return a_int < b_int;
                });
    }
  }
}

}  // namespace

namespace citcpp {

std::unique_ptr<cagen_exec_handle_ipog> compute_covering_array_ipog(
    model input_model, int t, const covering_array_computation_config& config) {

  sort_int_parameter_values(input_model);

  detail::cagen_exec_handle_ipog_impl* handle =
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
    model input_model, int t) {
  return compute_covering_array_ipog(std::move(input_model), t,
                                     covering_array_computation_config());
}

std::unique_ptr<cagen_exec_handle_ipog> compute_covering_array_ipog(
    model input_model, test_set tests, int t,
    const covering_array_computation_config& config) {

  sort_int_parameter_values(input_model);

  detail::cagen_exec_handle_ipog_impl* handle =
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
    model input_model, test_set tests, int t) {
  return compute_covering_array_ipog(std::move(input_model), std::move(tests),
                                     t, covering_array_computation_config());
}

std::unique_ptr<covm_exec_handle> measure_coverage(
    model input_model, test_set tests, unsigned int t,
    const coverage_measurement_config& config) {

  sort_int_parameter_values(input_model);

  auto covm_algo = std::make_unique<detail::citcpp_covm>(
      std::move(input_model), std::move(tests), config);

  covm_algo->set_interaction_strength(t);

  detail::covm_exec_handle_impl* handle = new detail::covm_exec_handle_impl();
  handle->set_runnable(std::move(covm_algo));
  return std::unique_ptr<covm_exec_handle>(handle);
}

std::unique_ptr<covm_exec_handle> measure_coverage(model input_model,
                                                   test_set tests,
                                                   unsigned int t) {
  return measure_coverage(std::move(input_model), std::move(tests), t,
                          coverage_measurement_config());
}

}  // namespace citcpp
