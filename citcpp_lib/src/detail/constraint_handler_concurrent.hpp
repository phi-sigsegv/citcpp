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
     * Returns whether this constraint handler is thread safe. This
     * method shall return true, if and only if:
     * 1. Each individual method can be called concurrently by different
     * threads.
     * 2. Different methods mentioned by this interface can be called
     * concurrently by different threads.
     */
    bool is_thread_safe() const;

    /**
     * This method reads the given partial test, in particular the
     * assignments of values to parameters, and returns whether the
     * combination of those assignments is valid.
     */
    bool is_valid_partial_test(const test& t) const;

    /**
     * This method reads the given partial test, in particular the
     * assignments of values to parameters, and returns a list of feasible
     * assignments of values to the given parameter in terms of a bitset.
     * The bitset has bits enabled at indices corresponding to the indices
     * of values in the domain definition of the parameter.
     */
    bitset_uint64 get_valid_parameter_assignments(const test& t,
                                                  unsigned int param_idx) const;

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

  private:
    const constraint_handler& handler_;
};

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_CONSTRAINT_HANDLER_CONCURRENT_HPP_ */
