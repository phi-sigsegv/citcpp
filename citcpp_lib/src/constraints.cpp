#include <citcpp/constraints.hpp>

namespace {

class constraint_copy_construction_visitor {
  public:
    std::unique_ptr<citcpp::constraint> operator()(
        const citcpp::boolean_proposition& prop) const {

      using namespace citcpp;

      return std::make_unique<boolean_proposition>(
          prop.get_parameter(), prop.get_operator(), prop.get_compared_value());
    }

    std::unique_ptr<citcpp::constraint> operator()(
        const citcpp::enum_proposition& prop) const {

      using namespace citcpp;

      return std::make_unique<enum_proposition>(
          prop.get_parameter(), prop.get_operator(), prop.get_compared_value());
    }

    std::unique_ptr<citcpp::constraint> operator()(
        const citcpp::int_proposition& prop) const {

      using namespace citcpp;

      return std::make_unique<int_proposition>(
          prop.get_parameter(), prop.get_operator(), prop.get_compared_value());
    }

    std::unique_ptr<citcpp::constraint> operator()(
        const citcpp::implication& impl) const {

      using namespace citcpp;

      return std::make_unique<implication>(
          impl.get_left_operand().create_copy(),
          impl.get_right_operand().create_copy());
    }

    std::unique_ptr<citcpp::constraint> operator()(
        const citcpp::and_expression& and_expr) const {

      using namespace citcpp;

      std::vector<constraint_holder> ops;
      for (const auto& operand : and_expr.get_operands()) {
        ops.push_back(std::move(operand->create_copy()));
      }

      return std::make_unique<and_expression>(std::move(ops));
    }

    std::unique_ptr<citcpp::constraint> operator()(
        const citcpp::or_expression& or_expr) const {

      using namespace citcpp;

      std::vector<constraint_holder> ops;
      for (const auto& operand : or_expr.get_operands()) {
        ops.push_back(std::move(operand->create_copy()));
      }

      return std::make_unique<or_expression>(std::move(ops));
    }
};

}  // namespace

namespace citcpp {

std::unique_ptr<constraint> constraint::create_copy() const {
  return this->accept<std::unique_ptr<citcpp::constraint>>(
      constraint_copy_construction_visitor());
}

}  // namespace citcpp
