#ifndef DETAIL_CONSTRAINT_HANDLER_VOID_HPP_
#define DETAIL_CONSTRAINT_HANDLER_VOID_HPP_

#include "constraint_handler.hpp"
#include "internal_model.hpp"

namespace citcpp {
namespace detail {

/**
 * This implements the constraint handler interface, specializing for the
 * case where no constraints exist at all.
 */
class constraint_handler_void : public constraint_handler {
    typedef constraint_handler base_type;

  public:
    constraint_handler_void(const internal_model& model);

    /**
     * See constraint_handler interface.
     */
    bool is_thread_safe() const override;

    /**
     * See constraint_handler interface.
     */
    bool is_valid_partial_test(const test& t) const override;

    /**
     * This method reads the given partial test, in particular the assignments
     * of values to parameters, and returns a list of feasible assignments
     * of values to the given parameter in terms of a bitset.
     * The bitset has bits enabled at indices corresponding to the indices
     * of values in the domain definition of the parameter.
     */
    bitset_uint64 get_valid_parameter_assignments(
        const test& t, unsigned int param_idx) const override;

  private:
    const internal_model& model_;
};

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_CONSTRAINT_HANDLER_VOID_HPP_ */
