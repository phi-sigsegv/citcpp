#include "constraint_handler_concurrent.hpp"

#include <mutex>

#include "shared_constants.hpp"

namespace {

class alignas(citcpp::detail::false_sharing_avoidance_alignment)
    check_test_validity_task {

  public:
    check_test_validity_task() = default;

    check_test_validity_task(const citcpp::detail::test* test,
                             unsigned int test_index,
                             const citcpp::detail::constraint_handler* handler,
                             citcpp::detail::bitset_uint64* result,
                             std::mutex* mut)
        : test_(test),
          test_index_(test_index),
          handler_(handler),
          result_(result),
          mut_(mut) {}

    virtual ~check_test_validity_task() {}

    void operator()() {
      const bool is_valid = handler_->is_valid_partial_test(*test_);

      if (is_valid) {
        mark_test_as_valid();
      }
    }

  private:
    void mark_test_as_valid() {
      std::lock_guard<std::mutex> guard(*mut_);
      result_->set(test_index_);
    }

  private:
    const citcpp::detail::test* test_;
    unsigned int test_index_;
    const citcpp::detail::constraint_handler* handler_;
    citcpp::detail::bitset_uint64* result_;
    std::mutex* mut_;
};

class alignas(citcpp::detail::false_sharing_avoidance_alignment)
    get_valid_parameter_assignments_task {

  public:
    get_valid_parameter_assignments_task() = default;

    get_valid_parameter_assignments_task(
        const citcpp::detail::test* test, unsigned int param_idx,
        unsigned int test_index,
        const citcpp::detail::constraint_handler* handler,
        std::vector<citcpp::detail::bitset_uint64>* results)
        : test_(test),
          param_idx_(param_idx),
          test_index_(test_index),
          handler_(handler),
          results_(results) {}

    virtual ~get_valid_parameter_assignments_task() {}

    void operator()() {
      (*results_)[test_index_] =
          handler_->get_valid_parameter_assignments(*test_, param_idx_);
    }

  private:
    const citcpp::detail::test* test_;
    unsigned int param_idx_;
    unsigned int test_index_;
    const citcpp::detail::constraint_handler* handler_;
    std::vector<citcpp::detail::bitset_uint64>* results_;
};

class alignas(citcpp::detail::false_sharing_avoidance_alignment)
    replace_dont_care_values_task {

  public:
    replace_dont_care_values_task() = default;

    replace_dont_care_values_task(
        citcpp::detail::test* test,
        const citcpp::detail::constraint_handler* handler)
        : test_(test), handler_(handler) {}

    virtual ~replace_dont_care_values_task() {}

    void operator()() { handler_->replace_dont_care_values(*test_); }

  private:
    citcpp::detail::test* test_;
    const citcpp::detail::constraint_handler* handler_;
};

}  // namespace

namespace citcpp {
namespace detail {

concurrent_constraint_handler::concurrent_constraint_handler(
    const constraint_handler& handler, functor_executor& exec)
    : base_type(), handler_(handler), exec_(exec) {}

bool concurrent_constraint_handler::is_thread_safe() const { return true; }

bool concurrent_constraint_handler::is_valid_partial_test(const test& t) const {

  return handler_.is_valid_partial_test(t);
}

bitset_uint64 concurrent_constraint_handler::check_validity_of_partial_tests(
    const internal_test_set& test_set) const {

  bitset_uint64 result(test_set.get_list_of_tests().size());
  std::mutex mut;

  std::vector<check_test_validity_task> tasks(
      test_set.get_list_of_tests().size());

  {
    auto exec_scope(exec_.create_execution_scope());
    unsigned int test_index = 0;
    for (const auto& t : test_set.get_list_of_tests()) {
      tasks[test_index] =
          check_test_validity_task(&t, test_index, &handler_, &result, &mut);
      exec_scope->spawn_execution(tasks[test_index]);
      ++test_index;
    }
  }

  return result;
}

bitset_uint64 concurrent_constraint_handler::get_valid_parameter_assignments(
    const test& t, unsigned int param_idx) const {

  return handler_.get_valid_parameter_assignments(t, param_idx);
}

std::vector<bitset_uint64>
concurrent_constraint_handler::get_valid_parameter_assignments(
    const internal_test_set& test_set, unsigned int param_idx) const {

  std::vector<bitset_uint64> result(test_set.get_list_of_tests().size());

  std::vector<get_valid_parameter_assignments_task> tasks(
      test_set.get_list_of_tests().size());

  {
    auto exec_scope(exec_.create_execution_scope());
    unsigned int test_index = 0;
    for (const auto& t : test_set.get_list_of_tests()) {
      tasks[test_index] = get_valid_parameter_assignments_task(
          &t, param_idx, test_index, &handler_, &result);
      exec_scope->spawn_execution(tasks[test_index]);
      ++test_index;
    }
  }

  return result;
}

void concurrent_constraint_handler::replace_dont_care_values(test& t) const {
  handler_.replace_dont_care_values(t);
}

void concurrent_constraint_handler::replace_dont_care_values(
    internal_test_set& test_set) const {

  std::vector<replace_dont_care_values_task> tasks(
      test_set.get_list_of_tests().size());

  {
    auto exec_scope(exec_.create_execution_scope());
    unsigned int test_index = 0;
    for (auto& t : test_set.get_list_of_tests()) {
      tasks[test_index] = replace_dont_care_values_task(&t, &handler_);
      exec_scope->spawn_execution(tasks[test_index]);
      ++test_index;
    }
  }
}

}  // namespace detail
}  // namespace citcpp
