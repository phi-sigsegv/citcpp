#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest.h>

#include <chrono>
#include <citcpp/acts_model_parser.hpp>
#include <citcpp/citcpp.hpp>
#include <duration_wrapper.hpp>
#include <iostream>

namespace {

citcpp::model create_acts_example_model() {
  using namespace citcpp;

  std::string model_str = R"([System]
-- specify system name
Name: TCAS

[Parameter]
-- general syntax is parameter_name (type) : value1, value2, ...
Cur_Vertical_Sep (int) : 299, 300, 601
High_Confidence (boolean) : TRUE, FALSE
Two_of_Three_Reports_Valid (boolean) : TRUE, FALSE
Own_Tracked_Alt (int) : 1, 2
Other_Tracked_Alt (int) : 1, 2
Own_Tracked_Alt_Rate (int) : 600, 601
Alt_Layer_Value (int) : 0, 1, 2, 3
Up_Separation (int) : 0, 399, 400, 499, 500, 639, 640, 739, 740, 840
Down_Separation (int) : 0, 399, 400, 499, 500, 639, 640, 739, 740, 840
Other_RAC (enum) : NO_INTENT, DO_NOT_CLIMB, DO_NOT_DESCEND
Other_Capability (enum) : TCAS_TA, OTHER
Climb_Inhibit (boolean) : TRUE, FALSE

[Constraint]
-- this section is also optional
Cur_Vertical_Sep != 299 => Other_Capability != "OTHER"
Climb_Inhibit = true => Up_Separation > 399
)";

  acts_model_parser acts_parser;

  citcpp::model model;
  acts_parser.parse_input_model(model_str, model);

  return model;
}

citcpp::model create_spinv_model() {
  using namespace citcpp;

  const std::string model_str = R"([System]
Name: benchmark_spinv

[Parameter]
p0 (int) : 0, 1
p1 (int) : 0, 1
p2 (int) : 0, 1
p3 (int) : 0, 1
p4 (int) : 0, 1
p5 (int) : 0, 1
p6 (int) : 0, 1
p7 (int) : 0, 1
p8 (int) : 0, 1
p9 (int) : 0, 1
p10 (int) : 0, 1
p11 (int) : 0, 1
p12 (int) : 0, 1
p13 (int) : 0, 1
p14 (int) : 0, 1
p15 (int) : 0, 1
p16 (int) : 0, 1
p17 (int) : 0, 1
p18 (int) : 0, 1
p19 (int) : 0, 1
p20 (int) : 0, 1
p21 (int) : 0, 1
p22 (int) : 0, 1
p23 (int) : 0, 1
p24 (int) : 0, 1
p25 (int) : 0, 1
p26 (int) : 0, 1
p27 (int) : 0, 1
p28 (int) : 0, 1
p29 (int) : 0, 1
p30 (int) : 0, 1
p31 (int) : 0, 1
p32 (int) : 0, 1
p33 (int) : 0, 1
p34 (int) : 0, 1
p35 (int) : 0, 1
p36 (int) : 0, 1
p37 (int) : 0, 1
p38 (int) : 0, 1
p39 (int) : 0, 1
p40 (int) : 0, 1
p41 (int) : 0, 1
p42 (int) : 0, 1, 2
p43 (int) : 0, 1, 2
p44 (int) : 0, 1, 2, 3
p45 (int) : 0, 1, 2, 3
p46 (int) : 0, 1, 2, 3
p47 (int) : 0, 1, 2, 3
p48 (int) : 0, 1, 2, 3
p49 (int) : 0, 1, 2, 3
p50 (int) : 0, 1, 2, 3
p51 (int) : 0, 1, 2, 3
p52 (int) : 0, 1, 2, 3
p53 (int) : 0, 1, 2, 3
p54 (int) : 0, 1, 2, 3

[Constraint]
(p1 != 1) || (p42 != 1)
(p1 != 1) || (p42 != 2)
(p42 != 1) || (p43 != 1)
(p42 != 1) || (p43 != 2)
(p42 != 2) || (p43 != 1)
(p42 != 2) || (p43 != 2)
(p1 != 1) || (p43 != 1)
(p1 != 1) || (p43 != 2)
(p2 != 1) || (p3 != 1)
(p2 != 1) || (p4 != 1)
(p2 != 1) || (p5 != 1)
(p3 != 1) || (p4 != 1)
(p3 != 1) || (p5 != 1)
(p4 != 1) || (p5 != 1)
(p44 != 1) || (p45 != 1)
(p44 != 1) || (p45 != 2)
(p44 != 1) || (p45 != 3)
(p44 != 2) || (p45 != 1)
(p44 != 2) || (p45 != 2)
(p44 != 2) || (p45 != 3)
(p44 != 3) || (p45 != 1)
(p44 != 3) || (p45 != 2)
(p44 != 3) || (p45 != 3)
(p6 != 1) || (p7 != 1)
(p8 != 1) || (p9 != 1)
(p8 != 1) || (p10 != 1)
(p11 != 1) || (p12 != 1)
(p6 != 1) || (p46 != 1)
(p6 != 1) || (p46 != 2)
(p6 != 1) || (p46 != 3)
(p13 != 1) || (p14 != 1)
(p10 != 1) || (p15 != 1)
(p9 != 1) || (p15 != 0)
(p16 != 1) || (p17 != 0)
(p14 != 0) || (p18 != 1)
(p10 != 1) || (p14 != 0)
(p9 != 0) || (p15 != 1)
(p12 != 0) || (p20 != 1)
(p12 != 0) || (p21 != 1)
(p12 != 0) || (p22 != 1)
(p23 != 1) || (p47 != 0)
(p48 != 1) || (p49 != 0)
(p48 != 2) || (p49 != 0)
(p48 != 3) || (p49 != 0)
(p24 != 1) || (p49 != 0)
(p16 != 0) || (p17 != 1)
(p12 != 0) || (p25 != 1)
(p12 != 1) || (p21 != 0) || (p26 != 1)
(p7 != 1) || (p9 != 0) || (p10 != 0)
)";

  acts_model_parser acts_parser;

  citcpp::model model;
  acts_parser.parse_input_model(model_str, model);

  return model;
}

}  // namespace

