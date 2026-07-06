#include "model_simplifier.hpp"

#include <unordered_map>
#include <unordered_set>
#include <utility>

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

class simplification_visitor {
  public:
    using value_set = std::unordered_set<citcpp::parameter_value,
                                         citcpp::parameter_value_hash>;
    using param_to_valid_values_map =
        std::unordered_map<citcpp::parameter_reference, value_set,
                           citcpp::parameter_reference_hash>;
    using return_type = std::pair<std::shared_ptr<citcpp::constraint>,
                                  param_to_valid_values_map>;

    simplification_visitor(citcpp::model& m)
        : m_(m),
          param_to_index_(create_param_to_index_map(m)),
          did_simplify_(false) {}

    return_type operator()(const citcpp::boolean_literal& lit) {
      return std::make_pair(std::make_shared<citcpp::boolean_literal>(lit),
                            param_to_valid_values_map());
    }

    return_type operator()(const citcpp::boolean_proposition& prop) {
      const auto& param =
          m_.get_parameters()[param_to_index_.find(prop.get_parameter())
                                  ->second];
      param_to_valid_values_map valid_value_map;
      value_set& valid_values = valid_value_map[prop.get_parameter()];

      switch (prop.get_operator()) {
        case citcpp::relational_operator::EQ:
          for (const auto& value : param.get_values()) {
            bool value_as_bool = value;
            if (value_as_bool == prop.get_compared_value()) {
              valid_values.insert(value);
            }
          }
          break;
        case citcpp::relational_operator::NEQ:
          for (const auto& value : param.get_values()) {
            bool value_as_bool = value;
            if (value_as_bool != prop.get_compared_value()) {
              valid_values.insert(value);
            }
          }
          break;
        default:
          break;
      }

      if (valid_values.empty()) {
        // No valid value at all. So this proposition is a contradiction.
        did_simplify_ = true;
        return std::make_pair(std::make_shared<citcpp::boolean_literal>(false),
                              param_to_valid_values_map());
      }

      if (valid_values.size() == param.get_values().size()) {
        // All values from the parameter's domain are allowed.
        // So this proposition is a tautology.
        did_simplify_ = true;
        return std::make_pair(std::make_shared<citcpp::boolean_literal>(true),
                              param_to_valid_values_map());
      }

      return std::make_pair(std::make_shared<citcpp::boolean_proposition>(
                                prop.get_parameter(), prop.get_operator(),
                                prop.get_compared_value()),
                            std::move(valid_value_map));
    }

    return_type operator()(const citcpp::enum_proposition& prop) {
      const auto& param =
          m_.get_parameters()[param_to_index_.find(prop.get_parameter())
                                  ->second];
      param_to_valid_values_map valid_value_map;
      value_set& valid_values = valid_value_map[prop.get_parameter()];

      bool required_value = false;
      switch (prop.get_operator()) {
        case citcpp::relational_operator::EQ:
          for (const auto& value : param.get_values()) {
            const std::string& value_as_string = value;
            if (value_as_string == prop.get_compared_value()) {
              valid_values.insert(value);
            }
          }
          break;
        case citcpp::relational_operator::NEQ:
          for (const auto& value : param.get_values()) {
            const std::string& value_as_string = value;
            if (value_as_string != prop.get_compared_value()) {
              valid_values.insert(value);
            }
          }
          break;
        default:
          break;
      }

      if (valid_values.empty()) {
        // No valid value at all. So this proposition is a contradiction.
        did_simplify_ = true;
        return std::make_pair(std::make_shared<citcpp::boolean_literal>(false),
                              param_to_valid_values_map());
      }

      if (valid_values.size() == param.get_values().size()) {
        // All values from the parameter's domain are allowed.
        // So this proposition is a tautology.
        did_simplify_ = true;
        return std::make_pair(std::make_shared<citcpp::boolean_literal>(true),
                              param_to_valid_values_map());
      }

      return std::make_pair(std::make_shared<citcpp::enum_proposition>(
                                prop.get_parameter(), prop.get_operator(),
                                prop.get_compared_value()),
                            std::move(valid_value_map));
    }

