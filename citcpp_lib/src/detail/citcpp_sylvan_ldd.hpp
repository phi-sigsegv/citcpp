#ifndef DETAIL_CITCPP_SYLVAN_LDD_HPP_
#define DETAIL_CITCPP_SYLVAN_LDD_HPP_

#include <citcpp/constraints.hpp>
#include <cstdint>
#include <vector>

#include "bitset.hpp"
#include "coverage_bitset.hpp"
#include "datatypes_config.hpp"

namespace citcpp {
namespace detail {

class sylvan_idd {
  public:
    /**
     * The default constructor creates an IDD representing false.
     */
    sylvan_idd();

    /**
     * Constructs the IDD representing the var = value.
     */
    sylvan_idd(uint32_t variable, uint32_t value);

    /**
     * Constructs the IDD representing the assignments
     * of variables to the given values, i.e. a cube.
     * The indices of the given
     * vector identify a variable and the value at that
     * index is its value. Negative values are ignored
     * and are thus skipped in the cube.
     * If variable_order is given, it specifies the mapping from level
     * to variable index.
     */
    sylvan_idd(const std::vector<int>& assignments,
               const std::vector<unsigned int>* variable_order = nullptr);

    /**
     * Create an IDD, which represents the expression X OP value, where OP
     * is a relational operator. The method needs in addition the size of
     * the domain of the variable X.
     */
    sylvan_idd(uint32_t variable, relational_operator op, uint32_t value,
               uint32_t variable_domain_size);

    sylvan_idd(const sylvan_idd& other);
    sylvan_idd(sylvan_idd&& other);

    /**
     * Destroys this IDD and removes the reference held on it.
     */
    ~sylvan_idd();

    sylvan_idd& operator=(const sylvan_idd& other);
    sylvan_idd& operator=(sylvan_idd&& other);

    /**
     * Returns the IDD representing "true"
     */
    static sylvan_idd iddTrue();

    /**
     * Returns the IDD representing "false"
     */
    static sylvan_idd iddFalse();

    bool operator==(const sylvan_idd& other) const;
    bool operator!=(const sylvan_idd& other) const;

    /**
     * Returns the IDD obtained by following the down node.
     */
    sylvan_idd get_down_node() const;

    /**
     * Returns the IDD obtained by following the right node.
     */
    sylvan_idd get_right_node() const;

    /**
     * Reads the encoded interval of the top-level node of this IDD.
     */
    uint32_t get_encoded_interval() const;

    /**
     * Reads the interval of the top-level node of this IDD.
     */
    void get_interval(uint32_t& lb, uint32_t& ub) const;

    /**
     * Returns a vector defining the ordered variables of this IDD.
     */
    const std::vector<uint32_t>& get_variables() const;

    static sylvan_idd project_intersect(const sylvan_idd& lhs,
                                        const sylvan_idd& rhs);
    sylvan_idd& project_intersect(const sylvan_idd& other);

    static sylvan_idd project_union(
        const sylvan_idd& lhs, const sylvan_idd& rhs,
        const std::vector<unsigned int>& domain_sizes);
    sylvan_idd& project_union(const sylvan_idd& other,
                              const std::vector<unsigned int>& domain_sizes);

    /**
     * Projects this IDD onto the specified target variables (existential
     * quantification). The resulting IDD will only contain variables that are
     * both in this IDD and in target_variables.
     *
     * @param target_variables The variables to keep. Should be sorted.
     * @return A new IDD representing the projection.
     */
    sylvan_idd project(const std::vector<uint32_t>& target_variables) const;

    /**
     * Traverses this IDD in parallel and marks all satisfying assignments
     * in the given bitset as valid.
     *
     * @param value_combinations The data structure to mark valid tuples in.
     * @param domain_sizes The domain sizes of all parameters in the model.
     */
    void mark_valid_value_combinations(
        coverage_bitset& value_combinations, const param_vector& param_indices,
        const std::vector<unsigned int>& domain_sizes,
        const std::vector<unsigned int>* parameter_to_level = nullptr) const;

    /**
     * Return the number of nodes in this IDD.
     * WARNING: This is not thread-safe.
     */
    size_t node_count() const;

    /**
     * Returns the number of satisfying assignments.
     */
    long double sat_count() const;

    /**
     * Gets a full satisfying assignment, writing it to the given
     * vector. The vector is expected to be large enough such that
     * for each variable of this IDD a value can be written to it.
     */
    void get_sat_one(
        std::vector<int>& assignment,
        const std::vector<unsigned int>* variable_order = nullptr) const;

