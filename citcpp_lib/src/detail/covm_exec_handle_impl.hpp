#ifndef COVM_EXEC_HANDLE_IMPL_HPP_
#define COVM_EXEC_HANDLE_IMPL_HPP_

#include <atomic>
#include <citcpp/covm_exec_handle.hpp>
#include <memory>
#include <thread>

#include "citcpp_covm.hpp"

namespace citcpp {
namespace detail {

class covm_exec_handle_impl : public virtual covm_exec_handle {
  public:
    covm_exec_handle_impl()
        : covm_exec_handle(),
          num_combinations_to_cover_(0),
          checked_combinations_(0),
          is_aborted_(),
          covm_result_(),
          duration_msec_(0),
          thread_(),
          runnable_() {}

    covm_exec_handle_impl(covm_exec_handle_impl&&) = delete;
    covm_exec_handle_impl(const covm_exec_handle_impl&) = delete;
    covm_exec_handle_impl& operator=(covm_exec_handle_impl&&) = delete;
    covm_exec_handle_impl& operator=(const covm_exec_handle_impl&) = delete;

    ~covm_exec_handle_impl() {
      abort();
      thread_.join();
    }

  public:
    unsigned long long get_number_of_combinations_to_cover() const {
      return num_combinations_to_cover_;
    }

    unsigned long long get_number_of_checked_combinations() const {
      return checked_combinations_;
    }

    void abort() { is_aborted_.test_and_set(); }

    std::future<citcpp::covm_exec_result> get_coverage_measurement() {
      return covm_result_.get_future();
    }

    unsigned int get_duration_in_milli_seconds() const {
      return duration_msec_;
    }

    void set_number_of_combinations_to_cover(
        unsigned long long num_combinations_to_cover) {
      num_combinations_to_cover_ = num_combinations_to_cover;
    }

    void set_number_of_checked_combinations(
        unsigned long long checked_combinations) {
      checked_combinations_ = checked_combinations;
    }

    void add_number_of_checked_combinations(
        unsigned long long checked_combinations) {
      checked_combinations_.fetch_add(checked_combinations,
                                      std::memory_order_acq_rel);
    }

    bool is_job_aborted() { return is_aborted_.test(); }

    void set_coverage_measurement(citcpp::covm_exec_result&& covm_result) {
      covm_result_.set_value(std::move(covm_result));
    }

    void set_duration_in_milli_seconds(unsigned int duration_msec) {
      duration_msec_ = duration_msec;
    }

    /**
     * Sets the runnable to be called by the thread of this execution
     * handle. The thread will invoke the runnable right away,
     * as soon as this method is being called.
     */
    void set_runnable(std::unique_ptr<citcpp_covm>&& runnable) {
      runnable_ = std::move(runnable);
      thread_ = std::thread(&citcpp_covm::entry_point, runnable_.get(),
                            std::ref(*this));
    }

  public:
    std::atomic_ullong num_combinations_to_cover_;
    std::atomic_ullong checked_combinations_;
    std::atomic_flag is_aborted_;
    std::promise<citcpp::covm_exec_result> covm_result_;
    std::atomic_uint duration_msec_;
    std::thread thread_;
    std::unique_ptr<citcpp_covm> runnable_;
};

}  // namespace detail
}  // namespace citcpp

#endif /* COVM_EXEC_HANDLE_IMPL_HPP_ */
