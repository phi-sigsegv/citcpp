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
    virtual ~test_set_data_consumer() = default;
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

    void operator()(const peg::SemanticValues&) {
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

#include <variant>

class test_set_parser::impl : test_set_data_consumer {
  public:
    impl(const model& model, std::string_view separator)
        : test_set_data_consumer(),
          model_(model),
          name_to_param_map_(),
          param_identifier_consumer_(param_identifier_consumer(this)),
          param_declarations_end_consumer_(
              param_declarations_end_consumer(this)),
          test_value_consumer_(test_value_consumer(this)),
          test_end_consumer_(test_end_consumer(this)),
          test_set_parser_(create_test_set_parser(separator)),
          error_occurred_(false),
          error_message_(),
          current_test_(),
          current_param_index_(0),
          test_set_(nullptr) {

      for (const parameter& param : model_.get_parameters()) {
        name_to_param_map_[param.get_name()] = &param;
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

    virtual ~impl() = default;

    void set_test_set(test_set* t) { test_set_ = t; }

    void set_param_identifier(const std::string& identifier) {
      auto it = name_to_param_map_.find(identifier);
      if (it != name_to_param_map_.end()) {
        test_set_->add_parameter(*(it->second));
      } else {
        error_occurred_ = true;
        std::ostringstream oss;
        oss << "Error in test set file -> Parameter '" << identifier
            << "' not found in the model";

        error_message_ = oss.str();

        // Defensively add an empty parameter to keep test_set parameters size
        // in sync
        parameter dummy;
        dummy.set_name(identifier);
        dummy.set_type(parameter_type::ENUM);
        test_set_->add_parameter(dummy);
      }
    }

    void end_param_declarations() {
      current_test_.clear();
      current_param_index_ = 0;
    }

    void parse_param_value(const std::string& value, size_t line, size_t col) {
      if (current_param_index_ < test_set_->get_parameters().size()) {
        const parameter& param =
            test_set_->get_parameters()[current_param_index_];
        if (value == "*") {
          current_test_.push_back(-1);
        } else {
          int found_idx = -1;
          const auto& param_values = param.get_values();

          if (param.get_type() == parameter_type::BOOLEAN) {
            bool bool_val = false;
            bool valid_bool = false;
            if (i_string_equals(value, "true")) {
              bool_val = true;
              valid_bool = true;
            } else if (i_string_equals(value, "false")) {
              bool_val = false;
              valid_bool = true;
            }

            if (valid_bool) {
              for (size_t i = 0; i < param_values.size(); ++i) {
                const auto& var_val = param_values[i].get_variant_value();
                if (std::holds_alternative<bool>(var_val) &&
                    std::get<bool>(var_val) == bool_val) {
                  found_idx = static_cast<int>(i);
                  break;
                }
              }
            }
          } else if (param.get_type() == parameter_type::INTEGER) {
            try {
              const int int_val = std::stoi(value);
              for (size_t i = 0; i < param_values.size(); ++i) {
                const auto& var_val = param_values[i].get_variant_value();
                if (std::holds_alternative<int>(var_val) &&
                    std::get<int>(var_val) == int_val) {
                  found_idx = static_cast<int>(i);
                  break;
                }
              }
            } catch (const std::exception& ex) {
              // std::stoi failed or type mismatch
            }
          } else if (param.get_type() == parameter_type::ENUM) {
            for (size_t i = 0; i < param_values.size(); ++i) {
              const auto& var_val = param_values[i].get_variant_value();
              if (std::holds_alternative<std::string>(var_val) &&
                  std::get<std::string>(var_val) == value) {
                found_idx = static_cast<int>(i);
                break;
              }
            }
          }

          if (found_idx != -1) {
            current_test_.push_back(found_idx);
          } else {
            error_occurred_ = true;
            std::ostringstream oss;
            oss << "Error in test set file at " << line << ":" << col
                << " -> Value '" << value
                << "' is not in the domain of parameter '" << param.get_name()
                << "'";

            error_message_ = oss.str();
            current_test_.push_back(-1);
          }
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
    std::unordered_map<std::string, const parameter*> name_to_param_map_;
    param_identifier_consumer param_identifier_consumer_;
    param_declarations_end_consumer param_declarations_end_consumer_;
    test_value_consumer test_value_consumer_;
    test_end_consumer test_end_consumer_;
    peg::parser test_set_parser_;
    bool error_occurred_;
    std::string error_message_;
    std::vector<int> current_test_;
    std::size_t current_param_index_;
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
