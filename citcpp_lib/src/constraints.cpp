#include <citcpp/constraints.hpp>

namespace {

class constraint_copy_construction_visitor {
  public:
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

}  // namespace

namespace citcpp {

std::shared_ptr<constraint> constraint::create_copy() const {
  return this->accept<std::shared_ptr<citcpp::constraint>>(
      constraint_copy_construction_visitor());
}

}  // namespace citcpp
