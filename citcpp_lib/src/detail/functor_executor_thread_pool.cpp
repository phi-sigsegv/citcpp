#include "functor_executor_thread_pool.hpp"

namespace citcpp {
namespace detail {

functor_execution_scope_thread_pool::functor_execution_scope_thread_pool(
    thread_pool* tp)
    : functor_execution_scope(), tg_(tp->createTaskGroup()), num_spawned_(0) {}

functor_execution_scope_thread_pool::~functor_execution_scope_thread_pool() {
  // Nothing special to do, as the destructor of task_group
  // will already do ths synchronization.
}

void functor_execution_scope_thread_pool::spawn_execution(
    function_ref<void()> functor_ref) {

  tg_.spawnCallable(num_spawned_++, functor_ref);
}

functor_executor_thread_pool::functor_executor_thread_pool(thread_pool& tp)
    : functor_executor(), tp_(tp) {}

unsigned int functor_executor_thread_pool::get_num_workers() const {
  return tp_.get_num_workers();
}

unsigned int functor_executor_thread_pool::get_worker_id() const {
  return tp_.get_worker_id();
}

std::unique_ptr<functor_execution_scope>
functor_executor_thread_pool::create_execution_scope() {
  return std::unique_ptr<functor_execution_scope>(
      new functor_execution_scope_thread_pool(&tp_));
}

}  // namespace detail
}  // namespace citcpp
