#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest.h>

#include "../citcpp_lib/src/detail/binom_coeff_table.hpp"

TEST_CASE("Testing binomail coefficient table, max 5") {

  using namespace citcpp::detail;

  binom_coeff_table binomial_coeffs(5);

  SUBCASE("Test maximal supported n") {
    CHECK(binomial_coeffs.get_max_n() == 5);
  }

  SUBCASE("Test C(5,X)") {
    CHECK(binomial_coeffs.get_coefficient(5, 0) == 1);
    CHECK(binomial_coeffs.get_coefficient(5, 1) == 5);
    CHECK(binomial_coeffs.get_coefficient(5, 2) == 10);
    CHECK(binomial_coeffs.get_coefficient(5, 3) == 10);
    CHECK(binomial_coeffs.get_coefficient(5, 4) == 5);
    CHECK(binomial_coeffs.get_coefficient(5, 5) == 1);
    CHECK(binomial_coeffs.get_coefficient(5, 6) == 0);
  }

  SUBCASE("Test C(4,X)") {
    CHECK(binomial_coeffs.get_coefficient(4, 0) == 1);
    CHECK(binomial_coeffs.get_coefficient(4, 1) == 4);
    CHECK(binomial_coeffs.get_coefficient(4, 2) == 6);
    CHECK(binomial_coeffs.get_coefficient(4, 3) == 4);
    CHECK(binomial_coeffs.get_coefficient(4, 4) == 1);
    CHECK(binomial_coeffs.get_coefficient(4, 5) == 0);
  }

  SUBCASE("Test C(3,X)") {
    CHECK(binomial_coeffs.get_coefficient(3, 0) == 1);
    CHECK(binomial_coeffs.get_coefficient(3, 1) == 3);
    CHECK(binomial_coeffs.get_coefficient(3, 2) == 3);
    CHECK(binomial_coeffs.get_coefficient(3, 3) == 1);
    CHECK(binomial_coeffs.get_coefficient(3, 4) == 0);
  }

  SUBCASE("Test C(2,X)") {
    CHECK(binomial_coeffs.get_coefficient(2, 0) == 1);
    CHECK(binomial_coeffs.get_coefficient(2, 1) == 2);
    CHECK(binomial_coeffs.get_coefficient(2, 2) == 1);
    CHECK(binomial_coeffs.get_coefficient(2, 3) == 0);
  }

  SUBCASE("Test C(1,X)") {
    CHECK(binomial_coeffs.get_coefficient(1, 0) == 1);
    CHECK(binomial_coeffs.get_coefficient(1, 1) == 1);
    CHECK(binomial_coeffs.get_coefficient(1, 2) == 0);
  }

  SUBCASE("Test C(0,X)") {
    CHECK(binomial_coeffs.get_coefficient(0, 0) == 1);
    CHECK(binomial_coeffs.get_coefficient(0, 1) == 0);
  }
}

TEST_CASE("Testing binomail coefficient table, max 1") {
  using namespace citcpp::detail;

  binom_coeff_table binomial_coeffs(1);

  SUBCASE("Test maximal supported n") {
    CHECK(binomial_coeffs.get_max_n() == 1);
  }

  SUBCASE("Test C(1,X)") {
    CHECK(binomial_coeffs.get_coefficient(1, 0) == 1);
    CHECK(binomial_coeffs.get_coefficient(1, 1) == 1);
    CHECK(binomial_coeffs.get_coefficient(1, 2) == 0);
  }

  SUBCASE("Test C(0,X)") {
    CHECK(binomial_coeffs.get_coefficient(0, 0) == 1);
    CHECK(binomial_coeffs.get_coefficient(0, 1) == 0);
  }
}

TEST_CASE("Testing binomail coefficient table, max 0") {
  using namespace citcpp::detail;

  binom_coeff_table binomial_coeffs(0);

  SUBCASE("Test maximal supported n") {
    CHECK(binomial_coeffs.get_max_n() == 0);
  }

  SUBCASE("Test C(0,X)") {
    CHECK(binomial_coeffs.get_coefficient(0, 0) == 1);
    CHECK(binomial_coeffs.get_coefficient(0, 1) == 0);
  }
}