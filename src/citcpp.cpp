#include "citcpp.hpp"
#include "detail/exec_handle_ipog_impl.hpp"
#include "detail/citcpp_ipog.hpp"

namespace citcpp
{
  std::unique_ptr<exec_handle_ipog>
  compute_covering_test_set_ipog (input_model input_model, unsigned int t)
  {
    auto ipog_algo = std::make_unique<detail::citcpp_ipog> (
	std::move (input_model));

    ipog_algo->set_interaction_strength (t);

    detail::exec_handle_ipog_impl *handle =
	new detail::exec_handle_ipog_impl ();
    handle->set_runnable (std::move (ipog_algo));
    return std::unique_ptr<exec_handle_ipog> (handle);
  }
}
