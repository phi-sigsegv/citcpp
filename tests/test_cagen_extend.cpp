#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest.h>

#include <chrono>
#include <citcpp/citcpp.hpp>
#include <iostream>
#include <iterator>

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

  const auto cagen_duration_seconds = std::chrono::duration<double>(
      std::chrono::milliseconds(cagen_handle->get_duration_in_milli_seconds()));
  std::cout << "Test set generated in " << cagen_duration_seconds << " and has "
            << t.get_list_of_tests().size() << " rows." << std::endl;

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

  const auto cagen_duration_seconds2 =
      std::chrono::duration<double>(std::chrono::milliseconds(
          cagen_handle2->get_duration_in_milli_seconds()));
  std::cout << "Test extended in " << cagen_duration_seconds2 << " and has "
            << t2.get_list_of_tests().size() << " rows." << std::endl;

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

  const auto cagen_duration_seconds = std::chrono::duration<double>(
      std::chrono::milliseconds(cagen_handle->get_duration_in_milli_seconds()));
  std::cout << "Test set generated in " << cagen_duration_seconds << " and has "
            << t.get_list_of_tests().size() << " rows." << std::endl;

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

  const auto cagen_duration_seconds2 =
      std::chrono::duration<double>(std::chrono::milliseconds(
          cagen_handle2->get_duration_in_milli_seconds()));
  std::cout << "Test extended in " << cagen_duration_seconds2 << " and has "
            << t2.get_list_of_tests().size() << " rows." << std::endl;

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

  const auto cagen_duration_seconds = std::chrono::duration<double>(
      std::chrono::milliseconds(cagen_handle->get_duration_in_milli_seconds()));
  std::cout << "Test set generated in " << cagen_duration_seconds << " and has "
            << t.get_list_of_tests().size() << " rows." << std::endl;

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

  const auto cagen_duration_seconds2 =
      std::chrono::duration<double>(std::chrono::milliseconds(
          cagen_handle2->get_duration_in_milli_seconds()));
  std::cout << "Test extended in " << cagen_duration_seconds2 << " and has "
            << t2.get_list_of_tests().size() << " rows." << std::endl;

  CHECK(t == t2);
}

TEST_CASE(
    "cagen, testing PICT example model, strength 4, extend generated test "
    "set") {
  using namespace citcpp;

  model model{create_pict_example_model()};

  std::unique_ptr<cagen_exec_handle_ipog> cagen_handle =
      compute_covering_array_ipog(
          model, 4,
          covering_array_computation_config().with_replace_dont_care_values(
              false));
  auto cagen_f = cagen_handle->get_test_set();
  cagen_exec_result cagen_result(cagen_f.get());
  const test_set& t = cagen_result.get_result();

  CHECK(cagen_result.get_result_code() ==
        cagen_exec_result::cagen_result_code::
            COVERING_ARRAY_GENERATION_COMPLETED);

  const auto cagen_duration_seconds = std::chrono::duration<double>(
      std::chrono::milliseconds(cagen_handle->get_duration_in_milli_seconds()));
  std::cout << "Test set generated in " << cagen_duration_seconds << " and has "
            << t.get_list_of_tests().size() << " rows." << std::endl;

  std::cout << "Now extending the exact same testset." << std::endl;

  std::unique_ptr<cagen_exec_handle_ipog> cagen_handle2 =
      compute_covering_array_ipog(
          model, t, 4,
          covering_array_computation_config().with_replace_dont_care_values(
              false));
  auto cagen_f2 = cagen_handle2->get_test_set();
  cagen_exec_result result2(cagen_f2.get());
  const test_set& t2 = result2.get_result();

  CHECK(result2.get_result_code() == cagen_exec_result::cagen_result_code::
                                         COVERING_ARRAY_GENERATION_COMPLETED);

  const auto cagen_duration_seconds2 =
      std::chrono::duration<double>(std::chrono::milliseconds(
          cagen_handle2->get_duration_in_milli_seconds()));
  std::cout << "Test extended in " << cagen_duration_seconds2 << " and has "
            << t2.get_list_of_tests().size() << " rows." << std::endl;

  CHECK(t == t2);
}

