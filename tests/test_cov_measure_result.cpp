#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest.h>

#include <citcpp/coverage_measurement.hpp>

TEST_CASE("coverage_measurement, testing accessing param combo coverage") {
  using namespace citcpp;

  coverage_measurement sut;

  coverage_measurement::t_coverage_level_to_num_param_combos
      coverage_level_to_num_param_combos{0};

  for (unsigned int i = 0;
       i < coverage_measurement::NUM_DIFFERENTIATED_COVERAGE_LEVELS; ++i) {

    coverage_level_to_num_param_combos[i] = i;
  }

  sut.set_coverage_level_to_num_param_combos(
      coverage_level_to_num_param_combos);
  sut.set_number_of_param_combos_to_cover(30);
  sut.set_number_of_combinations_to_cover(50);
  sut.set_coverered_tuples(
      std::vector<unsigned long long>{1, 2, 3, 4, 5, 6, 7});

  SUBCASE("Test access using fractions") {
    CHECK(sut[0.0] == 0);
    CHECK(sut[0.01] == 0);
    CHECK(sut[0.049] == 0);

    CHECK(sut[0.05] == 1);
    CHECK(sut[0.06] == 1);
    CHECK(sut[0.099] == 1);

    CHECK(sut[0.10] == 2);
    CHECK(sut[0.11] == 2);
    CHECK(sut[0.149] == 2);

    CHECK(sut[0.15] == 3);
    CHECK(sut[0.16] == 3);
    CHECK(sut[0.199] == 3);

    CHECK(sut[0.20] == 4);
    CHECK(sut[0.21] == 4);
    CHECK(sut[0.249] == 4);

    CHECK(sut[0.25] == 5);
    CHECK(sut[0.26] == 5);
    CHECK(sut[0.299] == 5);

    CHECK(sut[0.30] == 6);
    CHECK(sut[0.31] == 6);
    CHECK(sut[0.349] == 6);

    CHECK(sut[0.35] == 7);
    CHECK(sut[0.36] == 7);
    CHECK(sut[0.399] == 7);

    CHECK(sut[0.40] == 8);
    CHECK(sut[0.41] == 8);
    CHECK(sut[0.449] == 8);

    CHECK(sut[0.45] == 9);
    CHECK(sut[0.46] == 9);
    CHECK(sut[0.499] == 9);

    CHECK(sut[0.50] == 10);
    CHECK(sut[0.51] == 10);
    CHECK(sut[0.549] == 10);

    CHECK(sut[0.55] == 11);
    CHECK(sut[0.56] == 11);
    CHECK(sut[0.599] == 11);

    CHECK(sut[0.60] == 12);
    CHECK(sut[0.61] == 12);
    CHECK(sut[0.649] == 12);

    CHECK(sut[0.65] == 13);
    CHECK(sut[0.66] == 13);
    CHECK(sut[0.699] == 13);

    CHECK(sut[0.70] == 14);
    CHECK(sut[0.71] == 14);
    CHECK(sut[0.749] == 14);

    CHECK(sut[0.75] == 15);
    CHECK(sut[0.76] == 15);
    CHECK(sut[0.799] == 15);

    CHECK(sut[0.80] == 16);
    CHECK(sut[0.81] == 16);
    CHECK(sut[0.849] == 16);

    CHECK(sut[0.85] == 17);
    CHECK(sut[0.86] == 17);
    CHECK(sut[0.899] == 17);

    CHECK(sut[0.90] == 18);
    CHECK(sut[0.91] == 18);
    CHECK(sut[0.949] == 18);

    CHECK(sut[0.95] == 19);
    CHECK(sut[0.96] == 19);
    CHECK(sut[0.999] == 19);

    CHECK(sut[1.0] == 20);
    CHECK(sut[1.01] == 20);
  }

  SUBCASE("Test getters for other metrics") {
    CHECK(sut.get_number_of_param_combos_to_cover() == 30);
    CHECK(sut.get_number_of_combinations_to_cover() == 50);
    CHECK(sut.get_covered_tuples() ==
          std::vector<unsigned long long>{1, 2, 3, 4, 5, 6, 7});
  }
}

TEST_CASE("coverage_measurement, test adding coverage of param combos") {
  using namespace citcpp;

  coverage_measurement sut;

  sut.add_coverage_of_param_combos(20, 0.69);

  for (int i = 0; i <= 13; ++i) {
    const double frac = (double)i * 0.05;
    CHECK(sut[frac] == 20);
  }

  for (int i = 14; i <= 20; ++i) {
    const double frac = (double)i * 0.05;
    CHECK(sut[frac] == 0);
  }
}
