#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest.h>

#include <chrono>
#include <citcpp/citcpp.hpp>
#include <duration_wrapper.hpp>
#include <iostream>

namespace {

citcpp::model create_acts_example_model() {
  using namespace citcpp;

  model model;

  model.set_name("TCAS");

  model.add_parameter(parameter()
                          .type(parameter_type::INTEGER)
                          .name("Cur_Vertical_Sep")
                          .values({{299}, {300}, {601}}));
  model.add_parameter(parameter()
                          .type(parameter_type::BOOLEAN)
                          .name("High_Confidence")
                          .values({{true}, {false}}));
  model.add_parameter(parameter()
                          .type(parameter_type::BOOLEAN)
                          .name("Two_of_Three_Reports_Valid")
                          .values({{true}, {false}}));
  model.add_parameter(parameter()
                          .type(parameter_type::INTEGER)
                          .name("Own_Tracked_Alt")
                          .values({{1}, {2}}));
  model.add_parameter(parameter()
                          .type(parameter_type::INTEGER)
                          .name("Other_Tracked_Alt")
                          .values({{1}, {2}}));
  model.add_parameter(parameter()
                          .type(parameter_type::INTEGER)
                          .name("Own_Tracked_Alt_Rate")
                          .values({{600}, {601}}));
  model.add_parameter(parameter()
                          .type(parameter_type::INTEGER)
                          .name("Alt_Layer_Value")
                          .values({{0}, {1}, {2}, {3}}));
  model.add_parameter(parameter()
                          .type(parameter_type::INTEGER)
                          .name("Up_Separation")
                          .values({{0},
                                   {399},
                                   {400},
                                   {499},
                                   {500},
                                   {639},
                                   {640},
                                   {739},
                                   {740},
                                   {840}}));
  model.add_parameter(parameter()
                          .type(parameter_type::INTEGER)
                          .name("Down_Separation")
                          .values({{0},
                                   {399},
                                   {400},
                                   {499},
                                   {500},
                                   {639},
                                   {640},
                                   {739},
                                   {740},
                                   {840}}));
  model.add_parameter(
      parameter()
          .type(parameter_type::ENUM)
          .name("Other_RAC")
          .values({{"NO_INTENT"}, {"DO_NOT_CLIMB"}, {"DO_NOT_DESCEND"}}));
  model.add_parameter(parameter()
                          .type(parameter_type::ENUM)
                          .name("Other_Capability")
                          .values({{"TCAS_TA"}, {"OTHER"}}));
  model.add_parameter(parameter()
                          .type(parameter_type::BOOLEAN)
                          .name("Climb_Inhibit")
                          .values({{true}, {false}}));

  auto Cur_Vertical_Sep_NEQ_299 = constraint_holder(
      std::make_unique<int_proposition>(parameter_reference("Cur_Vertical_Sep"),
                                        relational_operator::NEQ, 299));
  auto Other_Capability_NEQ_OTHER =
      constraint_holder(std::make_unique<enum_proposition>(
          parameter_reference("Other_Capability"), relational_operator::NEQ,
          "OTHER"));
  model.add_constraint(
      std::make_unique<implication>(std::move(Cur_Vertical_Sep_NEQ_299),
                                    std::move(Other_Capability_NEQ_OTHER)));

  auto Climb_Inhibit_EQ_TRUE =
      constraint_holder(std::make_unique<boolean_proposition>(
          parameter_reference("Climb_Inhibit"), relational_operator::EQ, true));
  auto Up_Separation_GT_399 =
      constraint_holder(std::make_unique<int_proposition>(
          parameter_reference("Up_Separation"), relational_operator::GT, 399));
  model.add_constraint(std::make_unique<implication>(
      std::move(Climb_Inhibit_EQ_TRUE), std::move(Up_Separation_GT_399)));

  return model;
}

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

  test_set ipog_otf_test_set;
  {
    std::unique_ptr<cagen_exec_handle_ipog> handle =
        compute_covering_array_ipog(
            model, 1,
            covering_array_computation_config().with_algorithm(
                covering_array_computation_algorithm::IPOG_OTF));
    auto f = handle->get_test_set();
    cagen_exec_result result(f.get());
    ipog_otf_test_set = result.get_result();

    CHECK(result.get_result_code() == cagen_exec_result::cagen_result_code::
                                          COVERING_ARRAY_GENERATION_COMPLETED);

    std::cout << "Test set generated using IPOG_OTF in "
              << duration_wrapper(std::chrono::milliseconds(
                     handle->get_duration_in_milli_seconds()))
              << " and has " << ipog_otf_test_set.get_list_of_tests().size()
              << " rows." << std::endl;

    // The parameter with the largest number of values has 10 values.
    // Thus, for 1-way coverage we shall get a testset with exactly
    // 10 rows.
    CHECK(ipog_otf_test_set.get_list_of_tests().size() == 10);
  }

  CHECK(ipog_test_set == ipog_otf_test_set);
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

  test_set ipog_otf_test_set;
  {
    std::unique_ptr<cagen_exec_handle_ipog> handle =
        compute_covering_array_ipog(
            model, 2,
            covering_array_computation_config().with_algorithm(
                covering_array_computation_algorithm::IPOG_OTF));
    auto f = handle->get_test_set();
    cagen_exec_result result(f.get());
    ipog_otf_test_set = result.get_result();

    CHECK(result.get_result_code() == cagen_exec_result::cagen_result_code::
                                          COVERING_ARRAY_GENERATION_COMPLETED);

    std::cout << "Test set generated using IPOG_OTF in "
              << duration_wrapper(std::chrono::milliseconds(
                     handle->get_duration_in_milli_seconds()))
              << " and has " << ipog_otf_test_set.get_list_of_tests().size()
              << " rows." << std::endl;
  }

  CHECK(ipog_test_set == ipog_otf_test_set);
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

}  // namespace
