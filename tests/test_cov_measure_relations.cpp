#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest.h>

#include <chrono>
#include <citcpp/citcpp.hpp>
#include <duration_wrapper.hpp>
#include <iostream>

namespace {

citcpp::model create_pict_example_model() {
  using namespace citcpp;

  model model;

  model.add_parameter(parameter()
                          .type(parameter_type::ENUM)
                          .name("PLATFORM")
                          .values({{"x86"}, {"x64"}, {"arm"}}));
  model.add_parameter(parameter()
                          .type(parameter_type::INTEGER)
                          .name("CPUS")
                          .values({{1}, {2}, {4}}));
  model.add_parameter(parameter()
                          .type(parameter_type::ENUM)
                          .name("RAM")
                          .values({{"1GB"}, {"4GB"}, {"64GB"}}));
  model.add_parameter(parameter()
                          .type(parameter_type::ENUM)
                          .name("HDD")
                          .values({{"SCSI"}, {"IDE"}}));
  model.add_parameter(parameter()
                          .type(parameter_type::ENUM)
                          .name("OS")
                          .values({{"Win7"}, {"Win8"}, {"Win10"}}));
  model.add_parameter(
      parameter()
          .type(parameter_type::ENUM)
          .name("Browser")
          .values({{"Edge"}, {"Opera"}, {"Chrome"}, {"Firefox"}}));
  model.add_parameter(parameter()
                          .type(parameter_type::ENUM)
                          .name("APP")
                          .values({{"Word"}, {"Excel"}, {"Powerpoint"}}));

  return model;
}

citcpp::relation create_relation(const citcpp::model& model,
                                 std::string_view name,
                                 std::vector<std::string> param_names,
                                 unsigned int interaction_strength) {

  using namespace citcpp;

  relation rel;

  rel.set_name(name);

  for (const auto& param_name : param_names) {
    for (const auto& param : model.get_parameters()) {
      if (param_name == param.get_name()) {
        rel.add_parameter(param);
      }
    }
  }

  rel.set_interaction_strength(interaction_strength);

  return rel;
}

}  // namespace

TEST_CASE("covm, testing PICT example model, relation with strength 1") {
  using namespace citcpp;

  model model{create_pict_example_model()};

  std::unique_ptr<cagen_exec_handle_ipog> handle = compute_covering_array_ipog(
      model, 1, covering_array_computation_config());
  auto f = handle->get_test_set();
  cagen_exec_result result(f.get());
  test_set ipog_test_set = result.get_result();

  CHECK(result.get_result_code() == cagen_exec_result::cagen_result_code::
                                        COVERING_ARRAY_GENERATION_COMPLETED);

  std::cout << "Test set generated using IPOG in "
            << duration_wrapper(std::chrono::milliseconds(
                   handle->get_duration_in_milli_seconds()))
            << " and has " << ipog_test_set.get_list_of_tests().size()
            << " rows." << std::endl;

  // The parameter with the largest number of values has 4 values.
  // Thus, for 1-way coverage we shall get a testset with exactly
  // four rows.
  CHECK(ipog_test_set.get_list_of_tests().size() == 4);

  model.add_relation(
      create_relation(model, "R1", {"PLATFORM", "CPUS", "RAM", "HDD"}, 1));
  std::unique_ptr<covm_exec_handle> covm_handle =
      measure_coverage(model, ipog_test_set, -1);

  std::cout << "Coverage measured in "
            << duration_wrapper(std::chrono::milliseconds(
                   covm_handle->get_duration_in_milli_seconds()))
            << std::endl;

  auto covm_f = covm_handle->get_coverage_measurement();
  covm_exec_result covm_result(covm_f.get());
  CHECK(covm_result.get_result_code() ==
        covm_exec_result::covm_result_code::COVERAGE_MEASUREMENT_COMPLETED);

  const coverage_measurement& covm = covm_result.get_result().at("R1");
  CHECK(covm.get_covered_tuples()[covm.get_covered_tuples().size() - 1] ==
        covm.get_number_of_combinations_to_cover());
  CHECK(covm[1.0] == covm.get_number_of_param_combos_to_cover());
}

