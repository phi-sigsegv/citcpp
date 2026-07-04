#include "model_simplifier.hpp"

namespace {

/**
 * This visitor resolves implications, in the sense of
 * replacing A => B by NOT(A) || B.
 */
class implication_resolving_visitor {
  public:
    implication_resolving_visitor() : negate_(false) {}

    std::shared_ptr<citcpp::constraint> operator()(
        const citcpp::boolean_literal& lit) {

      const bool lit_value = lit;
      return std::make_shared<citcpp::boolean_literal>(negate_ ? !lit_value
                                                               : lit_value);
    }

    std::shared_ptr<citcpp::constraint> operator()(
        const citcpp::boolean_proposition& prop) {

      if (!negate_) {
        return std::make_shared<citcpp::boolean_proposition>(
            prop.get_parameter(), prop.get_operator(),
            prop.get_compared_value());
      }

      std::shared_ptr<citcpp::constraint> negated_prop;

      switch (prop.get_operator()) {
        case citcpp::relational_operator::EQ:
          negated_prop = std::make_shared<citcpp::boolean_proposition>(
              prop.get_parameter(), citcpp::relational_operator::NEQ,
              prop.get_compared_value());
          break;
        case citcpp::relational_operator::NEQ:
          negated_prop = std::make_shared<citcpp::boolean_proposition>(
              prop.get_parameter(), citcpp::relational_operator::EQ,
              prop.get_compared_value());
          break;
        case citcpp::relational_operator::GE:
          negated_prop = std::make_shared<citcpp::boolean_proposition>(
              prop.get_parameter(), citcpp::relational_operator::LT,
              prop.get_compared_value());
          break;
        case citcpp::relational_operator::GT:
          negated_prop = std::make_shared<citcpp::boolean_proposition>(
              prop.get_parameter(), citcpp::relational_operator::LE,
              prop.get_compared_value());
          break;
        case citcpp::relational_operator::LE:
          negated_prop = std::make_shared<citcpp::boolean_proposition>(
              prop.get_parameter(), citcpp::relational_operator::GT,
              prop.get_compared_value());
          break;
        case citcpp::relational_operator::LT:
          negated_prop = std::make_shared<citcpp::boolean_proposition>(
              prop.get_parameter(), citcpp::relational_operator::GE,
              prop.get_compared_value());
          break;
      }

      return negated_prop;
    }

    std::shared_ptr<citcpp::constraint> operator()(
        const citcpp::enum_proposition& prop) {

      if (!negate_) {
        return std::make_shared<citcpp::enum_proposition>(
            prop.get_parameter(), prop.get_operator(),
            prop.get_compared_value());
      }

      std::shared_ptr<citcpp::constraint> negated_prop;

      switch (prop.get_operator()) {
        case citcpp::relational_operator::EQ:
          negated_prop = std::make_shared<citcpp::enum_proposition>(
              prop.get_parameter(), citcpp::relational_operator::NEQ,
              prop.get_compared_value());
          break;
        case citcpp::relational_operator::NEQ:
          negated_prop = std::make_shared<citcpp::enum_proposition>(
              prop.get_parameter(), citcpp::relational_operator::EQ,
              prop.get_compared_value());
          break;
        case citcpp::relational_operator::GE:
          negated_prop = std::make_shared<citcpp::enum_proposition>(
              prop.get_parameter(), citcpp::relational_operator::LT,
              prop.get_compared_value());
          break;
        case citcpp::relational_operator::GT:
          negated_prop = std::make_shared<citcpp::enum_proposition>(
              prop.get_parameter(), citcpp::relational_operator::LE,
              prop.get_compared_value());
          break;
        case citcpp::relational_operator::LE:
          negated_prop = std::make_shared<citcpp::enum_proposition>(
              prop.get_parameter(), citcpp::relational_operator::GT,
              prop.get_compared_value());
          break;
        case citcpp::relational_operator::LT:
          negated_prop = std::make_shared<citcpp::enum_proposition>(
              prop.get_parameter(), citcpp::relational_operator::GE,
              prop.get_compared_value());
          break;
      }

      return negated_prop;
    }

