#include <mutex>

#include "constraint_handler_concurrent.hpp"
#include "shared_constants.hpp"

namespace citcpp {
namespace detail {

class alignas(false_sharing_avoidance_alignment) check_test_validity_task
    : public functor_task_base<check_test_validity_task> {

  private:
    typedef functor_task_base<check_test_validity_task> base_type;

  public:
    check_test_validity_task() = default;

    check_test_validity_task(const test* test, unsigned int test_index,
                             const constraint_handler* handler,
                             bitset_uint64* result, std::mutex* mut)
        : base_type(),
          test_(test),
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
    const test* test_;
    unsigned int test_index_;
    const constraint_handler* handler_;
    bitset_uint64* result_;
    std::mutex* mut_;
};

class alignas(false_sharing_avoidance_alignment)
    get_valid_parameter_assignments_task
    : public functor_task_base<get_valid_parameter_assignments_task> {

  private:
    typedef functor_task_base<get_valid_parameter_assignments_task> base_type;

  public:
    get_valid_parameter_assignments_task() = default;

    get_valid_parameter_assignments_task(const test* test,
                                         unsigned int param_idx,
                                         unsigned int test_index,
                                         const constraint_handler* handler,
                                         std::vector<bitset_uint64>* results)
        : base_type(),
          test_(test),
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
    const test* test_;
    unsigned int param_idx_;
    unsigned int test_index_;
    const constraint_handler* handler_;
    std::vector<bitset_uint64>* results_;
};

class alignas(false_sharing_avoidance_alignment) replace_dont_care_values_task
    : public functor_task_base<replace_dont_care_values_task> {

  private:
    typedef functor_task_base<replace_dont_care_values_task> base_type;

  public:
    replace_dont_care_values_task() = default;

    replace_dont_care_values_task(test* test, const constraint_handler* handler)
        : base_type(), test_(test), handler_(handler) {}

    virtual ~replace_dont_care_values_task() {}

    void operator()() { handler_->replace_dont_care_values(*test_); }

  private:
    test* test_;
    const constraint_handler* handler_;
};

template <conc_is_void_functor_executor T_EXEC>
concurrent_constraint_handler<T_EXEC>::concurrent_constraint_handler(
    constraint_handler& handler, T_EXEC& exec)
    : base_type(), handler_(handler), exec_(exec) {}

template <conc_is_void_functor_executor T_EXEC>
bool concurrent_constraint_handler<T_EXEC>::is_thread_safe() const {
  return true;
}

template <conc_is_void_functor_executor T_EXEC>
bool concurrent_constraint_handler<T_EXEC>::is_valid_partial_test(
    const test& t) const {

  return handler_.is_valid_partial_test(t);
}

template <conc_is_void_functor_executor T_EXEC>
void concurrent_constraint_handler<T_EXEC>::premark_valid_tuples(
    coverage_map_second_level& value_combinations) const {
  handler_.premark_valid_tuples(value_combinations);
}

template <conc_is_void_functor_executor T_EXEC>
bitset_uint64
concurrent_constraint_handler<T_EXEC>::check_validity_of_partial_tests(
    const internal_test_set& test_set) const {

  bitset_uint64 result(test_set.get_list_of_tests().size());
  std::mutex mut;

  thread_local_vector<check_test_validity_task> tasks(
      test_set.get_list_of_tests().size());

  {
    auto exec_scope(exec_.create_execution_scope());
    unsigned int test_index = 0;
    for (const auto& t : test_set.get_list_of_tests()) {
      tasks[test_index] =
          check_test_validity_task(&t, test_index, &handler_, &result, &mut);
      exec_scope.spawn_execution(tasks[test_index]);
      ++test_index;
    }
  }

  return result;
}

template <conc_is_void_functor_executor T_EXEC>
bitset_uint64
concurrent_constraint_handler<T_EXEC>::get_valid_parameter_assignments(
    const test& t, unsigned int param_idx) const {

  return handler_.get_valid_parameter_assignments(t, param_idx);
}

template <conc_is_void_functor_executor T_EXEC>
std::vector<bitset_uint64>
concurrent_constraint_handler<T_EXEC>::get_valid_parameter_assignments(
    const internal_test_set& test_set, unsigned int param_idx) const {

  std::vector<bitset_uint64> result(test_set.get_list_of_tests().size());

  thread_local_vector<get_valid_parameter_assignments_task> tasks(
      test_set.get_list_of_tests().size());

  {
    auto exec_scope(exec_.create_execution_scope());
    unsigned int test_index = 0;
    for (const auto& t : test_set.get_list_of_tests()) {
      tasks[test_index] = get_valid_parameter_assignments_task(
          &t, param_idx, test_index, &handler_, &result);
      exec_scope.spawn_execution(tasks[test_index]);
      ++test_index;
    }
  }

  return result;
}

template <conc_is_void_functor_executor T_EXEC>
void concurrent_constraint_handler<T_EXEC>::replace_dont_care_values(
    test& t) const {
  handler_.replace_dont_care_values(t);
}

template <conc_is_void_functor_executor T_EXEC>
void concurrent_constraint_handler<T_EXEC>::replace_dont_care_values(
    internal_test_set& test_set) const {

  thread_local_vector<replace_dont_care_values_task> tasks(
      test_set.get_list_of_tests().size());

  {
    auto exec_scope(exec_.create_execution_scope());
    unsigned int test_index = 0;
    for (auto& t : test_set.get_list_of_tests()) {
      tasks[test_index] = replace_dont_care_values_task(&t, &handler_);
      exec_scope.spawn_execution(tasks[test_index]);
      ++test_index;
    }
  }
}

template <conc_is_void_functor_executor T_EXEC>
void concurrent_constraint_handler<T_EXEC>::cache_partial_test(const test* t) {
  handler_.cache_partial_test(t);
}

template <conc_is_void_functor_executor T_EXEC>
void concurrent_constraint_handler<T_EXEC>::update_cached_partial_test(
    const test* t) {

  handler_.update_cached_partial_test(t);
}

template <conc_is_void_functor_executor T_EXEC>
void concurrent_constraint_handler<T_EXEC>::update_cached_partial_test(
    const test* t, unsigned int param_idx, int value) {

  handler_.update_cached_partial_test(t, param_idx, value);
}

}  // namespace detail
}  // namespace citcpp