    return_type operator()(const citcpp::int_proposition& prop) {
      const auto& param =
          m_.get_parameters()[param_to_index_.find(prop.get_parameter())
                                  ->second];
      param_to_valid_values_map valid_value_map;
      value_set& valid_values = valid_value_map[prop.get_parameter()];

      bool required_value = false;
      switch (prop.get_operator()) {
        case citcpp::relational_operator::EQ:
          for (const auto& value : param.get_values()) {
            int value_as_int = value;
            if (value_as_int == prop.get_compared_value()) {
              valid_values.insert(value);
            }
          }
          break;
        case citcpp::relational_operator::NEQ:
          for (const auto& value : param.get_values()) {
            int value_as_int = value;
            if (value_as_int != prop.get_compared_value()) {
              valid_values.insert(value);
            }
          }
          break;
        case citcpp::relational_operator::LT:
          for (const auto& value : param.get_values()) {
            int value_as_int = value;
            if (value_as_int < prop.get_compared_value()) {
              valid_values.insert(value);
            }
          }
          break;
        case citcpp::relational_operator::LE:
          for (const auto& value : param.get_values()) {
            int value_as_int = value;
            if (value_as_int <= prop.get_compared_value()) {
              valid_values.insert(value);
            }
          }
          break;
        case citcpp::relational_operator::GE:
          for (const auto& value : param.get_values()) {
            int value_as_int = value;
            if (value_as_int >= prop.get_compared_value()) {
              valid_values.insert(value);
            }
          }
          break;
        case citcpp::relational_operator::GT:
          for (const auto& value : param.get_values()) {
            int value_as_int = value;
            if (value_as_int > prop.get_compared_value()) {
              valid_values.insert(value);
            }
          }
          break;
      }

      if (valid_values.empty()) {
        // No valid value at all. So this proposition is a contradiction.
        did_simplify_ = true;
        return std::make_pair(std::make_shared<citcpp::boolean_literal>(false),
                              param_to_valid_values_map());
      }

      if (valid_values.size() == param.get_values().size()) {
        // All values from the parameter's domain are allowed.
        // So this proposition is a tautology.
        did_simplify_ = true;
        return std::make_pair(std::make_shared<citcpp::boolean_literal>(true),
                              param_to_valid_values_map());
      }

      return std::make_pair(std::make_shared<citcpp::int_proposition>(
                                prop.get_parameter(), prop.get_operator(),
                                prop.get_compared_value()),
                            std::move(valid_value_map));
    }

    return_type operator()(const citcpp::and_expression& and_expr) {
      param_to_valid_values_map valid_value_map;
      bool is_first_non_trivial = true;
      std::vector<std::shared_ptr<citcpp::constraint>> operands;
      for (int i = 0; i < and_expr.get_operands().size(); ++i) {
        const auto& operand = and_expr.get_operands()[i];
        return_type op_res(operand->accept<return_type>(*this));
        if (op_res.first->get_constraint_type() ==
            citcpp::constraint_type::LITERAL) {
          const citcpp::boolean_literal* lit =
              static_cast<citcpp::boolean_literal*>(op_res.first.get());
          if (!lit->get_value()) {
            // If we have at least one false literal in an and-expression,
            // then the whole and-expression can be simplified to false.
            return std::make_pair(
                std::make_shared<citcpp::boolean_literal>(false),
                param_to_valid_values_map());
          } else {
            // We intentionally do nothing in this case, as a true literal
            // in an and-expression can be skipped.
          }
        } else {
          operands.push_back(op_res.first);
          // Merge valid values.
          if (is_first_non_trivial) {
            valid_value_map = op_res.second;
            is_first_non_trivial = false;
          } else {
            param_to_valid_values_map& operand_valid_value_map = op_res.second;
            for (auto& operand_valid_values : operand_valid_value_map) {
              auto it = valid_value_map.find(operand_valid_values.first);
              if (it != valid_value_map.end()) {
                // Build the intersection of values, if parameter is contained
                // in both maps.
                std::erase_if(
                    it->second, [&operand_valid_values](
                                    const citcpp::parameter_value& value) {
                      return !operand_valid_values.second.contains(value);
                    });
                if (it->second.empty()) {
                  // No valid value at all for some parameter. So this
                  // and-expression is a contradiction.
                  did_simplify_ = true;
                  return std::make_pair(
                      std::make_shared<citcpp::boolean_literal>(false),
                      param_to_valid_values_map());
                }
              } else {
                // If parameter is only known in one of the maps, then it is
                // unconstrained in one of the operands of the and-expression,
                // but since all operands must be true, the value restrictions
                // carry over to the overall and-expression.
                valid_value_map[operand_valid_values.first] =
                    std::move(operand_valid_values.second);
              }
            }
          }
        }
      }

      if (operands.empty()) {
        // If there are no operands left, then this means that all have been
        // simplified to the literal true. Hence also this and-expression
        // can be simplified to true.
        did_simplify_ = true;
        return std::make_pair(std::make_shared<citcpp::boolean_literal>(true),
                              param_to_valid_values_map());
      }

      if (valid_value_map.size() == 1) {
        auto it = valid_value_map.begin();
        const auto& param =
            m_.get_parameters()[param_to_index_.find(it->first)->second];

        if (it->second.size() == param.get_values().size()) {
          // All values from the parameter's domain are allowed.
          // So since this and-expression only talks about this single
          // parameter, but in effect does not restrict it in any way,
          // it is a tautology.
          did_simplify_ = true;
          return std::make_pair(std::make_shared<citcpp::boolean_literal>(true),
                                param_to_valid_values_map());
        }
      }

      return std::make_pair(
          std::make_shared<citcpp::and_expression>(std::move(operands)),
          std::move(valid_value_map));
    }