    std::shared_ptr<citcpp::constraint> operator()(
        const citcpp::int_proposition& prop) {

      if (!negate_) {
        return std::make_shared<citcpp::int_proposition>(
            prop.get_parameter(), prop.get_operator(),
            prop.get_compared_value());
      }

      std::shared_ptr<citcpp::constraint> negated_prop;

      switch (prop.get_operator()) {
        case citcpp::relational_operator::EQ:
          negated_prop = std::make_shared<citcpp::int_proposition>(
              prop.get_parameter(), citcpp::relational_operator::NEQ,
              prop.get_compared_value());
          break;
        case citcpp::relational_operator::NEQ:
          negated_prop = std::make_shared<citcpp::int_proposition>(
              prop.get_parameter(), citcpp::relational_operator::EQ,
              prop.get_compared_value());
          break;
        case citcpp::relational_operator::GE:
          negated_prop = std::make_shared<citcpp::int_proposition>(
              prop.get_parameter(), citcpp::relational_operator::LT,
              prop.get_compared_value());
          break;
        case citcpp::relational_operator::GT:
          negated_prop = std::make_shared<citcpp::int_proposition>(
              prop.get_parameter(), citcpp::relational_operator::LE,
              prop.get_compared_value());
          break;
        case citcpp::relational_operator::LE:
          negated_prop = std::make_shared<citcpp::int_proposition>(
              prop.get_parameter(), citcpp::relational_operator::GT,
              prop.get_compared_value());
          break;
        case citcpp::relational_operator::LT:
          negated_prop = std::make_shared<citcpp::int_proposition>(
              prop.get_parameter(), citcpp::relational_operator::GE,
              prop.get_compared_value());
          break;
      }

      return negated_prop;
    }

    std::shared_ptr<citcpp::constraint> operator()(
        const citcpp::implication& impl) {

      const bool negated_impl = negate_;
      std::vector<std::shared_ptr<citcpp::constraint>> operands;
      negate_ = !negate_;
      operands.push_back(
          impl.get_left_operand()->accept<std::shared_ptr<citcpp::constraint>>(
              *this));
      negate_ = !negate_;

      operands.push_back(
          impl.get_right_operand()->accept<std::shared_ptr<citcpp::constraint>>(
              *this));

      if (negated_impl) {
        return std::make_shared<citcpp::and_expression>(std::move(operands));
      }

      return std::make_shared<citcpp::or_expression>(std::move(operands));
    }

    std::shared_ptr<citcpp::constraint> operator()(
        const citcpp::and_expression& and_expr) {

      std::vector<std::shared_ptr<citcpp::constraint>> operands;
      for (const auto& operand : and_expr.get_operands()) {
        operands.push_back(
            operand->accept<std::shared_ptr<citcpp::constraint>>(*this));
      }

      if (negate_) {
        return std::make_shared<citcpp::or_expression>(std::move(operands));
      }

      return std::make_shared<citcpp::and_expression>(std::move(operands));
    }

    std::shared_ptr<citcpp::constraint> operator()(
        const citcpp::or_expression& or_expr) {

      std::vector<std::shared_ptr<citcpp::constraint>> operands;
      for (const auto& operand : or_expr.get_operands()) {
        operands.push_back(
            operand->accept<std::shared_ptr<citcpp::constraint>>(*this));
      }

      if (negate_) {
        return std::make_shared<citcpp::and_expression>(std::move(operands));
      }

      return std::make_shared<citcpp::or_expression>(std::move(operands));
    }

  private:
    bool negate_;
};

/**
 * This visitor flattens nested nary expressions of the same type, i.e.
 * it turn an expression like (A && B && (C && D && (E || F)) && G)
 * into (A && B && C && D && (E || F) && G).
 */
class constraint_flattening_visitor {
  public:
    constraint_flattening_visitor() : last_nary_expr_(nullptr) {}

    citcpp::constraint_type operator()(const citcpp::boolean_literal& lit) {
      return citcpp::constraint_type::LITERAL;
    }

    citcpp::constraint_type operator()(
        const citcpp::boolean_proposition& prop) {
      return citcpp::constraint_type::PROP_BOOLEAN;
    }

    citcpp::constraint_type operator()(const citcpp::enum_proposition& prop) {
      return citcpp::constraint_type::PROP_ENUM;
    }

    citcpp::constraint_type operator()(const citcpp::int_proposition& prop) {
      return citcpp::constraint_type::PROP_INT;
    }

    citcpp::constraint_type operator()(const citcpp::implication& impl) {
      impl.get_left_operand()->accept<citcpp::constraint_type>(*this);
      impl.get_right_operand()->accept<citcpp::constraint_type>(*this);

      return citcpp::constraint_type::IMPLICATION;
    }