TEST_CASE(
    "cagen, testing PICT example model, strength 5, extend generated test "
    "set") {
  using namespace citcpp;

  model model{create_pict_example_model()};

  std::unique_ptr<cagen_exec_handle_ipog> cagen_handle =
      compute_covering_array_ipog(
          model, 5,
          covering_array_computation_config().with_replace_dont_care_values(
              false));
  auto cagen_f = cagen_handle->get_test_set();
  cagen_exec_result cagen_result(cagen_f.get());
  const test_set& t = cagen_result.get_result();

  CHECK(cagen_result.get_result_code() ==
        cagen_exec_result::cagen_result_code::
            COVERING_ARRAY_GENERATION_COMPLETED);

  const auto cagen_duration_seconds = std::chrono::duration<double>(
      std::chrono::milliseconds(cagen_handle->get_duration_in_milli_seconds()));
  std::cout << "Test set generated in " << cagen_duration_seconds << " and has "
            << t.get_list_of_tests().size() << " rows." << std::endl;

  std::cout << "Now extending the exact same testset." << std::endl;

  std::unique_ptr<cagen_exec_handle_ipog> cagen_handle2 =
      compute_covering_array_ipog(
          model, t, 5,
          covering_array_computation_config().with_replace_dont_care_values(
              false));
  auto cagen_f2 = cagen_handle2->get_test_set();
  cagen_exec_result result2(cagen_f2.get());
  const test_set& t2 = result2.get_result();

  CHECK(result2.get_result_code() == cagen_exec_result::cagen_result_code::
                                         COVERING_ARRAY_GENERATION_COMPLETED);

  const auto cagen_duration_seconds2 =
      std::chrono::duration<double>(std::chrono::milliseconds(
          cagen_handle2->get_duration_in_milli_seconds()));
  std::cout << "Test extended in " << cagen_duration_seconds2 << " and has "
            << t2.get_list_of_tests().size() << " rows." << std::endl;

  CHECK(t == t2);
}

TEST_CASE(
    "cagen, testing PICT example model, strength 6, extend generated test "
    "set") {
  using namespace citcpp;

  model model{create_pict_example_model()};

  std::unique_ptr<cagen_exec_handle_ipog> cagen_handle =
      compute_covering_array_ipog(
          model, 6,
          covering_array_computation_config().with_replace_dont_care_values(
              false));
  auto cagen_f = cagen_handle->get_test_set();
  cagen_exec_result cagen_result(cagen_f.get());
  const test_set& t = cagen_result.get_result();

  CHECK(cagen_result.get_result_code() ==
        cagen_exec_result::cagen_result_code::
            COVERING_ARRAY_GENERATION_COMPLETED);

  const auto cagen_duration_seconds = std::chrono::duration<double>(
      std::chrono::milliseconds(cagen_handle->get_duration_in_milli_seconds()));
  std::cout << "Test set generated in " << cagen_duration_seconds << " and has "
            << t.get_list_of_tests().size() << " rows." << std::endl;

  std::cout << "Now extending the exact same testset." << std::endl;

  std::unique_ptr<cagen_exec_handle_ipog> cagen_handle2 =
      compute_covering_array_ipog(
          model, t, 6,
          covering_array_computation_config().with_replace_dont_care_values(
              false));
  auto cagen_f2 = cagen_handle2->get_test_set();
  cagen_exec_result result2(cagen_f2.get());
  const test_set& t2 = result2.get_result();

  CHECK(result2.get_result_code() == cagen_exec_result::cagen_result_code::
                                         COVERING_ARRAY_GENERATION_COMPLETED);

  const auto cagen_duration_seconds2 =
      std::chrono::duration<double>(std::chrono::milliseconds(
          cagen_handle2->get_duration_in_milli_seconds()));
  std::cout << "Test extended in " << cagen_duration_seconds2 << " and has "
            << t2.get_list_of_tests().size() << " rows." << std::endl;

  CHECK(t == t2);
}

