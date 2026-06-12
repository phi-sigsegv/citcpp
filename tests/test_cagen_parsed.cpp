#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest.h>

#include <chrono>
#include <citcpp/acts_model_parser.hpp>
#include <citcpp/citcpp.hpp>
#include <duration_wrapper.hpp>
#include <iostream>
#include <sstream>

namespace {

citcpp::model create_acts_example_model() {
  using namespace citcpp;

  std::stringstream s;

  s << "[System]\n"
    << "Name: TCAS\n"
    << "\n"
    << "[Parameter]\n"
    << "Cur_Vertical_Sep (int) : 299, 300, 601\n"
    << "High_Confidence (boolean) : TRUE, FALSE\n"
    << "Two_of_Three_Reports_Valid (boolean) : TRUE, FALSE\n"
    << "Own_Tracked_Alt (int) : 1, 2\n"
    << "Other_Tracked_Alt (int) : 1, 2\n"
    << "Own_Tracked_Alt_Rate (int) : 600, 601\n"
    << "Alt_Layer_Value (int) : 0, 1, 2, 3\n"
    << "Up_Separation (int) : 0, 399, 400, 499, 500, 639, 640, 739, 740, 840\n"
    << "Down_Separation (int) : 0, 399, 400, 499, 500, 639, 640, 739, 740, "
       "840\n"
    << "Other_RAC (enum) : NO_INTENT, DO_NOT_CLIMB, DO_NOT_DESCEND\n"
    << "Other_Capability (enum) : TCAS_TA, OTHER\n"
    << "Climb_Inhibit (boolean) : TRUE, FALSE\n"
    << "\n"
    << "[Constraint]\n"
    << "Cur_Vertical_Sep != 299 => Other_Capability != \"OTHER\"\n"
    << "Climb_Inhibit = true => Up_Separation > 399" << std::endl;

  std::string model_str = s.str();

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
  }
}
