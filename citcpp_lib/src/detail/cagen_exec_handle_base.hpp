#ifndef CAGEN_EXEC_HANDLE_IMPL_HPP_
#define CAGEN_EXEC_HANDLE_IMPL_HPP_

#include <atomic>
#include <citcpp/cagen_exec_handle.hpp>
#include <thread>

#include "constraint_handler_init_progress.hpp"

namespace citcpp {
namespace detail {

class cagen_exec_handle_base : public virtual cagen_exec_handle {
  public:
    cagen_exec_handle_base()
        : cagen_exec_handle(),
          exec_phase_(0),
          c_handler_init_progress_(),
          num_combinations_to_process_(0),
          processed_combinations_(0),
          covered_combinations_(0),
          testset_size_(0),
          is_aborted_(),
          test_set_(),
          duration_msec_(0),
          thread_() {}

    cagen_exec_handle_base(cagen_exec_handle_base&&) = delete;
    cagen_exec_handle_base(const cagen_exec_handle_base&) = delete;
    cagen_exec_handle_base& operator=(cagen_exec_handle_base&&) = delete;
    cagen_exec_handle_base& operator=(const cagen_exec_handle_base&) = delete;

    ~cagen_exec_handle_base() {
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

    unsigned long long get_number_of_covered_combinations() const override {
      return covered_combinations_;
    }

    unsigned int get_testset_size() const override { return testset_size_; }

    void abort() override {
      is_aborted_.store(true, std::memory_order_relaxed);
    }

    std::future<citcpp::cagen_exec_result> get_test_set() override {
      return test_set_.get_future();
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

    void set_number_of_processed_combinations(
        unsigned long long processed_combinations) {
      processed_combinations_ = processed_combinations;
    }

    void add_number_of_processed_combinations(
        unsigned long long processed_combinations) {
      processed_combinations_.fetch_add(processed_combinations,
                                        std::memory_order_acq_rel);
    }

    void set_number_of_covered_combinations(
        unsigned long long covered_combinations) {
      covered_combinations_ = covered_combinations;
    }

    void add_number_of_covered_combinations(
        unsigned long long covered_combinations) {
      covered_combinations_.fetch_add(covered_combinations,
                                      std::memory_order_acq_rel);
    }

    void set_testset_size(unsigned int testset_size) {
      testset_size_ = testset_size;
    }

    void add_testset_size(unsigned int testset_size) {
      testset_size_.fetch_add(testset_size, std::memory_order_acq_rel);
    }

    bool is_job_aborted() {
      return is_aborted_.load(std::memory_order_relaxed);
    }

    void set_test_set(citcpp::cagen_exec_result&& test_set) {
      test_set_.set_value(std::move(test_set));
    }

    void set_duration_in_milli_seconds(unsigned int duration_msec) {
      duration_msec_ = duration_msec;
    }

  public:
    std::atomic_uint exec_phase_;
    constraint_handler_init_progress c_handler_init_progress_;
    std::atomic_ullong num_combinations_to_process_;
    std::atomic_ullong processed_combinations_;
    std::atomic_ullong covered_combinations_;
    std::atomic_uint testset_size_;
    std::atomic_bool is_aborted_;
    std::promise<citcpp::cagen_exec_result> test_set_;
    std::atomic_uint duration_msec_;
    std::thread thread_;
};

}  // namespace detail
}  // namespace citcpp

#endif /* CAGEN_EXEC_HANDLE_IMPL_HPP_ */
