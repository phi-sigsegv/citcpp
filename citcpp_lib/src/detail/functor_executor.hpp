#ifndef DETAIL_FUNCTOR_EXECUTOR_HPP_
#define DETAIL_FUNCTOR_EXECUTOR_HPP_

#include <concepts>
#include <vector>

namespace citcpp {
namespace detail {

struct mock_is_no_arg_functor {
    void operator()() {}
};

/**
 * This defines a concept for a scope of a usually parallel
 * execution of a set of function objects.
 * It allows a set of functions objects executions to be
 * spawned, as well as to synchronize on their completion.
 */
template <typename T>
concept conc_has_spawn_method_taking_functor =
    requires(T t, mock_is_no_arg_functor& task,
             std::vector<mock_is_no_arg_functor>& tasks) {
      { t.spawn_execution(task) };
      { t.spawn_execution(tasks) };
    };

/**
 * This defines a concept for a usually parallel execution
 * of a set of function objects.
 */
template <typename T>
concept conc_is_void_functor_executor = requires(const T t) {
  {
    const_cast<T&>(t).create_execution_scope()
  } -> conc_has_spawn_method_taking_functor;

  { t.get_num_workers() } -> std::integral;

  { t.get_worker_id() } -> std::integral;

  { const_cast<T&>(t).suspend_workers() } -> std::same_as<void>;
};

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_FUNCTOR_EXECUTOR_HPP_ */
