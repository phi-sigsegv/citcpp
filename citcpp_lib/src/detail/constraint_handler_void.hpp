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
     * See constraint_handler interface.
     */
    void mark_valid_tuples(coverage_bitset& value_combinations,
                           const param_vector& param_indices) const override;

    /**
     * See constraint_handler interface.
     */
    bitset_uint64 get_valid_parameter_assignments(
        const test& t, unsigned int param_idx) const override;

    /**
     * See constraint_handler interface.
     */
    void replace_dont_care_values(test& t) const override;

    /**
     * See constraint_handler interface.
     */
    void cache_partial_test(const test* t) override;

    /**
     * See constraint_handler interface.
     */
    void update_cached_partial_test(const test* t) override;

    /**
     * See constraint_handler interface.
     */
    void update_cached_partial_test(const test* t, unsigned int param_idx,
                                    int value) override;

  private:
    const internal_model& model_;
};

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_CONSTRAINT_HANDLER_VOID_HPP_ */
