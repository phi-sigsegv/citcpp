#include <citcpp/citcpp.hpp>

#include "detail/cagen_exec_handle_ipog_impl.hpp"
#include "detail/citcpp_ipog.hpp"

namespace citcpp {

std::unique_ptr<cagen_exec_handle_ipog> compute_covering_array_ipog(
    input_model input_model, unsigned int t,
    const covering_array_computation_config &config) {

  auto ipog_algo =
      std::make_unique<detail::citcpp_ipog>(std::move(input_model), config);

  ipog_algo->set_interaction_strength(t);

  detail::cagen_exec_handle_ipog_impl *handle =
      new detail::cagen_exec_handle_ipog_impl();
  handle->set_runnable(std::move(ipog_algo));
  return std::unique_ptr<cagen_exec_handle_ipog>(handle);
}

std::unique_ptr<cagen_exec_handle_ipog> compute_covering_array_ipog(
    input_model input_model, unsigned int t) {
  return compute_covering_array_ipog(input_model, t,
                                     covering_array_computation_config());
}

}  // namespace citcpp
