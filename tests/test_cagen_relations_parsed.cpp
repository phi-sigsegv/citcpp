#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest.h>

#include <chrono>
#include <citcpp/acts_model_parser.hpp>
#include <citcpp/citcpp.hpp>
#include <duration_wrapper.hpp>
#include <iostream>
#include <ranges>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace {

citcpp::model create_pict_example_model() {
  using namespace citcpp;

  std::stringstream s;

  s << "[System]\n"
    << "Name: PICT_example\n"
    << "\n"
    << "[Parameter]\n"
    << "PLATFORM (enum) : x86, x64, arm\n"
    << "CPUS (int) : 1, 2, 4\n"
    << "RAM (enum) : 1GB, 4GB, 64GB\n"
    << "HDD (enum) : SCSI, IDE\n"
    << "OS (enum) : Win7, Win8, Win10\n"
    << "Browser (enum) : Edge, Opera, Chrome, Firefox\n"
    << "APP (enum) : Word, Excel, Powerpoint" << std::endl;

  std::string model_str = s.str();

  acts_model_parser acts_parser;

  citcpp::model model;
  acts_parser.parse_input_model(model_str, model);

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

citcpp::model create_acts_example_model_with_relations() {
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
    << "[Relation]\n"
    << "R2: (Cur_Vertical_Sep, Other_RAC, Other_Capability, 2)\n"
    << "R3: (Cur_Vertical_Sep, Alt_Layer_Value, Other_Capability, "
       "Own_Tracked_Alt_Rate, "
       "3)\n"
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

void check_non_relation_params_are_dont_care(
    const citcpp::test_set& tests,
    std::unordered_set<std::string> relation_param_names) {

  using namespace citcpp;

  std::unordered_map<std::string, unsigned int> param_to_index_map;
  unsigned int param_idx = 0;
  for (const auto& param : tests.get_parameters()) {
    param_to_index_map[param.get_name()] = param_idx;
    ++param_idx;
  }

  auto not_in_relation_params = [&relation_param_names](const parameter& p) {
    return !relation_param_names.contains(p.get_name());
  };
  auto to_param_index = [&param_to_index_map](const parameter& p) {
    return param_to_index_map[p.get_name()];
  };
  std::vector<unsigned int> param_indices_expect_dont_care;
  for (const auto& param_idx : tests.get_parameters() |
                                   std::views::filter(not_in_relation_params) |
                                   std::views::transform(to_param_index)) {
    param_indices_expect_dont_care.push_back(param_idx);
  }

  for (const auto& t : tests.get_list_of_tests()) {
    for (unsigned int param_idx : param_indices_expect_dont_care) {
      CHECK(t[param_idx] == -1);
    }
  }
}

}  // namespace

TEST_CASE("cagen relations, testing PICT example model, R3") {
  using namespace citcpp;

  model model{create_pict_example_model()};
  model.add_relation(
      create_relation(model, "R3", {"PLATFORM", "CPUS", "RAM", "HDD"}, 3));

  test_set ipog_test_set;
  {
    std::unique_ptr<cagen_exec_handle_ipog> handle =
        compute_covering_array_ipog(
            model, -1,
            covering_array_computation_config().with_replace_dont_care_values(
                false));
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

    // Check all values in the created test set and whether they are don't care
    // for all parameters not contained in the relations.
    check_non_relation_params_are_dont_care(ipog_test_set,
                                            {"PLATFORM", "CPUS", "RAM", "HDD"});

    CHECK(handle->get_number_of_processed_parameters() == 4);
    CHECK(handle->get_number_of_covered_combinations() == 81);
  }
}

TEST_CASE("cagen relations, testing PICT example model, R4") {
  using namespace citcpp;

  model model{create_pict_example_model()};
  model.add_relation(
      create_relation(model, "R4", {"PLATFORM", "CPUS", "RAM", "HDD"}, 4));

  test_set ipog_test_set;
  {
    std::unique_ptr<cagen_exec_handle_ipog> handle =
        compute_covering_array_ipog(
            model, -1,
            covering_array_computation_config().with_replace_dont_care_values(
                false));
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

    // Check all values in the created test set and whether they are don't care
    // for all parameters not contained in the relations.
    check_non_relation_params_are_dont_care(ipog_test_set,
                                            {"PLATFORM", "CPUS", "RAM", "HDD"});

    CHECK(handle->get_number_of_processed_parameters() == 4);
    CHECK(handle->get_number_of_covered_combinations() == 54);
  }
}

TEST_CASE(
    "cagen relations, testing PICT example model, mixed strength "
    "non-overlapping") {
  using namespace citcpp;

  model model{create_pict_example_model()};
  model.add_relation(create_relation(model, "R2", {"CPUS", "RAM", "HDD"}, 2));
  model.add_relation(
      create_relation(model, "R3", {"PLATFORM", "OS", "Browser"}, 3));

  test_set ipog_test_set;
  {
    std::unique_ptr<cagen_exec_handle_ipog> handle =
        compute_covering_array_ipog(
            model, -1,
            covering_array_computation_config().with_replace_dont_care_values(
                false));
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

    // Check all values in the created test set and whether they are don't care
    // for all parameters not contained in the relations.
    check_non_relation_params_are_dont_care(
        ipog_test_set, {"PLATFORM", "CPUS", "RAM", "HDD", "OS", "Browser"});

    CHECK(handle->get_number_of_processed_parameters() == 6);
    CHECK(handle->get_number_of_covered_combinations() == 57);
  }
}

TEST_CASE(
    "cagen relations, testing PICT example model, mixed strength overlapping") {
  using namespace citcpp;

  model model{create_pict_example_model()};
  model.add_relation(
      create_relation(model, "R2", {"PLATFORM", "CPUS", "RAM", "HDD"}, 2));
  model.add_relation(
      create_relation(model, "R3", {"PLATFORM", "OS", "Browser"}, 3));

  test_set ipog_test_set;
  {
    std::unique_ptr<cagen_exec_handle_ipog> handle =
        compute_covering_array_ipog(
            model, -1,
            covering_array_computation_config().with_replace_dont_care_values(
                false));
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

    // Check all values in the created test set and whether they are don't care
    // for all parameters not contained in the relations.
    check_non_relation_params_are_dont_care(
        ipog_test_set, {"PLATFORM", "CPUS", "RAM", "HDD", "OS", "Browser"});

    CHECK(handle->get_number_of_processed_parameters() == 6);
    CHECK(handle->get_number_of_covered_combinations() == 81);
  }
}

TEST_CASE(
    "cagen relations, testing ACTS example model, mixed strength overlapping") {
  using namespace citcpp;

  model model{create_acts_example_model_with_relations()};

  test_set ipog_test_set;
  {
    std::unique_ptr<cagen_exec_handle_ipog> handle =
        compute_covering_array_ipog(model, -1,
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
        model, ipog_test_set, -1, coverage_measurement_config());
    auto covm_f = covm_handle->get_coverage_measurement();
    covm_exec_result covm_result(covm_f.get());

    std::cout << "Coverage measured in "
              << duration_wrapper(std::chrono::milliseconds(
                     covm_handle->get_duration_in_milli_seconds()))
              << std::endl;

    CHECK(covm_result.get_result_code() ==
          covm_exec_result::covm_result_code::COVERAGE_MEASUREMENT_COMPLETED);

    const coverage_measurement& covm_r2 = covm_result.get_result().at("R2");
    CHECK(
        covm_r2.get_covered_tuples()[covm_r2.get_covered_tuples().size() - 1] ==
        19);

    const coverage_measurement& covm_r3 = covm_result.get_result().at("R3");
    CHECK(
        covm_r3.get_covered_tuples()[covm_r3.get_covered_tuples().size() - 1] ==
        64);
  }
}
