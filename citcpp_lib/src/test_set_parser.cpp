#include <peglib.h>

#include <algorithm>
#include <cctype>
#include <citcpp/test_set_parser.hpp>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

#include "parser_utils.hpp"

namespace {

class test_set_data_consumer {
  public:
    virtual void set_param_identifier(const std::string& identifier) = 0;
    virtual void end_param_declarations() = 0;
    virtual void parse_param_value(const std::string& value, size_t line,
                                   size_t col) = 0;
    virtual void end_test(size_t line, size_t col) = 0;
};

class param_identifier_consumer {
  public:
    param_identifier_consumer(test_set_data_consumer* consumer)
        : consumer_(consumer) {}

    void operator()(const peg::SemanticValues& vs) {
      consumer_->set_param_identifier(vs.token_to_string());
    }

  private:
    test_set_data_consumer* consumer_;
};

class param_declarations_end_consumer {
  public:
    param_declarations_end_consumer(test_set_data_consumer* consumer)
        : consumer_(consumer) {}

    void operator()(const peg::SemanticValues& vs) {
      consumer_->end_param_declarations();
    }

  private:
    test_set_data_consumer* consumer_;
};

class test_value_consumer {
  public:
    test_value_consumer(test_set_data_consumer* consumer)
        : consumer_(consumer) {}

    void operator()(const peg::SemanticValues& vs) {
      auto line_info = vs.line_info();
      std::string str(vs.token_to_string());
      citcpp::detail::trim(str);
      consumer_->parse_param_value(str, line_info.first, line_info.second);
    }

  private:
    test_set_data_consumer* consumer_;
};

class test_end_consumer {
  public:
    test_end_consumer(test_set_data_consumer* consumer) : consumer_(consumer) {}

    void operator()(const peg::SemanticValues& vs) {
      auto line_info = vs.line_info();
      consumer_->end_test(line_info.first, line_info.second);
    }

  private:
    test_set_data_consumer* consumer_;
};

peg::parser create_test_set_parser(std::string_view separator) {
  using namespace peg;

  std::string trimmed_separator(separator);
  citcpp::detail::trim(trimmed_separator);

  std::stringstream grammar;

  grammar << "Root             <- _ ParameterList _ ParamDelcEnd __";
  grammar << " (Test __)*\n";
  grammar << "ParameterList    <- Parameter (_ '" << trimmed_separator
          << "' _ Parameter)*\n";
  grammar << "Parameter        <- [a-zA-Z_] [a-zA-Z0-9_]*\n";
  grammar << "ParamDelcEnd     <- Eol\n";
  grammar << "Test             <- TestValueList TestEnd\n";
  grammar << "TestValueList    <- TestValue ('" << trimmed_separator
          << "' _ TestValue)*\n";
  grammar << "TestValue        <- (!('" << trimmed_separator
          << "' / SpaceChar / Eol) .)"
          << " (!('" << trimmed_separator << "' / Eol) .)*\n";
  grammar << "TestEnd          <- Eol\n";
  grammar << "~_               <- (SpaceChar)*\n";
  grammar << "~__              <- (SpaceChar / Eol)*\n";
  grammar << "SpaceChar        <- ' ' / '\t'\n";
  grammar << "Eol              <- '\r\n' / '\n' / '\r'\n";

  parser p(grammar.str());

  return p;
}

bool i_char_equals(char a, char b) {
  return std::tolower(static_cast<unsigned char>(a)) ==
         std::tolower(static_cast<unsigned char>(b));
}

bool i_string_equals(const std::string& a, const std::string& b) {
  return a.size() == b.size() &&
         std::equal(a.begin(), a.end(), b.begin(), i_char_equals);
}

}  // namespace

namespace citcpp {

class test_set_parser::impl : test_set_data_consumer {
  public:
    impl(const model& model, std::string_view separator)
        : test_set_data_consumer(),
          model_(model),
          parameter_to_type_map_(),
          param_identifier_consumer_(param_identifier_consumer(this)),
          param_declarations_end_consumer_(
              param_declarations_end_consumer(this)),
          test_value_consumer_(test_value_consumer(this)),
          test_end_consumer_(test_end_consumer(this)),
          test_set_parser_(create_test_set_parser(separator)),
          error_occurred_(false),
          error_message_(),
          current_param_(parameter_def()),
          current_test_(),
          current_param_index_(0),
          test_set_(nullptr) {

      for (const parameter& param : model_.get_parameters()) {
        parameter_to_type_map_[param.get_name()] = param.get_type();
      }

      test_set_parser_["Parameter"] = param_identifier_consumer_;
      test_set_parser_["ParamDelcEnd"] = param_declarations_end_consumer_;
      test_set_parser_["TestValue"] = test_value_consumer_;
      test_set_parser_["TestEnd"] = test_end_consumer_;

      test_set_parser_.set_logger(
          [this](size_t line, size_t col, const std::string& msg) {
            std::ostringstream oss;
            oss << "Error in test set file at " << line << ":" << col << " -> "
                << msg;

            error_message_ = oss.str();
          });
    }

