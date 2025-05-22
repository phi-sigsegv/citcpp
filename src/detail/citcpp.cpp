#include "../citcpp.hpp"
#include "exec_handle_impl.hpp"
#include "citcpp_ipog.hpp"

namespace citcpp
{
  class CitCpp::impl
  {
  public:
    impl (const InputModel &input_model) :
	m_input_model (input_model)
    {
    }

  private:
    InputModel m_input_model;
  };

  CitCpp::CitCpp (const InputModel &input_model) :
      m_impl (new impl (input_model))
  {
  }

  CitCpp::~CitCpp () = default;

  std::unique_ptr<IExecHandle>
  CitCpp::computeCoveringTestSet (int t, Algorithm alg) const
  {
    detail::ExecHandleImpl *handle = new detail::ExecHandleImpl ();
    handle->setRunnable (std::make_unique<detail::CitCppIpog> ());
    return std::unique_ptr<IExecHandle> (handle);
  }
}
