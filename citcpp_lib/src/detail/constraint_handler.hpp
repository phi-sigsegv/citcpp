#ifndef DETAIL_CONSTRAINT_HANDLER_HPP_
#define DETAIL_CONSTRAINT_HANDLER_HPP_

#include "bitset.hpp"
#include "internal_test_set.hpp"

namespace citcpp {
namespace detail {

/**
 * This defines an interface for a constraint handler, as needed
 * by e.g. an IPOG algorithm.
 */
class constraint_handler {
  public:
    /**
     * Returns whether this constraint handler is thread safe. This
     * method shall return true, if and only if:
     * 1. Each individual method can be called concurrently by different
     * threads.
     * 2. Different methods mentioned by this interface can be called
     * concurrently by different threads.
     */
    virtual bool is_thread_safe() const = 0;

    /**
     * This method reads the given partial test, in particular the
     * assignments of values to parameters, and returns whether the
     * combination of those assignments is valid.
     */
    virtual bool is_valid_partial_test(const test& t) const = 0;

    /**
     * This method reads the given partial test, in particular the
     * assignments of values to parameters, and returns a list of feasible
     * assignments of values to the given parameter in terms of a bitset.
     * The bitset has bits enabled at indices corresponding to the indices
     * of values in the domain definition of the parameter.
     */
    virtual bitset_uint64 get_valid_parameter_assignments(
        const test& t, unsigned int param_idx) const = 0;

    /**
     * This method reads the given test, in particular the parameters
     * with don't care values, and replaces all of them by concrete
     * values such that the resulting test is valid.
     */
    virtual void replace_dont_care_values(test& t) const = 0;

    /**
     * This method reads the given tests, in particular the parameters
     * with don't care values, and replaces all of them by concrete
     * values such that the resulting tests are all valid.
     */
    void replace_dont_care_values(internal_test_set& test_set) const;
};

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_CONSTRAINT_HANDLER_HPP_ */
