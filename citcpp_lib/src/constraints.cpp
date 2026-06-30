#include <citcpp/constraints.hpp>

namespace {

class constraint_copy_construction_visitor {
  public:
    std::shared_ptr<citcpp::constraint> operator()(
        const citcpp::boolean_literal& lit) const {

      using namespace citcpp;

      return std::make_shared<boolean_literal>(lit);
    }

    std::shared_ptr<citcpp::constraint> operator()(
        const citcpp::boolean_proposition& prop) const {

      using namespace citcpp;

      return std::make_shared<boolean_proposition>(
          prop.get_parameter(), prop.get_operator(), prop.get_compared_value());
    }

    std::shared_ptr<citcpp::constraint> operator()(
        const citcpp::enum_proposition& prop) const {

      using namespace citcpp;

      return std::make_shared<enum_proposition>(
          prop.get_parameter(), prop.get_operator(), prop.get_compared_value());
    }

    std::shared_ptr<citcpp::constraint> operator()(
        const citcpp::int_proposition& prop) const {

      using namespace citcpp;

      return std::make_shared<int_proposition>(
          prop.get_parameter(), prop.get_operator(), prop.get_compared_value());
    }

    std::shared_ptr<citcpp::constraint> operator()(
        const citcpp::implication& impl) const {

      using namespace citcpp;

      return std::make_shared<implication>(
          impl.get_left_operand()->create_copy(),
          impl.get_right_operand()->create_copy());
    }

    std::shared_ptr<citcpp::constraint> operator()(
        const citcpp::and_expression& and_expr) const {

      using namespace citcpp;

      std::vector<std::shared_ptr<citcpp::constraint>> ops;
      for (const auto& operand : and_expr.get_operands()) {
        ops.push_back(std::move(operand->create_copy()));
      }

      return std::make_shared<and_expression>(std::move(ops));
    }

    std::shared_ptr<citcpp::constraint> operator()(
        const citcpp::or_expression& or_expr) const {

      using namespace citcpp;

      std::vector<std::shared_ptr<citcpp::constraint>> ops;
      for (const auto& operand : or_expr.get_operands()) {
        ops.push_back(std::move(operand->create_copy()));
      }

      return std::make_shared<or_expression>(std::move(ops));
    }
};

std::ostream& operator<<(std::ostream& os,
                         const citcpp::relational_operator& op) {

  using namespace citcpp;

  switch (op) {
    case relational_operator::EQ:
      os << "=";
      break;
    case relational_operator::NEQ:
      os << "!=";
      break;
    case relational_operator::GE:
      os << ">=";
      break;
    case relational_operator::LE:
      os << "<=";
      break;
    case relational_operator::GT:
      os << ">";
      break;
    case relational_operator::LT:
      os << "<";
      break;
  }

  return os;
}

class constraint_streaming_visitor {
  public:
    constraint_streaming_visitor(std::ostream& os) : os_(os) {}

    void operator()(const citcpp::boolean_literal& lit) const {
      if (lit) {
        os_ << "true";
      } else {
        os_ << "false";
      }
    }

    void operator()(const citcpp::boolean_proposition& prop) const {
      os_ << prop.get_parameter() << " ";
      os_ << prop.get_operator() << " ";
      if (prop.get_compared_value()) {
        os_ << "true";
      } else {
        os_ << "false";
      }
    }

    void operator()(const citcpp::enum_proposition& prop) const {
      os_ << prop.get_parameter() << " ";
      os_ << prop.get_operator() << " ";
      os_ << "\"" << prop.get_compared_value() << "\"";
    }

    void operator()(const citcpp::int_proposition& prop) const {
      os_ << prop.get_parameter() << " ";
      os_ << prop.get_operator() << " ";
      os_ << prop.get_compared_value();
    }

    void operator()(const citcpp::implication& impl) const {
      os_ << "(" << *impl.get_left_operand().get() << " => "
          << *impl.get_right_operand().get() << ")";
    }

    void operator()(const citcpp::and_expression& and_expr) const {
      const std::string EMPTY_SEP = "";
      const std::string REAL_SEP = " && ";
      const std::string* sep = &EMPTY_SEP;
      os_ << "(";
      for (const auto& op : and_expr.get_operands()) {
        os_ << *sep << *op.get();
        sep = &REAL_SEP;
      }
      os_ << ")";
    }

    void operator()(const citcpp::or_expression& or_expr) const {
      const std::string EMPTY_SEP = "";
      const std::string REAL_SEP = " || ";
      const std::string* sep = &EMPTY_SEP;
      os_ << "(";
      for (const auto& op : or_expr.get_operands()) {
        os_ << *sep << *op.get();
        sep = &REAL_SEP;
      }
      os_ << ")";
    }

  private:
    std::ostream& os_;
};

}  // namespace

namespace citcpp {

const boolean_literal FALSE_LITERAL{false};
const boolean_literal TRUE_LITERAL{true};

std::shared_ptr<constraint> constraint::create_copy() const {
  return this->accept<std::shared_ptr<citcpp::constraint>>(
      constraint_copy_construction_visitor());
}

std::ostream& operator<<(std::ostream& os, const constraint& constr) {
  constr.accept<void>(constraint_streaming_visitor(os));

  return os;
}

}  // namespace citcpp