TEST_CASE(
    "cagen, testing PICT example model, strength 7, extend generated test "
    "set") {
  using namespace citcpp;

  model model{create_pict_example_model()};

  std::unique_ptr<cagen_exec_handle_ipog> cagen_handle =
      compute_covering_array_ipog(
          model, 7,
          covering_array_computation_config().with_replace_dont_care_values(
              false));
  auto cagen_f = cagen_handle->get_test_set();
  cagen_exec_result cagen_result(cagen_f.get());
  const test_set& t = cagen_result.get_result();

  CHECK(cagen_result.get_result_code() ==
        cagen_exec_result::cagen_result_code::
            COVERING_ARRAY_GENERATION_COMPLETED);

  const auto cagen_duration_seconds = std::chrono::duration<double>(
      std::chrono::milliseconds(cagen_handle->get_duration_in_milli_seconds()));
  std::cout << "Test set generated in " << cagen_duration_seconds << " and has "
            << t.get_list_of_tests().size() << " rows." << std::endl;

  std::cout << "Now extending the exact same testset." << std::endl;

  std::unique_ptr<cagen_exec_handle_ipog> cagen_handle2 =
      compute_covering_array_ipog(
          model, t, 7,
          covering_array_computation_config().with_replace_dont_care_values(
              false));
  auto cagen_f2 = cagen_handle2->get_test_set();
  cagen_exec_result result2(cagen_f2.get());
  const test_set& t2 = result2.get_result();

  CHECK(result2.get_result_code() == cagen_exec_result::cagen_result_code::
                                         COVERING_ARRAY_GENERATION_COMPLETED);

  const auto cagen_duration_seconds2 =
      std::chrono::duration<double>(std::chrono::milliseconds(
          cagen_handle2->get_duration_in_milli_seconds()));
  std::cout << "Test extended in " << cagen_duration_seconds2 << " and has "
            << t2.get_list_of_tests().size() << " rows." << std::endl;

  CHECK(t == t2);
}

TEST_CASE(
    "cagen, testing PICT example model, strength 1, extend generated but "
    "reduced test set") {
  using namespace citcpp;

  model model{create_pict_example_model()};

  std::unique_ptr<cagen_exec_handle_ipog> cagen_handle =
      compute_covering_array_ipog(model, 1,
                                  covering_array_computation_config());
  auto cagen_f = cagen_handle->get_test_set();
  cagen_exec_result cagen_result(cagen_f.get());
  test_set t = cagen_result.get_result();

  CHECK(cagen_result.get_result_code() ==
        cagen_exec_result::cagen_result_code::
            COVERING_ARRAY_GENERATION_COMPLETED);

  const auto cagen_duration_seconds = std::chrono::duration<double>(
      std::chrono::milliseconds(cagen_handle->get_duration_in_milli_seconds()));
  std::cout << "Test set generated in " << cagen_duration_seconds << " and has "
            << t.get_list_of_tests().size() << " rows." << std::endl;

  std::cout << "Deleteing the first 33% of the tests and reversing their order."
            << std::endl;
  int num_deleted_tests = t.get_list_of_tests().size() / 3;
  auto t_it = t.get_list_of_tests().begin();
  std::advance(t_it, num_deleted_tests);
  t.get_list_of_tests().erase(t.get_list_of_tests().begin(), t_it);
  t.get_list_of_tests().reverse();
  std::cout << "Our test set now has just " << t.get_list_of_tests().size()
            << " tests." << std::endl;

  std::cout << "Now extending the exact same testset." << std::endl;

  std::unique_ptr<cagen_exec_handle_ipog> cagen_handle2 =
      compute_covering_array_ipog(model, t, 1,
                                  covering_array_computation_config());
  auto cagen_f2 = cagen_handle2->get_test_set();
  cagen_exec_result result2(cagen_f2.get());
  const test_set& t2 = result2.get_result();

  CHECK(result2.get_result_code() == cagen_exec_result::cagen_result_code::
                                         COVERING_ARRAY_GENERATION_COMPLETED);

  const auto cagen_duration_seconds2 =
      std::chrono::duration<double>(std::chrono::milliseconds(
          cagen_handle2->get_duration_in_milli_seconds()));
  std::cout << "Test extended in " << cagen_duration_seconds2 << " and has "
            << t2.get_list_of_tests().size() << " rows." << std::endl;

  std::unique_ptr<covm_exec_handle> covm_handle =
      measure_coverage(model, t2, 1, coverage_measurement_config());
  auto covm_f = covm_handle->get_coverage_measurement();
  covm_exec_result covm_result(covm_f.get());
  const coverage_measurement& covm = covm_result.get_result();

  CHECK(covm_result.get_result_code() ==
        covm_exec_result::covm_result_code::COVERAGE_MEASUREMENT_COMPLETED);

  const auto covm_duration_seconds = std::chrono::duration<double>(
      std::chrono::milliseconds(covm_handle->get_duration_in_milli_seconds()));
  std::cout << "Coverage measured in " << covm_duration_seconds << std::endl;

  std::cout << covm << std::endl;

  CHECK(covm.get_covered_tuples()[covm.get_covered_tuples().size() - 1] ==
        covm.get_number_of_combinations_to_cover());
  CHECK(covm[1.0] == covm.get_number_of_param_combos_to_cover());
}

