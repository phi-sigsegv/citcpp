#include "internal_model.hpp"

#include "citcpp_utils.hpp"

namespace citcpp {
namespace detail {

internal_model::internal_model(const model &input_model)
    : input_model_(input_model), parameters_() {
  for (const parameter &p : input_model.get_parameters()) {
    parameters_.push_back(p.get_values().size());
  }
}

const model &internal_model::get_input_model() const { return input_model_; }

test_set internal_model::create_from_internal_test_set(
    const internal_test_set &int_test_set) const {

  test_set ret(DEFAULT_VALUE_SEPARATOR);

  convert_test_set(int_test_set, ret);

  return ret;
}

test_set internal_model::create_from_internal_test_set(
    const internal_test_set &int_test_set,
    std::string_view value_separator) const {

  test_set ret(value_separator);

  convert_test_set(int_test_set, ret);

  return ret;
}

void internal_model::convert_test_set(const internal_test_set &src,
                                      test_set &tgt) const {

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

void internal_model::convert_test(const test &src,
                                  std::vector<parameter_value> &tgt) const {

  for (test::size_type p = 0; p < src.get_values().size(); ++p) {
    int pv = src.get_values().at(p);

    const parameter &param = input_model_.get_parameters()[p];
    if (pv >= 0 && (std::vector<parameter_value>::size_type)pv <
                       param.get_values().size()) {
      tgt[p] = param.get_values()[pv];
    } else {
      tgt[p] = DONT_CARE_PARAMETER_VALUE;
    }
  }
}

internal_relation::internal_relation(
    const std::vector<unsigned int> &parameters,
    unsigned int specified_interaction_strength)
    : parameters_(parameters),
      specified_interaction_strength_(specified_interaction_strength),
      current_interaction_strength_(1),
      current_param_idx_(0) {}

}  // namespace detail
}  // namespace citcpp