    return_type operator()(const citcpp::or_expression& or_expr) {
      param_to_valid_values_map valid_value_map;
      bool is_first_non_trivial = true;
      std::vector<std::shared_ptr<citcpp::constraint>> operands;
      for (int i = 0; i < or_expr.get_operands().size(); ++i) {
        const auto& operand = or_expr.get_operands()[i];
        return_type op_res(operand->accept<return_type>(*this));
        if (op_res.first->get_constraint_type() ==
            citcpp::constraint_type::LITERAL) {
          const citcpp::boolean_literal* lit =
              static_cast<citcpp::boolean_literal*>(op_res.first.get());
          if (lit->get_value()) {
            // If we have at least one true literal in an or-expression,
            // then the whole or-expression can be simplified to true.
            return std::make_pair(
                std::make_shared<citcpp::boolean_literal>(true),
                param_to_valid_values_map());
          } else {
            // We intentionally do nothing in this case, as a false literal
            // in an or-expression can be skipped.
          }
        } else {
          operands.push_back(op_res.first);
          // Merge valid values.
          if (is_first_non_trivial) {
            valid_value_map = op_res.second;
            is_first_non_trivial = false;
          } else {
            param_to_valid_values_map& operand_valid_value_map = op_res.second;
            for (auto& operand_valid_values : operand_valid_value_map) {
              auto it = valid_value_map.find(operand_valid_values.first);
              if (it != valid_value_map.end()) {
                // Build the union of values, if parameter is contained in both
                // maps.
                it->second.insert(operand_valid_values.second.begin(),
                                  operand_valid_values.second.end());
              } else {
                // If parameter is only known in one of the maps, then it is
                // unconstrained in one of the operands of the or-expression,
                // meaning that it can take an arbitary value.
              }
            }

            // If parameter is only known in one of the maps, then it is
            // unconstrained in one of the operands of the or-expression,
            // meaning that it can take an arbitary value.
            std::erase_if(valid_value_map, [&operand_valid_value_map](
                                               const auto& param_and_values) {
              return !operand_valid_value_map.contains(param_and_values.first);
            });
          }
        }
      }

      if (operands.empty()) {
        // If there are no operands left, then this means that all have been
        // simplified to the literal false. Hence also this or-expression
        // can be simplified to false.
        did_simplify_ = true;
        return std::make_pair(std::make_shared<citcpp::boolean_literal>(false),
                              param_to_valid_values_map());
      }

      if (valid_value_map.size() == 1) {
        auto it = valid_value_map.begin();
        const auto& param =
            m_.get_parameters()[param_to_index_.find(it->first)->second];

        if (it->second.size() == param.get_values().size()) {
          // All values from the parameter's domain are allowed.
          // So since this or-expression only talks about this single
          // parameter, but in effect does not restrict it in any way,
          // it is a tautology.
          did_simplify_ = true;
          return std::make_pair(std::make_shared<citcpp::boolean_literal>(true),
                                param_to_valid_values_map());
        }
      }

      return std::make_pair(
          std::make_shared<citcpp::or_expression>(std::move(operands)),
          std::move(valid_value_map));
    }

