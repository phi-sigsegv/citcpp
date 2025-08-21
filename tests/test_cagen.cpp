#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest.h>

#include <chrono>
#include <citcpp/citcpp.hpp>
#include <iostream>

namespace {

citcpp::input_model create_pict_example_model() {
  using namespace citcpp;

  input_model model;

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

TEST_CASE("cagen, testing PICT example model, strength 1") {
  using namespace citcpp;

  input_model model{create_pict_example_model()};

  std::unique_ptr<cagen_exec_handle_ipog> handle = compute_covering_array_ipog(
      model, 1, covering_array_computation_config());
  auto f = handle->get_test_set();
  cagen_exec_result result(f.get());
  const test_set& t = result.get_result();

  CHECK(result.get_result_code() == cagen_exec_result::cagen_result_code::
                                        COVERING_ARRAY_GENERATION_COMPLETED);

  const auto duration_seconds = std::chrono::duration<double>(
      std::chrono::milliseconds(handle->get_duration_in_milli_seconds()));
  std::cout << "Test generated in " << duration_seconds << " and has "
            << t.get_list_of_tests().size() << " rows." << std::endl;

  // The parameter with the largest number of values has 4 values.
  // Thus, for 1-way coverage we shall get a testset with exactly
  // four rows.
  CHECK(t.get_list_of_tests().size() == 4);
}

TEST_CASE("cagen, testing PICT example model, strength 2") {
  using namespace citcpp;

  input_model model{create_pict_example_model()};

  std::unique_ptr<cagen_exec_handle_ipog> handle = compute_covering_array_ipog(
      model, 2, covering_array_computation_config());
  auto f = handle->get_test_set();
  cagen_exec_result result(f.get());
  const test_set& t = result.get_result();

  CHECK(result.get_result_code() == cagen_exec_result::cagen_result_code::
                                        COVERING_ARRAY_GENERATION_COMPLETED);

  const auto duration_seconds = std::chrono::duration<double>(
      std::chrono::milliseconds(handle->get_duration_in_milli_seconds()));
  std::cout << "Test generated in " << duration_seconds << " and has "
            << t.get_list_of_tests().size() << " rows." << std::endl;
}

TEST_CASE("cagen, testing PICT example model, strength 3") {
  using namespace citcpp;

  input_model model{create_pict_example_model()};

  std::unique_ptr<cagen_exec_handle_ipog> handle = compute_covering_array_ipog(
      model, 3, covering_array_computation_config());
  auto f = handle->get_test_set();
  cagen_exec_result result(f.get());
  const test_set& t = result.get_result();

  CHECK(result.get_result_code() == cagen_exec_result::cagen_result_code::
                                        COVERING_ARRAY_GENERATION_COMPLETED);

  const auto duration_seconds = std::chrono::duration<double>(
      std::chrono::milliseconds(handle->get_duration_in_milli_seconds()));
  std::cout << "Test generated in " << duration_seconds << " and has "
            << t.get_list_of_tests().size() << " rows." << std::endl;
}

TEST_CASE("cagen, testing PICT example model, strength 4") {
  using namespace citcpp;

  input_model model{create_pict_example_model()};

  std::unique_ptr<cagen_exec_handle_ipog> handle = compute_covering_array_ipog(
      model, 4, covering_array_computation_config());
  auto f = handle->get_test_set();
  cagen_exec_result result(f.get());
  const test_set& t = result.get_result();

  CHECK(result.get_result_code() == cagen_exec_result::cagen_result_code::
                                        COVERING_ARRAY_GENERATION_COMPLETED);

  const auto duration_seconds = std::chrono::duration<double>(
      std::chrono::milliseconds(handle->get_duration_in_milli_seconds()));
  std::cout << "Test generated in " << duration_seconds << " and has "
            << t.get_list_of_tests().size() << " rows." << std::endl;
}

TEST_CASE("cagen, testing PICT example model, strength 5") {
  using namespace citcpp;

  input_model model{create_pict_example_model()};

  std::unique_ptr<cagen_exec_handle_ipog> handle = compute_covering_array_ipog(
      model, 5, covering_array_computation_config());
  auto f = handle->get_test_set();
  cagen_exec_result result(f.get());
  const test_set& t = result.get_result();

  CHECK(result.get_result_code() == cagen_exec_result::cagen_result_code::
                                        COVERING_ARRAY_GENERATION_COMPLETED);

  const auto duration_seconds = std::chrono::duration<double>(
      std::chrono::milliseconds(handle->get_duration_in_milli_seconds()));
  std::cout << "Test generated in " << duration_seconds << " and has "
            << t.get_list_of_tests().size() << " rows." << std::endl;
}

TEST_CASE("cagen, testing PICT example model, strength 6") {
  using namespace citcpp;

  input_model model{create_pict_example_model()};

  std::unique_ptr<cagen_exec_handle_ipog> handle = compute_covering_array_ipog(
      model, 6, covering_array_computation_config());
  auto f = handle->get_test_set();
  cagen_exec_result result(f.get());
  const test_set& t = result.get_result();

  CHECK(result.get_result_code() == cagen_exec_result::cagen_result_code::
                                        COVERING_ARRAY_GENERATION_COMPLETED);

  const auto duration_seconds = std::chrono::duration<double>(
      std::chrono::milliseconds(handle->get_duration_in_milli_seconds()));
  std::cout << "Test generated in " << duration_seconds << " and has "
            << t.get_list_of_tests().size() << " rows." << std::endl;
}

TEST_CASE("cagen, testing PICT example model, strength 7") {
  using namespace citcpp;

  input_model model{create_pict_example_model()};

  std::unique_ptr<cagen_exec_handle_ipog> handle = compute_covering_array_ipog(
      model, 7, covering_array_computation_config());
  auto f = handle->get_test_set();
  cagen_exec_result result(f.get());
  const test_set& t = result.get_result();

  CHECK(result.get_result_code() == cagen_exec_result::cagen_result_code::
                                        COVERING_ARRAY_GENERATION_COMPLETED);

  const auto duration_seconds = std::chrono::duration<double>(
      std::chrono::milliseconds(handle->get_duration_in_milli_seconds()));
  std::cout << "Test generated in " << duration_seconds << " and has "
            << t.get_list_of_tests().size() << " rows." << std::endl;
}
