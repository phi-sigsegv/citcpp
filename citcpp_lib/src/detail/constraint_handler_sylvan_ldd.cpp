#include "constraint_handler_sylvan_ldd.hpp"

namespace {

class constraint_to_ldd_visitor {
  public:
    constraint_to_ldd_visitor(const citcpp::detail::internal_model& model)
        : model_(model), param_to_index_(), negate_(false) {

      int idx = 0;
      for (const auto& param : model_.get_input_model().get_parameters()) {
        param_to_index_.emplace(param.get_name(), idx);
        ++idx;
      }
    }

    citcpp::detail::sylvan_ldd operator()(
        const citcpp::boolean_proposition& prop) {

      using namespace citcpp::detail;
      using namespace citcpp;

      const parameter& param = model_.get_input_model().get_parameter(
          prop.get_parameter().get_name());
      const int param_idx = param_to_index_.at(param.get_name());
      const int domain_size = param.get_values().size();

      int value_index = 0;
      for (const auto& value : param.get_values()) {
        bool value_as_bool = value;
        if (value_as_bool == prop.get_compared_value()) {
          break;
        }
        ++value_index;
      }

      switch (prop.get_operator()) {
        case relational_operator::EQ:
          if (value_index >= param.get_values().size()) {
            return negate_ ? sylvan_ldd::lddTrue() : sylvan_ldd::lddFalse();
          } else {
            return sylvan_ldd(
                param_idx,
                negate_ ? relational_operator::NEQ : relational_operator::EQ,
                value_index, domain_size);
          }
        default:
          if (value_index >= param.get_values().size()) {
            return negate_ ? sylvan_ldd::lddFalse() : sylvan_ldd::lddTrue();
          } else {
            return sylvan_ldd(
                param_idx,
                negate_ ? relational_operator::EQ : relational_operator::NEQ,
                value_index, domain_size);
          }
      }
    }

    citcpp::detail::sylvan_ldd operator()(
        const citcpp::enum_proposition& prop) {

      using namespace citcpp::detail;
      using namespace citcpp;

      const parameter& param = model_.get_input_model().get_parameter(
          prop.get_parameter().get_name());
      const int param_idx = param_to_index_.at(param.get_name());
      const int domain_size = param.get_values().size();

      int value_index = 0;
      for (const auto& value : param.get_values()) {
        const std::string& value_as_string = value;
        if (value_as_string == prop.get_compared_value()) {
          break;
        }
        ++value_index;
      }

      switch (prop.get_operator()) {
        case relational_operator::EQ:
          if (value_index >= param.get_values().size()) {
            return negate_ ? sylvan_ldd::lddTrue() : sylvan_ldd::lddFalse();
          } else {
            return sylvan_ldd(
                param_idx,
                negate_ ? relational_operator::NEQ : relational_operator::EQ,
                value_index, domain_size);
          }
        default:
          if (value_index >= param.get_values().size()) {
            return negate_ ? sylvan_ldd::lddFalse() : sylvan_ldd::lddTrue();
          } else {
            return sylvan_ldd(
                param_idx,
                negate_ ? relational_operator::EQ : relational_operator::NEQ,
                value_index, domain_size);
          }
      }
    }

    citcpp::detail::sylvan_ldd operator()(
        const citcpp::int_proposition& prop) const {

      using namespace citcpp::detail;
      using namespace citcpp;

      const parameter& param = model_.get_input_model().get_parameter(
          prop.get_parameter().get_name());
      const int param_idx = param_to_index_.at(param.get_name());
      const int domain_size = param.get_values().size();

      switch (prop.get_operator()) {
        case relational_operator::EQ: {
          int value_index = 0;
          for (const auto& value : param.get_values()) {
            int value_as_int = value;
            if (value_as_int == prop.get_compared_value()) {
              return sylvan_ldd(
                  param_idx,
                  negate_ ? relational_operator::NEQ : relational_operator::EQ,
                  value_index, domain_size);
            }
            ++value_index;
          }

          return negate_ ? sylvan_ldd::lddTrue() : sylvan_ldd::lddFalse();
        }
        case relational_operator::LE: {
          if ((int)param.get_values()[param.get_values().size() - 1] <=
              prop.get_compared_value()) {
            return negate_ ? sylvan_ldd::lddFalse() : sylvan_ldd::lddTrue();
          }

          for (int v = param.get_values().size() - 1; v >= 0; --v) {
            int value_as_int = param.get_values()[v];
            if (value_as_int <= prop.get_compared_value()) {
              return sylvan_ldd(
                  param_idx,
                  negate_ ? relational_operator::GT : relational_operator::LE,
                  v, domain_size);
            }
          }

          return negate_ ? sylvan_ldd::lddTrue() : sylvan_ldd::lddFalse();
        }
        case relational_operator::LT: {
          if ((int)param.get_values()[param.get_values().size() - 1] <
              prop.get_compared_value()) {
            return negate_ ? sylvan_ldd::lddFalse() : sylvan_ldd::lddTrue();
          }

          for (int v = param.get_values().size() - 1; v >= 0; --v) {
            int value_as_int = param.get_values()[v];
            if (value_as_int < prop.get_compared_value()) {
              return sylvan_ldd(
                  param_idx,
                  negate_ ? relational_operator::GE : relational_operator::LT,
                  v, domain_size);
            }
          }

          return negate_ ? sylvan_ldd::lddTrue() : sylvan_ldd::lddFalse();
        }
        case relational_operator::GE: {
          if ((int)param.get_values()[0] >= prop.get_compared_value()) {
            return negate_ ? sylvan_ldd::lddFalse() : sylvan_ldd::lddTrue();
          }

          for (int v = 0; v < param.get_values().size(); ++v) {
            int value_as_int = param.get_values()[v];
            if (value_as_int >= prop.get_compared_value()) {
              return sylvan_ldd(
                  param_idx,
                  negate_ ? relational_operator::LT : relational_operator::GE,
                  v, domain_size);
            }
          }

          return negate_ ? sylvan_ldd::lddTrue() : sylvan_ldd::lddFalse();
        }
        case relational_operator::GT: {
          if ((int)param.get_values()[0] > prop.get_compared_value()) {
            return negate_ ? sylvan_ldd::lddFalse() : sylvan_ldd::lddTrue();
          }

          for (int v = 0; v < param.get_values().size(); ++v) {
            int value_as_int = param.get_values()[v];
            if (value_as_int > prop.get_compared_value()) {
              return sylvan_ldd(
                  param_idx,
                  negate_ ? relational_operator::LE : relational_operator::GT,
                  v, domain_size);
            }
          }

          return negate_ ? sylvan_ldd::lddTrue() : sylvan_ldd::lddFalse();
        }
        default: {
          int value_index = 0;
          for (const auto& value : param.get_values()) {
            int value_as_int = value;
            if (value_as_int == prop.get_compared_value()) {
              return sylvan_ldd(
                  param_idx,
                  negate_ ? relational_operator::EQ : relational_operator::NEQ,
                  value_index, domain_size);
            }
            ++value_index;
          }

          return negate_ ? sylvan_ldd::lddFalse() : sylvan_ldd::lddTrue();
        }
      }
    }