TEST_CASE(
    "cagen, testing PICT example model, strength 2, extend generated but "
    "reduced test set") {
  using namespace citcpp;

  model model{create_pict_example_model()};

  std::unique_ptr<cagen_exec_handle_ipog> cagen_handle =
      compute_covering_array_ipog(model, 2,
                                  covering_array_computation_config());
  auto cagen_f = cagen_handle->get_test_set();
  cagen_exec_result cagen_result(cagen_f.get());
  test_set t = cagen_result.get_result();

  CHECK(cagen_result.get_result_code() ==
        cagen_exec_result::cagen_result_code::
            COVERING_ARRAY_GENERATION_COMPLETED);

  const auto cagen_duration_seconds = std::chrono::duration<double>(
      std::chrono::milliseconds(cagen_handle->get_duration_in_milli_seconds()));
  std::cout << "Test set generated in " << cagen_duration_seconds << " and has "
            << t.get_list_of_tests().size() << " rows." << std::endl;

  std::cout << "Deleteing the first 33% of the tests and reversing their order."
            << std::endl;
  int num_deleted_tests = t.get_list_of_tests().size() / 3;
  auto t_it = t.get_list_of_tests().begin();
  std::advance(t_it, num_deleted_tests);
  t.get_list_of_tests().erase(t.get_list_of_tests().begin(), t_it);
  t.get_list_of_tests().reverse();
  std::cout << "Our test set now has just " << t.get_list_of_tests().size()
            << " tests." << std::endl;

  std::cout << "Now extending the exact same testset." << std::endl;

  std::unique_ptr<cagen_exec_handle_ipog> cagen_handle2 =
      compute_covering_array_ipog(model, t, 2,
                                  covering_array_computation_config());
  auto cagen_f2 = cagen_handle2->get_test_set();
  cagen_exec_result result2(cagen_f2.get());
  const test_set& t2 = result2.get_result();

  CHECK(result2.get_result_code() == cagen_exec_result::cagen_result_code::
                                         COVERING_ARRAY_GENERATION_COMPLETED);

  const auto cagen_duration_seconds2 =
      std::chrono::duration<double>(std::chrono::milliseconds(
          cagen_handle2->get_duration_in_milli_seconds()));
  std::cout << "Test extended in " << cagen_duration_seconds2 << " and has "
            << t2.get_list_of_tests().size() << " rows." << std::endl;

  std::unique_ptr<covm_exec_handle> covm_handle =
      measure_coverage(model, t2, 2, coverage_measurement_config());
  auto covm_f = covm_handle->get_coverage_measurement();
  covm_exec_result covm_result(covm_f.get());
  const coverage_measurement& covm = covm_result.get_result();

  CHECK(covm_result.get_result_code() ==
        covm_exec_result::covm_result_code::COVERAGE_MEASUREMENT_COMPLETED);

  const auto covm_duration_seconds = std::chrono::duration<double>(
      std::chrono::milliseconds(covm_handle->get_duration_in_milli_seconds()));
  std::cout << "Coverage measured in " << covm_duration_seconds << std::endl;

  std::cout << covm << std::endl;

  CHECK(covm.get_covered_tuples()[covm.get_covered_tuples().size() - 1] ==
        covm.get_number_of_combinations_to_cover());
  CHECK(covm[1.0] == covm.get_number_of_param_combos_to_cover());
}

