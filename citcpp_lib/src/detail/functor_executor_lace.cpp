#include "functor_executor_lace.hpp"

#include <lace.h>

#include <atomic>

#include "lace_lifecycle.hpp"

namespace {

static int get_thread_id() {
  static std::atomic<int> counter{0};
  // This initialization happens once per thread
  thread_local int id = counter.fetch_add(1);

  return id;
}

VOID_TASK_1(lace_execute_functor, citcpp::function_ref<void()>*, functor) {
  (*functor)();
}

VOID_TASK_1(lace_execute_functors, std::vector<citcpp::function_ref<void()>>*,
            functors) {

  for (int i = 0; i < functors->size(); ++i) {
    SPAWN(lace_execute_functor, &(*functors)[i]);
  }

  for (int i = 0; i < functors->size(); ++i) {
    SYNC(lace_execute_functor);
  }
}

}  // namespace

namespace citcpp {
namespace detail {

functor_execution_scope_lace::~functor_execution_scope_lace() {
  RUN(lace_execute_functors, &functor_refs_);
}

void functor_execution_scope_lace::spawn_execution(
    function_ref<void()> functor_ref) {

  functor_refs_.push_back(std::move(functor_ref));
}

functor_executor_lace::functor_executor_lace(unsigned int n_workers)
    : functor_executor(), n_workers_(n_workers), workers_suspended_(false) {

  lace_init(n_workers, 0);
  suspend_workers();
}

functor_executor_lace::~functor_executor_lace() { lace_quit(); }

unsigned int functor_executor_lace::get_num_workers() const {
  return n_workers_;
}

unsigned int functor_executor_lace::get_worker_id() const {
  return get_thread_id();
}

void functor_executor_lace::suspend_workers() {
  if (!workers_suspended_) {
    lace_suspend();
    workers_suspended_ = true;
  }
}

std::unique_ptr<functor_execution_scope>
functor_executor_lace::create_execution_scope() {
  if (workers_suspended_) {
    workers_suspended_ = false;
    lace_resume();
  }

  return std::unique_ptr<functor_execution_scope>(
      new functor_execution_scope_lace());
}

}  // namespace detail
}  // namespace citcpp
