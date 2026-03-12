#ifndef DETAIL_FUNCTOR_EXECUTOR_HPP_
#define DETAIL_FUNCTOR_EXECUTOR_HPP_

#include <citcpp/function_ref.hpp>
#include <memory>

namespace citcpp {
namespace detail {

/**
 * This defines an interface for a scope of a usually parallel
 * execution of a set of function objects.
 * It allows a set of functions objects executions to be
 * spawned, as well as to synchronize on their completion.
 */
class functor_execution_scope {
  protected:
    functor_execution_scope() = default;

  public:
    functor_execution_scope(const functor_execution_scope&) = delete;
    functor_execution_scope& operator=(const functor_execution_scope&) = delete;

    /**
     * The destructor of actual implementations of this interface is assumed
     * to implement a synchronization logic, that waits for all executions
     * spawned through this scope to terminate.
     *
     * NOTE: It is left open to the implementation of this interface, whether
     * the asynchronous functor executions already start at the point
     * when the method spawn_execution is called, or whether the functors
     * will be executed when this scope is about to be destructed.
     */
    virtual ~functor_execution_scope() = default;

    /**
     * Spawns the execution of a given function object. The function object
     * is given by reference. The caller must ensure that the lifetime
     * of the function object extends to the point in time after
     * this scope is destroyed.
     *
     * IMPORTANT: You must guarantee that this method is NOT called from
     * different threads, and you must guarantee that the thread calling this
     * method is the same one that also destroys this object (thereby
     * synchronizing with the asynchronouly executed functors).
     *
     * NOTE: It is left open to the implementation of this interface, whether
     * the asynchronous functor executions already start at the point
     * when the method spawn_execution is called, or whether the functors
     * will be executed when this scope is about to be destructed.
     */
    virtual void spawn_execution(function_ref<void()> functor_ref) = 0;
};

/**
 * This defines an interface for a usually parallel execution
 * of a set of function objects.
 */
class functor_executor {
  public:
    virtual ~functor_executor() = default;

    /**
     * Returns the number of workers of this executor
     *
     * @return the number of workers of this executor
     */
    virtual unsigned int get_num_workers() const = 0;

    /**
     * Returns the ID of the calling worker thread.
     * Each thread of a thread pool is assigned a unique ID.
     * These IDs form a contiguous range starting from 0.
     * This allows using them as indexes of an array.
     *
     * @return the ID of the calling worker thread
     */
    virtual unsigned int get_worker_id() const = 0;

    /**
     * Suspends all worker threads.
     */
    virtual void suspend_workers() = 0;

    /**
     * Returns a new scope for usually parallelized execution
     * of a set of function objects.
     * The caller synchronized with those executions at the time
     * this scope is destroyed.
     */
    virtual std::unique_ptr<functor_execution_scope>
    create_execution_scope() = 0;
};

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_FUNCTOR_EXECUTOR_HPP_ */