TEST_CASE(
    "cagen, testing PICT example model, strength 3, extend generated but "
    "reduced test set") {
  using namespace citcpp;

  model model{create_pict_example_model()};

  std::unique_ptr<cagen_exec_handle_ipog> cagen_handle =
      compute_covering_array_ipog(model, 3,
                                  covering_array_computation_config());
  auto cagen_f = cagen_handle->get_test_set();
  cagen_exec_result cagen_result(cagen_f.get());
  test_set t = cagen_result.get_result();

  CHECK(cagen_result.get_result_code() ==
        cagen_exec_result::cagen_result_code::
            COVERING_ARRAY_GENERATION_COMPLETED);

  const auto cagen_duration_seconds = std::chrono::duration<double>(
      std::chrono::milliseconds(cagen_handle->get_duration_in_milli_seconds()));
  std::cout << "Test set generated in " << cagen_duration_seconds << " and has "
            << t.get_list_of_tests().size() << " rows." << std::endl;

  std::cout << "Deleteing the first 33% of the tests and reversing their order."
            << std::endl;
  int num_deleted_tests = t.get_list_of_tests().size() / 3;
  auto t_it = t.get_list_of_tests().begin();
  std::advance(t_it, num_deleted_tests);
  t.get_list_of_tests().erase(t.get_list_of_tests().begin(), t_it);
  t.get_list_of_tests().reverse();
  std::cout << "Our test set now has just " << t.get_list_of_tests().size()
            << " tests." << std::endl;

  std::cout << "Now extending the exact same testset." << std::endl;

  std::unique_ptr<cagen_exec_handle_ipog> cagen_handle2 =
      compute_covering_array_ipog(model, t, 3,
                                  covering_array_computation_config());
  auto cagen_f2 = cagen_handle2->get_test_set();
  cagen_exec_result result2(cagen_f2.get());
  const test_set& t2 = result2.get_result();

  CHECK(result2.get_result_code() == cagen_exec_result::cagen_result_code::
                                         COVERING_ARRAY_GENERATION_COMPLETED);

  const auto cagen_duration_seconds2 =
      std::chrono::duration<double>(std::chrono::milliseconds(
          cagen_handle2->get_duration_in_milli_seconds()));
  std::cout << "Test extended in " << cagen_duration_seconds2 << " and has "
            << t2.get_list_of_tests().size() << " rows." << std::endl;

  std::unique_ptr<covm_exec_handle> covm_handle =
      measure_coverage(model, t2, 3, coverage_measurement_config());
  auto covm_f = covm_handle->get_coverage_measurement();
  covm_exec_result covm_result(covm_f.get());
  const coverage_measurement& covm = covm_result.get_result();

  CHECK(covm_result.get_result_code() ==
        covm_exec_result::covm_result_code::COVERAGE_MEASUREMENT_COMPLETED);

  const auto covm_duration_seconds = std::chrono::duration<double>(
      std::chrono::milliseconds(covm_handle->get_duration_in_milli_seconds()));
  std::cout << "Coverage measured in " << covm_duration_seconds << std::endl;

  std::cout << covm << std::endl;

  CHECK(covm.get_covered_tuples()[covm.get_covered_tuples().size() - 1] ==
        covm.get_number_of_combinations_to_cover());
  CHECK(covm[1.0] == covm.get_number_of_param_combos_to_cover());
}

