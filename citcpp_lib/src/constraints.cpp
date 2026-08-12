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
        ops.push_back(operand->create_copy());
      }

      return std::make_shared<and_expression>(std::move(ops));
    }

    std::shared_ptr<citcpp::constraint> operator()(
        const citcpp::or_expression& or_expr) const {

      using namespace citcpp;

      std::vector<std::shared_ptr<citcpp::constraint>> ops;
      for (const auto& operand : or_expr.get_operands()) {
        ops.push_back(operand->create_copy());
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

std::shared_ptr<constraint> constraint::create_copy() const {
  return this->accept<std::shared_ptr<citcpp::constraint>>(
      constraint_copy_construction_visitor());
}

std::ostream& operator<<(std::ostream& os, const constraint& constr) {
  constr.accept<void>(constraint_streaming_visitor(os));

  return os;
}

boolean_literal::boolean_literal(bool value) noexcept : value_(value) {}

constraint_type boolean_literal::get_constraint_type() const {
  return constraint_type::LITERAL;
}

bool boolean_literal::get_value() const { return value_; }

boolean_literal::operator bool() const { return get_value(); }

void boolean_literal::dispatch(function_ref<void(mutable_constraint_view)> cb) {
  cb(std::ref(*this));
}

void boolean_literal::dispatch(
    function_ref<void(const_constraint_view)> cb) const {

  cb(std::cref(*this));
}

const boolean_literal FALSE_LITERAL{false};
const boolean_literal TRUE_LITERAL{true};

atomic_proposition::atomic_proposition(parameter_reference param,
                                       relational_operator op)
    : param_(param), op_(op) {}

const parameter_reference& atomic_proposition::get_parameter() const {
  return param_;
}

relational_operator atomic_proposition::get_operator() const { return op_; }

boolean_proposition::boolean_proposition(parameter_reference param,
                                         relational_operator op,
                                         parameter_value value)
    : atomic_proposition(param, op), value_(value) {}

constraint_type boolean_proposition::get_constraint_type() const {
  return constraint_type::PROP_BOOLEAN;
}

bool boolean_proposition::get_compared_value() const { return value_; }

void boolean_proposition::dispatch(
    function_ref<void(mutable_constraint_view)> cb) {

  cb(std::ref(*this));
}
void boolean_proposition::dispatch(
    function_ref<void(const_constraint_view)> cb) const {

  cb(std::cref(*this));
}

enum_proposition::enum_proposition(parameter_reference param,
                                   relational_operator op,
                                   parameter_value value)
    : atomic_proposition(param, op), value_(value) {}

constraint_type enum_proposition::get_constraint_type() const {
  return constraint_type::PROP_ENUM;
}

void enum_proposition::dispatch(
    function_ref<void(mutable_constraint_view)> cb) {

  cb(std::ref(*this));
}
void enum_proposition::dispatch(
    function_ref<void(const_constraint_view)> cb) const {

  cb(std::cref(*this));
}

int_proposition::int_proposition(parameter_reference param,
                                 relational_operator op, parameter_value value)
    : atomic_proposition(param, op), value_(value) {}

constraint_type int_proposition::get_constraint_type() const {
  return constraint_type::PROP_INT;
}

int int_proposition::get_compared_value() const { return value_; }

void int_proposition::dispatch(function_ref<void(mutable_constraint_view)> cb) {
  cb(std::ref(*this));
}
void int_proposition::dispatch(
    function_ref<void(const_constraint_view)> cb) const {

  cb(std::cref(*this));
}

const std::string& enum_proposition::get_compared_value() const {
  return value_;
}

binary_operation::binary_operation(std::shared_ptr<constraint> lhs,
                                   binary_operator op,
                                   std::shared_ptr<constraint> rhs)
    : op_(op), lhs_(lhs), rhs_(rhs) {}

binary_operator binary_operation::get_operator() const { return op_; }

std::shared_ptr<constraint>& binary_operation::get_left_operand() {
  return lhs_;
}

const std::shared_ptr<constraint>& binary_operation::get_left_operand() const {
  return lhs_;
}

std::shared_ptr<constraint>& binary_operation::get_right_operand() {
  return rhs_;
}

const std::shared_ptr<constraint>& binary_operation::get_right_operand() const {
  return rhs_;
}

void binary_operation::set_left_operand(std::shared_ptr<constraint> lhs) {
  lhs_ = lhs;
}

void binary_operation::set_right_operand(std::shared_ptr<constraint> rhs) {
  rhs_ = rhs;
}

implication::implication(std::shared_ptr<constraint> lhs,
                         std::shared_ptr<constraint> rhs)
    : binary_operation(std::move(lhs), binary_operator::IMPL, std::move(rhs)) {}

constraint_type implication::get_constraint_type() const {
  return constraint_type::IMPLICATION;
}

void implication::dispatch(function_ref<void(mutable_constraint_view)> cb) {
  cb(std::ref(*this));
}

void implication::dispatch(function_ref<void(const_constraint_view)> cb) const {
  cb(std::cref(*this));
}

nary_operation::nary_operation(
    nray_operator op, std::vector<std::shared_ptr<constraint>> operands)
    : op_(op), operands_(std::move(operands)) {}

nray_operator nary_operation::get_operator() const { return op_; }

std::vector<std::shared_ptr<constraint>>& nary_operation::get_operands() {
  return operands_;
}

const std::vector<std::shared_ptr<constraint>>& nary_operation::get_operands()
    const {

  return operands_;
}

and_expression::and_expression(
    std::vector<std::shared_ptr<constraint>> operands)
    : nary_operation(nray_operator::AND, std::move(operands)) {}

constraint_type and_expression::get_constraint_type() const {
  return constraint_type::AND_EXPR;
}

void and_expression::dispatch(function_ref<void(mutable_constraint_view)> cb) {
  cb(std::ref(*this));
}
void and_expression::dispatch(
    function_ref<void(const_constraint_view)> cb) const {

  cb(std::cref(*this));
}

or_expression::or_expression(std::vector<std::shared_ptr<constraint>> operands)
    : nary_operation(nray_operator::OR, std::move(operands)) {}

constraint_type or_expression::get_constraint_type() const {
  return constraint_type::OR_EXPR;
}

void or_expression::dispatch(function_ref<void(mutable_constraint_view)> cb) {
  cb(std::ref(*this));
}
void or_expression::dispatch(
    function_ref<void(const_constraint_view)> cb) const {

  cb(std::cref(*this));
}

}  // namespace citcpp