TEST_CASE("cagen, testing ACTS example model, strength 1") {
  using namespace citcpp;

  model model{create_acts_example_model()};

  test_set ipog_test_set;
  {
    std::unique_ptr<cagen_exec_handle_ipog> handle =
        compute_covering_array_ipog(model, 1,
                                    covering_array_computation_config());
    auto f = handle->get_test_set();
    cagen_exec_result result(f.get());
    ipog_test_set = result.get_result();

    CHECK(result.get_result_code() == cagen_exec_result::cagen_result_code::
                                          COVERING_ARRAY_GENERATION_COMPLETED);

    std::cout << "Test set generated using IPOG in "
              << duration_wrapper(std::chrono::milliseconds(
                     handle->get_duration_in_milli_seconds()))
              << " and has " << ipog_test_set.get_list_of_tests().size()
              << " rows." << std::endl;

    // The parameter with the largest number of values has 10 values.
    // Thus, for 1-way coverage we shall get a testset with exactly
    // 10 rows.
    CHECK(ipog_test_set.get_list_of_tests().size() == 10);

    std::unique_ptr<covm_exec_handle> covm_handle = measure_coverage(
        model, ipog_test_set, 1, coverage_measurement_config());
    auto covm_f = covm_handle->get_coverage_measurement();
    covm_exec_result covm_result(covm_f.get());
    const coverage_measurement& covm = covm_result.get_result().at("");

    CHECK(covm_result.get_result_code() ==
          covm_exec_result::covm_result_code::COVERAGE_MEASUREMENT_COMPLETED);

    std::cout << "Coverage measured in "
              << duration_wrapper(std::chrono::milliseconds(
                     covm_handle->get_duration_in_milli_seconds()))
              << std::endl;

    CHECK(covm.get_covered_tuples()[covm.get_covered_tuples().size() - 1] ==
          44);

    CHECK(covm_result.get_invalid_test_indices().empty());
    CHECK(covm.get_covered_tuples()[covm.get_covered_tuples().size() - 1] ==
          covm.get_number_of_combinations_to_cover());
    CHECK(covm[1.0] == covm.get_number_of_param_combos_to_cover());
  }
}

TEST_CASE("cagen, testing ACTS example model, strength 2") {
  using namespace citcpp;

  model model{create_acts_example_model()};

  test_set ipog_test_set;
  {
    std::unique_ptr<cagen_exec_handle_ipog> handle =
        compute_covering_array_ipog(model, 2,
                                    covering_array_computation_config());
    auto f = handle->get_test_set();
    cagen_exec_result result(f.get());
    ipog_test_set = result.get_result();

    CHECK(result.get_result_code() == cagen_exec_result::cagen_result_code::
                                          COVERING_ARRAY_GENERATION_COMPLETED);

    std::cout << "Test set generated using IPOG in "
              << duration_wrapper(std::chrono::milliseconds(
                     handle->get_duration_in_milli_seconds()))
              << " and has " << ipog_test_set.get_list_of_tests().size()
              << " rows." << std::endl;

    std::unique_ptr<covm_exec_handle> covm_handle = measure_coverage(
        model, ipog_test_set, 2, coverage_measurement_config());
    auto covm_f = covm_handle->get_coverage_measurement();
    covm_exec_result covm_result(covm_f.get());
    const coverage_measurement& covm = covm_result.get_result().at("");

    CHECK(covm_result.get_result_code() ==
          covm_exec_result::covm_result_code::COVERAGE_MEASUREMENT_COMPLETED);

    std::cout << "Coverage measured in "
              << duration_wrapper(std::chrono::milliseconds(
                     covm_handle->get_duration_in_milli_seconds()))
              << std::endl;

    CHECK(covm.get_covered_tuples()[covm.get_covered_tuples().size() - 1] ==
          833);

    CHECK(covm_result.get_invalid_test_indices().empty());
    CHECK(covm.get_covered_tuples()[covm.get_covered_tuples().size() - 1] ==
          covm.get_number_of_combinations_to_cover());
    CHECK(covm[1.0] == covm.get_number_of_param_combos_to_cover());
  }
}

