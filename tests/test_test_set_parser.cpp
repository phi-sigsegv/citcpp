#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest.h>

#include <citcpp/acts_model_parser.hpp>
#include <citcpp/test_set_parser.hpp>
#include <string>
#include <vector>

namespace {

citcpp::model create_model() {
  std::string model_str = R"([System]
Name: TestSetParser

[Parameter]
A (int) : 0, 1, 2, 3, 4
B (int) : 1, 2, 3
C (enum) : red, green, blue, black
D (boolean) : true, false
E (enum) : only
F (int) : -2, -1, 0, 1, 2, 3, 4, 5, 6, 7
G (enum) : x, y, z
H (bool) : TRUE, FALSE
I (int) : 10, 20, 30
J (enum) : p, q
K (int) : 1, 2, 3, 4, 5, 6
)";
  citcpp::acts_model_parser acts_parser;
  citcpp::model model;
  acts_parser.parse_input_model(model_str, model);
  return model;
}

struct parse_result {
    bool ok;
    std::string message;
    citcpp::test_set tests;
};

parse_result parse(const std::string& input, const std::string& sep = ",") {
  const citcpp::model model = create_model();
  citcpp::test_set_parser parser(model, sep);
  parse_result r;
  r.ok = parser.parse_test_set(input, r.tests);
  r.message = std::string(parser.get_last_error_message());
  return r;
}

std::vector<std::vector<int>> tests_of(const parse_result& r) {
  return {r.tests.get_list_of_tests().begin(),
          r.tests.get_list_of_tests().end()};
}

void check_error(const std::string& input, const std::string& message,
                 const std::string& sep = ",") {
  const parse_result r = parse(input, sep);
  CHECK_FALSE(r.ok);
  CHECK(r.message == "Error in test set file at " + message);
}

const std::string H = "A,B,C,D,E,F,G,H,I,J,K";
const std::string R1 = "2,3,green,true,only,1,x,TRUE,10,p,1";
const std::string R2 = "4,3,blue,false,only,5,y,FALSE,30,q,6";
const std::string R3 = "0,1,red,true,only,-2,z,TRUE,20,q,3";
const std::vector<int> V1 = {2, 2, 1, 0, 0, 3, 0, 0, 0, 0, 0};
const std::vector<int> V2 = {4, 2, 2, 1, 0, 7, 1, 1, 2, 1, 5};
const std::vector<int> V3 = {0, 0, 0, 0, 0, 0, 2, 0, 1, 1, 2};

}  // namespace

TEST_CASE("test set parser: tests and parameters") {
  const parse_result r = parse(H + "\n" + R1 + "\n" + R2 + "\n" + R3 + "\n");
  REQUIRE(r.ok);
  CHECK(r.message.empty());
  REQUIRE(r.tests.get_parameters().size() == 11);
  CHECK(r.tests.get_parameters()[2].get_name() == "C");
  CHECK(tests_of(r) == std::vector<std::vector<int>>{V1, V2, V3});
}

TEST_CASE("test set parser: line ends") {
  const std::vector<std::vector<int>> expected{V1, V2, V3};
  CHECK(tests_of(parse(H + "\r\n" + R1 + "\r\n" + R2 + "\r\n" + R3 + "\r\n")) ==
        expected);
  CHECK(tests_of(parse(H + "\r" + R1 + "\r" + R2 + "\r" + R3 + "\r")) ==
        expected);
  CHECK(tests_of(parse(H + "\r\n" + R1 + "\n" + R2 + "\r" + R3 + "\r\n")) ==
        expected);
}

TEST_CASE("test set parser: blank lines and whitespace") {
  CHECK(tests_of(parse(H + "\n\n  \n" + R1 + "\n\n\t\n" + R2 + "\n \n" + R3 +
                       "\n\n\n")) ==
        std::vector<std::vector<int>>{V1, V2, V3});
  CHECK(tests_of(parse("A , B ,C,  D,E,F,G,H,I,J,K  \n"
                       " 2 , 3 ,green , true,only,1,x,TRUE,10,p,1  \n" +
                       R2 + "\t\n")) == std::vector<std::vector<int>>{V1, V2});
  CHECK(tests_of(parse("A,\tB,C,D,E,F,G,H,I,J,K\n"
                       "2,\t3,green,true,only,1,x,TRUE,10,p,1\t\n")) ==
        std::vector<std::vector<int>>{V1});
  CHECK(tests_of(parse("  \t" + H + "\n" + R1 + "\n")) ==
        std::vector<std::vector<int>>{V1});
}

TEST_CASE("test set parser: values") {
  // Don't care values.
  CHECK(tests_of(parse(H + "\n*,3,*,true,only,* ,x,TRUE,10,p,1\n")) ==
        std::vector<std::vector<int>>{{-1, 2, -1, 0, 0, -1, 0, 0, 0, 0, 0}});
  // Boolean values in any case.
  CHECK(tests_of(parse(H + "\n2,3,green,True,only,1,x,true,10,p,1\n"
                           "2,3,green,FALSE,only,1,x,False,10,p,1\n")) ==
        std::vector<std::vector<int>>{V1, {2, 2, 1, 1, 0, 3, 0, 1, 0, 0, 0}});
  // Integers with a sign, and spaces around them.
  CHECK(tests_of(parse(H + "\n+2,3 ,green,true,only, +1,x,TRUE,10,p,1\n")) ==
        std::vector<std::vector<int>>{V1});
  CHECK(tests_of(parse(H + "\n0,1,red,true,only,-2,z,TRUE,+20,q,3\n")) ==
        std::vector<std::vector<int>>{V3});
}

