#ifndef DETAIL_CITCPP_ALGO_COMMON_HPP_
#define DETAIL_CITCPP_ALGO_COMMON_HPP_

#include "functor_executor.hpp"
#include "internal_model.hpp"

namespace citcpp {
namespace detail {

/**
 * Returns the number of t-tuples to cover for n parameters
 * (indices [0, ... ,n-1]) from the given model and
 * interaction strength t.
 * Depending on the value of the parameter \a fixed_last_parameter, the last
 * parameter is fixed. Or in other words: We count tuples of length t-1
 * from the parameters [0, ... ,n-2], and extend those by always
 * appending a value from parameter n-1 to them.
 */
unsigned long long number_of_combinations_to_cover(
    unsigned int n, const internal_model& model,
    const std::vector<unsigned int>& parameter_index_map, unsigned int t,
    bool fixed_last_parameter);

/**
 * Returns the number of t-tuples to cover for n parameters
 * (indices [0, ... ,n-1]) from the given model and
 * interaction strength t.
 * Depending on the value of the parameter \a fixed_last_parameter, the last
 * parameter is fixed. Or in other words: We count tuples of length t-1
 * from the parameters [0, ... ,n-2], and extend those by always
 * appending a value from parameter n-1 to them.
 */
template <conc_is_void_functor_executor T_EXEC>
unsigned long long number_of_combinations_to_cover(
    unsigned int n, const internal_model& model,
    const std::vector<unsigned int>& parameter_index_map, unsigned int t,
    bool fixed_last_parameter, T_EXEC& exec);

struct number_of_combinations {
    unsigned long long num_combos_to_cover;
    unsigned long long num_covered_combos;
};

/**
 * Returns the number of t-tuples to cover for n parameters
 * (indices [0, ... ,n-1]) from the given model and
 * interaction strength t.
 * Depending on the value of the parameter \a fixed_last_parameter, the last
 * parameter is fixed. Or in other words: We count tuples of length t-1
 * from the parameters [0, ... ,n-2], and extend those by always
 * appending a value from parameter n-1 to them.
 * In addition, this method analyzes the given test set and checks how many
 * of the combinations are covered by it.
 */
number_of_combinations get_number_of_combinations(
    unsigned int n, const internal_model& model,
    const std::vector<unsigned int>& parameter_index_map, unsigned int t,
    bool fixed_last_parameter, const internal_test_set& test_set);

/**
 * Creates an index mapping for the parameters referenced by a given set of
 * relations.
 */
std::vector<unsigned int> create_parameter_index_map(
    const std::vector<internal_relation>& relations,
    const internal_model& internal_model);

/**
 * Returns a list of internal relations according to the given model
 * and the specified interaction strength. If that interaction
 * strength is < 1, then the relations from the given model are used
 * to derive internal relations from. Otherwise, the relation in the
 * given model are ignored, and a default internal relation is
 * constructed, which refers to all parameter of the given model and
 * the specified interaction strength.
 *
 * Note that superfluous relations are skipped. This is because a
 * relation r is pointless, if its parameters are all contained in
 * another relation r' and the interaction strength of r' is >= the
 * interaction strength of relation r. In such a case, coverage of
 * relation r' would always imply coverage of relation r, and
 * therefore we just have to keep relation r' as a relation that has
 * to be covered. Note that more possibilities exist for avoiding
 * overlaps between relations, but these would be more complex, which
 * is why this method only implements the optimization mentioned
 * above.
 */
std::vector<internal_relation> create_relations_for_cagen(
    const model& model, const internal_model& internal_model, int strength);

}  // namespace detail
}  // namespace citcpp

#include "citcpp_algo_common.tpp"

#endif /* DETAIL_CITCPP_ALGO_COMMON_HPP_ */
