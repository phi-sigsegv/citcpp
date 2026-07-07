#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

#include <citcpp/model.hpp>
#include <memory>

#include "../citcpp_lib/src/detail/constraint_handler_sylvan_ldd.hpp"
#include "../citcpp_lib/src/detail/internal_model.hpp"
#include "../citcpp_lib/src/detail/internal_test_set.hpp"

using namespace citcpp;
using namespace citcpp::detail;

TEST_CASE("constraint handler sylvan IDD, testing custom variable order") {
  model m;
  parameter p1;
  p1.name("P1").type(parameter_type::BOOLEAN).values({true, false});
  parameter p2;
  p2.name("P2").type(parameter_type::BOOLEAN).values({true, false});
  parameter p3;
  p3.name("P3").type(parameter_type::BOOLEAN).values({true, false});

  m.add_parameter(p1);
  m.add_parameter(p2);
  m.add_parameter(p3);

  // Constraint: P1 = true => P3 = false
  auto prop1 = std::make_shared<boolean_proposition>(
      parameter_reference("P1"), relational_operator::EQ, true);
  auto prop3 = std::make_shared<boolean_proposition>(
      parameter_reference("P3"), relational_operator::EQ, false);
  m.add_constraint(std::make_shared<implication>(prop1, prop3));

  internal_model im(m);

  // Custom order: P3 (level 0), P1 (level 1), P2 (level 2)
  // parameter_index_map[level] = param_idx
  // P1: idx 0, P2: idx 1, P3: idx 2
  std::vector<unsigned int> custom_order = {2, 0, 1};

  constraint_handler_sylvan_idd handler(im, custom_order, 1,
                                        (std::size_t)1 * 1024 * 1024 * 1024);

  SUBCASE("is_valid_partial_test") {
    test t1({1, -1,
             1});  // P1=false, P2=dont_care, P3=false -> valid (premise false)
    // Note: internal representation of true is 0, false is 1 for ACTS boolean
    // Wait, let's check how boolean values are mapped.
    // In constraint_handler_sylvan_ldd.cpp:
    // for (const auto& value : param.get_values()) {
    //   bool value_as_bool = value;
    //   if (value_as_bool == prop.get_compared_value()) break;
    //   ++value_index;
    // }
    // If values are {true, false}, then true is index 0, false is index 1.

    test t_valid({1, -1, 0});  // P1=false, P3=true -> valid (premise false)
    CHECK(handler.is_valid_partial_test(t_valid) == true);

    test t_invalid(
        {0, -1, 0});  // P1=true, P3=true -> invalid (violates implication)
    CHECK(handler.is_valid_partial_test(t_invalid) == false);

    test t_valid2({0, -1, 1});  // P1=true, P3=false -> valid
    CHECK(handler.is_valid_partial_test(t_valid2) == true);
  }

  SUBCASE("get_valid_parameter_assignments") {
    test t({0, -1, -1});  // P1=true
    // P3 must be false (index 1)
    bitset_uint64 valid_p3 = handler.get_valid_parameter_assignments(t, 2);
    CHECK(valid_p3.test(0) == false);  // true is index 0
    CHECK(valid_p3.test(1) == true);   // false is index 1

    // P2 is unconstrained
    bitset_uint64 valid_p2 = handler.get_valid_parameter_assignments(t, 1);
    CHECK(valid_p2.test(0) == true);
    CHECK(valid_p2.test(1) == true);
  }

  SUBCASE("replace_dont_care_values") {
    test t({0, -1, -1});  // P1=true
    handler.replace_dont_care_values(t);
    CHECK(t.get_values()[0] == 0);  // P1 still true
    CHECK(t.get_values()[2] == 1);  // P3 must be false (index 1)
    CHECK(t.get_values()[1] == 0);  // P2 replaced by 0 (default)
  }

  SUBCASE("mark_valid_tuples") {
    // Combination of P1 (idx 0) and P3 (idx 2)
    param_vector p_indices = {0, 2};
    coverage_bitset cov(4);
    handler.mark_valid_tuples(cov, p_indices);

    // P1=true(0), P3=true(0) -> index 0*2 + 0 = 0 -> invalid
    CHECK(cov.is_valid(0) == false);
    // P1=true(0), P3=false(1) -> index 0*2 + 1 = 1 -> valid
    CHECK(cov.is_valid(1) == true);
    // P1=false(1), P3=true(0) -> index 1*2 + 0 = 2 -> valid
    CHECK(cov.is_valid(2) == true);
    // P1=false(1), P3=false(1) -> index 1*2 + 1 = 3 -> valid
    CHECK(cov.is_valid(3) == true);
  }
}
