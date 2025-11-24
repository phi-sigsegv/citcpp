#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest.h>

#include <chrono>
#include <citcpp/citcpp.hpp>
#include <iostream>
#include <ranges>
#include <unordered_map>
#include <unordered_set>

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

citcpp::model create_simple_four_param_model() {
  using namespace citcpp;

  model model;

  model.add_parameter(parameter()
                          .type(parameter_type::ENUM)
                          .name("P1")
                          .values({{"a"}, {"b"}, {"c"}, {"d"}, {"e"}}));
  model.add_parameter(parameter()
                          .type(parameter_type::ENUM)
                          .name("P2")
                          .values({{"a"}, {"b"}, {"c"}, {"d"}}));
  model.add_parameter(parameter()
                          .type(parameter_type::ENUM)
                          .name("P3")
                          .values({{"a"}, {"b"}, {"c"}}));
  model.add_parameter(
      parameter().type(parameter_type::ENUM).name("P4").values({{"a"}, {"b"}}));

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

const citcpp::parameter_value DONT_CARE_PARAMETER_VALUE{"*"};

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

  auto not_in_relation_params =
      [&relation_param_names](const parameter_def& p) {
        return !relation_param_names.contains(p.get_name());
      };
  auto to_param_index = [&param_to_index_map](const parameter_def& p) {
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
      CHECK(t[param_idx] == DONT_CARE_PARAMETER_VALUE);
    }
  }
}

}  // namespace

TEST_CASE("cagen relations, testing PICT example model, R1") {
  using namespace citcpp;

  model model{create_pict_example_model()};
  model.add_relation(
      create_relation(model, "R1", {"PLATFORM", "CPUS", "RAM", "HDD"}, 1));

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

    const auto duration_seconds = std::chrono::duration<double>(
        std::chrono::milliseconds(handle->get_duration_in_milli_seconds()));
    std::cout << "Test set generated using IPOG in " << duration_seconds
              << " and has " << ipog_test_set.get_list_of_tests().size()
              << " rows." << std::endl;

    // The parameter of the relation with the largest number of values has 3
    // values. Thus, for 1-way coverage we shall get a testset with exactly
    // three rows.
    CHECK(ipog_test_set.get_list_of_tests().size() == 3);

    // Check all values in the created test set and whether they are don't care
    // for all parameters not contained in the relations.
    check_non_relation_params_are_dont_care(ipog_test_set,
                                            {"PLATFORM", "CPUS", "RAM", "HDD"});

    CHECK(handle->get_number_of_processed_parameters() == 4);
    CHECK(handle->get_number_of_covered_combinations() == 11);
  }
}

TEST_CASE("cagen relations, testing PICT example model, R2") {
  using namespace citcpp;

  model model{create_pict_example_model()};
  model.add_relation(
      create_relation(model, "R2", {"PLATFORM", "CPUS", "RAM", "HDD"}, 2));

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

    const auto duration_seconds = std::chrono::duration<double>(
        std::chrono::milliseconds(handle->get_duration_in_milli_seconds()));
    std::cout << "Test set generated using IPOG in " << duration_seconds
              << " and has " << ipog_test_set.get_list_of_tests().size()
              << " rows." << std::endl;

    // Check all values in the created test set and whether they are don't care
    // for all parameters not contained in the relations.
    check_non_relation_params_are_dont_care(ipog_test_set,
                                            {"PLATFORM", "CPUS", "RAM", "HDD"});

    CHECK(handle->get_number_of_processed_parameters() == 4);
    CHECK(handle->get_number_of_covered_combinations() == 45);
  }
}

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

    const auto duration_seconds = std::chrono::duration<double>(
        std::chrono::milliseconds(handle->get_duration_in_milli_seconds()));
    std::cout << "Test set generated using IPOG in " << duration_seconds
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

    const auto duration_seconds = std::chrono::duration<double>(
        std::chrono::milliseconds(handle->get_duration_in_milli_seconds()));
    std::cout << "Test set generated using IPOG in " << duration_seconds
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
    "cagen relations, testing simple model, mixed strength non-overlapping") {
  using namespace citcpp;

  model model{create_simple_four_param_model()};
  model.add_relation(create_relation(model, "R2", {"P1", "P3"}, 2));
  model.add_relation(create_relation(model, "R1", {"P2"}, 1));

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

    const auto duration_seconds = std::chrono::duration<double>(
        std::chrono::milliseconds(handle->get_duration_in_milli_seconds()));
    std::cout << "Test set generated using IPOG in " << duration_seconds
              << " and has " << ipog_test_set.get_list_of_tests().size()
              << " rows." << std::endl;

    // Check all values in the created test set and whether they are don't care
    // for all parameters not contained in the relations.
    check_non_relation_params_are_dont_care(ipog_test_set, {"P1", "P2", "P3"});

    CHECK(handle->get_number_of_processed_parameters() == 3);
    CHECK(handle->get_number_of_covered_combinations() == 19);
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

    const auto duration_seconds = std::chrono::duration<double>(
        std::chrono::milliseconds(handle->get_duration_in_milli_seconds()));
    std::cout << "Test set generated using IPOG in " << duration_seconds
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

    const auto duration_seconds = std::chrono::duration<double>(
        std::chrono::milliseconds(handle->get_duration_in_milli_seconds()));
    std::cout << "Test set generated using IPOG in " << duration_seconds
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
