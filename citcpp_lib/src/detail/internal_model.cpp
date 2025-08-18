#include "internal_model.hpp"

#include "citcpp_utils.hpp"

namespace {

std::vector<unsigned int> compute_parameter_index_map(
    const citcpp::input_model &input_model,
    const std::vector<citcpp::parameter> &parameter_order) {
  std::vector<unsigned int> parameter_index_map;

  for (unsigned int internal_param_idx = 0;
       internal_param_idx < parameter_order.size(); ++internal_param_idx) {
    const citcpp::parameter &param = parameter_order[internal_param_idx];
    // Find the parameter in the user input model, in particular its index.
    for (unsigned int user_param_index = 0;
         user_param_index < input_model.get_parameters().size();
         ++user_param_index) {
      if (input_model.get_parameters()[user_param_index] == param) {
        parameter_index_map.push_back(user_param_index);
        break;
      }
    }
  }

  return parameter_index_map;
}

}  // namespace

namespace citcpp {
namespace detail {

model::model(const input_model &input_model,
             const std::vector<parameter> &ordered_parameters)
    : input_model_(input_model),
      parameter_index_map_(
          compute_parameter_index_map(input_model, ordered_parameters)),
      parameters_() {
  for (const parameter &p : ordered_parameters) {
    parameters_.push_back(p.get_values().size());
  }
}

const input_model &model::get_input_model() const { return input_model_; }

citcpp::test_set model::create_from_internal_test_set(
    const citcpp::detail::test_set &test_set) const {

  citcpp::test_set ret(DEFAULT_VALUE_SEPARATOR);

  convert_test_set(test_set, ret);

  return ret;
}

citcpp::test_set model::create_from_internal_test_set(
    const citcpp::detail::test_set &test_set,
    std::string_view value_separator) const {

  citcpp::test_set ret(value_separator);

  convert_test_set(test_set, ret);

  return ret;
}

void model::convert_test_set(const citcpp::detail::test_set &src,
                             citcpp::test_set &tgt) const {

  for (const parameter &param : input_model_.get_parameters()) {
    tgt.add_parameter(
        parameter_def().name(param.get_name()).type(param.get_type()));
  }

  for (const test &test : src.get_list_of_tests()) {
    tgt.get_list_of_tests().emplace_back(std::vector<parameter_value>(
        test.get_values().size(), DONT_CARE_PARAMETER_VALUE));
    convert_test(test, tgt.get_list_of_tests().back());
  }
}

void model::convert_test(const test &src,
                         std::vector<parameter_value> &tgt) const {

  for (test::size_type p = 0; p < src.get_values().size(); ++p) {
    int pv = src.get_values().at(p);
    unsigned param_index_in_model = parameter_index_map_[p];

    const parameter &param =
        input_model_.get_parameters()[param_index_in_model];
    if (pv >= 0 && (std::vector<parameter_value>::size_type)pv <
                       param.get_values().size()) {
      tgt[param_index_in_model] = param.get_values()[pv];
    } else {
      tgt[param_index_in_model] = DONT_CARE_PARAMETER_VALUE;
    }
  }
}

relation::relation(const std::vector<unsigned int> &parameters,
                   unsigned int specified_interaction_strength)
    : parameters_(parameters),
      specified_interaction_strength_(specified_interaction_strength),
      current_interaction_strength_(1) {}

}  // namespace detail
}  // namespace citcpp
