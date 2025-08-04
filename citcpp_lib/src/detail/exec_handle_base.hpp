#ifndef EXEC_HANDLE_IMPL_HPP_
#define EXEC_HANDLE_IMPL_HPP_

#include <atomic>
#include <citcpp/exec_handle.hpp>
#include <functional>
#include <thread>

namespace citcpp {
namespace detail {

class exec_handle_base : public virtual exec_handle {
  public:
    exec_handle_base()
        : exec_handle(),
          num_combinations_to_cover_(0),
          covered_combinations_(0),
          testset_size_(0),
          is_aborted_(),
          test_set_(),
          duration_msec_(0),
          thread_() {}

    exec_handle_base(exec_handle_base&&) = default;

    exec_handle_base(const exec_handle_base&) = delete;

    exec_handle_base& operator=(exec_handle_base&&) = default;

    exec_handle_base& operator=(const exec_handle_base&) = delete;

    ~exec_handle_base() {
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

    unsigned int get_testset_size() const { return testset_size_; }

    void abort() { is_aborted_.test_and_set(); }

    std::future<citcpp::test_set> get_test_set() {
      return test_set_.get_future();
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

    void set_testset_size_(unsigned int testset_size) {
      testset_size_ = testset_size;
    }

    void add_testset_size_(unsigned int testset_size) {
      testset_size_.fetch_add(testset_size, std::memory_order_acq_rel);
    }

    bool is_job_aborted() { return is_aborted_.test(); }

    void set_test_set(citcpp::test_set&& test_set) {
      test_set_.set_value(std::move(test_set));
    }

    void set_duration_in_milli_seconds(unsigned int duration_msec) {
      duration_msec_ = duration_msec;
    }

  public:
    std::atomic_ullong num_combinations_to_cover_;
    std::atomic_ullong covered_combinations_;
    std::atomic_uint testset_size_;
    std::atomic_flag is_aborted_;
    std::promise<citcpp::test_set> test_set_;
    std::atomic_uint duration_msec_;
    std::thread thread_;
};

}  // namespace detail
}  // namespace citcpp

#endif /* EXEC_HANDLE_IMPL_HPP_ */
