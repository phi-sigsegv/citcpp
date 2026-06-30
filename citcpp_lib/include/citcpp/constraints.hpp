#ifndef CONSTRAINTS_HPP_
#define CONSTRAINTS_HPP_

#include <functional>
#include <memory>
#include <optional>
#include <ostream>
#include <type_traits>
#include <variant>
#include <vector>

#include "function_ref.hpp"
#include "parameter.hpp"

namespace citcpp {

class boolean_literal;
class boolean_proposition;
class enum_proposition;
class int_proposition;
class implication;
class and_expression;
class or_expression;

template <typename... Types>
struct constraint_view_helper {
    using mutable_constraint_view =
        std::variant<std::reference_wrapper<Types>...>;
    using const_constraint_view =
        std::variant<std::reference_wrapper<const Types>...>;
};

using constraint_view_types =
    constraint_view_helper<boolean_literal, boolean_proposition,
                           enum_proposition, int_proposition, implication,
                           and_expression, or_expression>;

using mutable_constraint_view = constraint_view_types::mutable_constraint_view;
using const_constraint_view = constraint_view_types::const_constraint_view;

/**
 * This is the base class of all kinds of constraints.
 */
class constraint {
  public:
    virtual ~constraint() = default;

    /**
     * A method allowing to call a visitor with the correct type
     * of this constraints. The visitor may return a value in its
     * visitation method, which will be moved and returned by this
     * method.
     *
     * The visitor is just a callable that accepts every possible alternative
     * type of constraint. This is in fact similar to the concept of a visitor
     * passed to std::visit, so refer to that documentation. Of course you
     * can also implement a function call operator that accept an object of
     * type "auto" in order to catch all types which you do not have a
     * concrete function call operator for. And just like for std::visit,
     * you could also have a set of lambdas passed  in, one for each constraint
     * type.
     */
    template <typename R = void, typename V>
    R accept(V&& visitor) {
      if constexpr (std::is_void_v<R>) {
        this->dispatch([&](mutable_constraint_view v) {
          std::visit(std::forward<V>(visitor), v);
        });
      } else {
        // We use an optional to delay construction of R
        std::optional<R> result;

        this->dispatch([&](mutable_constraint_view v) {
          // std::visit returns the value, we move it into the optional
          result.emplace(std::visit(std::forward<V>(visitor), v));
        });

        // Move the value out of the optional
        return std::move(*result);
      }
    }

    /**
     * A method allowing to call a visitor with the correct type
     * of this constraints. The visitor may return a value in its
     * visitation method, which will be moved and returned by this
     * method.
     *
     * The visitor is just a callable that accepts every possible alternative
     * type of constraint. This is in fact similar to the concept of a visitor
     * passed to std::visit, so refer to that documentation. Of course you
     * can also implement a function call operator that accept an object of
     * type "auto" in order to catch all types which you do not have a
     * concrete function call operator for. And just like for std::visit,
     * you could also have a set of lambdas passed  in, one for each constraint
     * type.
     */
    template <typename R = void, typename V>
    R accept(V&& visitor) const {
      if constexpr (std::is_void_v<R>) {
        this->dispatch([&](const_constraint_view v) {
          std::visit(std::forward<V>(visitor), v);
        });
      } else {
        // We use an optional to delay construction of R
        std::optional<R> result;

        this->dispatch([&](const_constraint_view v) {
          // std::visit returns the value, we move it into the optional
          result.emplace(std::visit(std::forward<V>(visitor), v));
        });

        // Move the value out of the optional
        return std::move(*result);
      }
    }

    /**
     * Creates a copy of this constraint.
     */
    std::shared_ptr<constraint> create_copy() const;

    friend std::ostream& operator<<(std::ostream& os, const constraint& constr);

  protected:
    /**
     * A virtual function allowing a callback to be executed with a reference
     * to this constraint, but the type resolved by dispatch.
     */
    virtual void dispatch(function_ref<void(mutable_constraint_view)> cb) = 0;
    virtual void dispatch(
        function_ref<void(const_constraint_view)> cb) const = 0;
};

/**
 * This class represents a boolean literal.
 */
class boolean_literal : public constraint {
  public:
    boolean_literal(bool value) : value_(value) {}

    bool get_value() const { return value_; }

    operator bool() const { return get_value(); }

  protected:
    void dispatch(function_ref<void(mutable_constraint_view)> cb) override {
      cb(std::ref(*this));
    }
    void dispatch(function_ref<void(const_constraint_view)> cb) const override {
      cb(std::cref(*this));
    }

  private:
    bool value_;
};

extern const boolean_literal FALSE_LITERAL;
extern const boolean_literal TRUE_LITERAL;

enum class relational_operator { EQ, NEQ, LT, LE, GE, GT };

/**
 * This class represents an atomic proposition about a single
 * variable like e.g. X = a or X <= b.
 */
class atomic_proposition : public constraint {
  public:
    atomic_proposition(parameter_reference param, relational_operator op)
        : param_(param), op_(op) {}

    const parameter_reference& get_parameter() const { return param_; }

    relational_operator get_operator() const { return op_; }

