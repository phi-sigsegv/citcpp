#ifndef DETAIL_CONSTRAINT_HANDLER_SYLVAN_LDD_HPP_
#define DETAIL_CONSTRAINT_HANDLER_SYLVAN_LDD_HPP_

#include "citcpp_sylvan_ldd.hpp"
#include "constraint_handler.hpp"
#include "internal_model.hpp"

namespace citcpp {
namespace detail {

class constraint_handler_sylvan_base : public constraint_handler {
    typedef constraint_handler base_type;

  public:
    constraint_handler_sylvan_base(int num_workers);
    virtual ~constraint_handler_sylvan_base();
};

/**
 * This implements the constraint handler interface, using list
 * decision diagrams (LDDs).
 */
class constraint_handler_sylvan_ldd : public constraint_handler_sylvan_base {
    typedef constraint_handler_sylvan_base base_type;

  public:
    constraint_handler_sylvan_ldd(const internal_model& model, int num_workers);

    constraint_handler_sylvan_ldd(
        const internal_model& model, int num_workers,
        constraint_handler_init_progress& exec_handle);

    /**
     * See constraint_handler interface.
     */
    bool is_thread_safe() const override;

    /**
     * See constraint_handler interface.
     */
    bool is_valid_partial_test(const test& t) const override;

    /**
     * See constraint_handler interface.
     */
    bitset_uint64 get_valid_parameter_assignments(
        const test& t, unsigned int param_idx) const override;

    /**
     * See constraint_handler interface.
     */
    void replace_dont_care_values(test& t) const override;

  private:
    const internal_model& model_;
    sylvan_ldd ldd_;
};

/**
 * This implements the constraint handler interface, using interval
 * decision diagrams (IDDs).
 */
class constraint_handler_sylvan_idd : public constraint_handler_sylvan_base {
    typedef constraint_handler_sylvan_base base_type;

  public:
    constraint_handler_sylvan_idd(const internal_model& model, int num_workers);

    constraint_handler_sylvan_idd(
        const internal_model& model, int num_workers,
        constraint_handler_init_progress& exec_handle);

    /**
     * See constraint_handler interface.
     */
    bool is_thread_safe() const override;

    /**
     * See constraint_handler interface.
     */
    bool is_valid_partial_test(const test& t) const override;

    /**
     * See constraint_handler interface.
     */
    bitset_uint64 get_valid_parameter_assignments(
        const test& t, unsigned int param_idx) const override;

    /**
     * See constraint_handler interface.
     */
    void replace_dont_care_values(test& t) const override;

  private:
    const internal_model& model_;
    sylvan_idd idd_;
};

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_CONSTRAINT_HANDLER_SYLVAN_LDD_HPP_ */
