#ifndef DETAIL_PARAMETER_PREPROCESSOR_HPP_
#define DETAIL_PARAMETER_PREPROCESSOR_HPP_

#include <vector>

#include "internal_model.hpp"

namespace citcpp {
namespace detail {

/**
 * Computes a static variable ordering for the parameters of the given model
 * using the Max-Constraint Master-First (MCMF) heuristic.
 *
 * The heuristic aims to place highly constrained parameters earlier in the
 * ordering and to group parameters that interact within the same constraints.
 *
 * @param model The internal model containing parameters and constraints.
 * @return A vector where each element is the index of a parameter in the model,
 *         defining the suggested variable ordering from root to leaves.
 */
std::vector<unsigned int> compute_mcmf_variable_order(
    const internal_model& model);

/**
 * Computes a static variable ordering for the parameters of the given model
 * where parameters are ordered by decreasing domain size.
 *
 * @param model The internal model containing parameters and constraints.
 * @return A vector where each element is the index of a parameter in the model,
 *         defining the suggested variable ordering from root to leaves.
 */
std::vector<unsigned int> compute_deceasing_domain_size_variable_order(
    const internal_model& model);

/**
 * Compute a partition of parameter into disjoints sets, which are the connected
 * components in the sense that a parameter inside a partition either directly
 * or indirectly influences another parameter via some constraint. But across
 * the partitions, the parameters are pair-wise independent.
 *
 * Such a partitions can be exploited in a constraint handling implementation by
 * dividing constraints into disjoint sets, and creating multiple smaller
 * constraint solving problem instances, instead of working on a single
 * monolithic one.
 *
 * Note that the given order of parameters is respected in the sense that
 * the order of parameters inside a partition is consistent with it, and also
 * the list of returned partitions is ordered in the following sense: The
 * parameter of each partition that is minimal with respect to the order is used
 * to define an order of the list of partitions. A partition whose minimal
 * parameter is ordered before the minimal parameter of another partition also
 * comes first in the returned list.
 *
 * @param model The internal model containing parameters and constraints.
 * @param parameter_order The order of parameters
 * @return A vector where each element is the index of a parameter in the model,
 *         defining the suggested variable ordering from root to leaves.
 */
std::vector<std::vector<unsigned int>> compute_parameter_partitions(
    const internal_model& model,
    const std::vector<unsigned int>& parameter_order);

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_PARAMETER_PREPROCESSOR_HPP_ */
