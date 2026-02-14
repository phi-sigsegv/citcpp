#ifndef DETAIL_CONSTRAINT_HANDLER_SYLVAN_LDD_HPP_
#define DETAIL_CONSTRAINT_HANDLER_SYLVAN_LDD_HPP_

#include "cagen_exec_handle_base.hpp"
#include "citcpp_sylvan_ldd.hpp"
#include "constraint_handler.hpp"
#include "internal_model.hpp"

namespace citcpp {
namespace detail {

/**
 * This implements the constraint handler interface, specializing for the
 * case where no constraints exist at all.
 */
class constraint_handler_sylvan_ldd : public constraint_handler {
    typedef constraint_handler base_type;

  private:
    class number_of_instances_tracker {
      public:
        number_of_instances_tracker(int num_workers);
        ~number_of_instances_tracker();
    };

  public:
    constraint_handler_sylvan_ldd(const internal_model& model, int num_workers);

    constraint_handler_sylvan_ldd(const internal_model& model, int num_workers,
                                  cagen_exec_handle_base& exec_handle);

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
    const number_of_instances_tracker instances_tracker_;
    const internal_model& model_;
    sylvan_ldd ldd_;
};

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_CONSTRAINT_HANDLER_SYLVAN_LDD_HPP_ */
