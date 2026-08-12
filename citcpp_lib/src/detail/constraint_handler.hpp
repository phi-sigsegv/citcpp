#ifndef DETAIL_CONSTRAINT_HANDLER_HPP_
#define DETAIL_CONSTRAINT_HANDLER_HPP_

#include "bitset.hpp"
#include "constraint_handler_init_progress.hpp"
#include "coverage_bitset.hpp"
#include "datatypes_config.hpp"
#include "internal_test_set.hpp"

namespace citcpp {
namespace detail {

/**
 * This defines an interface for a constraint handler, as needed
 * by e.g. an IPOG algorithm.
 */
class constraint_handler {
  public:
    virtual ~constraint_handler() = default;

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
     * This method reads the given coverage map, and marks the bits representing
     * the validity of value combinations that are feasible according to
     * constraints.
     */
    virtual void mark_valid_tuples(coverage_bitset& value_combinations,
                                   const param_vector& param_indices) const = 0;

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
    virtual bitset_uint64 check_validity_of_partial_tests(
        const internal_test_set& test_set) const;

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
     * This method reads the given partial tests, in particular the
     * assignments of values to parameters, and returns for each such test,
     * a list of feasible assignments of values to the given parameter in terms
     * of a bitset. The bitset has bits enabled at indices corresponding to the
     * indices of values in the domain definition of the parameter.
     */
    virtual std::vector<bitset_uint64> get_valid_parameter_assignments(
        const internal_test_set& test_set, unsigned int param_idx) const;

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
    virtual void replace_dont_care_values(internal_test_set& test_set) const;

    /**
     * Returns the first test in the given list of tests, where the specified
     * assignment can be applied and the test is still valid with respect
     * to the constraints.
     */
    virtual test_list_intrusive_integ* get_first_test_valid_for_assignment(
        list_intrusive<test_list_intrusive_integ>& test_list,
        const param_vector& param_indices,
        const value_vector& value_indices) const = 0;

    /**
     * Calling this method causes the constraint handler to remember
     * the given test and its current assignments, such that if
     * the test is being referenced by other calls, then those calls can make
     * use of the cached information.
     *
     * Note that clients of this method are free to add more assignments
     * to a test after it has been cached. It is not necessary to call this
     * method again. But an assignment of a test given to this method
     * must not be changed afterwards.
     */
    virtual void cache_partial_test(const test* t) = 0;

    /**
     * Calling this method causes the constraint handler to update
     * its cache regarding the given test by adding its assignments,
     * such that if the test is being referenced by other calls,
     * then those calls can make use of the cached information.
     *
     * Note that clients of this method are free to add more assignments
     * to a test after it has been cached. It is not necessary to call this
     * method again. But an assignment of a test given to this method
     * must not be changed afterwards.
     *
     * Note that this is only well-defined if current assignments found
     * in the given test are consistent with the assignments at the point
     * when the cache was last updated (in the sense that some don't care values
     * are not replaced by concrete assigned values). Otherwise the behavior of
     * the constraint handler is undefined and may cause crashes or simply
     * deliver wrong results.
     */
    virtual void update_cached_partial_test(const test* t) = 0;

    /**
     * Calling this method causes the constraint handler to update
     * its cache regarding the given test by adding the given assignment
     * to it, such that if the test is being referenced by other calls,
     * then those calls can make use of the cached information.
     *
     * Note that clients of this method are free to add more assignments
     * to a test after it has been cached. It is not necessary to call this
     * method again. But an assignment of a test given to this method
     * must not be changed afterwards.
     *
     * Note that this is only well-defined if the parameter was previously
     * unassigned (a don't care value) and now has a concrete assigned
     * value. Otherwise the behavior of the constraint handler is undefined
     * and may cause crashes or simply deliver wrong results.
     */
    virtual void update_cached_partial_test(const test* t,
                                            unsigned int param_idx,
                                            int value) = 0;

    /**
     * Creates a constraint handler for the given model.
     */
    static std::unique_ptr<constraint_handler> create_constraint_handler(
        const internal_model& model, unsigned int num_worker_threads,
        std::size_t memory_limit_in_bytes,
        constraint_handler_init_progress& exec_handle);
};

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_CONSTRAINT_HANDLER_HPP_ */
