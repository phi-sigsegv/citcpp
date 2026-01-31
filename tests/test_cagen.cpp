#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest.h>

#include <chrono>
#include <citcpp/citcpp.hpp>
#include <duration_wrapper.hpp>
#include <iostream>

namespace {

citcpp::model create_simple_four_param_model() {
  using namespace citcpp;

  model model;

  model.add_parameter(parameter()
                          .type(parameter_type::ENUM)
                          .name("P1")
                          .values({{"a"}, {"b"}, {"c"}}));
  model.add_parameter(parameter()
                          .type(parameter_type::ENUM)
                          .name("P2")
                          .values({{"a"}, {"b"}, {"c"}}));
  model.add_parameter(parameter()
                          .type(parameter_type::ENUM)
                          .name("P3")
                          .values({{"a"}, {"b"}, {"c"}}));
  model.add_parameter(parameter()
                          .type(parameter_type::ENUM)
                          .name("P4")
                          .values({{"a"}, {"b"}, {"c"}}));

  return model;
}

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
          .name("Alt_Layer_Value")
          .values({{"NO_INTENT"}, {"DO_NOT_CLIMB"}, {"DO_NOT_DESCEND"}}));
  model.add_parameter(parameter()
                          .type(parameter_type::ENUM)
                          .name("Other_Capability")
                          .values({{"TCAS_TA"}, {"OTHER"}}));
  model.add_parameter(parameter()
                          .type(parameter_type::BOOLEAN)
                          .name("Climb_Inhibit")
                          .values({{true}, {false}}));

  return model;
}

}  // namespace

TEST_CASE("cagen, testing simple model, strength 1") {
  using namespace citcpp;

  model model{create_simple_four_param_model()};

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
  }

  // The parameter with the largest number of values has 3 values.
  // Thus, for 1-way coverage we shall get a testset with exactly
  // three rows.
  CHECK(ipog_test_set.get_list_of_tests().size() == 3);

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
  }

  // The parameter with the largest number of values has 3 values.
  // Thus, for 1-way coverage we shall get a testset with exactly
  // three rows.
  CHECK(ipog_otf_test_set.get_list_of_tests().size() == 3);

  CHECK(ipog_test_set == ipog_otf_test_set);
}

TEST_CASE("cagen, testing simple model, strength 2") {
  using namespace citcpp;

  model model{create_simple_four_param_model()};

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

TEST_CASE("cagen, testing simple model, strength 3") {
  using namespace citcpp;

  model model{create_simple_four_param_model()};

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
  }

  test_set ipog_otf_test_set;
  {
    std::unique_ptr<cagen_exec_handle_ipog> handle =
        compute_covering_array_ipog(
            model, 3,
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

TEST_CASE("cagen, testing PICT example model, strength 1") {
  using namespace citcpp;

  model model{create_pict_example_model()};

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

    // The parameter with the largest number of values has 4 values.
    // Thus, for 1-way coverage we shall get a testset with exactly
    // four rows.
    CHECK(ipog_test_set.get_list_of_tests().size() == 4);
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

    // The parameter with the largest number of values has 4 values.
    // Thus, for 1-way coverage we shall get a testset with exactly
    // four rows.
    CHECK(ipog_otf_test_set.get_list_of_tests().size() == 4);
  }

  CHECK(ipog_test_set == ipog_otf_test_set);
}

TEST_CASE("cagen, testing PICT example model, strength 2") {
  using namespace citcpp;

  model model{create_pict_example_model()};

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

TEST_CASE("cagen, testing PICT example model, strength 3") {
  using namespace citcpp;

  model model{create_pict_example_model()};

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
  }

  test_set ipog_otf_test_set;
  {
    std::unique_ptr<cagen_exec_handle_ipog> handle =
        compute_covering_array_ipog(
            model, 3,
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

TEST_CASE("cagen, testing PICT example model, strength 4") {
  using namespace citcpp;

  model model{create_pict_example_model()};

  test_set ipog_test_set;
  {
    std::unique_ptr<cagen_exec_handle_ipog> handle =
        compute_covering_array_ipog(model, 4,
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
  }

  test_set ipog_otf_test_set;
  {
    std::unique_ptr<cagen_exec_handle_ipog> handle =
        compute_covering_array_ipog(
            model, 4,
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

TEST_CASE("cagen, testing PICT example model, strength 5") {
  using namespace citcpp;

  model model{create_pict_example_model()};

  test_set ipog_test_set;
  {
    std::unique_ptr<cagen_exec_handle_ipog> handle =
        compute_covering_array_ipog(model, 5,
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
  }

  test_set ipog_otf_test_set;
  {
    std::unique_ptr<cagen_exec_handle_ipog> handle =
        compute_covering_array_ipog(
            model, 5,
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

TEST_CASE("cagen, testing PICT example model, strength 6") {
  using namespace citcpp;

  model model{create_pict_example_model()};

  test_set ipog_test_set;
  {
    std::unique_ptr<cagen_exec_handle_ipog> handle =
        compute_covering_array_ipog(model, 6,
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
  }

  test_set ipog_otf_test_set;
  {
    std::unique_ptr<cagen_exec_handle_ipog> handle =
        compute_covering_array_ipog(
            model, 6,
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

TEST_CASE("cagen, testing PICT example model, strength 7") {
  using namespace citcpp;

  model model{create_pict_example_model()};

  test_set ipog_test_set;
  {
    std::unique_ptr<cagen_exec_handle_ipog> handle =
        compute_covering_array_ipog(model, 7,
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
  }

  test_set ipog_otf_test_set;
  {
    std::unique_ptr<cagen_exec_handle_ipog> handle =
        compute_covering_array_ipog(
            model, 7,
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
  }
}
