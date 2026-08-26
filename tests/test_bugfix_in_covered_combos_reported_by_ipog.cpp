#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest.h>

#include <chrono>
#include <citcpp/acts_model_parser.hpp>
#include <citcpp/citcpp.hpp>
#include <duration_wrapper.hpp>
#include <iostream>

namespace {

citcpp::model create_model_with_single_valued_param_unconstrained() {
  using namespace citcpp;

  std::string model_str = R"([System]
Name: single_value_param_constraint_bug

[Parameter]
B (enum) : b1, b2, b3
A (enum) : a1, a2
C (enum) : c1, c2
Mode (enum) : on
)";

  acts_model_parser acts_parser;

  citcpp::model model;
  acts_parser.parse_input_model(model_str, model);

  return model;
}

citcpp::model create_model_with_single_valued_param_constrained() {
  using namespace citcpp;

  std::string model_str = R"([System]
Name: single_value_param_constraint_bug

[Parameter]
B (enum) : b1, b2, b3
A (enum) : a1, a2
C (enum) : c1, c2
Mode (enum) : on

[Constraint]
((Mode = "on" && A = "a1") => B != "b1")
)";

  acts_model_parser acts_parser;

  citcpp::model model;
  acts_parser.parse_input_model(model_str, model);

  return model;
}

}  // namespace

TEST_CASE(
    "cagen, testing unconstrained model with single valued param, strength 3") {
  using namespace citcpp;

  model model{create_model_with_single_valued_param_unconstrained()};

  test_set ipog_test_set;
  {
    std::unique_ptr<cagen_exec_handle_ipog> handle =
        compute_covering_array_ipog(model, 3,
                                    covering_array_computation_config());
    auto f = handle->get_test_set();
    cagen_exec_result result(f.get());
    ipog_test_set = result.get_result();

    CHECK(result.get_result_code() == cagen_exec_result::cagen_result_code::
                                          COVERING_ARRAY_GENERATION_COMPLETED);

    std::unique_ptr<covm_exec_handle> covm_handle = measure_coverage(
        model, ipog_test_set, 3, coverage_measurement_config());
    auto covm_f = covm_handle->get_coverage_measurement();
    covm_exec_result covm_result(covm_f.get());
    const coverage_measurement& covm = covm_result.get_result().at("");

    CHECK(covm_result.get_result_code() ==
          covm_exec_result::covm_result_code::COVERAGE_MEASUREMENT_COMPLETED);

    CHECK(handle->get_number_of_combinations_to_process() == 28);
    CHECK(handle->get_number_of_processed_combinations() == 28);

    CHECK(covm.get_covered_tuples()[covm.get_covered_tuples().size() - 1] ==
          28);
    CHECK(covm.get_covered_tuples()[covm.get_covered_tuples().size() - 1] ==
          handle->get_number_of_covered_combinations());

    CHECK(covm_result.get_invalid_test_indices().empty());
    CHECK(covm.get_covered_tuples()[covm.get_covered_tuples().size() - 1] ==
          covm.get_number_of_combinations_to_cover());
    CHECK(covm[1.0] == covm.get_number_of_param_combos_to_cover());
  }
}

TEST_CASE(
    "cagen, testing constrained model with single valued param, strength 3") {
  using namespace citcpp;

  model model{create_model_with_single_valued_param_constrained()};

  test_set ipog_test_set;
  {
    std::unique_ptr<cagen_exec_handle_ipog> handle =
        compute_covering_array_ipog(model, 3,
                                    covering_array_computation_config());
    auto f = handle->get_test_set();
    cagen_exec_result result(f.get());
    ipog_test_set = result.get_result();

    CHECK(result.get_result_code() == cagen_exec_result::cagen_result_code::
                                          COVERING_ARRAY_GENERATION_COMPLETED);

    std::unique_ptr<covm_exec_handle> covm_handle = measure_coverage(
        model, ipog_test_set, 3, coverage_measurement_config());
    auto covm_f = covm_handle->get_coverage_measurement();
    covm_exec_result covm_result(covm_f.get());
    const coverage_measurement& covm = covm_result.get_result().at("");

    CHECK(covm_result.get_result_code() ==
          covm_exec_result::covm_result_code::COVERAGE_MEASUREMENT_COMPLETED);

    CHECK(handle->get_number_of_combinations_to_process() == 28);
    CHECK(handle->get_number_of_processed_combinations() == 28);

    CHECK(covm.get_covered_tuples()[covm.get_covered_tuples().size() - 1] ==
          25);
    CHECK(covm.get_covered_tuples()[covm.get_covered_tuples().size() - 1] ==
          handle->get_number_of_covered_combinations());

    CHECK(covm_result.get_invalid_test_indices().empty());
    CHECK(covm.get_covered_tuples()[covm.get_covered_tuples().size() - 1] ==
          covm.get_number_of_combinations_to_cover());
    CHECK(covm[1.0] == covm.get_number_of_param_combos_to_cover());
  }
}
