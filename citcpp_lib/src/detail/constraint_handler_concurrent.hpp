#ifndef DETAIL_CONSTRAINT_HANDLER_CONCURRENT_HPP_
#define DETAIL_CONSTRAINT_HANDLER_CONCURRENT_HPP_

#include <vector>

#include "constraint_handler.hpp"
#include "datatypes_config.hpp"

namespace citcpp {
namespace detail {

/**
 * This is a wrapper of a constraint handler, whose purpose is
 * to provide an API that exploits the thread safety of the underlying
 * wrapper constraint handler.
 */
class concurrent_constraint_handler : public constraint_handler {
    typedef constraint_handler base_type;

  public:
    concurrent_constraint_handler(const constraint_handler& handler,
                                  thread_pool& tp);

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

  private:
    const constraint_handler& handler_;
    thread_pool& tp_;
};

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_CONSTRAINT_HANDLER_CONCURRENT_HPP_ */