TEST_CASE("cagen, testing ACTS example model, strength 3") {
  using namespace citcpp;

  model model{create_acts_example_model()};

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

    std::cout << "Test set generated using IPOG in "
              << duration_wrapper(std::chrono::milliseconds(
                     handle->get_duration_in_milli_seconds()))
              << " and has " << ipog_test_set.get_list_of_tests().size()
              << " rows." << std::endl;

    std::unique_ptr<covm_exec_handle> covm_handle = measure_coverage(
        model, ipog_test_set, 3, coverage_measurement_config());
    auto covm_f = covm_handle->get_coverage_measurement();
    covm_exec_result covm_result(covm_f.get());
    const coverage_measurement& covm = covm_result.get_result().at("");

    CHECK(covm_result.get_result_code() ==
          covm_exec_result::covm_result_code::COVERAGE_MEASUREMENT_COMPLETED);

    std::cout << "Coverage measured in "
              << duration_wrapper(std::chrono::milliseconds(
                     covm_handle->get_duration_in_milli_seconds()))
              << std::endl;

    CHECK(covm.get_covered_tuples()[covm.get_covered_tuples().size() - 1] ==
          9016);

    CHECK(covm_result.get_invalid_test_indices().empty());
    CHECK(covm.get_covered_tuples()[covm.get_covered_tuples().size() - 1] ==
          covm.get_number_of_combinations_to_cover());
    CHECK(covm[1.0] == covm.get_number_of_param_combos_to_cover());
  }
}

TEST_CASE("cagen, testing spinv model, strength 3, sequential") {
  using namespace citcpp;

  model model{create_spinv_model()};

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

    std::cout << "Test set generated using IPOG in "
              << duration_wrapper(std::chrono::milliseconds(
                     handle->get_duration_in_milli_seconds()))
              << " and has " << ipog_test_set.get_list_of_tests().size()
              << " rows." << std::endl;

    std::unique_ptr<covm_exec_handle> covm_handle = measure_coverage(
        model, ipog_test_set, 3, coverage_measurement_config());
    auto covm_f = covm_handle->get_coverage_measurement();
    covm_exec_result covm_result(covm_f.get());
    const coverage_measurement& covm = covm_result.get_result().at("");

    CHECK(covm_result.get_result_code() ==
          covm_exec_result::covm_result_code::COVERAGE_MEASUREMENT_COMPLETED);

    std::cout << "Coverage measured in "
              << duration_wrapper(std::chrono::milliseconds(
                     covm_handle->get_duration_in_milli_seconds()))
              << std::endl;

    CHECK(covm_result.get_invalid_test_indices().empty());
    CHECK(covm.get_covered_tuples()[covm.get_covered_tuples().size() - 1] ==
          covm.get_number_of_combinations_to_cover());
    CHECK(covm[1.0] == covm.get_number_of_param_combos_to_cover());
  }
}

TEST_CASE("cagen, testing spinv model, strength 3, parallel") {
  using namespace citcpp;

  model model{create_spinv_model()};

  test_set ipog_test_set;
  {
    covering_array_computation_config config;
    config.with_number_of_threads(2);
    std::unique_ptr<cagen_exec_handle_ipog> handle =
        compute_covering_array_ipog(model, 3, config);
    auto f = handle->get_test_set();
    cagen_exec_result result(f.get());
    ipog_test_set = result.get_result();

    CHECK(result.get_result_code() == cagen_exec_result::cagen_result_code::
                                          COVERING_ARRAY_GENERATION_COMPLETED);

    std::cout << "Test set generated using IPOG in "
              << duration_wrapper(std::chrono::milliseconds(
                     handle->get_duration_in_milli_seconds()))
              << " and has " << ipog_test_set.get_list_of_tests().size()
              << " rows." << std::endl;

    std::unique_ptr<covm_exec_handle> covm_handle = measure_coverage(
        model, ipog_test_set, 3, coverage_measurement_config());
    auto covm_f = covm_handle->get_coverage_measurement();
    covm_exec_result covm_result(covm_f.get());
    const coverage_measurement& covm = covm_result.get_result().at("");

    CHECK(covm_result.get_result_code() ==
          covm_exec_result::covm_result_code::COVERAGE_MEASUREMENT_COMPLETED);

    std::cout << "Coverage measured in "
              << duration_wrapper(std::chrono::milliseconds(
                     covm_handle->get_duration_in_milli_seconds()))
              << std::endl;

    CHECK(covm_result.get_invalid_test_indices().empty());
    CHECK(covm.get_covered_tuples()[covm.get_covered_tuples().size() - 1] ==
          covm.get_number_of_combinations_to_cover());
    CHECK(covm[1.0] == covm.get_number_of_param_combos_to_cover());
  }
}
