#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest.h>

#include <citcpp/acts_model_parser.hpp>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct parse_result {
    bool ok;
    std::string message;
    citcpp::model model;
};

parse_result parse(const std::string& input) {
  citcpp::acts_model_parser parser;
  parse_result r;
  r.ok = parser.parse_input_model(input, r.model);
  r.message = std::string(parser.get_last_error_message());
  return r;
}

template <typename T>
std::string printed(const T& t) {
  std::ostringstream oss;
  oss << t;
  return oss.str();
}

std::vector<std::string> printed_parameters(const citcpp::model& model) {
  std::vector<std::string> result;
  for (const auto& param : model.get_parameters()) {
    result.push_back(printed(param));
  }
  return result;
}

std::vector<std::string> printed_constraints(const citcpp::model& model) {
  std::vector<std::string> result;
  for (const auto& constr : model.get_constraints()) {
    result.push_back(printed(*constr));
  }
  return result;
}

void check_error(const std::string& input, const std::string& message) {
  const parse_result r = parse(input);
  CHECK_FALSE(r.ok);
  CHECK(r.message == message);
}

const std::string HEADER = "[System]\nName: M\n\n[Parameter]\n";

const std::string PARAMETERS = "A (int) : 0, 1, -2\n"
                               "D (enum) : red, dark green\n";

}  // namespace

TEST_CASE("acts model parser: sections and declarations") {
  const parse_result r = parse(R"([System]
Name: Parser Test

[Parameter]
A (int) : 0, 1, -2
B (boolean) : true, false
C (Bool) : FALSE, TRUE
D (enum) : red, dark green , blue
E (INTEGER) : 5, 6

[Relation]
R1 : (A, B, D, 2)
R2 : (C, E, 1)

[Constraint]
A = 1 => D = "red"
B = true || C != false && E > 5
(A = 0 || A = 1) && D != "dark green"
)");
  REQUIRE(r.ok);
  CHECK(r.model.get_name() == "Parser Test");
  CHECK(printed_parameters(r.model) ==
        std::vector<std::string>{"A (int) : 0, 1, -2",
                                 "B (boolean) : true, false",
                                 "C (boolean) : false, true",
                                 "D (enum) : red, dark green, blue",
                                 "E (int) : 5, 6"});

  REQUIRE(r.model.get_relations().size() == 2);
  const citcpp::relation& r1 = r.model.get_relations()[0];
  CHECK(r1.get_name() == "R1");
  CHECK(r1.get_interaction_strength() == 2);
  REQUIRE(r1.get_parameters().size() == 3);
  CHECK(r1.get_parameters()[0].get_name() == "A");
  CHECK(r1.get_parameters()[1].get_name() == "B");
  CHECK(r1.get_parameters()[2].get_name() == "D");
  const citcpp::relation& r2 = r.model.get_relations()[1];
  CHECK(r2.get_name() == "R2");
  CHECK(r2.get_interaction_strength() == 1);
  CHECK(r2.get_parameters().size() == 2);

  CHECK(printed_constraints(r.model) ==
        std::vector<std::string>{
            "(A = 1 => D = \"red\")",
            "(B = true || (C != false && E > 5))",
            "((A = 0 || A = 1) && D != \"dark green\")"});
}

TEST_CASE("acts model parser: line ends, comments and blank lines") {
  const std::string lf = "-- A model.\n[System]\nName: M\n\n[Parameter]\n"
                         "A (int) : 0, 1\nD (enum) : red,\n  green\n\n"
                         "[Constraint]\n-- A comment.\nA = 1 => D = \"red\"\n\n"
                         "A = 0\n";
  const parse_result r = parse(lf);
  REQUIRE(r.ok);
  CHECK(printed_parameters(r.model) ==
        std::vector<std::string>{"A (int) : 0, 1", "D (enum) : red, green"});
  CHECK(printed_constraints(r.model) ==
        std::vector<std::string>{"(A = 1 => D = \"red\")", "A = 0"});

  for (const std::string eol : {"\r\n", "\r"}) {
    std::string other;
    for (const char c : lf) {
      if (c == '\n') {
        other += eol;
      } else {
        other += c;
      }
    }
    const parse_result o = parse(other);
    REQUIRE(o.ok);
    CHECK(printed(o.model) == printed(r.model));
  }
}

TEST_CASE("acts model parser: syntax errors") {
  check_error("[System]\nName: M\n",
              "Error in model file at 3:1 -> Expected '['");
  check_error(HEADER + "A (int) 0, 1\n",
              "Error in model file at 5:9 -> Expected ':'");
  // The column is counted in code points.
  check_error(HEADER + "D (enum) : \xc3\xa9, \xc3\xbc;\n",
              "Error in model file at 5:16 -> Expected ',' or the end of the "
              "line");
  check_error(HEADER + PARAMETERS + "[Constraint]\nA = 1 x\n",
              "Error in model file at 8:7 -> Expected '&&', '||', '=>' or the "
              "end of the line");
}

