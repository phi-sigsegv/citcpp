#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest.h>

#include <chrono>
#include <citcpp/acts_model_parser.hpp>
#include <citcpp/citcpp.hpp>
#include <duration_wrapper.hpp>
#include <iostream>
#include <iterator>
#include <sstream>

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

citcpp::model create_acts_example_model_unconstrained() {
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
    << "Climb_Inhibit (boolean) : TRUE, FALSE" << std::endl;

  std::string model_str = s.str();

  acts_model_parser acts_parser;

  citcpp::model model;
  acts_parser.parse_input_model(model_str, model);

  return model;
}

}  // namespace

TEST_CASE(
    "cagen, testing PICT example model, strength 1, extend generated test "
    "set") {
  using namespace citcpp;

  model model{create_pict_example_model()};

  std::unique_ptr<cagen_exec_handle_ipog> cagen_handle =
      compute_covering_array_ipog(
          model, 1,
          covering_array_computation_config().with_replace_dont_care_values(
              false));
  auto cagen_f = cagen_handle->get_test_set();
  cagen_exec_result cagen_result(cagen_f.get());
  const test_set& t = cagen_result.get_result();

  CHECK(cagen_result.get_result_code() ==
        cagen_exec_result::cagen_result_code::
            COVERING_ARRAY_GENERATION_COMPLETED);

  std::cout << "Test set generated in "
            << duration_wrapper(std::chrono::milliseconds(
                   cagen_handle->get_duration_in_milli_seconds()))
            << " and has " << t.get_list_of_tests().size() << " rows."
            << std::endl;

  std::cout << "Now extending the exact same testset." << std::endl;

  std::unique_ptr<cagen_exec_handle_ipog> cagen_handle2 =
      compute_covering_array_ipog(
          model, t, 1,
          covering_array_computation_config().with_replace_dont_care_values(
              false));
  auto cagen_f2 = cagen_handle2->get_test_set();
  cagen_exec_result result2(cagen_f2.get());
  const test_set& t2 = result2.get_result();

  CHECK(result2.get_result_code() == cagen_exec_result::cagen_result_code::
                                         COVERING_ARRAY_GENERATION_COMPLETED);

  std::cout << "Test extended in "
            << duration_wrapper(std::chrono::milliseconds(
                   cagen_handle2->get_duration_in_milli_seconds()))
            << " and has " << t2.get_list_of_tests().size() << " rows."
            << std::endl;

  CHECK(t == t2);
}

TEST_CASE(
    "cagen, testing PICT example model, strength 2, extend generated test "
    "set") {
  using namespace citcpp;

  model model{create_pict_example_model()};

  std::unique_ptr<cagen_exec_handle_ipog> cagen_handle =
      compute_covering_array_ipog(
          model, 2,
          covering_array_computation_config().with_replace_dont_care_values(
              false));
  auto cagen_f = cagen_handle->get_test_set();
  cagen_exec_result cagen_result(cagen_f.get());
  const test_set& t = cagen_result.get_result();

  CHECK(cagen_result.get_result_code() ==
        cagen_exec_result::cagen_result_code::
            COVERING_ARRAY_GENERATION_COMPLETED);

  std::cout << "Test set generated in "
            << duration_wrapper(std::chrono::milliseconds(
                   cagen_handle->get_duration_in_milli_seconds()))
            << " and has " << t.get_list_of_tests().size() << " rows."
            << std::endl;

  std::cout << "Now extending the exact same testset." << std::endl;

  std::unique_ptr<cagen_exec_handle_ipog> cagen_handle2 =
      compute_covering_array_ipog(
          model, t, 2,
          covering_array_computation_config().with_replace_dont_care_values(
              false));
  auto cagen_f2 = cagen_handle2->get_test_set();
  cagen_exec_result result2(cagen_f2.get());
  const test_set& t2 = result2.get_result();

  CHECK(result2.get_result_code() == cagen_exec_result::cagen_result_code::
                                         COVERING_ARRAY_GENERATION_COMPLETED);

  std::cout << "Test extended in "
            << duration_wrapper(std::chrono::milliseconds(
                   cagen_handle2->get_duration_in_milli_seconds()))
            << " and has " << t2.get_list_of_tests().size() << " rows."
            << std::endl;

  CHECK(t == t2);
}