TEST_CASE(
    "cagen, testing PICT example model, strength 4, extend generated but "
    "reduced test set") {
  using namespace citcpp;

  model model{create_pict_example_model()};

  std::unique_ptr<cagen_exec_handle_ipog> cagen_handle =
      compute_covering_array_ipog(model, 4,
                                  covering_array_computation_config());
  auto cagen_f = cagen_handle->get_test_set();
  cagen_exec_result cagen_result(cagen_f.get());
  test_set t = cagen_result.get_result();

  CHECK(cagen_result.get_result_code() ==
        cagen_exec_result::cagen_result_code::
            COVERING_ARRAY_GENERATION_COMPLETED);

  const auto cagen_duration_seconds = std::chrono::duration<double>(
      std::chrono::milliseconds(cagen_handle->get_duration_in_milli_seconds()));
  std::cout << "Test set generated in " << cagen_duration_seconds << " and has "
            << t.get_list_of_tests().size() << " rows." << std::endl;

  std::cout << "Deleteing the first 33% of the tests and reversing their order."
            << std::endl;
  int num_deleted_tests = t.get_list_of_tests().size() / 3;
  auto t_it = t.get_list_of_tests().begin();
  std::advance(t_it, num_deleted_tests);
  t.get_list_of_tests().erase(t.get_list_of_tests().begin(), t_it);
  t.get_list_of_tests().reverse();
  std::cout << "Our test set now has just " << t.get_list_of_tests().size()
            << " tests." << std::endl;

  std::cout << "Now extending the exact same testset." << std::endl;

  std::unique_ptr<cagen_exec_handle_ipog> cagen_handle2 =
      compute_covering_array_ipog(model, t, 4,
                                  covering_array_computation_config());
  auto cagen_f2 = cagen_handle2->get_test_set();
  cagen_exec_result result2(cagen_f2.get());
  const test_set& t2 = result2.get_result();

  CHECK(result2.get_result_code() == cagen_exec_result::cagen_result_code::
                                         COVERING_ARRAY_GENERATION_COMPLETED);

  const auto cagen_duration_seconds2 =
      std::chrono::duration<double>(std::chrono::milliseconds(
          cagen_handle2->get_duration_in_milli_seconds()));
  std::cout << "Test extended in " << cagen_duration_seconds2 << " and has "
            << t2.get_list_of_tests().size() << " rows." << std::endl;

  std::unique_ptr<covm_exec_handle> covm_handle =
      measure_coverage(model, t2, 4, coverage_measurement_config());
  auto covm_f = covm_handle->get_coverage_measurement();
  covm_exec_result covm_result(covm_f.get());
  const coverage_measurement& covm = covm_result.get_result();

  CHECK(covm_result.get_result_code() ==
        covm_exec_result::covm_result_code::COVERAGE_MEASUREMENT_COMPLETED);

  const auto covm_duration_seconds = std::chrono::duration<double>(
      std::chrono::milliseconds(covm_handle->get_duration_in_milli_seconds()));
  std::cout << "Coverage measured in " << covm_duration_seconds << std::endl;

  std::cout << covm << std::endl;

  CHECK(covm.get_covered_tuples()[covm.get_covered_tuples().size() - 1] ==
        covm.get_number_of_combinations_to_cover());
  CHECK(covm[1.0] == covm.get_number_of_param_combos_to_cover());
}

