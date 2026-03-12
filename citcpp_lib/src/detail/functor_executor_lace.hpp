#ifndef DETAIL_FUNCTOR_EXECUTOR_LACE_HPP_
#define DETAIL_FUNCTOR_EXECUTOR_LACE_HPP_

#include <vector>

#include "functor_executor.hpp"

namespace citcpp {
namespace detail {

class functor_execution_scope_lace : public functor_execution_scope {
  public:
    functor_execution_scope_lace() = default;
    functor_execution_scope_lace(const functor_execution_scope_lace&) = delete;
    functor_execution_scope_lace& operator=(
        const functor_execution_scope_lace&) = delete;

    ~functor_execution_scope_lace();

    void spawn_execution(function_ref<void()> functor_ref) override;

  private:
    std::vector<function_ref<void()>> functor_refs_;
};

/**
 * This defines an interface for a usually parallel execution
 * of a set of function objects.
 */
class functor_executor_lace : public functor_executor {
  public:
    functor_executor_lace(unsigned int n_workers);
    ~functor_executor_lace();

    unsigned int get_num_workers() const override;

    unsigned int get_worker_id() const override;

    std::unique_ptr<functor_execution_scope> create_execution_scope() override;

  private:
    unsigned int n_workers_;
};

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_FUNCTOR_EXECUTOR_LACE_HPP_ */
