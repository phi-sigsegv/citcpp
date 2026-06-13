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