TEST_CASE(
    "cagen, testing PICT example model, strength 5, extend generated but "
    "reduced test set") {
  using namespace citcpp;

  model model{create_pict_example_model()};

  std::unique_ptr<cagen_exec_handle_ipog> cagen_handle =
      compute_covering_array_ipog(model, 5,
                                  covering_array_computation_config());
  auto cagen_f = cagen_handle->get_test_set();
  cagen_exec_result cagen_result(cagen_f.get());
  test_set t = cagen_result.get_result();

  CHECK(cagen_result.get_result_code() ==
        cagen_exec_result::cagen_result_code::
            COVERING_ARRAY_GENERATION_COMPLETED);

  const auto cagen_duration_seconds = std::chrono::duration<double>(
      std::chrono::milliseconds(cagen_handle->get_duration_in_milli_seconds()));
  std::cout << "Test set generated in " << cagen_duration_seconds << " and has "
            << t.get_list_of_tests().size() << " rows." << std::endl;

  std::cout << "Deleteing the first 33% of the tests and reversing their order."
            << std::endl;
  int num_deleted_tests = t.get_list_of_tests().size() / 3;
  auto t_it = t.get_list_of_tests().begin();
  std::advance(t_it, num_deleted_tests);
  t.get_list_of_tests().erase(t.get_list_of_tests().begin(), t_it);
  t.get_list_of_tests().reverse();
  std::cout << "Our test set now has just " << t.get_list_of_tests().size()
            << " tests." << std::endl;

  std::cout << "Now extending the exact same testset." << std::endl;

  std::unique_ptr<cagen_exec_handle_ipog> cagen_handle2 =
      compute_covering_array_ipog(model, t, 5,
                                  covering_array_computation_config());
  auto cagen_f2 = cagen_handle2->get_test_set();
  cagen_exec_result result2(cagen_f2.get());
  const test_set& t2 = result2.get_result();

  CHECK(result2.get_result_code() == cagen_exec_result::cagen_result_code::
                                         COVERING_ARRAY_GENERATION_COMPLETED);

  const auto cagen_duration_seconds2 =
      std::chrono::duration<double>(std::chrono::milliseconds(
          cagen_handle2->get_duration_in_milli_seconds()));
  std::cout << "Test extended in " << cagen_duration_seconds2 << " and has "
            << t2.get_list_of_tests().size() << " rows." << std::endl;

  std::unique_ptr<covm_exec_handle> covm_handle =
      measure_coverage(model, t2, 5, coverage_measurement_config());
  auto covm_f = covm_handle->get_coverage_measurement();
  covm_exec_result covm_result(covm_f.get());
  const coverage_measurement& covm = covm_result.get_result();

  CHECK(covm_result.get_result_code() ==
        covm_exec_result::covm_result_code::COVERAGE_MEASUREMENT_COMPLETED);

  const auto covm_duration_seconds = std::chrono::duration<double>(
      std::chrono::milliseconds(covm_handle->get_duration_in_milli_seconds()));
  std::cout << "Coverage measured in " << covm_duration_seconds << std::endl;

  std::cout << covm << std::endl;

  CHECK(covm.get_covered_tuples()[covm.get_covered_tuples().size() - 1] ==
        covm.get_number_of_combinations_to_cover());
  CHECK(covm[1.0] == covm.get_number_of_param_combos_to_cover());
}

TEST_CASE(
    "cagen, testing PICT example model, strength 6, extend generated but "
    "reduced test set") {
  using namespace citcpp;

  model model{create_pict_example_model()};

  std::unique_ptr<cagen_exec_handle_ipog> cagen_handle =
      compute_covering_array_ipog(model, 6,
                                  covering_array_computation_config());
  auto cagen_f = cagen_handle->get_test_set();
  cagen_exec_result cagen_result(cagen_f.get());
  test_set t = cagen_result.get_result();

  CHECK(cagen_result.get_result_code() ==
        cagen_exec_result::cagen_result_code::
            COVERING_ARRAY_GENERATION_COMPLETED);

  const auto cagen_duration_seconds = std::chrono::duration<double>(
      std::chrono::milliseconds(cagen_handle->get_duration_in_milli_seconds()));
  std::cout << "Test set generated in " << cagen_duration_seconds << " and has "
            << t.get_list_of_tests().size() << " rows." << std::endl;

  std::cout << "Deleteing the first 33% of the tests and reversing their order."
            << std::endl;
  int num_deleted_tests = t.get_list_of_tests().size() / 3;
  auto t_it = t.get_list_of_tests().begin();
  std::advance(t_it, num_deleted_tests);
  t.get_list_of_tests().erase(t.get_list_of_tests().begin(), t_it);
  t.get_list_of_tests().reverse();
  std::cout << "Our test set now has just " << t.get_list_of_tests().size()
            << " tests." << std::endl;

  std::cout << "Now extending the exact same testset." << std::endl;

  std::unique_ptr<cagen_exec_handle_ipog> cagen_handle2 =
      compute_covering_array_ipog(model, t, 6,
                                  covering_array_computation_config());
  auto cagen_f2 = cagen_handle2->get_test_set();
  cagen_exec_result result2(cagen_f2.get());
  const test_set& t2 = result2.get_result();

  CHECK(result2.get_result_code() == cagen_exec_result::cagen_result_code::
                                         COVERING_ARRAY_GENERATION_COMPLETED);

  const auto cagen_duration_seconds2 =
      std::chrono::duration<double>(std::chrono::milliseconds(
          cagen_handle2->get_duration_in_milli_seconds()));
  std::cout << "Test extended in " << cagen_duration_seconds2 << " and has "
            << t2.get_list_of_tests().size() << " rows." << std::endl;

  std::unique_ptr<covm_exec_handle> covm_handle =
      measure_coverage(model, t2, 6, coverage_measurement_config());
  auto covm_f = covm_handle->get_coverage_measurement();
  covm_exec_result covm_result(covm_f.get());
  const coverage_measurement& covm = covm_result.get_result();

  CHECK(covm_result.get_result_code() ==
        covm_exec_result::covm_result_code::COVERAGE_MEASUREMENT_COMPLETED);

  const auto covm_duration_seconds = std::chrono::duration<double>(
      std::chrono::milliseconds(covm_handle->get_duration_in_milli_seconds()));
  std::cout << "Coverage measured in " << covm_duration_seconds << std::endl;

  std::cout << covm << std::endl;

  CHECK(covm.get_covered_tuples()[covm.get_covered_tuples().size() - 1] ==
        covm.get_number_of_combinations_to_cover());
  CHECK(covm[1.0] == covm.get_number_of_param_combos_to_cover());
}

