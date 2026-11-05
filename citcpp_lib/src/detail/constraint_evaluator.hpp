#ifndef DETAIL_CONSTRAINT_EVALUATOR_HPP_
#define DETAIL_CONSTRAINT_EVALUATOR_HPP_

#include <citcpp/constraints.hpp>
#include <citcpp/model.hpp>
#include <cstdint>
#include <vector>

#include "internal_test_set.hpp"

namespace citcpp {
namespace detail {

/**
 * This class provides means to evaluate satisfaction of the constraints of a
 * model by a given (complete) test. The constraints are compiled once from
 * the model: each proposition becomes a table over the value indices of its
 * parameter, so an evaluation needs neither lookups of parameters nor
 * comparisons of values. A proposition about a parameter that is not in the
 * model is false. Read only after construction, so it may be used by several
 * threads.
 */
class constraint_evaluator {
  public:
    explicit constraint_evaluator(const model& model);

    /**
     * Evaluates whether the given test fulfills all constraints of the model.
     */
    bool operator()(const test& test) const;

    enum class node_kind : std::uint8_t { CONSTANT, ATOM, NOT_OR, AND, OR };

    struct node {
        node_kind kind;
        // CONSTANT: the value; ATOM: the parameter index; NOT_OR, AND, OR:
        // the index of the first child in children_.
        std::uint32_t a;
        // ATOM: the offset of the table in tables_; NOT_OR, AND, OR: the
        // number of children.
        std::uint32_t b;
        // ATOM: the number of values of the parameter.
        std::uint32_t size;
    };

  private:
    bool eval(std::uint32_t node_idx, const test& test) const;

    // The nodes of all constraints, the children of the inner nodes, the
    // tables of the atoms (per value index of the parameter: whether the
    // proposition holds), and the root node of each constraint.
    std::vector<node> nodes_;
    std::vector<std::uint32_t> children_;
    std::vector<char> tables_;
    std::vector<std::uint32_t> roots_;
};

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_CONSTRAINT_EVALUATOR_HPP_ */
