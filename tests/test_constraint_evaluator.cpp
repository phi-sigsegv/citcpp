#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest.h>

#include <citcpp/acts_model_parser.hpp>
#include <string>
#include <vector>

#include "../citcpp_lib/src/detail/constraint_evaluator.hpp"
#include "../citcpp_lib/src/detail/internal_test_set.hpp"

TEST_CASE("constraint evaluator: all tests of a model with every operator") {
  std::string model_str = R"([System]
Name: Operators

[Parameter]
A (int) : 0, 1, 2, 3, 4
B (int) : 1, 2, 3
C (enum) : red, green, blue, black
D (boolean) : true, false
F (int) : -2, -1, 0, 1, 2, 3, 4, 5, 6, 7
J (enum) : p, q

[Constraint]
A > 1 && B > 1 => C != "black"
A = 0 || A = 3 || D = true
A >= 3 => F > 0
B = 1 => F <= 1 || J = "q"
D = false => F != 0
A < 2 || B >= 2
C = "green" => F > -2 && J = "p"
)";
  citcpp::acts_model_parser acts_parser;
  citcpp::model model;
  REQUIRE(acts_parser.parse_input_model(model_str, model));
  const citcpp::detail::constraint_evaluator constr_eval(model);

  const std::vector<int> a{0, 1, 2, 3, 4}, b{1, 2, 3}, f{-2, -1, 0, 1, 2,
                                                         3,  4,  5, 6, 7};
  std::size_t num_valid = 0;
  for (int ia = 0; ia < 5; ++ia) {
    for (int ib = 0; ib < 3; ++ib) {
      for (int ic = 0; ic < 4; ++ic) {
        for (int id = 0; id < 2; ++id) {
          for (int i_f = 0; i_f < 10; ++i_f) {
            for (int ij = 0; ij < 2; ++ij) {
              const int A = a[ia], B = b[ib], F = f[i_f];
              const bool black = ic == 3, green = ic == 1, D = id == 0,
                         q = ij == 1;
              auto implies = [](bool x, bool y) { return !x || y; };
              const bool expected =
                  implies(A > 1 && B > 1, !black) &&
                  (A == 0 || A == 3 || D) && implies(A >= 3, F > 0) &&
                  implies(B == 1, F <= 1 || q) && implies(!D, F != 0) &&
                  (A < 2 || B >= 2) && implies(green, F > -2 && !q);
              const std::vector<int> values{ia, ib, ic, id, i_f, ij};
              const citcpp::detail::test t(values.begin(), values.end());
              CHECK(constr_eval(t) == expected);
              num_valid += expected;
            }
          }
        }
      }
    }
  }
  // (Not trivial: some tests are valid, some are not.)
  CHECK(num_valid > 0);
  CHECK(num_valid < 2400);
}

TEST_CASE("constraint evaluator: a model without constraints") {
  std::string model_str = R"([System]
Name: Unconstrained

[Parameter]
A (int) : 0, 1
B (enum) : x, y
)";
  citcpp::acts_model_parser acts_parser;
  citcpp::model model;
  REQUIRE(acts_parser.parse_input_model(model_str, model));
  const citcpp::detail::constraint_evaluator constr_eval(model);
  const std::vector<int> values{1, 0};
  CHECK(constr_eval(citcpp::detail::test(values.begin(), values.end())));
}
