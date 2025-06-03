#include "citcpp.hpp"
#include "detail/exec_handle_impl.hpp"
#include "detail/citcpp_ipog.hpp"

namespace citcpp
{
  std::unique_ptr<exec_handle>
  compute_covering_test_set (const input_model &input_model, unsigned int t,
			     algorithm alg)
  {
    auto ipog_algo = std::make_unique<detail::citcpp_ipog> (input_model);

    ipog_algo->set_interaction_strength (t);

    detail::exec_handle_impl *handle = new detail::exec_handle_impl ();
    handle->set_runnable (std::move (ipog_algo));
    return std::unique_ptr<exec_handle> (handle);
  }
}