    return_type operator()(const citcpp::implication& impl_expr) {
      return return_type();
    }

    bool did_simplify() const { return did_simplify_; }

    void set_did_simplify(bool value) { did_simplify_ = value; }

    static std::unordered_map<citcpp::parameter_reference, int,
                              citcpp::parameter_reference_hash>
    create_param_to_index_map(const citcpp::model& m) {

      std::unordered_map<citcpp::parameter_reference, int,
                         citcpp::parameter_reference_hash>
          param_to_index;
      int idx = 0;
      for (const auto& param : m.get_parameters()) {
        param_to_index.emplace(param, idx);
        ++idx;
      }

      return param_to_index;
    }

  private:
    citcpp::model& m_;
    const std::unordered_map<citcpp::parameter_reference, int,
                             citcpp::parameter_reference_hash>
        param_to_index_;
    bool did_simplify_;
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
    // points to does not immediately delete the constraint.
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

  simplification_visitor simpl_visitor(m);
  bool modified = true;
  simpl_visitor.set_did_simplify(true);

  while (modified || simpl_visitor.did_simplify()) {
    modified = false;
    simpl_visitor.set_did_simplify(false);

    simplification_visitor::param_to_valid_values_map valid_value_map;
    bool is_first_non_trivial = true;
    std::vector<std::shared_ptr<constraint>> new_constraints;
    new_constraints.reserve(m.get_constraints().size());

    for (int i = 0; i < m.get_constraints().size(); ++i) {
      const std::shared_ptr<citcpp::constraint>& constr =
          m.get_constraints()[i];

      simplification_visitor::return_type constr_and_valid_values =
          constr->accept<simplification_visitor::return_type>(simpl_visitor);
      if (constr_and_valid_values.first->get_constraint_type() ==
          citcpp::constraint_type::LITERAL) {
        const citcpp::boolean_literal* lit =
            static_cast<citcpp::boolean_literal*>(
                constr_and_valid_values.first.get());
        if (!lit->get_value()) {
          // If we have at least one false literal as a top-level
          // constraint, then the whole model is inconsistent.
          m.get_constraints().clear();
          m.get_constraints().push_back(
              std::make_shared<citcpp::boolean_literal>(false));
          return m;
        } else {
          // We intentionally do nothing in this case, as a true literal
          // as a top-level constraint does not restrict anything.
          continue;
        }
      } else {
        new_constraints.push_back(constr_and_valid_values.first);
        // Merge valid values.
        if (is_first_non_trivial) {
          valid_value_map = constr_and_valid_values.second;
          is_first_non_trivial = false;
        } else {
          const simplification_visitor::param_to_valid_values_map&
              constr_valid_value_map = constr_and_valid_values.second;
          for (const auto& constr_valid_values : constr_valid_value_map) {
            auto it = valid_value_map.find(constr_valid_values.first);
            if (it != valid_value_map.end()) {
              // Build the intersection of values, if parameter is contained
              // in both maps.
              std::erase_if(
                  it->second,
                  [&constr_valid_values](const citcpp::parameter_value& value) {
                    return !constr_valid_values.second.contains(value);
                  });
              if (it->second.empty()) {
                // No valid value at all for some parameter. So this
                // and-expression is a contradiction.
                m.get_constraints().clear();
                m.get_constraints().push_back(
                    std::make_shared<citcpp::boolean_literal>(false));
                return m;
              }
            } else {
              // If parameter is only known in one of the maps, then it is
              // unconstrained in one of the operands of the and-expression,
              // but since all operands must be true, the value restrictions
              // carry over to the overall and-expression.
              valid_value_map[constr_valid_values.first] =
                  std::move(constr_valid_values.second);
            }
          }
        }
      }
    }

    m.get_constraints() = std::move(new_constraints);

    for (auto& param : m.get_parameters()) {
      auto it = valid_value_map.find(param);
      if (it != valid_value_map.end()) {
        const auto& valid_values = it->second;
        std::erase_if(param.get_values(),
                      [&modified, &valid_values](const parameter_value& value) {
                        bool remove_value = !valid_values.contains(value);
                        modified = modified || remove_value;
                        return remove_value;
                      });
      }
    }
  }

  return m;
}

}  // namespace detail
}  // namespace citcpp
