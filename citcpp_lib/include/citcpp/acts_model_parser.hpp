#ifndef ACTS_MODEL_PARSER_HPP_
#define ACTS_MODEL_PARSER_HPP_

#include <memory>
#include <string_view>

#include "model.hpp"

namespace citcpp {

class acts_model_parser {
  public:
    /**
     * Creates a parser for ACTS models.
     */
    acts_model_parser();

    acts_model_parser(const acts_model_parser& other) = delete;

    acts_model_parser(acts_model_parser&& other) noexcept;

    ~acts_model_parser();

    acts_model_parser& operator=(const acts_model_parser& other) = delete;

    acts_model_parser& operator=(acts_model_parser&& other) noexcept;

    /**
     * Parses the input_model from a given string_view.
     *
     * @param sv the string_view to parse the input_model from
     * @param model the input_model to put the parsed data into
     * @return \a true if parsing was successful, \a false otherwise
     */
    bool parse_input_model(std::string_view sv, model& model);

    /**
     * Returns the last error message of the parser.
     */
    std::string_view get_last_error_message() const;

  private:
    class impl;
    std::unique_ptr<impl> impl_;
};

}  // namespace citcpp

#endif /* ACTS_MODEL_PARSER_HPP_ */