TEST_CASE(
    "cagen, testing PICT example model, strength 3, extend generated test "
    "set") {
  using namespace citcpp;

  model model{create_pict_example_model()};

  std::unique_ptr<cagen_exec_handle_ipog> cagen_handle =
      compute_covering_array_ipog(
          model, 3,
          covering_array_computation_config().with_replace_dont_care_values(
              false));
  auto cagen_f = cagen_handle->get_test_set();
  cagen_exec_result cagen_result(cagen_f.get());
  const test_set& t = cagen_result.get_result();

  CHECK(cagen_result.get_result_code() ==
        cagen_exec_result::cagen_result_code::
            COVERING_ARRAY_GENERATION_COMPLETED);

  std::cout << "Test set generated in "
            << duration_wrapper(std::chrono::milliseconds(
                   cagen_handle->get_duration_in_milli_seconds()))
            << " and has " << t.get_list_of_tests().size() << " rows."
            << std::endl;

  std::cout << "Now extending the exact same testset." << std::endl;

  std::unique_ptr<cagen_exec_handle_ipog> cagen_handle2 =
      compute_covering_array_ipog(
          model, t, 3,
          covering_array_computation_config().with_replace_dont_care_values(
              false));
  auto cagen_f2 = cagen_handle2->get_test_set();
  cagen_exec_result result2(cagen_f2.get());
  const test_set& t2 = result2.get_result();

  CHECK(result2.get_result_code() == cagen_exec_result::cagen_result_code::
                                         COVERING_ARRAY_GENERATION_COMPLETED);

  std::cout << "Test extended in "
            << duration_wrapper(std::chrono::milliseconds(
                   cagen_handle2->get_duration_in_milli_seconds()))
            << " and has " << t2.get_list_of_tests().size() << " rows."
            << std::endl;

  CHECK(t == t2);
}

TEST_CASE(
    "cagen, testing ACTS example model, strength 2, extend empty test "
    "set") {
  using namespace citcpp;

  model model{create_acts_example_model()};

  std::unique_ptr<cagen_exec_handle_ipog> cagen_handle =
      compute_covering_array_ipog(
          model, 2,
          covering_array_computation_config().with_replace_dont_care_values(
              false));
  auto cagen_f = cagen_handle->get_test_set();
  cagen_exec_result cagen_result(cagen_f.get());
  const test_set& t = cagen_result.get_result();

  CHECK(cagen_result.get_result_code() ==
        cagen_exec_result::cagen_result_code::
            COVERING_ARRAY_GENERATION_COMPLETED);

  std::cout << "Test set generated in "
            << duration_wrapper(std::chrono::milliseconds(
                   cagen_handle->get_duration_in_milli_seconds()))
            << " and has " << t.get_list_of_tests().size() << " rows."
            << std::endl;

  test_set empty_testset;
  for (const auto& param : model.get_parameters()) {
    empty_testset.add_parameter(param);
  }

  std::cout << "Now extending an empty testset." << std::endl;

  std::unique_ptr<cagen_exec_handle_ipog> cagen_handle2 =
      compute_covering_array_ipog(
          model, empty_testset, 2,
          covering_array_computation_config().with_replace_dont_care_values(
              false));
  auto cagen_f2 = cagen_handle2->get_test_set();
  cagen_exec_result result2(cagen_f2.get());
  const test_set& t2 = result2.get_result();

  CHECK(result2.get_result_code() == cagen_exec_result::cagen_result_code::
                                         COVERING_ARRAY_GENERATION_COMPLETED);

  std::cout << "Test extended in "
            << duration_wrapper(std::chrono::milliseconds(
                   cagen_handle2->get_duration_in_milli_seconds()))
            << " and has " << t2.get_list_of_tests().size() << " rows."
            << std::endl;

  CHECK(t == t2);
}

TEST_CASE(
    "cagen, testing unconstrained ACTS example model, strength 2, extend "
    "generated test "
    "set") {
  using namespace citcpp;

  model model{create_acts_example_model_unconstrained()};

  std::unique_ptr<cagen_exec_handle_ipog> cagen_handle =
      compute_covering_array_ipog(
          model, 2,
          covering_array_computation_config().with_replace_dont_care_values(
              false));
  auto cagen_f = cagen_handle->get_test_set();
  cagen_exec_result cagen_result(cagen_f.get());
  const test_set& t = cagen_result.get_result();

  CHECK(cagen_result.get_result_code() ==
        cagen_exec_result::cagen_result_code::
            COVERING_ARRAY_GENERATION_COMPLETED);

  std::cout << "Test set generated in "
            << duration_wrapper(std::chrono::milliseconds(
                   cagen_handle->get_duration_in_milli_seconds()))
            << " and has " << t.get_list_of_tests().size() << " rows."
            << std::endl;

  std::cout << "Now extending the exact same testset." << std::endl;

  std::unique_ptr<cagen_exec_handle_ipog> cagen_handle2 =
      compute_covering_array_ipog(
          model, t, 2,
          covering_array_computation_config().with_replace_dont_care_values(
              false));
  auto cagen_f2 = cagen_handle2->get_test_set();
  cagen_exec_result result2(cagen_f2.get());
  const test_set& t2 = result2.get_result();

  CHECK(result2.get_result_code() == cagen_exec_result::cagen_result_code::
                                         COVERING_ARRAY_GENERATION_COMPLETED);

  std::cout << "Test extended in "
            << duration_wrapper(std::chrono::milliseconds(
                   cagen_handle2->get_duration_in_milli_seconds()))
            << " and has " << t2.get_list_of_tests().size() << " rows."
            << std::endl;

  CHECK(t == t2);
}

