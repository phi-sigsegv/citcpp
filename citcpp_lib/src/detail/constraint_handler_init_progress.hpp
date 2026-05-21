#ifndef CONSTRAINT_HANDLER_INIT_PROGRESS_HPP_
#define CONSTRAINT_HANDLER_INIT_PROGRESS_HPP_

#include <atomic>
#include <citcpp/covm_exec_handle.hpp>
#include <memory>
#include <thread>

#include "citcpp_covm.hpp"

namespace citcpp {
namespace detail {

class constraint_handler_init_progress {
  public:
    constraint_handler_init_progress()
        : constr_init_progress_tgt_(0), constr_init_progress_cur_(0) {}

    constraint_handler_init_progress(constraint_handler_init_progress&&) =
        delete;
    constraint_handler_init_progress(const constraint_handler_init_progress&) =
        delete;
    constraint_handler_init_progress& operator=(
        constraint_handler_init_progress&&) = delete;
    constraint_handler_init_progress& operator=(
        const constraint_handler_init_progress&) = delete;

    virtual ~constraint_handler_init_progress() {}

  public:
    unsigned int get_constraint_handler_init_progress_target() const {
      return constr_init_progress_tgt_;
    }

    unsigned int get_constraint_handler_init_progress_current() const {
      return constr_init_progress_cur_;
    }

    void set_constraint_handler_init_progress_target(
        unsigned int constr_init_progress_tgt) {
      constr_init_progress_tgt_ = constr_init_progress_tgt;
    }

    void set_constraint_handler_init_progress_current(
        unsigned int constr_init_progress_cur) {
      constr_init_progress_cur_ = constr_init_progress_cur;
    }

    void add_constraint_handler_init_progress_current(
        unsigned int constr_init_progress_cur) {
      constr_init_progress_cur_.fetch_add(constr_init_progress_cur,
                                          std::memory_order_acq_rel);
    }

  public:
    std::atomic_uint constr_init_progress_tgt_;
    std::atomic_uint constr_init_progress_cur_;
};

}  // namespace detail
}  // namespace citcpp

#endif /* CONSTRAINT_HANDLER_INIT_PROGRESS_HPP_ */
