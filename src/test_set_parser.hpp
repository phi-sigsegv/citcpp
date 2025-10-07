#ifndef TEST_SET_PARSER_HPP_
#define TEST_SET_PARSER_HPP_

#include <citcpp/model.hpp>
#include <citcpp/test_set.hpp>
#include <memory>
#include <string_view>

namespace citcpp {
namespace detail {

class test_set_parser {
  public:
    /**
     * Creates a parser for test sets conforming to the specified model
     * concerning the parameters and their types.
     */
    test_set_parser(const model& model, std::string_view separator);

    ~test_set_parser();

    /**
     * Parses the test_set from a given string_view.
     *
     * @param sv the string_view to parse the test_set from
     * @param t the itest_set to put the parsed data into
     * @return \a true if parsing was successful, \a false otherwise
     */
    bool parse_test_set(std::string_view sv, test_set& t);

    /**
     * Returns the last error message of the parser.
     */
    std::string_view get_last_error_message() const;

  private:
    class impl;
    std::unique_ptr<impl> impl_;
};

}  // namespace detail
}  // namespace citcpp

#endif /* TEST_SET_PARSER_HPP_ */
