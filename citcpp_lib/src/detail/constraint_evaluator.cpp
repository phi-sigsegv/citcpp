#include "constraint_evaluator.hpp"

namespace {

class internal_test_constraint_eval_visitor {
  public:
    internal_test_constraint_eval_visitor(
        const std::unordered_map<citcpp::parameter_reference, int,
                                 citcpp::parameter_reference_hash>&
            param_to_index,
        const citcpp::detail::test& test, const citcpp::model& model)
        : param_to_index_(param_to_index), test_(test), model_(model) {}

    bool operator()(const citcpp::boolean_proposition& prop) const {
      using namespace citcpp::detail;
      using namespace citcpp;

      bool eval_result = false;
      auto it = param_to_index_.find(prop.get_parameter());
      if (it != param_to_index_.end()) {
        bool assigned_value = model_.get_parameters()[it->second]
                                  .get_values()[test_.get_values()[it->second]];

        switch (prop.get_operator()) {
          case relational_operator::EQ:
            eval_result = (assigned_value == prop.get_compared_value());
            break;
          default:
            eval_result = (assigned_value != prop.get_compared_value());
            break;
        }
      }

      return eval_result;
    }

    bool operator()(const citcpp::enum_proposition& prop) const {
      using namespace citcpp::detail;
      using namespace citcpp;

      bool eval_result = false;
      auto it = param_to_index_.find(prop.get_parameter());
      if (it != param_to_index_.end()) {
        const std::string& assigned_value =
            model_.get_parameters()[it->second]
                .get_values()[test_.get_values()[it->second]];

        switch (prop.get_operator()) {
          case relational_operator::EQ:
            eval_result = (assigned_value == prop.get_compared_value());
            break;
          default:
            eval_result = (assigned_value != prop.get_compared_value());
            break;
        }
      }

      return eval_result;
    }

    bool operator()(const citcpp::int_proposition& prop) const {
      using namespace citcpp::detail;
      using namespace citcpp;

      bool eval_result = false;
      auto it = param_to_index_.find(prop.get_parameter());
      if (it != param_to_index_.end()) {
        int assigned_value = model_.get_parameters()[it->second]
                                 .get_values()[test_.get_values()[it->second]];

        switch (prop.get_operator()) {
          case relational_operator::EQ:
            eval_result = (assigned_value == prop.get_compared_value());
            break;
          case relational_operator::LE:
            eval_result = (assigned_value <= prop.get_compared_value());
            break;
          case relational_operator::LT:
            eval_result = (assigned_value < prop.get_compared_value());
            break;
          case relational_operator::GE:
            eval_result = (assigned_value >= prop.get_compared_value());
            break;
          case relational_operator::GT:
            eval_result = (assigned_value > prop.get_compared_value());
            break;
          default:
            eval_result = (assigned_value != prop.get_compared_value());
            break;
        }
      }

      return eval_result;
    }

    bool operator()(const citcpp::implication& impl) const {
      return !impl.get_left_operand()->accept<bool>(*this) ||
             impl.get_right_operand()->accept<bool>(*this);
    }

    bool operator()(const citcpp::and_expression& and_expr) const {
      for (const auto& operand : and_expr.get_operands()) {
        if (!operand->accept<bool>(*this)) {
          // Short circuit: If anything is false, then the entire expression is
          // false.
          return false;
        }
      }

      // If all operands evaluate to true, then the entire expression is true.
      return true;
    }

    bool operator()(const citcpp::or_expression& or_expr) const {
      for (const auto& operand : or_expr.get_operands()) {
        if (operand->accept<bool>(*this)) {
          // Short circuit: If anything is true, then the entire expression is
          // true.
          return true;
        }
      }

      // If all operands evaluate to false, then the entire expression is false.
      return false;
    }

  private:
    const std::unordered_map<citcpp::parameter_reference, int,
                             citcpp::parameter_reference_hash>& param_to_index_;
    const citcpp::detail::test& test_;
    const citcpp::model& model_;
};

}  // namespace

namespace citcpp {
namespace detail {

bool constraint_evaluator::operator()(const test& test,
                                      const constraint& constr,
                                      const model& model) const {

  bool ret = constr.accept<bool>(
      internal_test_constraint_eval_visitor(param_to_index_, test, model));

  return ret;
}

bool constraint_evaluator::operator()(const test& test,
                                      const model& model) const {

  for (const auto& constr : model.get_constraints()) {
    if (!(*this)(test, *constr, model)) {
      return false;
    }
  }

  return true;
}

bool constraint_evaluator::operator()(const internal_test_set& tests,
                                      const citcpp::model& model) const {

  for (const auto& test : tests.get_list_of_tests()) {
    if (!(*this)(test, model)) {
      return false;
    }
  }

  return true;
}

}  // namespace detail
}  // namespace citcpp
