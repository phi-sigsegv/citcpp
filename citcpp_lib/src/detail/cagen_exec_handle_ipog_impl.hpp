#ifndef DETAIL_CAGEN_EXEC_HANDLE_IPOG_IMPL_CPP_
#define DETAIL_CAGEN_EXEC_HANDLE_IPOG_IMPL_CPP_

#include <citcpp/cagen_exec_handle_ipog.hpp>
#include <memory>

#include "cagen_exec_handle_base.hpp"
#include "citcpp_ipog.hpp"

namespace citcpp {
namespace detail {

class cagen_exec_handle_ipog_impl : public virtual cagen_exec_handle_ipog,
                                    public cagen_exec_handle_base {
  protected:
    // Only created as joining_cagen_exec_handle (see there).
    cagen_exec_handle_ipog_impl() = default;

  public:
    unsigned int get_number_of_parameters_to_process() const override {
      return num_parameters_to_process_;
    }

    void set_number_of_parameters_to_process(
        unsigned int num_parameters_to_process) {
      num_parameters_to_process_ = num_parameters_to_process;
    }

    unsigned int get_number_of_processed_parameters() const override {
      return num_processed_parameters_;
    }

    void set_number_of_processed_parameters(
        unsigned int num_processed_parameters) {
      num_processed_parameters_ = num_processed_parameters;
    }

    /**
     * Sets the runnable to be called by the thread of this execution
     * handle. The thread will invoke the runnable right away,
     * as soon as this method is being called.
     */
    void set_runnable(std::unique_ptr<citcpp_ipog>&& runnable) {
      start(std::move(runnable), *this);
    }

  private:
    unsigned int num_parameters_to_process_{0};
    unsigned int num_processed_parameters_{0};
};

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_CAGEN_EXEC_HANDLE_IPOG_IMPL_CPP_ */
