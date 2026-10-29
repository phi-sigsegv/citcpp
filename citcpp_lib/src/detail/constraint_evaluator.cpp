#include "constraint_evaluator.hpp"

#include <unordered_map>

namespace {

// Compiles a constraint into the nodes of a constraint_evaluator; returns the
// index of its root node.
class constraint_compile_visitor {
  public:
    using node = citcpp::detail::constraint_evaluator::node;
    using node_kind = citcpp::detail::constraint_evaluator::node_kind;

    constraint_compile_visitor(
        const std::unordered_map<citcpp::parameter_reference, int,
                                 citcpp::parameter_reference_hash>&
            param_to_index,
        const citcpp::model& model, std::vector<node>& nodes,
        std::vector<std::uint32_t>& children, std::vector<char>& tables)
        : param_to_index_(param_to_index),
          model_(model),
          nodes_(nodes),
          children_(children),
          tables_(tables) {}

    std::uint32_t operator()(const citcpp::boolean_literal& lit) const {
      return add({node_kind::CONSTANT, static_cast<bool>(lit) ? 1u : 0u, 0, 0});
    }

    std::uint32_t operator()(const citcpp::boolean_proposition& prop) const {
      using namespace citcpp;
      return atom(prop.get_parameter(), [&](const parameter_value& v) {
        bool assigned_value = v;
        return prop.get_operator() == relational_operator::EQ
                   ? assigned_value == prop.get_compared_value()
                   : assigned_value != prop.get_compared_value();
      });
    }

    std::uint32_t operator()(const citcpp::enum_proposition& prop) const {
      using namespace citcpp;
      return atom(prop.get_parameter(), [&](const parameter_value& v) {
        const std::string& assigned_value = v;
        return prop.get_operator() == relational_operator::EQ
                   ? assigned_value == prop.get_compared_value()
                   : assigned_value != prop.get_compared_value();
      });
    }

    std::uint32_t operator()(const citcpp::int_proposition& prop) const {
      using namespace citcpp;
      return atom(prop.get_parameter(), [&](const parameter_value& v) {
        int assigned_value = v;
        switch (prop.get_operator()) {
          case relational_operator::EQ:
            return assigned_value == prop.get_compared_value();
          case relational_operator::LE:
            return assigned_value <= prop.get_compared_value();
          case relational_operator::LT:
            return assigned_value < prop.get_compared_value();
          case relational_operator::GE:
            return assigned_value >= prop.get_compared_value();
          case relational_operator::GT:
            return assigned_value > prop.get_compared_value();
          default:
            return assigned_value != prop.get_compared_value();
        }
      });
    }

    // a => b as (not a) or b.
    std::uint32_t operator()(const citcpp::implication& impl) const {
      const std::uint32_t l =
          impl.get_left_operand()->accept<std::uint32_t>(*this);
      const std::uint32_t r =
          impl.get_right_operand()->accept<std::uint32_t>(*this);
      return add_inner(node_kind::NOT_OR, {l, r});
    }

    std::uint32_t operator()(const citcpp::and_expression& and_expr) const {
      return add_inner(node_kind::AND, compile_all(and_expr.get_operands()));
    }

    std::uint32_t operator()(const citcpp::or_expression& or_expr) const {
      return add_inner(node_kind::OR, compile_all(or_expr.get_operands()));
    }

  private:
    // A proposition about a parameter: its table over the parameter's values.
    template <typename Pred>
    std::uint32_t atom(const citcpp::parameter_reference& param,
                       Pred pred) const {
      auto it = param_to_index_.find(param);
      if (it == param_to_index_.end()) {
        return add({node_kind::CONSTANT, 0, 0, 0});
      }
      const auto& values = model_.get_parameters()[it->second].get_values();
      const auto offset = static_cast<std::uint32_t>(tables_.size());
      for (const auto& v : values) {
        tables_.push_back(pred(v) ? 1 : 0);
      }
      return add({node_kind::ATOM, static_cast<std::uint32_t>(it->second),
                  offset, static_cast<std::uint32_t>(values.size())});
    }

    std::vector<std::uint32_t> compile_all(
        const std::vector<std::shared_ptr<citcpp::constraint>>& operands)
        const {
      std::vector<std::uint32_t> res;
      for (const auto& operand : operands) {
        res.push_back(operand->accept<std::uint32_t>(*this));
      }
      return res;
    }

    std::uint32_t add_inner(node_kind kind,
                            const std::vector<std::uint32_t>& kids) const {
      const auto first = static_cast<std::uint32_t>(children_.size());
      children_.insert(children_.end(), kids.begin(), kids.end());
      return add({kind, first, static_cast<std::uint32_t>(kids.size()), 0});
    }

    std::uint32_t add(const node& n) const {
      nodes_.push_back(n);
      return static_cast<std::uint32_t>(nodes_.size() - 1);
    }

    const std::unordered_map<citcpp::parameter_reference, int,
                             citcpp::parameter_reference_hash>& param_to_index_;
    const citcpp::model& model_;
    std::vector<node>& nodes_;
    std::vector<std::uint32_t>& children_;
    std::vector<char>& tables_;
};

}  // namespace

namespace citcpp {
namespace detail {

constraint_evaluator::constraint_evaluator(const model& model) {
  std::unordered_map<parameter_reference, int, parameter_reference_hash>
      param_to_index;
  int idx = 0;
  for (const auto& param : model.get_parameters()) {
    param_to_index.emplace(param, idx);
    ++idx;
  }
  constraint_compile_visitor visitor(param_to_index, model, nodes_, children_,
                                     tables_);
  for (const auto& constr : model.get_constraints()) {
    roots_.push_back(constr->accept<std::uint32_t>(visitor));
  }
}

bool constraint_evaluator::eval(std::uint32_t node_idx,
                                const test& test) const {
  const node& n = nodes_[node_idx];
  switch (n.kind) {
    case node_kind::CONSTANT:
      return n.a != 0;
    case node_kind::ATOM: {
      const int v = test.get_values()[n.a];
      return v >= 0 && static_cast<std::uint32_t>(v) < n.size &&
             tables_[n.b + static_cast<std::uint32_t>(v)] != 0;
    }
    case node_kind::NOT_OR:
      return !eval(children_[n.a], test) || eval(children_[n.a + 1], test);
    case node_kind::AND:
      // Short circuit: if anything is false, the entire expression is false.
      for (std::uint32_t i = 0; i < n.b; ++i) {
        if (!eval(children_[n.a + i], test)) {
          return false;
        }
      }
      return true;
    default:
      // Short circuit: if anything is true, the entire expression is true.
      for (std::uint32_t i = 0; i < n.b; ++i) {
        if (eval(children_[n.a + i], test)) {
          return true;
        }
      }
      return false;
  }
}

bool constraint_evaluator::operator()(const test& test) const {
  for (const std::uint32_t root : roots_) {
    if (!eval(root, test)) {
      return false;
    }
  }
  return true;
}

}  // namespace detail
}  // namespace citcpp
