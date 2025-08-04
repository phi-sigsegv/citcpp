#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest.h>

#include "../citcpp_lib/src/detail/binom_coeff_table.hpp"

TEST_CASE("Testing binomail coefficient table, max 5") {

  using namespace citcpp::detail;

  binom_coeff_table binomial_coeffs(5);

  SUBCASE("Test maximal supported n") {
    CHECK(binomial_coeffs.get_max_n() == 5);
  }
}