    void set_test_set(test_set* t) { test_set_ = t; }

    void set_param_identifier(const std::string& identifier) {
      current_param_.set_name(identifier);

      auto it = parameter_to_type_map_.find(identifier);
      if (it != parameter_to_type_map_.end()) {
        current_param_.set_type(it->second);
      } else {
        current_param_.set_type(parameter_type::ENUM);
      }

      test_set_->add_parameter(current_param_);
    }

    void end_param_declarations() {
      current_test_.clear();
      current_param_index_ = 0;
    }

    void parse_param_value(const std::string& value, size_t line, size_t col) {
      if (current_param_index_ < test_set_->get_parameters().size()) {
        switch (test_set_->get_parameters()[current_param_index_].get_type()) {
          case parameter_type::BOOLEAN:
            if (value == "*") {
              current_test_.push_back(std::move(parameter_value(value)));
            } else {
              if (i_string_equals(value, "true")) {
                current_test_.push_back(std::move(parameter_value(true)));
              } else if (i_string_equals(value, "false")) {
                current_test_.push_back(std::move(parameter_value(false)));
              } else {
                error_occurred_ = true;
                std::ostringstream oss;
                oss << "Error in test set file at " << line << ":" << col
                    << " -> Expecting boolean value or *";

                error_message_ = oss.str();
              }
            }
            break;
          case parameter_type::INTEGER:
            if (value == "*") {
              current_test_.push_back(std::move(parameter_value(value)));
            } else {
              try {
                const int int_value{std::stoi(value)};
                current_test_.push_back(std::move(parameter_value(int_value)));
              } catch (std::invalid_argument const& ex) {
                error_occurred_ = true;
                std::ostringstream oss;
                oss << "Error in test set file at " << line << ":" << col
                    << " -> Expecting integer value or *";

                error_message_ = oss.str();
              }
            }
            break;
          case parameter_type::ENUM:
            current_test_.push_back(std::move(parameter_value(value)));
            break;
        }
      } else {
        error_occurred_ = true;
        std::ostringstream oss;
        oss << "Error in test set file at " << line << ":" << col
            << " -> Test has more values than parameter declarations";

        error_message_ = oss.str();
      }

      ++current_param_index_;
    }

    void end_test(size_t line, size_t col) {
      if (current_param_index_ == test_set_->get_parameters().size()) {
        test_set_->get_list_of_tests().push_back(current_test_);
      } else if (current_param_index_ > 0) {
        error_occurred_ = true;
        std::ostringstream oss;
        oss << "Error in test set file at " << line << ":" << col
            << " -> Test has less values than parameter declarations";

        error_message_ = oss.str();
      } else {
        // The case that the testset has too many values is handled in another
        // rule. So this else case is concerned with a test having zero values.
        // This is fine however and just indicates an empty line in the testset,
        // or a line with just whitespace. So we simply ignore that one.
      }
      current_test_.clear();
      current_param_index_ = 0;
    }

    bool parse_test_set(std::string_view sv) {
      error_occurred_ = false;
      error_message_.clear();
      current_param_index_ = 0;
      bool ret = test_set_parser_.parse(sv);

      return ret && !error_occurred_;
    }

    std::string_view get_last_error_message() const { return error_message_; }

  private:
    const model model_;
    std::unordered_map<std::string, parameter_type> parameter_to_type_map_;
    param_identifier_consumer param_identifier_consumer_;
    param_declarations_end_consumer param_declarations_end_consumer_;
    test_value_consumer test_value_consumer_;
    test_end_consumer test_end_consumer_;
    peg::parser test_set_parser_;
    bool error_occurred_;
    std::string error_message_;
    parameter_def current_param_;
    std::vector<parameter_value> current_test_;
    int current_param_index_;
    test_set* test_set_;
};

test_set_parser::test_set_parser(const model& model, std::string_view separator)
    : impl_{std::make_unique<impl>(model, separator)} {}

test_set_parser::~test_set_parser() {}

bool test_set_parser::parse_test_set(std::string_view sv, test_set& t) {
  impl_->set_test_set(&t);
  return impl_->parse_test_set(sv);
}

std::string_view test_set_parser::get_last_error_message() const {
  return impl_->get_last_error_message();
}

}  // namespace citcpp