TEST_CASE(
    "cagen, testing ACTS example model, strength 2, extend generated test "
    "set") {
  using namespace citcpp;

  model model{create_acts_example_model()};

  std::unique_ptr<cagen_exec_handle_ipog> cagen_handle =
      compute_covering_array_ipog(
          model, 2,
          covering_array_computation_config().with_replace_dont_care_values(
              false));
  auto cagen_f = cagen_handle->get_test_set();
  cagen_exec_result cagen_result(cagen_f.get());
  const test_set& t = cagen_result.get_result();

  CHECK(cagen_result.get_result_code() ==
        cagen_exec_result::cagen_result_code::
            COVERING_ARRAY_GENERATION_COMPLETED);

  std::cout << "Test set generated in "
            << duration_wrapper(std::chrono::milliseconds(
                   cagen_handle->get_duration_in_milli_seconds()))
            << " and has " << t.get_list_of_tests().size() << " rows."
            << std::endl;

  std::cout << "Now extending the exact same testset." << std::endl;

  std::unique_ptr<cagen_exec_handle_ipog> cagen_handle2 =
      compute_covering_array_ipog(
          model, t, 2,
          covering_array_computation_config().with_replace_dont_care_values(
              false));
  auto cagen_f2 = cagen_handle2->get_test_set();
  cagen_exec_result result2(cagen_f2.get());
  const test_set& t2 = result2.get_result();

  CHECK(result2.get_result_code() == cagen_exec_result::cagen_result_code::
                                         COVERING_ARRAY_GENERATION_COMPLETED);

  std::cout << "Test extended in "
            << duration_wrapper(std::chrono::milliseconds(
                   cagen_handle2->get_duration_in_milli_seconds()))
            << " and has " << t2.get_list_of_tests().size() << " rows."
            << std::endl;

  CHECK(t == t2);
}

TEST_CASE(
    "cagen, testing ACTS example model, strength 2, extend generated but "
    "reduced test set") {
  using namespace citcpp;

  model model{create_acts_example_model()};

  std::unique_ptr<cagen_exec_handle_ipog> cagen_handle =
      compute_covering_array_ipog(
          model, 2,
          covering_array_computation_config().with_replace_dont_care_values(
              false));
  auto cagen_f = cagen_handle->get_test_set();
  cagen_exec_result cagen_result(cagen_f.get());
  test_set t = cagen_result.get_result();

  CHECK(cagen_result.get_result_code() ==
        cagen_exec_result::cagen_result_code::
            COVERING_ARRAY_GENERATION_COMPLETED);

  std::cout << "Test set generated in "
            << duration_wrapper(std::chrono::milliseconds(
                   cagen_handle->get_duration_in_milli_seconds()))
            << " and has " << t.get_list_of_tests().size() << " rows."
            << std::endl;

  std::cout << "Deleteing all but the first 10 test of the tests and reversing "
               "their order."
            << std::endl;
  auto t_it = t.get_list_of_tests().begin();
  std::advance(t_it, 10);
  t.get_list_of_tests().erase(t_it, t.get_list_of_tests().end());
  t.get_list_of_tests().reverse();
  std::cout << "Our test set now has just " << t.get_list_of_tests().size()
            << " tests." << std::endl;

  std::cout << "Now extending the exact same testset." << std::endl;

  std::unique_ptr<cagen_exec_handle_ipog> cagen_handle2 =
      compute_covering_array_ipog(
          model, t, 2,
          covering_array_computation_config().with_replace_dont_care_values(
              false));
  auto cagen_f2 = cagen_handle2->get_test_set();
  cagen_exec_result result2(cagen_f2.get());
  const test_set& t2 = result2.get_result();

  CHECK(result2.get_result_code() == cagen_exec_result::cagen_result_code::
                                         COVERING_ARRAY_GENERATION_COMPLETED);

  std::cout << "Test extended in "
            << duration_wrapper(std::chrono::milliseconds(
                   cagen_handle2->get_duration_in_milli_seconds()))
            << " and has " << t2.get_list_of_tests().size() << " rows."
            << std::endl;

  std::unique_ptr<covm_exec_handle> covm_handle =
      measure_coverage(model, t2, 2, coverage_measurement_config());
  auto covm_f = covm_handle->get_coverage_measurement();
  covm_exec_result covm_result(covm_f.get());
  const coverage_measurement& covm = covm_result.get_result().at("");

  CHECK(covm_result.get_result_code() ==
        covm_exec_result::covm_result_code::COVERAGE_MEASUREMENT_COMPLETED);

  std::cout << "Coverage measured in "
            << duration_wrapper(std::chrono::milliseconds(
                   covm_handle->get_duration_in_milli_seconds()))
            << std::endl;

  std::cout << covm << std::endl;

  CHECK(covm_result.get_invalid_test_indices().empty());
  CHECK(covm.get_covered_tuples()[covm.get_covered_tuples().size() - 1] ==
        covm.get_number_of_combinations_to_cover());
  CHECK(covm[1.0] == covm.get_number_of_param_combos_to_cover());
}