TEST_CASE("covm, testing PICT example model, relation with strength 2") {
  using namespace citcpp;

  model model{create_pict_example_model()};

  std::unique_ptr<cagen_exec_handle_ipog> handle = compute_covering_array_ipog(
      model, 2, covering_array_computation_config());
  auto f = handle->get_test_set();
  cagen_exec_result result(f.get());
  test_set ipog_test_set = result.get_result();

  CHECK(result.get_result_code() == cagen_exec_result::cagen_result_code::
                                        COVERING_ARRAY_GENERATION_COMPLETED);

  std::cout << "Test set generated using IPOG in "
            << duration_wrapper(std::chrono::milliseconds(
                   handle->get_duration_in_milli_seconds()))
            << " and has " << ipog_test_set.get_list_of_tests().size()
            << " rows." << std::endl;

  model.add_relation(
      create_relation(model, "R2", {"PLATFORM", "CPUS", "RAM", "HDD"}, 2));
  std::unique_ptr<covm_exec_handle> covm_handle =
      measure_coverage(model, ipog_test_set, -1);

  std::cout << "Coverage measured in "
            << duration_wrapper(std::chrono::milliseconds(
                   covm_handle->get_duration_in_milli_seconds()))
            << std::endl;

  auto covm_f = covm_handle->get_coverage_measurement();
  covm_exec_result covm_result(covm_f.get());
  CHECK(covm_result.get_result_code() ==
        covm_exec_result::covm_result_code::COVERAGE_MEASUREMENT_COMPLETED);

  const coverage_measurement& covm = covm_result.get_result().at("R2");
  CHECK(covm.get_covered_tuples()[covm.get_covered_tuples().size() - 1] ==
        covm.get_number_of_combinations_to_cover());
  CHECK(covm[1.0] == covm.get_number_of_param_combos_to_cover());
}

TEST_CASE("covm, testing PICT example model, relation with strength 3") {
  using namespace citcpp;

  model model{create_pict_example_model()};

  std::unique_ptr<cagen_exec_handle_ipog> handle = compute_covering_array_ipog(
      model, 3, covering_array_computation_config());
  auto f = handle->get_test_set();
  cagen_exec_result result(f.get());
  test_set ipog_test_set = result.get_result();

  CHECK(result.get_result_code() == cagen_exec_result::cagen_result_code::
                                        COVERING_ARRAY_GENERATION_COMPLETED);

  std::cout << "Test set generated using IPOG in "
            << duration_wrapper(std::chrono::milliseconds(
                   handle->get_duration_in_milli_seconds()))
            << " and has " << ipog_test_set.get_list_of_tests().size()
            << " rows." << std::endl;

  model.add_relation(
      create_relation(model, "R3", {"PLATFORM", "CPUS", "RAM", "HDD"}, 3));
  std::unique_ptr<covm_exec_handle> covm_handle =
      measure_coverage(model, ipog_test_set, -1);

  std::cout << "Coverage measured in "
            << duration_wrapper(std::chrono::milliseconds(
                   covm_handle->get_duration_in_milli_seconds()))
            << std::endl;

  auto covm_f = covm_handle->get_coverage_measurement();
  covm_exec_result covm_result(covm_f.get());
  CHECK(covm_result.get_result_code() ==
        covm_exec_result::covm_result_code::COVERAGE_MEASUREMENT_COMPLETED);

  const coverage_measurement& covm = covm_result.get_result().at("R3");
  CHECK(covm.get_covered_tuples()[covm.get_covered_tuples().size() - 1] ==
        covm.get_number_of_combinations_to_cover());
  CHECK(covm[1.0] == covm.get_number_of_param_combos_to_cover());
}

TEST_CASE("covm, testing PICT example model, relation with strength 4") {
  using namespace citcpp;

  model model{create_pict_example_model()};

  std::unique_ptr<cagen_exec_handle_ipog> handle = compute_covering_array_ipog(
      model, 4, covering_array_computation_config());
  auto f = handle->get_test_set();
  cagen_exec_result result(f.get());
  test_set ipog_test_set = result.get_result();

  CHECK(result.get_result_code() == cagen_exec_result::cagen_result_code::
                                        COVERING_ARRAY_GENERATION_COMPLETED);

  std::cout << "Test set generated using IPOG in "
            << duration_wrapper(std::chrono::milliseconds(
                   handle->get_duration_in_milli_seconds()))
            << " and has " << ipog_test_set.get_list_of_tests().size()
            << " rows." << std::endl;

  model.add_relation(
      create_relation(model, "R4", {"PLATFORM", "CPUS", "RAM", "HDD"}, 4));
  std::unique_ptr<covm_exec_handle> covm_handle =
      measure_coverage(model, ipog_test_set, -1);

  std::cout << "Coverage measured in "
            << duration_wrapper(std::chrono::milliseconds(
                   covm_handle->get_duration_in_milli_seconds()))
            << std::endl;

  auto covm_f = covm_handle->get_coverage_measurement();
  covm_exec_result covm_result(covm_f.get());
  CHECK(covm_result.get_result_code() ==
        covm_exec_result::covm_result_code::COVERAGE_MEASUREMENT_COMPLETED);

  const coverage_measurement& covm = covm_result.get_result().at("R4");
  CHECK(covm.get_covered_tuples()[covm.get_covered_tuples().size() - 1] ==
        covm.get_number_of_combinations_to_cover());
  CHECK(covm[1.0] == covm.get_number_of_param_combos_to_cover());
}

