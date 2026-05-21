#ifndef DETAIL_FUNCTOR_EXECUTOR_THREAD_POOL_HPP_
#define DETAIL_FUNCTOR_EXECUTOR_THREAD_POOL_HPP_

#include <vector>

#include "datatypes_config.hpp"

namespace citcpp {
namespace detail {

class functor_execution_scope_thread_pool {
  public:
    functor_execution_scope_thread_pool(thread_pool& tp)
        : tg_(tp.createTaskGroup()) {}

    functor_execution_scope_thread_pool(
        const functor_execution_scope_thread_pool&) = delete;
    functor_execution_scope_thread_pool& operator=(
        const functor_execution_scope_thread_pool&) = delete;

    ~functor_execution_scope_thread_pool() {}

    template <class T_CALLABLE>
      requires std::derived_from<T_CALLABLE, functor_task_base<T_CALLABLE>>
    void spawn_execution(T_CALLABLE& task) {
      tg_.spawn(num_spawned_++, &task);
    }

    template <class T_CALLABLE>
      requires(!std::derived_from<T_CALLABLE, functor_task_base<T_CALLABLE>>)
    void spawn_execution(T_CALLABLE& callable) {
      tg_.spawnCallable(num_spawned_++, callable);
    }

    template <class T_CALLABLE, typename T_ALLOC>
      requires std::derived_from<T_CALLABLE, functor_task_base<T_CALLABLE>>
    void spawn_execution(std::vector<T_CALLABLE, T_ALLOC>& tasks) {
      for (int i = 0; i < tasks.size() - 1; ++i) {
        T_CALLABLE& task = tasks[i];
        tg_.spawn(num_spawned_++, &task);
      }

      T_CALLABLE& last_task = tasks[tasks.size() - 1];
      tg_.spawn_and_wait(&last_task);
    }

    template <class T_CALLABLE, typename T_ALLOC>
      requires(!std::derived_from<T_CALLABLE, functor_task_base<T_CALLABLE>>)
    void spawn_execution(std::vector<T_CALLABLE, T_ALLOC>& callables) {
      for (int i = 0; i < callables.size() - 1; ++i) {
        T_CALLABLE& callable = callables[i];
        tg_.spawnCallable(num_spawned_++, callable);
      }

      T_CALLABLE& last_callable = callables[callables.size() - 1];
      tg_.spawn_callable_and_wait(last_callable);
    }

  private:
    task_group tg_;
    int num_spawned_;
};

class functor_executor_thread_pool {
  public:
    functor_executor_thread_pool(thread_pool& tp) : tp_(tp) {}
    ~functor_executor_thread_pool() = default;

    unsigned int get_num_workers() const { return tp_.get_num_workers(); }

    unsigned int get_worker_id() const { return tp_.get_worker_id(); }

    void suspend_workers() { tp_.stop_workers(); }

    functor_execution_scope_thread_pool create_execution_scope() {
      return functor_execution_scope_thread_pool(tp_);
    }

  private:
    thread_pool& tp_;
};

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_FUNCTOR_EXECUTOR_THREAD_POOL_HPP_ */
