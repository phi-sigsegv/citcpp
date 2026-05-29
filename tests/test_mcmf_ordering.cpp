#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

#include <citcpp/model.hpp>

#include "../citcpp_lib/src/detail/idd_variable_ordering.hpp"
#include "../citcpp_lib/src/detail/internal_model.hpp"

using namespace citcpp;
using namespace citcpp::detail;

TEST_CASE("MCMF variable ordering heuristic") {
  model m;
  parameter p0, p1, p2, p3;
  p0.name("P0").type(parameter_type::BOOLEAN).values({true, false});  // Sub 1
  p1.name("P1").type(parameter_type::BOOLEAN).values({true, false});  // Sub 2
  p2.name("P2").type(parameter_type::BOOLEAN).values({true, false});  // Sub 3
  p3.name("P3").type(parameter_type::BOOLEAN).values({true, false});  // Master

  m.add_parameter(p0);
  m.add_parameter(p1);
  m.add_parameter(p2);
  m.add_parameter(p3);

  // P3 interacts with P0, P1, P2
  // P3 is the master (connected to 3 others)
  // P0, P1, P2 only connected to P3 (connected to 1 other)

  auto ref0 = parameter_reference("P0");
  auto ref1 = parameter_reference("P1");
  auto ref2 = parameter_reference("P2");
  auto ref3 = parameter_reference("P3");

  // C1: P3=true => P0=false
  m.add_constraint(
      std::make_shared<implication>(std::make_shared<boolean_proposition>(
                                        ref3, relational_operator::EQ, true),
                                    std::make_shared<boolean_proposition>(
                                        ref0, relational_operator::EQ, false)));

  // C2: P3=true => P1=true
  m.add_constraint(
      std::make_shared<implication>(std::make_shared<boolean_proposition>(
                                        ref3, relational_operator::EQ, true),
                                    std::make_shared<boolean_proposition>(
                                        ref1, relational_operator::EQ, true)));

  // C3: P3=false => P2=true
  m.add_constraint(
      std::make_shared<implication>(std::make_shared<boolean_proposition>(
                                        ref3, relational_operator::EQ, false),
                                    std::make_shared<boolean_proposition>(
                                        ref2, relational_operator::EQ, true)));

  internal_model im(m);
  std::vector<unsigned int> order = compute_mcmf_variable_order(im);

  // Expect P3 (index 3) to be first
  REQUIRE(order.size() == 4);
  CHECK(order[0] == 3);

  // Subsequent should be its neighbors P0, P1, P2 (indices 0, 1, 2)
  // The tie-break is original index, so 0, then 1, then 2.
  CHECK(order[1] == 0);
  CHECK(order[2] == 1);
  CHECK(order[3] == 2);
}

TEST_CASE("MCMF variable ordering, unconstrained parameters") {
  model m;
  parameter p0, p1, p2;
  p0.name("P0").type(parameter_type::BOOLEAN).values({true, false});
  p1.name("P1").type(parameter_type::BOOLEAN).values({true, false});
  p2.name("P2")
      .type(parameter_type::BOOLEAN)
      .values({true, false});  // Unconstrained

  m.add_parameter(p0);
  m.add_parameter(p1);
  m.add_parameter(p2);

  auto ref0 = parameter_reference("P0");
  auto ref1 = parameter_reference("P1");

  // C1: P0=true => P1=false
  m.add_constraint(
      std::make_shared<implication>(std::make_shared<boolean_proposition>(
                                        ref0, relational_operator::EQ, true),
                                    std::make_shared<boolean_proposition>(
                                        ref1, relational_operator::EQ, false)));

  internal_model im(m);
  std::vector<unsigned int> order = compute_mcmf_variable_order(im);

  REQUIRE(order.size() == 3);
  // P0 and P1 are connected. P2 is not.
  // P0 (idx 0) and P1 (idx 1) have same degree (1). Tie-break is original
  // index.
  CHECK(order[0] == 0);
  CHECK(order[1] == 1);
  CHECK(order[2] == 2);  // P2 at the end
}
