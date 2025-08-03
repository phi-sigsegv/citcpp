#ifndef DETAIL_EXEC_HANDLE_IPOG_IMPL_CPP_
#define DETAIL_EXEC_HANDLE_IPOG_IMPL_CPP_

#include <memory>
#include <citcpp/exec_handle_ipog.hpp>
#include "citcpp_ipog.hpp"
#include "exec_handle_base.hpp"

namespace citcpp
{
  namespace detail
  {
    class exec_handle_ipog_impl : public virtual exec_handle_ipog,
	public exec_handle_base
    {
    public:
      exec_handle_ipog_impl () :
	  exec_handle_ipog (), exec_handle_base (), num_processed_parameters_ (
	      0), runnable_ ()
      {
      }

      exec_handle_ipog_impl (exec_handle_ipog_impl&&) = default;

      exec_handle_ipog_impl (const exec_handle_ipog_impl&) = delete;

      exec_handle_ipog_impl&
      operator= (exec_handle_ipog_impl&&) = default;

      exec_handle_ipog_impl&
      operator= (const exec_handle_ipog_impl&) = delete;

      ~exec_handle_ipog_impl ()
      {
      }

    public:
      unsigned int
      get_number_of_processed_parameters () const
      {
	return num_processed_parameters_;
      }

      void
      set_number_of_processed_parameters (unsigned int num_processed_parameters)
      {
	num_processed_parameters_ = num_processed_parameters;
      }

      /**
       * Sets the runnable to be called by the thread of this execution
       * handle. The thread will invoke the runnable right away,
       * as soon as this method is being called.
       */
      void
      set_runnable (std::unique_ptr<citcpp_ipog> &&runnable)
      {
	runnable_ = std::move (runnable);
	thread_ = std::thread (&citcpp_ipog::entry_point, runnable_.get (),
			       std::ref (*this));
      }

    private:
      std::atomic_uint num_processed_parameters_;
      std::unique_ptr<citcpp_ipog> runnable_;
    };
  }
}

#endif /* DETAIL_EXEC_HANDLE_IPOG_IMPL_CPP_ */
