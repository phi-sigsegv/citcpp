#ifndef DETAIL_FUNCTOR_EXECUTOR_THREAD_POOL_HPP_
#define DETAIL_FUNCTOR_EXECUTOR_THREAD_POOL_HPP_

#include "datatypes_config.hpp"
#include "functor_executor.hpp"

namespace citcpp {
namespace detail {

class functor_execution_scope_thread_pool : public functor_execution_scope {
  public:
    functor_execution_scope_thread_pool(thread_pool* tp);

    functor_execution_scope_thread_pool(
        const functor_execution_scope_thread_pool&) = delete;
    functor_execution_scope_thread_pool& operator=(
        const functor_execution_scope_thread_pool&) = delete;

    ~functor_execution_scope_thread_pool();

    void spawn_execution(function_ref<void()> functor_ref) override;

  private:
    task_group tg_;
    int num_spawned_;
};

/**
 * This defines an interface for a usually parallel execution
 * of a set of function objects.
 */
class functor_executor_thread_pool : public functor_executor {
  public:
    functor_executor_thread_pool(thread_pool& tp);
    ~functor_executor_thread_pool() = default;

    unsigned int get_num_workers() const override;

    unsigned int get_worker_id() const override;

    void suspend_workers() override;

    std::unique_ptr<functor_execution_scope> create_execution_scope() override;

  private:
    thread_pool& tp_;
};

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_FUNCTOR_EXECUTOR_THREAD_POOL_HPP_ */