    citcpp::detail::sylvan_ldd operator()(const citcpp::implication& impl) {
      using namespace citcpp::detail;
      using namespace citcpp;

      negate_ = !negate_;
      sylvan_ldd premise = impl.get_left_operand().accept<sylvan_ldd>(*this);
      negate_ = !negate_;

      sylvan_ldd consequence =
          impl.get_right_operand().accept<sylvan_ldd>(*this);

      sylvan_ldd ldd =
          negate_ ? (premise * consequence) : (premise + consequence);

      return ldd;
    }

    citcpp::detail::sylvan_ldd operator()(
        const citcpp::and_expression& and_expr) {

      using namespace citcpp::detail;
      using namespace citcpp;

      sylvan_ldd ldd_false = sylvan_ldd::lddTrue();
      sylvan_ldd ldd = ldd_false;
      for (const auto& operand : and_expr.get_operands()) {
        if (ldd == ldd_false) {
          ldd = operand->accept<sylvan_ldd>(*this);
        } else {
          if (negate_) {
            ldd += operand->accept<sylvan_ldd>(*this);
          } else {
            ldd *= operand->accept<sylvan_ldd>(*this);
          }
        }
      }

      return ldd;
    }

    citcpp::detail::sylvan_ldd operator()(
        const citcpp::or_expression& or_expr) {

      using namespace citcpp::detail;
      using namespace citcpp;

      sylvan_ldd ldd_false = sylvan_ldd::lddFalse();
      sylvan_ldd ldd = ldd_false;
      for (const auto& operand : or_expr.get_operands()) {
        if (ldd == ldd_false) {
          ldd = operand->accept<sylvan_ldd>(*this);
        } else {
          if (negate_) {
            ldd *= operand->accept<sylvan_ldd>(*this);
          } else {
            ldd += operand->accept<sylvan_ldd>(*this);
          }
        }
      }

      return ldd;
    }

  private:
    const citcpp::detail::internal_model& model_;
    std::unordered_map<citcpp::parameter_reference, int,
                       citcpp::parameter_reference_hash>
        param_to_index_;
    bool negate_;
};

}  // namespace

namespace citcpp {
namespace detail {

constraint_handler_sylvan_ldd::constraint_handler_sylvan_ldd(
    const internal_model& model)
    : model_(model), ldd_() {

  constraint_to_ldd_visitor visitor(model);
  sylvan_ldd ldd_true = sylvan_ldd::lddTrue();
  sylvan_ldd ldd = ldd_true;
  for (const auto& constr : model.get_input_model().get_constraints()) {
    if (ldd == ldd_true) {
      ldd = constr->accept<sylvan_ldd>(visitor);
    } else {
      ldd *= constr->accept<sylvan_ldd>(visitor);
    }
  }

  ldd_ = ldd;
}

bool constraint_handler_sylvan_ldd::is_thread_safe() const { return true; }

bool constraint_handler_sylvan_ldd::is_valid_partial_test(const test& t) const {
  return ldd_.is_sat_with_partial_assignment(t.get_values());
}

bitset_uint64 constraint_handler_sylvan_ldd::get_valid_parameter_assignments(
    const test& t, unsigned int param_idx) const {

  return ldd_.get_valid_variable_assignments(
      param_idx, model_.get_parameter_num_values()[param_idx], t.get_values());
}

void constraint_handler_sylvan_ldd::replace_dont_care_values(test& t) const {
  // Conjunct the LDD with a one representing the assignments
  // found in the given test.
  // On the resulting LDD we just extract an arbitrary full
  // assignment.
  (ldd_ * sylvan_ldd(t.get_values())).get_sat_one(t.get_values());
}

}  // namespace detail
}  // namespace citcpp
