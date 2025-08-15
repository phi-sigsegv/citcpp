#ifndef COVM_EXEC_HANDLE_IMPL_HPP_
#define COVM_EXEC_HANDLE_IMPL_HPP_

#include <atomic>
#include <citcpp/covm_exec_handle.hpp>
#include <thread>

namespace citcpp {
namespace detail {

class covm_exec_handle_impl : public virtual covm_exec_handle {
  public:
    covm_exec_handle_impl()
        : covm_exec_handle(),
          num_combinations_to_cover_(0),
          covered_combinations_(0),
          num_measured_test_(0),
          is_aborted_(),
          covm_result_(),
          duration_msec_(0),
          thread_() {}

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

    unsigned long long get_number_of_covered_combinations() const {
      return covered_combinations_;
    }

    unsigned int get_number_of_measured_tests() const {
      return num_measured_test_;
    }

    void abort() { is_aborted_.test_and_set(); }

    std::future<citcpp::coverage_measurement> get_coverage_measurement() {
      return covm_result_.get_future();
    }

    unsigned int get_duration_in_milli_seconds() const {
      return duration_msec_;
    }

    void set_number_of_combinations_to_cover(
        unsigned long long num_combinations_to_cover) {
      num_combinations_to_cover_ = num_combinations_to_cover;
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

    void set_number_of_measured_tests(unsigned int num_measured_test) {
      num_measured_test_ = num_measured_test;
    }

    void add_number_of_measured_tests(unsigned int num_measured_test) {
      num_measured_test_.fetch_add(num_measured_test,
                                   std::memory_order_acq_rel);
    }

    bool is_job_aborted() { return is_aborted_.test(); }

    void set_coverage_measurement(citcpp::coverage_measurement&& covm_result) {
      covm_result_.set_value(std::move(covm_result));
    }

    void set_duration_in_milli_seconds(unsigned int duration_msec) {
      duration_msec_ = duration_msec;
    }

  public:
    std::atomic_ullong num_combinations_to_cover_;
    std::atomic_ullong covered_combinations_;
    std::atomic_uint num_measured_test_;
    std::atomic_flag is_aborted_;
    std::promise<citcpp::coverage_measurement> covm_result_;
    std::atomic_uint duration_msec_;
    std::thread thread_;
};

}  // namespace detail
}  // namespace citcpp

#endif /* COVM_EXEC_HANDLE_IMPL_HPP_ */
