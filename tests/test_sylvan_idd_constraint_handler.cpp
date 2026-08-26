#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest.h>

#include <citcpp/citcpp.hpp>

#include "../citcpp_lib/src/detail/constraint_handler_sylvan_ldd.hpp"
#include "../citcpp_lib/src/detail/internal_model.hpp"
#include "../citcpp_lib/src/detail/internal_test_set.hpp"

namespace {

citcpp::model create_simple_five_param_model() {
  using namespace citcpp;

  model model;

  model.add_parameter(parameter()
                          .type(parameter_type::INTEGER)
                          .name("P1")
                          .values({{10}, {11}, {12}, {13}, {14}, {15}}));
  model.add_parameter(parameter()
                          .type(parameter_type::INTEGER)
                          .name("P2")
                          .values({{100}, {200}, {300}, {400}, {500}}));
  model.add_parameter(parameter()
                          .type(parameter_type::INTEGER)
                          .name("P3")
                          .values({{5}, {6}, {7}, {8}}));
  model.add_parameter(parameter()
                          .type(parameter_type::INTEGER)
                          .name("P4")
                          .values({{5000}, {6000}, {7000}}));
  model.add_parameter(parameter()
                          .type(parameter_type::INTEGER)
                          .name("P5")
                          .values({{-13}, {-11}}));

  return model;
}

TEST_CASE("constraint handler sylvan IDD, testing atomic prop") {
  using namespace citcpp;
  using namespace citcpp::detail;

  model model(create_simple_five_param_model());

  auto P2_GT_200 = std::make_shared<int_proposition>(
      parameter_reference("P2"), relational_operator::GT, 200);
  model.add_constraint(std::move(P2_GT_200));

  internal_model i_model(model);

  constraint_handler_sylvan_idd c_handler(i_model, 1,
                                          (std::size_t)1 * 1024 * 1024 * 1024);
  std::cout << "Number of nodes: " << c_handler.node_count() << std::endl;
  std::cout << "SAT count: " << c_handler.sat_count() << std::endl;

  test valid_test_1({-1, 2, 3, 2, 1});
  CHECK(c_handler.is_valid_partial_test(valid_test_1));

  test valid_test_2({-1, 4, -1, -1, -1});
  CHECK(c_handler.is_valid_partial_test(valid_test_2));

  test invalid_test_1({-1, 0, -1, -1, -1});
  CHECK(!c_handler.is_valid_partial_test(invalid_test_1));

  test invalid_test_2({-1, 1, -1, -1, -1});
  CHECK(!c_handler.is_valid_partial_test(invalid_test_2));

  test test_with_dont_care({-1, -1, -1, -1, -1});
  auto values =
      c_handler.get_valid_parameter_assignments(test_with_dont_care, 1);
  CHECK(values.count() == 3);
  CHECK(values.at(2));
  CHECK(values.at(3));
  CHECK(values.at(4));
}

TEST_CASE(
    "constraint handler sylvan IDD, testing atomic prop, out of domain value") {
  using namespace citcpp;
  using namespace citcpp::detail;

  model model(create_simple_five_param_model());

  auto P2_GT_250 = std::make_shared<int_proposition>(
      parameter_reference("P2"), relational_operator::GT, 250);
  model.add_constraint(std::move(P2_GT_250));

  internal_model i_model(model);

  constraint_handler_sylvan_idd c_handler(i_model, 1,
                                          (std::size_t)1 * 1024 * 1024 * 1024);
  std::cout << "Number of nodes: " << c_handler.node_count() << std::endl;
  std::cout << "SAT count: " << c_handler.sat_count() << std::endl;

  test valid_test_1({-1, 2, 3, 2, 1});
  CHECK(c_handler.is_valid_partial_test(valid_test_1));

  test valid_test_2({-1, 4, -1, -1, -1});
  CHECK(c_handler.is_valid_partial_test(valid_test_2));

  test invalid_test_1({-1, 0, -1, -1, -1});
  CHECK(!c_handler.is_valid_partial_test(invalid_test_1));

  test invalid_test_2({-1, 1, -1, -1, -1});
  CHECK(!c_handler.is_valid_partial_test(invalid_test_2));

  test test_with_dont_care({-1, -1, -1, -1, -1});
  auto values =
      c_handler.get_valid_parameter_assignments(test_with_dont_care, 1);
  CHECK(values.count() == 3);
  CHECK(values.at(2));
  CHECK(values.at(3));
  CHECK(values.at(4));
}

TEST_CASE("constraint handler sylvan IDD, testing atomic prop & AND") {
  using namespace citcpp;
  using namespace citcpp::detail;

  model model(create_simple_five_param_model());

  auto P2_GT_200 = std::make_shared<int_proposition>(
      parameter_reference("P2"), relational_operator::GT, 200);

  auto P4_NEQ_6000 = std::make_shared<int_proposition>(
      parameter_reference("P4"), relational_operator::NEQ, 6000);

  std::vector<std::shared_ptr<constraint>> ops;
  ops.push_back(std::move(P2_GT_200));
  ops.push_back(std::move(P4_NEQ_6000));
  model.add_constraint(std::make_shared<and_expression>(std::move(ops)));

  internal_model i_model(model);

  constraint_handler_sylvan_idd c_handler(i_model, 1,
                                          (std::size_t)1 * 1024 * 1024 * 1024);
  std::cout << "Number of nodes: " << c_handler.node_count() << std::endl;
  std::cout << "SAT count: " << c_handler.sat_count() << std::endl;

  test valid_test_1({-1, 2, 3, 2, 1});
  CHECK(c_handler.is_valid_partial_test(valid_test_1));

  test valid_test_2({-1, 4, -1, -1, -1});
  CHECK(c_handler.is_valid_partial_test(valid_test_2));

  test invalid_test_1({-1, 0, -1, -1, -1});
  CHECK(!c_handler.is_valid_partial_test(invalid_test_1));

  test invalid_test_2({-1, 3, -1, 1, -1});
  CHECK(!c_handler.is_valid_partial_test(invalid_test_2));

  test test_with_dont_care({-1, -1, -1, -1, -1});
  auto values =
      c_handler.get_valid_parameter_assignments(test_with_dont_care, 1);
  CHECK(values.count() == 3);
  CHECK(values.at(2));
  CHECK(values.at(3));
  CHECK(values.at(4));
}

TEST_CASE("constraint handler sylvan IDD, testing atomic prop & OR") {
  using namespace citcpp;
  using namespace citcpp::detail;

  model model(create_simple_five_param_model());

  auto P2_GT_200 = std::make_shared<int_proposition>(
      parameter_reference("P2"), relational_operator::GT, 200);

  auto P4_NEQ_6000 = std::make_shared<int_proposition>(
      parameter_reference("P4"), relational_operator::NEQ, 6000);

  std::vector<std::shared_ptr<constraint>> ops;
  ops.push_back(std::move(P2_GT_200));
  ops.push_back(std::move(P4_NEQ_6000));
  model.add_constraint(std::make_shared<or_expression>(std::move(ops)));

  internal_model i_model(model);

  constraint_handler_sylvan_idd c_handler(i_model, 1,
                                          (std::size_t)1 * 1024 * 1024 * 1024);
  std::cout << "Number of nodes: " << c_handler.node_count() << std::endl;
  std::cout << "SAT count: " << c_handler.sat_count() << std::endl;

  test valid_test_1({-1, 2, 3, 1, 1});
  CHECK(c_handler.is_valid_partial_test(valid_test_1));

  test valid_test_2({-1, 0, -1, 0, -1});
  CHECK(c_handler.is_valid_partial_test(valid_test_2));

  test invalid_test_1({-1, 0, -1, 1, -1});
  CHECK(!c_handler.is_valid_partial_test(invalid_test_1));

  test invalid_test_2({-1, 1, -1, 1, -1});
  CHECK(!c_handler.is_valid_partial_test(invalid_test_2));

  test test_with_dont_care_1({-1, -1, -1, -1, -1});
  auto values_1 =
      c_handler.get_valid_parameter_assignments(test_with_dont_care_1, 1);
  CHECK(values_1.count() == 5);
  CHECK(values_1.at(0));
  CHECK(values_1.at(1));
  CHECK(values_1.at(2));
  CHECK(values_1.at(3));
  CHECK(values_1.at(4));

  test test_with_dont_care_2({-1, -1, -1, 1, -1});
  auto values_2 =
      c_handler.get_valid_parameter_assignments(test_with_dont_care_2, 1);
  CHECK(values_2.count() == 3);
  CHECK(values_2.at(2));
  CHECK(values_2.at(3));
  CHECK(values_2.at(4));

  c_handler.replace_dont_care_values(test_with_dont_care_2);
  CHECK(test_with_dont_care_2.get_values()[0] == 0);
  CHECK(test_with_dont_care_2.get_values()[1] >= 2);
  CHECK(test_with_dont_care_2.get_values()[2] == 0);
  CHECK(test_with_dont_care_2.get_values()[3] == 1);
  CHECK(test_with_dont_care_2.get_values()[4] == 0);
}

TEST_CASE("constraint handler sylvan IDD, testing atomic prop & IMPL") {
  using namespace citcpp;
  using namespace citcpp::detail;

  model model(create_simple_five_param_model());

  auto P2_GT_200 = std::make_shared<int_proposition>(
      parameter_reference("P2"), relational_operator::GT, 200);

  auto P4_NEQ_5000 = std::make_shared<int_proposition>(
      parameter_reference("P4"), relational_operator::NEQ, 5000);

  model.add_constraint(std::make_shared<implication>(std::move(P2_GT_200),
                                                     std::move(P4_NEQ_5000)));

  internal_model i_model(model);

  constraint_handler_sylvan_idd c_handler(i_model, 1,
                                          (std::size_t)1 * 1024 * 1024 * 1024);
  std::cout << "Number of nodes: " << c_handler.node_count() << std::endl;
  std::cout << "SAT count: " << c_handler.sat_count() << std::endl;

  test valid_test_1({-1, 2, 3, 2, 1});
  CHECK(c_handler.is_valid_partial_test(valid_test_1));

  test valid_test_2({-1, 3, -1, 1, -1});
  CHECK(c_handler.is_valid_partial_test(valid_test_2));

  test valid_test_3({-1, 0, -1, 0, -1});
  CHECK(c_handler.is_valid_partial_test(valid_test_3));

  test valid_test_4({-1, 1, -1, 0, -1});
  CHECK(c_handler.is_valid_partial_test(valid_test_4));

  test invalid_test_1({-1, 2, -1, 0, -1});
  CHECK(!c_handler.is_valid_partial_test(invalid_test_1));

  test invalid_test_2({-1, 3, -1, 0, -1});
  CHECK(!c_handler.is_valid_partial_test(invalid_test_2));

  test test_with_dont_care_1({-1, -1, -1, -1, -1});
  auto values_1 =
      c_handler.get_valid_parameter_assignments(test_with_dont_care_1, 1);
  CHECK(values_1.count() == 5);
  CHECK(values_1.at(0));
  CHECK(values_1.at(1));
  CHECK(values_1.at(2));
  CHECK(values_1.at(3));
  CHECK(values_1.at(4));

  test test_with_dont_care_2({-1, -1, -1, 0, -1});
  auto values_2 =
      c_handler.get_valid_parameter_assignments(test_with_dont_care_2, 1);
  CHECK(values_2.count() == 2);
  CHECK(values_2.at(0));
  CHECK(values_2.at(1));

  test test_with_dont_care_3({-1, 4, -1, -1, -1});
  auto values_3 =
      c_handler.get_valid_parameter_assignments(test_with_dont_care_3, 3);
  CHECK(values_3.count() == 2);
  CHECK(values_3.at(1));
  CHECK(values_3.at(2));

  c_handler.replace_dont_care_values(test_with_dont_care_3);
  CHECK(test_with_dont_care_3.get_values()[0] == 0);
  CHECK(test_with_dont_care_3.get_values()[1] == 4);
  CHECK(test_with_dont_care_3.get_values()[2] == 0);
  CHECK(test_with_dont_care_3.get_values()[3] >= 1);
  CHECK(test_with_dont_care_3.get_values()[4] == 0);
}

TEST_CASE(
    "constraint handler sylvan IDD, testing atomic prop & IMPL implies prop") {
  using namespace citcpp;
  using namespace citcpp::detail;

  model model(create_simple_five_param_model());

  auto P2_GT_200 = std::make_shared<int_proposition>(
      parameter_reference("P2"), relational_operator::GT, 200);

  auto P3_GE_7 = std::make_shared<int_proposition>(parameter_reference("P3"),
                                                   relational_operator::GE, 7);

  auto P4_NEQ_5000 = std::make_shared<int_proposition>(
      parameter_reference("P4"), relational_operator::NEQ, 5000);

  auto nested_impl =
      std::make_shared<implication>(std::move(P2_GT_200), std::move(P3_GE_7));

  model.add_constraint(std::make_shared<implication>(std::move(nested_impl),
                                                     std::move(P4_NEQ_5000)));

  internal_model i_model(model);

  constraint_handler_sylvan_idd c_handler(i_model, 1,
                                          (std::size_t)1 * 1024 * 1024 * 1024);
  std::cout << "Number of nodes: " << c_handler.node_count() << std::endl;
  std::cout << "SAT count: " << c_handler.sat_count() << std::endl;

  test valid_test_1({-1, 2, 3, 2, 1});
  CHECK(c_handler.is_valid_partial_test(valid_test_1));

  test valid_test_2({-1, 3, -1, 0, -1});
  CHECK(c_handler.is_valid_partial_test(valid_test_2));

  test valid_test_3({-1, 3, -1, 1, -1});
  CHECK(c_handler.is_valid_partial_test(valid_test_3));

  test valid_test_4({-1, 2, 0, 0, -1});
  CHECK(c_handler.is_valid_partial_test(valid_test_4));

  test invalid_test_1({-1, 2, 2, 0, -1});
  CHECK(!c_handler.is_valid_partial_test(invalid_test_1));

  test invalid_test_2({-1, 3, 3, 0, -1});
  CHECK(!c_handler.is_valid_partial_test(invalid_test_2));

  test invalid_test_3({-1, 1, 0, 1, -1});
  CHECK(c_handler.is_valid_partial_test(invalid_test_3));

  test test_with_dont_care_1({-1, -1, -1, -1, -1});
  auto values_1 =
      c_handler.get_valid_parameter_assignments(test_with_dont_care_1, 1);
  CHECK(values_1.count() == 5);
  CHECK(values_1.at(0));
  CHECK(values_1.at(1));
  CHECK(values_1.at(2));
  CHECK(values_1.at(3));
  CHECK(values_1.at(4));

  test test_with_dont_care_2({-1, -1, -1, 0, -1});
  auto values_2 =
      c_handler.get_valid_parameter_assignments(test_with_dont_care_2, 1);
  CHECK(values_2.count() == 3);
  CHECK(values_2.at(2));
  CHECK(values_2.at(3));
  CHECK(values_2.at(4));

  values_2 =
      c_handler.get_valid_parameter_assignments(test_with_dont_care_2, 2);
  CHECK(values_2.count() == 2);
  CHECK(values_2.at(0));
  CHECK(values_2.at(1));

  test test_with_dont_care_3({-1, 4, 2, -1, -1});
  auto values_3 =
      c_handler.get_valid_parameter_assignments(test_with_dont_care_3, 3);
  CHECK(values_3.count() == 2);
  CHECK(values_3.at(1));
  CHECK(values_3.at(2));

  test test_with_dont_care_4({-1, 4, -1, 0, -1});
  auto values_4 =
      c_handler.get_valid_parameter_assignments(test_with_dont_care_4, 2);
  CHECK(values_4.count() == 2);
  CHECK(values_4.at(0));
  CHECK(values_4.at(1));

  c_handler.replace_dont_care_values(test_with_dont_care_2);
  CHECK(test_with_dont_care_2.get_values()[0] == 0);
  CHECK(test_with_dont_care_2.get_values()[1] >= 2);
  CHECK(test_with_dont_care_2.get_values()[2] <= 1);
  CHECK(test_with_dont_care_2.get_values()[3] == 0);
  CHECK(test_with_dont_care_2.get_values()[4] == 0);

  c_handler.replace_dont_care_values(test_with_dont_care_3);
  CHECK(test_with_dont_care_3.get_values()[0] == 0);
  CHECK(test_with_dont_care_3.get_values()[1] == 4);
  CHECK(test_with_dont_care_3.get_values()[2] == 2);
  CHECK(test_with_dont_care_3.get_values()[3] >= 1);
  CHECK(test_with_dont_care_3.get_values()[4] == 0);
}

TEST_CASE("constraint handler sylvan IDD, testing more cmplex constraints") {
  using namespace citcpp;
  using namespace citcpp::detail;

  model model;

  model.add_parameter(parameter()
                          .type(parameter_type::INTEGER)
                          .name("P1")
                          .values({{0}, {1}, {2}, {3}}));
  model.add_parameter(parameter()
                          .type(parameter_type::INTEGER)
                          .name("P2")
                          .values({{0}, {1}, {2}, {3}}));
  model.add_parameter(
      parameter().type(parameter_type::INTEGER).name("P3").values({{0}, {1}}));

  {
    std::vector<std::shared_ptr<constraint>> ops;
    ops.push_back(std::make_shared<and_expression>(
        std::vector<std::shared_ptr<constraint>>(
            {std::make_shared<int_proposition>(parameter_reference("P3"),
                                               relational_operator::EQ, 1),
             std::make_shared<int_proposition>(parameter_reference("P2"),
                                               relational_operator::EQ, 2)})));
    ops.push_back(std::make_shared<and_expression>(
        std::vector<std::shared_ptr<constraint>>(
            {std::make_shared<int_proposition>(parameter_reference("P3"),
                                               relational_operator::EQ, 0),
             std::make_shared<int_proposition>(parameter_reference("P2"),
                                               relational_operator::EQ, 1)})));
    ops.push_back(std::make_shared<and_expression>(
        std::vector<std::shared_ptr<constraint>>(
            {std::make_shared<int_proposition>(parameter_reference("P3"),
                                               relational_operator::EQ, 0),
             std::make_shared<int_proposition>(parameter_reference("P2"),
                                               relational_operator::EQ, 2)})));
    ops.push_back(std::make_shared<and_expression>(
        std::vector<std::shared_ptr<constraint>>(
            {std::make_shared<int_proposition>(parameter_reference("P3"),
                                               relational_operator::EQ, 1),
             std::make_shared<int_proposition>(parameter_reference("P2"),
                                               relational_operator::EQ, 3)})));
    ops.push_back(std::make_shared<and_expression>(
        std::vector<std::shared_ptr<constraint>>(
            {std::make_shared<int_proposition>(parameter_reference("P3"),
                                               relational_operator::EQ, 0),
             std::make_shared<int_proposition>(parameter_reference("P2"),
                                               relational_operator::EQ, 3)})));

    model.add_constraint(std::make_shared<or_expression>(std::move(ops)));
  }

  {
    std::vector<std::shared_ptr<constraint>> ops;
    ops.push_back(std::make_shared<and_expression>(
        std::vector<std::shared_ptr<constraint>>(
            {std::make_shared<int_proposition>(parameter_reference("P1"),
                                               relational_operator::EQ, 1),
             std::make_shared<int_proposition>(parameter_reference("P2"),
                                               relational_operator::EQ, 0)})));
    ops.push_back(std::make_shared<and_expression>(
        std::vector<std::shared_ptr<constraint>>(
            {std::make_shared<int_proposition>(parameter_reference("P1"),
                                               relational_operator::EQ, 2),
             std::make_shared<int_proposition>(parameter_reference("P2"),
                                               relational_operator::EQ, 0)})));
    ops.push_back(std::make_shared<and_expression>(
        std::vector<std::shared_ptr<constraint>>(
            {std::make_shared<int_proposition>(parameter_reference("P1"),
                                               relational_operator::EQ, 0),
             std::make_shared<int_proposition>(parameter_reference("P2"),
                                               relational_operator::EQ, 1)})));
    ops.push_back(std::make_shared<and_expression>(
        std::vector<std::shared_ptr<constraint>>(
            {std::make_shared<int_proposition>(parameter_reference("P1"),
                                               relational_operator::EQ, 3),
             std::make_shared<int_proposition>(parameter_reference("P2"),
                                               relational_operator::EQ, 2)})));

    model.add_constraint(std::make_shared<or_expression>(std::move(ops)));
  }

  internal_model i_model(model);

  constraint_handler_sylvan_idd c_handler(i_model, 1,
                                          (std::size_t)1 * 1024 * 1024 * 1024);
  std::cout << "Number of nodes: " << c_handler.node_count() << std::endl;
  std::cout << "SAT count: " << c_handler.sat_count() << std::endl;

  CHECK(c_handler.sat_count() == 3.0);
}

}  // namespace
