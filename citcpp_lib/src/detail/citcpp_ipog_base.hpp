#ifndef DETAIL_CITCPP_IPOG_BASE_HPP_
#define DETAIL_CITCPP_IPOG_BASE_HPP_

#include <memory>
#include <vector>

#include "constraint_handler.hpp"
#include "internal_model.hpp"

namespace citcpp {
namespace detail {

// Forward declaration of cagen_exec_handle_ipog_impl due to usage of
// citcpp_ipog_base by cagen_exec_handle_ipog_impl definition.
class cagen_exec_handle_ipog_impl;

/**
 * This class provides an implementation of the IPOG algorithm.
 */
class citcpp_ipog_base {
  public:
    virtual ~citcpp_ipog_base() {}

    /**
     * This is the entry point to be called by a thread.
     */
    virtual void entry_point(cagen_exec_handle_ipog_impl& exec_handle) = 0;

    /**
     * Creates an index mapping for the parameters of a given system model.
     * The order of parameters indentified by the created index mapping
     * is consistent with the order of parameters in the relations created
     * by the other method provided by this class.
     */
    static std::vector<unsigned int> create_parameter_index_map(
        const internal_model& internal_model);

    /**
     * Creates an index mapping for the parameters referenced by a given set of
     * relations.
     */
    static std::vector<unsigned int> create_parameter_index_map(
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
    static std::vector<internal_relation> create_relations(
        const model& model, const internal_model& internal_model, int strength);

    /**
     * Returns the length of the common prefix in terms of parameter
     * indices referenced by the given pair of parameter index mapping and
     * relation.
     */
    static unsigned int length_of_common_param_prefix(
        const citcpp::detail::internal_relation& rel,
        const std::vector<unsigned int>& parameter_index_map);

    /**
     * Creates a constraint handler for the given model.
     */
    static std::unique_ptr<constraint_handler> create_constraint_handler(
        const internal_model& model);
};

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_CITCPP_IPOG_BASE_HPP_ */