    citcpp::constraint_type operator()(citcpp::and_expression& and_expr) {
      for (int i = 0; i < and_expr.get_operands().size(); ++i) {
        // We create a new shared_ptr pointing at the operand, so that replacing
        // the object which the shared_ptr at and_expr.get_operands()[i]
        // points to does not immediately deletes the operand.
        std::shared_ptr<citcpp::constraint> operand =
            and_expr.get_operands()[i];
        citcpp::constraint_type operand_type =
            operand->accept<citcpp::constraint_type>(*this);

        if (operand_type == citcpp::constraint_type::AND_EXPR) {
          // Remove the sub-expression of the same type, and replace it
          // by the operands of the sub-expression.
          citcpp::nary_operation& sub_expr = *last_nary_expr_;

          // We replace the object that the shared_ptr at position
          // i points to by the first operand of the sub-expression.
          and_expr.get_operands()[i] = sub_expr.get_operands()[0];

          // Since we already moved by the first operand of the sub-expression,
          // we begin to copy the operands of the sub-expression starting
          // from index 1.
          and_expr.get_operands().insert(
              std::next(and_expr.get_operands().begin(), i + 1),
              std::next(sub_expr.get_operands().begin(), 1),
              sub_expr.get_operands().end());

          // Since we moved the first operand of the sub-expression
          // to index i of the current expression and inserted
          // the remaining operands of the sub-expression after
          // position i, we have to increment i by the number
          // of the operands of the sub-expression - 1, in order
          // for the increment of i in the outer loop to point to
          // the next sub-expression.
          i += (sub_expr.get_operands().size() - 1);
        }
      }

      last_nary_expr_ = &and_expr;

      return citcpp::constraint_type::AND_EXPR;
    }

    citcpp::constraint_type operator()(citcpp::or_expression& or_expr) {
      for (int i = 0; i < or_expr.get_operands().size(); ++i) {
        // We create a new shared_ptr pointing at the operand, so that replacing
        // the object which the shared_ptr at or_expr.get_operands()[i]
        // points to does not immediately deletes the operand.
        std::shared_ptr<citcpp::constraint> operand = or_expr.get_operands()[i];
        citcpp::constraint_type operand_type =
            operand->accept<citcpp::constraint_type>(*this);

        if (operand_type == citcpp::constraint_type::OR_EXPR) {
          // Remove the sub-expression of the same type, and replace it
          // by the operands of the sub-expression.
          citcpp::nary_operation& sub_expr = *last_nary_expr_;

          // We replace the object that the shared_ptr at position
          // i points to by the first operand of the sub-expression.
          or_expr.get_operands()[i] = sub_expr.get_operands()[0];

          // Since we already moved by the first operand of the sub-expression,
          // we begin to copy the operands of the sub-expression starting
          // from index 1.
          or_expr.get_operands().insert(
              std::next(or_expr.get_operands().begin(), i + 1),
              std::next(sub_expr.get_operands().begin(), 1),
              sub_expr.get_operands().end());

          // Since we moved the first operand of the sub-expression
          // to index i of the current expression and inserted
          // the remaining operands of the sub-expression after
          // position i, we have to increment i by the number
          // of the operands of the sub-expression - 1, in order
          // for the increment of i in the outer loop to point to
          // the next sub-expression.
          i += (sub_expr.get_operands().size() - 1);
        }
      }

      last_nary_expr_ = &or_expr;

      return citcpp::constraint_type::OR_EXPR;
    }

    citcpp::nary_operation* get_last_nary_expression() const {
      return last_nary_expr_;
    }

  private:
    citcpp::nary_operation* last_nary_expr_;
};

}  // namespace

namespace citcpp {
namespace detail {

model& simplify_model(model& m) {
  implication_resolving_visitor impl_resolver;
  for (auto& constr : m.get_constraints()) {
    constr = constr->accept<std::shared_ptr<citcpp::constraint>>(impl_resolver);
  }

  constraint_flattening_visitor flattening_visitor;
  for (int i = 0; i < m.get_constraints().size(); ++i) {
    // We create a new shared_ptr pointing at the constraint, so that replacing
    // the object which the shared_ptr at m.get_constraints()[i]
    // points to does not immediately deletes the constraint.
    std::shared_ptr<citcpp::constraint> constr = m.get_constraints()[i];
    constraint_type constr_type =
        constr->accept<constraint_type>(flattening_visitor);

    if (constr_type == constraint_type::AND_EXPR) {
      // Remove the and-expression, and replace it by the operands
      // of the and-expression, turning them into top-level
      // constraints.
      citcpp::nary_operation& and_expr =
          *flattening_visitor.get_last_nary_expression();

      // We replace the object that the shared_ptr at position
      // i points to by the first operand of the and-expression.
      m.get_constraints()[i] = and_expr.get_operands()[0];

      // Since we already moved by the first operand of the and-expression,
      // we begin to copy the operands of the and-expression starting
      // from index 1.
      m.get_constraints().insert(std::next(m.get_constraints().begin(), i + 1),
                                 std::next(and_expr.get_operands().begin(), 1),
                                 and_expr.get_operands().end());

      // Since we moved the first operand of the and-expression
      // to index i of the list of constraint and inserted
      // the remaining operands of the and-expression after
      // position i, we have to increment i by the number
      // of the operands of the and-expression - 1, in order
      // for the increment of i in the outer loop to point to
      // the next constraint.
      i += (and_expr.get_operands().size() - 1);
    }
  }

  return m;
}

}  // namespace detail
}  // namespace citcpp
