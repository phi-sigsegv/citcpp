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

enum class constraint_type {
  LITERAL,
  PROP_BOOLEAN,
  PROP_ENUM,
  PROP_INT,
  IMPLICATION,
  AND_EXPR,
  OR_EXPR
};

/**
 * This is the base class of all kinds of constraints.
 */
class constraint {
  public:
    virtual ~constraint() = default;

    virtual constraint_type get_constraint_type() const = 0;

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
    boolean_literal(bool value) noexcept;

    constraint_type get_constraint_type() const override;

    bool get_value() const;

    operator bool() const;

  protected:
    void dispatch(function_ref<void(mutable_constraint_view)> cb) override;
    void dispatch(function_ref<void(const_constraint_view)> cb) const override;

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
    atomic_proposition(parameter_reference param, relational_operator op);

    const parameter_reference& get_parameter() const;

    relational_operator get_operator() const;

  private:
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
                        parameter_value value);

    constraint_type get_constraint_type() const override;

    bool get_compared_value() const;

  protected:
    void dispatch(function_ref<void(mutable_constraint_view)> cb) override;
    void dispatch(function_ref<void(const_constraint_view)> cb) const override;

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
                     parameter_value value);

    constraint_type get_constraint_type() const override;

    const std::string& get_compared_value() const;

  protected:
    void dispatch(function_ref<void(mutable_constraint_view)> cb) override;
    void dispatch(function_ref<void(const_constraint_view)> cb) const override;

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
                    parameter_value value);

    constraint_type get_constraint_type() const override;

    int get_compared_value() const;

  protected:
    void dispatch(function_ref<void(mutable_constraint_view)> cb) override;
    void dispatch(function_ref<void(const_constraint_view)> cb) const override;

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
                     std::shared_ptr<constraint> rhs);

    binary_operator get_operator() const;

    std::shared_ptr<constraint>& get_left_operand();
    const std::shared_ptr<constraint>& get_left_operand() const;
    std::shared_ptr<constraint>& get_right_operand();
    const std::shared_ptr<constraint>& get_right_operand() const;

    void set_left_operand(std::shared_ptr<constraint> lhs);
    void set_right_operand(std::shared_ptr<constraint> rhs);

  private:
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
                std::shared_ptr<constraint> rhs);

    constraint_type get_constraint_type() const override;

  protected:
    void dispatch(function_ref<void(mutable_constraint_view)> cb) override;
    void dispatch(function_ref<void(const_constraint_view)> cb) const override;
};

enum class nray_operator { AND, OR };

/**
 * This is a base class for n-ary operators like AND and OR.
 */
class nary_operation : public constraint {
  public:
    nary_operation(nray_operator op,
                   std::vector<std::shared_ptr<constraint>> operands);

    nray_operator get_operator() const;

    std::vector<std::shared_ptr<constraint>>& get_operands();
    const std::vector<std::shared_ptr<constraint>>& get_operands() const;

  private:
    nray_operator op_;
    std::vector<std::shared_ptr<constraint>> operands_;
};

/**
 * This models a logical AND expression.
 */
class and_expression : public nary_operation {
  public:
    and_expression(std::vector<std::shared_ptr<constraint>> operands);

    constraint_type get_constraint_type() const override;

  protected:
    void dispatch(function_ref<void(mutable_constraint_view)> cb) override;
    void dispatch(function_ref<void(const_constraint_view)> cb) const override;
};

/**
 * This models a logical OR expression.
 */
class or_expression : public nary_operation {
  public:
    or_expression(std::vector<std::shared_ptr<constraint>> operands);

    constraint_type get_constraint_type() const override;

  protected:
    void dispatch(function_ref<void(mutable_constraint_view)> cb) override;
    void dispatch(function_ref<void(const_constraint_view)> cb) const override;
};

}  // namespace citcpp

#endif /* CONSTRAINTS_HPP_ */