  protected:
    parameter_reference param_;
    relational_operator op_;
};

/**
 * This class represents an atomic proposition about a single
 * boolean variable.
 */
class boolean_proposition : public atomic_proposition {
  public:
    boolean_proposition(parameter_reference param, relational_operator op,
                        parameter_value value)
        : atomic_proposition(param, op), value_(value) {}

    bool get_compared_value() const { return value_; }

  protected:
    void dispatch(function_ref<void(mutable_constraint_view)> cb) override {
      cb(std::ref(*this));
    }
    void dispatch(function_ref<void(const_constraint_view)> cb) const override {
      cb(std::cref(*this));
    }

  private:
    bool value_;
};

/**
 * This class represents an atomic proposition about a single
 * enum variable.
 */
class enum_proposition : public atomic_proposition {
  public:
    enum_proposition(parameter_reference param, relational_operator op,
                     parameter_value value)
        : atomic_proposition(param, op), value_(value) {}

    const std::string& get_compared_value() const { return value_; }

  protected:
    void dispatch(function_ref<void(mutable_constraint_view)> cb) override {
      cb(std::ref(*this));
    }
    void dispatch(function_ref<void(const_constraint_view)> cb) const override {
      cb(std::cref(*this));
    }

  private:
    std::string value_;
};

/**
 * This class represents an atomic proposition about a single
 * enum variable.
 */
class int_proposition : public atomic_proposition {
  public:
    int_proposition(parameter_reference param, relational_operator op,
                    parameter_value value)
        : atomic_proposition(param, op), value_(value) {}

    int get_compared_value() const { return value_; }

  protected:
    void dispatch(function_ref<void(mutable_constraint_view)> cb) override {
      cb(std::ref(*this));
    }
    void dispatch(function_ref<void(const_constraint_view)> cb) const override {
      cb(std::cref(*this));
    }

  private:
    int value_;
};

enum class binary_operator { IMPL };

/**
 * This is a base class for binary operators like implication.
 */
class binary_operation : public constraint {
  public:
    binary_operation(std::shared_ptr<constraint> lhs, binary_operator op,
                     std::shared_ptr<constraint> rhs)
        : lhs_(std::move(lhs)), op_(op), rhs_(std::move(rhs)) {}

    binary_operator get_operator() const { return op_; }

    std::shared_ptr<constraint>& get_left_operand() { return lhs_; }
    const std::shared_ptr<constraint>& get_left_operand() const { return lhs_; }
    std::shared_ptr<constraint>& get_right_operand() { return rhs_; }
    const std::shared_ptr<constraint>& get_right_operand() const {
      return rhs_;
    }

    void set_left_operand(std::shared_ptr<constraint> lhs) {
      lhs_ = std::move(lhs);
    }

    void set_right_operand(std::shared_ptr<constraint> rhs) {
      rhs_ = std::move(rhs);
    }

  protected:
    binary_operator op_;
    std::shared_ptr<constraint> lhs_;
    std::shared_ptr<constraint> rhs_;
};

/**
 * This models a logical implication.
 */
class implication : public binary_operation {
  public:
    implication(std::shared_ptr<constraint> lhs,
                std::shared_ptr<constraint> rhs)
        : binary_operation(std::move(lhs), binary_operator::IMPL,
                           std::move(rhs)) {}

  protected:
    void dispatch(function_ref<void(mutable_constraint_view)> cb) override {
      cb(std::ref(*this));
    }
    void dispatch(function_ref<void(const_constraint_view)> cb) const override {
      cb(std::cref(*this));
    }
};

enum class nray_operator { AND, OR };

/**
 * This is a base class for n-ary operators like AND and OR.
 */
class nary_operation : public constraint {
  public:
    nary_operation(nray_operator op,
                   std::vector<std::shared_ptr<constraint>> operands)
        : op_(op), operands_(std::move(operands)) {}

    nray_operator get_operator() const { return op_; }

    std::vector<std::shared_ptr<constraint>>& get_operands() {
      return operands_;
    }

    const std::vector<std::shared_ptr<constraint>>& get_operands() const {
      return operands_;
    }

  protected:
    nray_operator op_;
    std::vector<std::shared_ptr<constraint>> operands_;
};

/**
 * This models a logical AND expression.
 */
class and_expression : public nary_operation {
  public:
    and_expression(std::vector<std::shared_ptr<constraint>> operands)
        : nary_operation(nray_operator::AND, std::move(operands)) {}

  protected:
    void dispatch(function_ref<void(mutable_constraint_view)> cb) override {
      cb(std::ref(*this));
    }
    void dispatch(function_ref<void(const_constraint_view)> cb) const override {
      cb(std::cref(*this));
    }
};

/**
 * This models a logical OR expression.
 */
class or_expression : public nary_operation {
  public:
    or_expression(std::vector<std::shared_ptr<constraint>> operands)
        : nary_operation(nray_operator::OR, std::move(operands)) {}

  protected:
    void dispatch(function_ref<void(mutable_constraint_view)> cb) override {
      cb(std::ref(*this));
    }
    void dispatch(function_ref<void(const_constraint_view)> cb) const override {
      cb(std::cref(*this));
    }
};

}  // namespace citcpp

#endif /* CONSTRAINTS_HPP_ */