TEST_CASE(
    "cagen, testing PICT example model, strength 7, extend generated but "
    "reduced test set") {
  using namespace citcpp;

  model model{create_pict_example_model()};

  std::unique_ptr<cagen_exec_handle_ipog> cagen_handle =
      compute_covering_array_ipog(model, 7,
                                  covering_array_computation_config());
  auto cagen_f = cagen_handle->get_test_set();
  cagen_exec_result cagen_result(cagen_f.get());
  test_set t = cagen_result.get_result();

  CHECK(cagen_result.get_result_code() ==
        cagen_exec_result::cagen_result_code::
            COVERING_ARRAY_GENERATION_COMPLETED);

  const auto cagen_duration_seconds = std::chrono::duration<double>(
      std::chrono::milliseconds(cagen_handle->get_duration_in_milli_seconds()));
  std::cout << "Test set generated in " << cagen_duration_seconds << " and has "
            << t.get_list_of_tests().size() << " rows." << std::endl;

  std::cout << "Deleteing the first 33% of the tests and reversing their order."
            << std::endl;
  int num_deleted_tests = t.get_list_of_tests().size() / 3;
  auto t_it = t.get_list_of_tests().begin();
  std::advance(t_it, num_deleted_tests);
  t.get_list_of_tests().erase(t.get_list_of_tests().begin(), t_it);
  t.get_list_of_tests().reverse();
  std::cout << "Our test set now has just " << t.get_list_of_tests().size()
            << " tests." << std::endl;

  std::cout << "Now extending the exact same testset." << std::endl;

  std::unique_ptr<cagen_exec_handle_ipog> cagen_handle2 =
      compute_covering_array_ipog(model, t, 7,
                                  covering_array_computation_config());
  auto cagen_f2 = cagen_handle2->get_test_set();
  cagen_exec_result result2(cagen_f2.get());
  const test_set& t2 = result2.get_result();

  CHECK(result2.get_result_code() == cagen_exec_result::cagen_result_code::
                                         COVERING_ARRAY_GENERATION_COMPLETED);

  const auto cagen_duration_seconds2 =
      std::chrono::duration<double>(std::chrono::milliseconds(
          cagen_handle2->get_duration_in_milli_seconds()));
  std::cout << "Test extended in " << cagen_duration_seconds2 << " and has "
            << t2.get_list_of_tests().size() << " rows." << std::endl;

  std::unique_ptr<covm_exec_handle> covm_handle =
      measure_coverage(model, t2, 7, coverage_measurement_config());
  auto covm_f = covm_handle->get_coverage_measurement();
  covm_exec_result covm_result(covm_f.get());
  const coverage_measurement& covm = covm_result.get_result();

  CHECK(covm_result.get_result_code() ==
        covm_exec_result::covm_result_code::COVERAGE_MEASUREMENT_COMPLETED);

  const auto covm_duration_seconds = std::chrono::duration<double>(
      std::chrono::milliseconds(covm_handle->get_duration_in_milli_seconds()));
  std::cout << "Coverage measured in " << covm_duration_seconds << std::endl;

  std::cout << covm << std::endl;

  CHECK(covm.get_covered_tuples()[covm.get_covered_tuples().size() - 1] ==
        covm.get_number_of_combinations_to_cover());
  CHECK(covm[1.0] == covm.get_number_of_param_combos_to_cover());
}
