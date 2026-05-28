#ifndef DETAIL_IDD_VARIABLE_ORDERING_HPP_
#define DETAIL_IDD_VARIABLE_ORDERING_HPP_

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

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_IDD_VARIABLE_ORDERING_HPP_ */