TEST_CASE("covm, testing PICT example model, mixed strength non-overlapping") {
  using namespace citcpp;

  model model{create_pict_example_model()};

  std::unique_ptr<cagen_exec_handle_ipog> handle = compute_covering_array_ipog(
      model, 3, covering_array_computation_config());
  auto f = handle->get_test_set();
  cagen_exec_result result(f.get());
  test_set ipog_test_set = result.get_result();

  CHECK(result.get_result_code() == cagen_exec_result::cagen_result_code::
                                        COVERING_ARRAY_GENERATION_COMPLETED);

  std::cout << "Test set generated using IPOG in "
            << duration_wrapper(std::chrono::milliseconds(
                   handle->get_duration_in_milli_seconds()))
            << " and has " << ipog_test_set.get_list_of_tests().size()
            << " rows." << std::endl;

  model.add_relation(create_relation(model, "R2", {"CPUS", "RAM", "HDD"}, 2));
  model.add_relation(
      create_relation(model, "R3", {"PLATFORM", "OS", "Browser"}, 3));
  std::unique_ptr<covm_exec_handle> covm_handle =
      measure_coverage(model, ipog_test_set, -1);

  std::cout << "Coverage measured in "
            << duration_wrapper(std::chrono::milliseconds(
                   covm_handle->get_duration_in_milli_seconds()))
            << std::endl;

  auto covm_f = covm_handle->get_coverage_measurement();
  covm_exec_result covm_result(covm_f.get());
  CHECK(covm_result.get_result_code() ==
        covm_exec_result::covm_result_code::COVERAGE_MEASUREMENT_COMPLETED);

  {
    const coverage_measurement& covm = covm_result.get_result().at("R2");
    CHECK(covm.get_covered_tuples()[covm.get_covered_tuples().size() - 1] ==
          covm.get_number_of_combinations_to_cover());
    CHECK(covm[1.0] == covm.get_number_of_param_combos_to_cover());
  }
  {
    const coverage_measurement& covm = covm_result.get_result().at("R3");
    CHECK(covm.get_covered_tuples()[covm.get_covered_tuples().size() - 1] ==
          covm.get_number_of_combinations_to_cover());
    CHECK(covm[1.0] == covm.get_number_of_param_combos_to_cover());
  }
}

TEST_CASE("covm, testing PICT example model, mixed strength overlapping") {
  using namespace citcpp;

  model model{create_pict_example_model()};

  std::unique_ptr<cagen_exec_handle_ipog> handle = compute_covering_array_ipog(
      model, 3, covering_array_computation_config());
  auto f = handle->get_test_set();
  cagen_exec_result result(f.get());
  test_set ipog_test_set = result.get_result();

  CHECK(result.get_result_code() == cagen_exec_result::cagen_result_code::
                                        COVERING_ARRAY_GENERATION_COMPLETED);

  std::cout << "Test set generated using IPOG in "
            << duration_wrapper(std::chrono::milliseconds(
                   handle->get_duration_in_milli_seconds()))
            << " and has " << ipog_test_set.get_list_of_tests().size()
            << " rows." << std::endl;

  model.add_relation(
      create_relation(model, "R2", {"PLATFORM", "CPUS", "RAM", "HDD"}, 2));
  model.add_relation(
      create_relation(model, "R3", {"PLATFORM", "OS", "Browser"}, 3));
  std::unique_ptr<covm_exec_handle> covm_handle =
      measure_coverage(model, ipog_test_set, -1);

  std::cout << "Coverage measured in "
            << duration_wrapper(std::chrono::milliseconds(
                   covm_handle->get_duration_in_milli_seconds()))
            << std::endl;

  auto covm_f = covm_handle->get_coverage_measurement();
  covm_exec_result covm_result(covm_f.get());
  CHECK(covm_result.get_result_code() ==
        covm_exec_result::covm_result_code::COVERAGE_MEASUREMENT_COMPLETED);

  {
    const coverage_measurement& covm = covm_result.get_result().at("R2");
    CHECK(covm.get_covered_tuples()[covm.get_covered_tuples().size() - 1] ==
          covm.get_number_of_combinations_to_cover());
    CHECK(covm[1.0] == covm.get_number_of_param_combos_to_cover());
  }
  {
    const coverage_measurement& covm = covm_result.get_result().at("R3");
    CHECK(covm.get_covered_tuples()[covm.get_covered_tuples().size() - 1] ==
          covm.get_number_of_combinations_to_cover());
    CHECK(covm[1.0] == covm.get_number_of_param_combos_to_cover());
  }
}
