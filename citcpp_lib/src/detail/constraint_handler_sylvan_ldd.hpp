#ifndef DETAIL_CONSTRAINT_HANDLER_SYLVAN_LDD_HPP_
#define DETAIL_CONSTRAINT_HANDLER_SYLVAN_LDD_HPP_

#include <unordered_map>

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
    void premark_valid_tuples(
        coverage_map_second_level& value_combinations) const override;

    /**
     * See constraint_handler interface.
     */
    bitset_uint64 check_validity_of_partial_tests(
        const internal_test_set& test_set) const override;

    /**
     * See constraint_handler interface.
     */
    bitset_uint64 get_valid_parameter_assignments(
        const test& t, unsigned int param_idx) const override;

    /**
     * See constraint_handler interface.
     */
    std::vector<bitset_uint64> get_valid_parameter_assignments(
        const internal_test_set& test_set,
        unsigned int param_idx) const override;

    /**
     * See constraint_handler interface.
     */
    void replace_dont_care_values(test& t) const override;

    /**
     * See constraint_handler interface.
     */
    void replace_dont_care_values(internal_test_set& test_set) const override;

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

    /**
     * Returns whether using an IDD per test is enabled.
     */
    bool is_per_test_idd_enabled() const;

    /**
     * Sets whether using an IDD per test is enabled.
     */
    void use_per_test_idd(bool enabled);

    /**
     * Returns the IDD used by the constraint handler.
     */
    const sylvan_idd& getIdd() const;

  private:
    const internal_model& model_;
    sylvan_idd idd_;
    std::unordered_map<const test*, sylvan_idd> test_to_idd_;
    bool is_per_test_idd_enabled_;
};

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_CONSTRAINT_HANDLER_SYLVAN_LDD_HPP_ */
