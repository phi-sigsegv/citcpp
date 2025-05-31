#include "citcpp.hpp"
#include "detail/exec_handle_impl.hpp"
#include "detail/citcpp_ipog.hpp"

namespace citcpp
{
  class CitCpp::impl
  {
  public:
    impl (const InputModel &input_model) :
	input_model_ (input_model)
    {
    }

    const InputModel&
    getInputModel () const
    {
      return input_model_;
    }

  private:
    InputModel input_model_;
  };

  CitCpp::CitCpp (const InputModel &input_model) :
      impl_ (new impl (input_model))
  {
  }

  CitCpp::~CitCpp () = default;

  std::unique_ptr<IExecHandle>
  CitCpp::computeCoveringTestSet (unsigned int t, Algorithm alg) const
  {
    auto ipog_algo = std::make_unique<detail::CitCppIpog> (
	impl_->getInputModel ());

    ipog_algo->setInteractionStrength (t);

    detail::ExecHandleImpl *handle = new detail::ExecHandleImpl ();
    handle->setRunnable (std::move (ipog_algo));
    return std::unique_ptr<IExecHandle> (handle);
  }
}