    /**
     * Writes this IDD to the specified file as a DOT representation.
     */
    void print_dot(const std::string& file_path) const;

    /**
     * Returns whether the given partial assignment is contained in this IDD.
     * Note that although the assignment maybe partial, the size of the given
     * vector specifying the assignment must be fully defined with respect to
     * the variables of this IDD. An unassigned/free variable shall be denoted
     * by a negative values at the corresponding index of the given
     * assignment specification.
     * If variable_order is given, it specifies the mapping from level
     * to variable index.
     */
    bool is_sat_with_partial_assignment(
        const std::vector<int>& partial_assignment,
        const std::vector<unsigned int>* variable_order = nullptr) const;

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
     *
     * If variable_order is given, it specifies the mapping from level
     * to variable index.
     */
    bitset_uint64 get_valid_variable_assignments(
        uint32_t variable, uint32_t domain_size,
        const std::vector<int>& partial_assignment,
        const std::vector<unsigned int>* variable_order = nullptr) const;

    /**
     * Gets a full satisfying assignment, writing it to the given
     * vector. The vector is expected to be large enough such that
     * for each variable of this IDD a value can be written to it.
     *
     * Compared to {@link #get_sat_one}, this method analyzes the
     * given vector for a partial assignment, such that the extracted
     * full assignment is guaranteed to be consistent with it.
     *
     * If variable_order is given, it specifies the mapping from level
     * to variable index.
     */
    void get_sat_one_under_partial_assignment(
        std::vector<int>& assignment,
        const std::vector<unsigned int>* variable_order = nullptr) const;

  private:
    sylvan_idd(uint64_t ldd, const std::vector<uint32_t>& variables);
    sylvan_idd(uint64_t ldd, std::vector<uint32_t>&& variables);

  private:
    uint64_t idd_;
    std::vector<uint32_t> variables_;
};

class sylvan {
  public:
    /**
     * Start Lace with <n_workers> workers and a a task deque size of <dqsize>
     * per worker. If <n_workers> is set to 0, automatically detects available
     * cores. If <dqsize> is est to 0, uses a reasonable default value.
     */
    static void init_lace(unsigned int n_workers, std::size_t dqsize);

    /**
     * Initializes the Sylvan framework, call this only once in your
     * program.
     *
     * @param initialTableSize the initial size of the nodes table. Must be a
     * power of two.
     * @param maxTableSize the maximum size of the nodes table. Must be a power
     * of two.
     * @param initialCacheSize the initial size of the operation cache. Must be
     * a power of two.
     * @param maxCacheSize the maximum size of the operation cache. Must be a
     * power of two.
     */
    static void init_package(std::size_t initialTableSize,
                             std::size_t maxTableSize,
                             std::size_t initialCacheSize,
                             std::size_t maxCacheSize);

    /**
     * Initializes the Sylvan framework, call this only once in your
     * program.
     * This function computes max_tablesize and max_cachesize to fit the memory
     * cap. The memory cap is in bytes.
     *
     * The parameter table_ratio controls the ratio between the nodes table and
     * the cache. For the value 0, both tables are of the same size. For values
     * 1, 2, 3 ... the nodes table will be 2x, 4x, 8x ... as big as the cache
     * For values -1, -2, -3 ... the cache will be 2x, 4x, 8x ... as big as the
     * nodes table
     *
     * The parameter initial_ratio controls how much smaller the initial table
     * sizes are. For values of 1, 2, 3, 4 the tables will initially be 2, 4, 8,
     * 16 times smaller.
     *
     * @param memory_cap the memory cap in bytes
     * @param table_ratio controls how much bigger the nodes table is compared
     * to the cache table
     * @param initial_ratio controls how much smaller the initial tables are
     * with respect to their maximum size limit
     */
    static void init_package(std::size_t memory_cap, int table_ratio,
                             int initial_ratio);

    /**
     * Initializes the MTBDD module of the Sylvan framework.
     */
    static void init_mtbdd();

    /**
     * Initializes the LDD module of the Sylvan framework.
     */
    static void init_ldd();

    /**
     * Stops all Lace workers.
     */
    static void quit_lace();

    /**
     * Frees all memory in use by Sylvan.
     * Warning: if you have any BDD or LDD objects which are not the trivial
     * terminal nodes after this, your program may crash!
     */
    static void quit_package();
};

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_CITCPP_SYLVAN_LDD_HPP_ */
