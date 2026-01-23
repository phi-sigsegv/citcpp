#ifndef DETAIL_CITCPP_SYLVAN_LDD_HPP_
#define DETAIL_CITCPP_SYLVAN_LDD_HPP_

#include <citcpp/constraints.hpp>
#include <cstdint>
#include <vector>

#include "bitset.hpp"

namespace citcpp {
namespace detail {

class sylvan_ldd {
  public:
    /**
     * The default constructor create an LDD representing false.
     */
    sylvan_ldd();

    /**
     * Constructs the LDD representing the var = value.
     */
    sylvan_ldd(uint32_t variable, uint32_t value);

    /**
     * Constructs the LDD representing the assignments
     * of variables to the given values, i.e. a cube.
     * The indices of the given
     * vector identify a variable and the value at that
     * index is its value. Negative values are ignored
     * and are thus skipped in the cube.
     */
    sylvan_ldd(const std::vector<int>& assignments);

    /**
     * Create an LDD, which represents the expression X OP value, where OP
     * is a relational operator. The method needs in addition the size of
     * the domain of the variable X.
     */
    sylvan_ldd(uint32_t variable, relational_operator op, uint32_t value,
               uint32_t variable_domain_size);

    sylvan_ldd(const sylvan_ldd& other);
    sylvan_ldd(sylvan_ldd&& other);

    /**
     * Destroys this LDD and removes the reference held on it.
     */
    ~sylvan_ldd();

    sylvan_ldd& operator=(const sylvan_ldd& other);
    sylvan_ldd& operator=(sylvan_ldd&& other);

    /**
     * Returns the LDD representing "true"
     */
    static sylvan_ldd lddTrue();

    /**
     * Returns the LDD representing "false"
     */
    static sylvan_ldd lddFalse();

    bool operator==(const sylvan_ldd& other) const;
    bool operator!=(const sylvan_ldd& other) const;

    /**
     * Returns the value of the top-level node of this LDD.
     */
    uint32_t get_value() const;

    friend sylvan_ldd operator*(const sylvan_ldd& lhs, const sylvan_ldd& rhs);
    sylvan_ldd& operator*=(const sylvan_ldd& other);

    friend sylvan_ldd operator+(const sylvan_ldd& lhs, const sylvan_ldd& rhs);
    sylvan_ldd& operator+=(const sylvan_ldd& other);

    /**
     * Return the number of nodes in this LDD.
     * WARNING: This is not thread-safe.
     */
    size_t node_count() const;

    /**
     * Returns whether the given partial assignment is contained in this LDD.
     * Note that although the assignment maybe partial, the size of the given
     * vector specifying the assignment must be fully defined with respect to
     * the variables of this LDD. An unassigned/free variable shall be denoted
     * by a negative values at the corresponding index of the given
     * assignment specification.
     */
    bool is_sat_with_partial_assignment(
        const std::vector<int>& partial_assignment) const;

    /**
     * Returns a bitset that represents which values of the specified variable
     * are valid, in the sense of the existence of a full assignment to all
     * variables. This collection of valid values of a variable can be
     * optionally limited to paths that are consistent with the specified
     * partial assignment with respect to other variables. It is also possible
     * to pass in an empty partial assignment, in which case the search for
     * valid values of the specified variable in done in a global manner.
     *
     * The returned bitset has bits enabled at indices corresponding to the
     * indices of values in the domain definition of the variable.
     */
    bitset_uint64 get_valid_variable_assignments(
        uint32_t variable, uint32_t domain_size,
        const std::vector<int>& partial_assignment) const;

    /**
     * Gets a full satisfying assignment, writing it to the given
     * vector. The vector is expected to be large enough such that
     * for each variable of this LDD a value can be written to it.
     */
    void get_sat_one(std::vector<int>& assignment) const;

  private:
    sylvan_ldd(uint64_t ldd, const std::vector<uint32_t>& variables);
    sylvan_ldd(uint64_t ldd, std::vector<uint32_t>&& variables);

  private:
    uint64_t ldd_;
    std::vector<uint32_t> variables_;
};

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_CITCPP_SYLVAN_LDD_HPP_ */
