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
    concurrent_constraint_handler(const constraint_handler& handler);

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

    /**
     * This method reads the given partial tests, in particular the
     * assignments of values to parameters, and returns for each such test,
     * whether the respective combination of those assignments is valid.
     * This method is similar to the method #is_valid_partial_test,
     * but instead of evaluating the validity of a single given test,
     * a whole set of tests is checked in parallel. The returned bitset has
     * bits enabled at indices corresponding to the indices of test
     * in the given test set. A bit is enabled, if the correponding test
     * is valid.
     */
    bitset_uint64 check_validity_of_partial_tests(
        const internal_test_set& test_set, thread_pool& tp) const;

    /**
     * This method reads the given partial tests, in particular the
     * assignments of values to parameters, and returns for each such test,
     * a list of feasible assignments of values to the given parameter in terms
     * of a bitset. The bitset has bits enabled at indices corresponding to the
     * indices of values in the domain definition of the parameter.
     */
    std::vector<bitset_uint64> get_valid_parameter_assignments(
        const internal_test_set& test_set, unsigned int param_idx,
        thread_pool& tp) const;

    /**
     * This method reads the given tests, in particular the parameters
     * with don't care values, and replaces all of them by concrete
     * values such that the resulting tests are all valid.
     */
    void replace_dont_care_values(internal_test_set& test_set,
                                  thread_pool& tp) const;

  private:
    const constraint_handler& handler_;
};

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_CONSTRAINT_HANDLER_CONCURRENT_HPP_ */
