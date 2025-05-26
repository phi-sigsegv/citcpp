#include "citcpp.hpp"
#include "detail/exec_handle_impl.hpp"
#include "detail/citcpp_ipog.hpp"

namespace citcpp
{
  class CitCpp::impl
  {
  public:
    impl (const InputModel &input_model) :
	m_input_model (input_model)
    {
    }

    const InputModel&
    getInputModel () const
    {
      return m_input_model;
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
    auto ipog_algo = std::make_unique<detail::CitCppIpog> (
	m_impl->getInputModel ());

    detail::ExecHandleImpl *handle = new detail::ExecHandleImpl ();
    handle->setRunnable (std::move (ipog_algo));
    return std::unique_ptr<IExecHandle> (handle);
  }
}