TEST_CASE("test set parser: parameter declarations") {
  // Only the header.
  const parse_result header_only = parse(H + "\n");
  CHECK(header_only.ok);
  CHECK(header_only.tests.get_parameters().size() == 11);
  CHECK(header_only.tests.get_list_of_tests().empty());
  // A subset of the parameters, in any order, also repeated.
  CHECK(tests_of(parse("C,A\ngreen,2\nblue,4\n")) ==
        std::vector<std::vector<int>>{{1, 2}, {2, 4}});
  CHECK(tests_of(parse("A,A,B\n2,2,3\n")) ==
        std::vector<std::vector<int>>{{2, 2, 2}});
}

TEST_CASE("test set parser: separators") {
  std::string h = H, r1 = R1, r2 = R2;
  auto replace = [](std::string s, const std::string& with) {
    std::string out;
    for (char c : s) {
      out += c == ',' ? with : std::string(1, c);
    }
    return out;
  };
  // The separator is trimmed; spaces around it are skipped.
  CHECK(tests_of(parse(replace(h, ";;") + "\n" + replace(r1, " ;; ") + "\n" +
                           replace(r2, ";; ") + "\n",
                       " ;; ")) == std::vector<std::vector<int>>{V1, V2});
  // A single ';' is part of a value.
  check_error(replace(h, ";;") + "\n" + replace(r2, ";;") + ";\n",
              "2:46 -> Value '6;' is not in the domain of parameter 'K'", ";;");
}

TEST_CASE("test set parser: syntax errors") {
  check_error("", "1:1 -> Expected a parameter name");
  check_error("\n\n", "1:1 -> Expected a parameter name");
  check_error("\n" + H + "\n" + R1 + "\n", "1:1 -> Expected a parameter name");
  check_error("1A,B\n1,2\n", "1:1 -> Expected a parameter name");
  check_error(H + ",\n" + R1 + "\n", "1:23 -> Expected a parameter name");
  check_error("A B,C\n1,2\n",
              "1:3 -> Expected a separator or the end of the line");
  check_error(H, "1:22 -> Expected a separator or the end of the line");
  check_error(H + "\n2,,green,true,only,1,x,TRUE,10,p,1\n",
              "2:3 -> Expected a value");
  check_error(H + "\n2, ,green,true,only,1,x,TRUE,10,p,1\n",
              "2:4 -> Expected a value");
  check_error(H + "\n" + R1 + ",\n", "2:37 -> Expected a value");
  check_error(H + "\n" + R1 + "\n" + R2, "3:37 -> Expected the end of the line");
}

TEST_CASE("test set parser: invalid content") {
  check_error("A,B,C,D,E,F,G,H,I,J,Z\n" + R1 + "\n",
              "1:21 -> Parameter 'Z' not found in the model");
  check_error(H + "\n2,3,purple,true,only,1,x,TRUE,10,p,1\n" + R2 + "\n",
              "2:5 -> Value 'purple' is not in the domain of parameter 'C'");
  check_error(H + "\n2,3,gre en,true,only,1,x,TRUE,10,p,1\n",
              "2:5 -> Value 'gre en' is not in the domain of parameter 'C'");
  check_error(H + "\nx2,3,green,true,only,1,x,TRUE,10,p,1\n",
              "2:1 -> Value 'x2' is not in the domain of parameter 'A'");
  // An integer is the whole value.
  check_error(H + "\n2,3abc,green,true,only,1,x,TRUE,10,p,1\n",
              "2:3 -> Value '3abc' is not in the domain of parameter 'B'");
  check_error(H + "\n2,1.5,green,true,only,1,x,TRUE,10,p,1\n",
              "2:3 -> Value '1.5' is not in the domain of parameter 'B'");
  check_error(H + "\n2,3,green,true,only,+-1,x,TRUE,10,p,1\n",
              "2:21 -> Value '+-1' is not in the domain of parameter 'F'");
  check_error(H + "\n2,3,green,true,only,+,x,TRUE,10,p,1\n",
              "2:21 -> Value '+' is not in the domain of parameter 'F'");
  check_error(H + "\n2,3,green,true,only,1,x,TRUE,10,p,4294967297\n",
              "2:35 -> Value '4294967297' is not in the domain of parameter "
              "'K'");
  check_error(H + "\n2,3,green,yes,only,1,x,TRUE,10,p,1\n",
              "2:11 -> Value 'yes' is not in the domain of parameter 'D'");
  check_error(H + "\n" + R1 + ",5\n",
              "2:37 -> Test has more values than parameter declarations");
  check_error(H + "\n2,3,green\n" + R2 + "\n",
              "2:10 -> Test has fewer values than parameter declarations");
}

TEST_CASE("test set parser: the first error is reported") {
  check_error(H + "\n2,3\n2,3,purple,true,only,1,x,TRUE,10,p,1\n",
              "2:4 -> Test has fewer values than parameter declarations");
  check_error(H + "\n2,3,purple,true,only,1,x,TRUE,10,p,1\n" + R1 + ",\n",
              "2:5 -> Value 'purple' is not in the domain of parameter 'C'");
  // Lines ending with '\r' only are counted as lines, too.
  check_error(H + "\r" + R1 + "\r2,3,purple,true,only,1,x,TRUE,10,p,1\r",
              "3:5 -> Value 'purple' is not in the domain of parameter 'C'");
}
