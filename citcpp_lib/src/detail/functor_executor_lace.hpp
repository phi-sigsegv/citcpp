#ifndef DETAIL_FUNCTOR_EXECUTOR_LACE_HPP_
#define DETAIL_FUNCTOR_EXECUTOR_LACE_HPP_

#include <citcpp/function_ref.hpp>
#include <memory>
#include <vector>

namespace citcpp {
namespace detail {

class functor_execution_scope_lace {
  public:
    functor_execution_scope_lace() = default;

    functor_execution_scope_lace(const functor_execution_scope_lace&) = delete;
    functor_execution_scope_lace(functor_execution_scope_lace&&) = default;

    ~functor_execution_scope_lace();

    functor_execution_scope_lace& operator=(
        const functor_execution_scope_lace&) = delete;
    functor_execution_scope_lace& operator=(functor_execution_scope_lace&&) =
        default;

    void spawn_execution(function_ref<void()> functor_ref);

    template <class T_CALLABLE, typename T_ALLOC>
    void spawn_execution(std::vector<T_CALLABLE, T_ALLOC>& callables) {
      for (std::size_t i = 0; i < callables.size(); ++i) {
        spawn_execution(callables[i]);
      }
    }

  private:
    std::vector<function_ref<void()>> functor_refs_;
};

/**
 * This defines an interface for a usually parallel execution
 * of a set of function objects.
 */
class functor_executor_lace {
  public:
    functor_executor_lace(unsigned int n_workers);

    functor_executor_lace(const functor_executor_lace&) = delete;
    functor_executor_lace(functor_executor_lace&&) = delete;

    ~functor_executor_lace();

    functor_executor_lace& operator=(const functor_executor_lace&) = delete;
    functor_executor_lace& operator=(functor_executor_lace&&) = delete;

    unsigned int get_num_workers() const;

    std::size_t get_worker_id() const;

    void suspend_workers();

    functor_execution_scope_lace create_execution_scope();

  private:
    unsigned int n_workers_;
};

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_FUNCTOR_EXECUTOR_LACE_HPP_ */