TEST_CASE("acts model parser: layout") {
  const parse_result r = parse(
      "[System]\nName: A -- name  \n\n[Parameter]\n  A (int) : 0, 1\n\n"
      "-- A comment.\n\tD (enum) : red -- note, green\n"
      "[Constraint]\nA = 1 => D = \"red -- note\"  \n-- A comment.\nA = 0\n"
      "A = 1");
  REQUIRE(r.ok);
  CHECK(r.model.get_name() == "A -- name");
  CHECK(printed_parameters(r.model) ==
        std::vector<std::string>{"A (int) : 0, 1",
                                 "D (enum) : red -- note, green"});
  CHECK(printed_constraints(r.model) ==
        std::vector<std::string>{"(A = 1 => D = \"red -- note\")", "A = 0",
                                 "A = 1"});

  // The last line needs no end of line.
  CHECK(parse(HEADER + "A (int) : 0, 1").ok);
  CHECK(parse(HEADER + "A (int) : 0, 1\n-- A comment.").ok);
  // An empty name.
  const parse_result empty = parse("[System]\nName:\n[Parameter]\nA (int) : 0\n");
  REQUIRE(empty.ok);
  CHECK(empty.model.get_name().empty());

  CHECK(parse(HEADER + PARAMETERS + "[Constraint]").ok);
  // A relation can span lines.
  const parse_result rel =
      parse(HEADER + PARAMETERS + "[Relation]\nR : (A,\n  D, 2)\n");
  REQUIRE(rel.ok);
  CHECK(rel.model.get_relations().size() == 1);

  // Comments are lines of their own.
  check_error(HEADER + "A (int) : 0, 1 -- A comment.\nB (int) : 0, 1\n",
              "Error in model file at 5:16 -> Expected ',' or the end of the "
              "line");
  CHECK_FALSE(parse(HEADER + "B (bool) : true, false -- A comment.\n").ok);
  CHECK_FALSE(parse(HEADER + "A (int) : -- A comment.\n0, 1\n").ok);
  CHECK_FALSE(parse("[System]\nName: M\n[Parameter] -- A comment.\n"
                    "A (int) : 0\n")
                  .ok);
  CHECK_FALSE(parse(HEADER + PARAMETERS +
                    "[Relation]\nR : (A, D, 2) -- A comment.\n")
                  .ok);
  CHECK_FALSE(
      parse(HEADER + PARAMETERS + "[Constraint]\nA = 0 -- A comment.\n").ok);

  // Relations are one per line, and so are section headers.
  CHECK_FALSE(parse(HEADER + PARAMETERS +
                    "[Relation]\nR1 : (A, D, 2) R2 : (A, D, 1)\n")
                  .ok);
  CHECK_FALSE(parse("[System] Name: M\n[Parameter]\nA (int) : 0\n").ok);
}

TEST_CASE("acts model parser: semantic errors") {
  check_error(HEADER + PARAMETERS + "[Relation]\nR : (A, X, 2)\n",
              "Error in relation at 8:9 -> Cannot find declaration of "
              "parameter X");
  check_error(HEADER + PARAMETERS + "[Constraint]\nB = 1\n",
              "Error in constraint at 8:1 -> Cannot find declaration of "
              "parameter B");
  check_error(HEADER + PARAMETERS + "[Constraint]\nA = 1 && D < \"red\"\n",
              "Error in constraint at 8:10 -> Cannot have operator for a "
              "non-integer parameter that needs ordered values");
}

TEST_CASE("acts model parser: integers") {
  const parse_result r =
      parse(HEADER + "A (int) : +5, -3, 2147483647, -2147483648\n"
                     "[Constraint]\nA = +5\n");
  REQUIRE(r.ok);
  CHECK(printed_parameters(r.model) ==
        std::vector<std::string>{"A (int) : 5, -3, 2147483647, -2147483648"});
  CHECK(printed_constraints(r.model) == std::vector<std::string>{"A = 5"});

  check_error(HEADER + "A (int) : 0, 2147483648\n",
              "Error in model file at 5:14 -> Integer value out of range");
  check_error(HEADER + PARAMETERS + "[Constraint]\nA = -99999999999\n",
              "Error in model file at 8:5 -> Integer value out of range");
  check_error(HEADER + PARAMETERS + "[Relation]\nR : (A, D, 99999999999)\n",
              "Error in model file at 8:12 -> Integer value out of range");
}

TEST_CASE("acts model parser: the first error is reported") {
  check_error(HEADER + PARAMETERS + "[Constraint]\nB = 1\nC = 1\n",
              "Error in constraint at 8:1 -> Cannot find declaration of "
              "parameter B");
}
