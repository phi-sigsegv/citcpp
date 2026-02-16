#ifndef COVM_EXEC_HANDLE_IMPL_HPP_
#define COVM_EXEC_HANDLE_IMPL_HPP_

#include <atomic>
#include <citcpp/covm_exec_handle.hpp>
#include <memory>
#include <thread>

#include "citcpp_covm.hpp"
#include "constraint_handler_init_progress.hpp"

namespace citcpp {
namespace detail {

class covm_exec_handle_impl : public virtual covm_exec_handle {
  public:
    covm_exec_handle_impl()
        : covm_exec_handle(),
          exec_phase_(0),
          c_handler_init_progress_(),
          num_combinations_to_process_(0),
          processed_combinations_(0),
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
    phase get_execution_phase() const override {
      unsigned int v = exec_phase_;
      return static_cast<phase>(v);
    }

    unsigned int get_constraint_handler_init_progress_target() const override {
      return c_handler_init_progress_
          .get_constraint_handler_init_progress_target();
    }

    unsigned int get_constraint_handler_init_progress_current() const override {
      return c_handler_init_progress_
          .get_constraint_handler_init_progress_current();
    }

    unsigned long long get_number_of_combinations_to_process() const override {
      return num_combinations_to_process_;
    }

    unsigned long long get_number_of_processed_combinations() const override {
      return processed_combinations_;
    }

    void abort() override { is_aborted_.test_and_set(); }

    std::future<citcpp::covm_exec_result> get_coverage_measurement() override {
      return covm_result_.get_future();
    }

    unsigned int get_duration_in_milli_seconds() const override {
      return duration_msec_;
    }

    void set_execution_phase(phase p) {
      unsigned int v = static_cast<unsigned int>(p);
      exec_phase_ = v;
    }

    constraint_handler_init_progress& get_constraint_handler_init_progress() {
      return c_handler_init_progress_;
    }

    void set_number_of_combinations_to_process(
        unsigned long long num_combinations_to_process) {
      num_combinations_to_process_ = num_combinations_to_process;
    }

    void add_number_of_combinations_to_process(
        unsigned long long num_combinations_to_process) {
      num_combinations_to_process_.fetch_add(num_combinations_to_process,
                                             std::memory_order_acq_rel);
    }

    void set_number_of_processed_combinations(
        unsigned long long processed_combinations) {
      processed_combinations_ = processed_combinations;
    }

    void add_number_of_processed_combinations(
        unsigned long long processed_combinations) {
      processed_combinations_.fetch_add(processed_combinations,
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
    std::atomic_uint exec_phase_;
    constraint_handler_init_progress c_handler_init_progress_;
    std::atomic_ullong num_combinations_to_process_;
    std::atomic_ullong processed_combinations_;
    std::atomic_flag is_aborted_;
    std::promise<citcpp::covm_exec_result> covm_result_;
    std::atomic_uint duration_msec_;
    std::thread thread_;
    std::unique_ptr<citcpp_covm> runnable_;
};

}  // namespace detail
}  // namespace citcpp

#endif /* COVM_EXEC_HANDLE_IMPL_HPP_ */
