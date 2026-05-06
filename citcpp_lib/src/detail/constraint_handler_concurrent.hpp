#ifndef DETAIL_CONSTRAINT_HANDLER_CONCURRENT_HPP_
#define DETAIL_CONSTRAINT_HANDLER_CONCURRENT_HPP_

#include <vector>

#include "constraint_handler.hpp"
#include "functor_executor.hpp"

namespace citcpp {
namespace detail {

/**
 * This is a wrapper of a constraint handler, whose purpose is
 * to provide an API that exploits the thread safety of the underlying
 * wrapper constraint handler.
 */
template <conc_is_void_functor_executor T_EXEC>
class concurrent_constraint_handler : public constraint_handler {
    typedef constraint_handler base_type;

  public:
    concurrent_constraint_handler(constraint_handler& handler, T_EXEC& exec);

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

  private:
    constraint_handler& handler_;
    T_EXEC& exec_;
};

}  // namespace detail
}  // namespace citcpp

#include "constraint_handler_concurrent.tpp"

#endif /* DETAIL_CONSTRAINT_HANDLER_CONCURRENT_HPP_ */
