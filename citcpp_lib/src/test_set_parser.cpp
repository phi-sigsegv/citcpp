#include <array>
#include <citcpp/test_set_parser.hpp>
#include <functional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include "parser_utils.hpp"

namespace citcpp {

/**
 * The parser of the test set format: a line with the names of the
 * parameters, followed by one line per test with its values ('*' for a don't
 * care value), separated by the separator. In detail, as a PEG (Sep is the
 * trimmed separator):
 *
 *   Root          <- _ ParameterList _ Eol __ (Test __)*
 *   ParameterList <- Parameter (_ Sep _ Parameter)*
 *   Parameter     <- [a-zA-Z_] [a-zA-Z0-9_]*
 *   Test          <- TestValue (Sep _ TestValue)* Eol
 *   TestValue     <- (!(Sep / SpaceChar / Eol) .) (!(Sep / Eol) .)*
 *   _             <- SpaceChar*
 *   __            <- (SpaceChar / Eol)*
 *   SpaceChar     <- ' ' / '\t'
 *   Eol           <- '\r\n' / '\n' / '\r'
 *
 * The values are trimmed. The input is scanned directly in one pass, and the
 * parsing stops at the first error. Positions are line:column, counting
 * columns in UTF-8 code points.
 */
class test_set_parser::impl {
  public:
    impl(const model& model, std::string_view separator)
        : model_(model), separator_(detail::trimmed(separator)) {
      for (const parameter& param : model_.get_parameters()) {
        name_to_param_map_[param.get_name()] = &param;
      }
    }

    bool parse_test_set(std::string_view sv, test_set& t) {
      input_ = sv;
      pos_ = 0;
      line_ = 1;
      line_start_ = 0;
      test_set_ = &t;
      error_message_.clear();
      return parse_root();
    }

    std::string_view get_last_error_message() const { return error_message_; }

  private:
    // Root <- _ ParameterList _ Eol __ (Test __)*
    bool parse_root() {
      skip_spaces();
      if (!parse_parameter_list()) {
        return false;
      }
      skip_spaces();
      if (!eol()) {
        return syntax_error("Expected a separator or the end of the line");
      }
      init_value_indices();
      skip_spaces_and_eols();
      while (pos_ < input_.size()) {
        if (!parse_test()) {
          return false;
        }
        skip_spaces_and_eols();
      }
      return true;
    }

    // ParameterList <- Parameter (_ Sep _ Parameter)*
    bool parse_parameter_list() {
      if (!parse_parameter()) {
        return error_message_.empty()
                   ? syntax_error("Expected a parameter name")
                   : false;
      }
      while (true) {
        const std::size_t backtrack = pos_;
        skip_spaces();
        if (!at_separator()) {
          pos_ = backtrack;
          return true;
        }
        pos_ += separator_.size();
        skip_spaces();
        if (!parse_parameter()) {
          // (A separator not followed by a name cannot end the list either:
          // the line has to end after it.)
          return error_message_.empty()
                     ? syntax_error("Expected a parameter name")
                     : false;
        }
      }
    }

    // Parameter <- [a-zA-Z_] [a-zA-Z0-9_]*; adds the parameter to the test
    // set. Returns false without a message if there is no name at the
    // position, and with a message if the parameter is not in the model.
    bool parse_parameter() {
      if (pos_ >= input_.size() || !detail::is_identifier_start(input_[pos_])) {
        return false;
      }
      const std::size_t start = pos_;
      while (pos_ < input_.size() && detail::is_identifier_char(input_[pos_])) {
        ++pos_;
      }
      const std::string_view name = input_.substr(start, pos_ - start);
      auto it = name_to_param_map_.find(name);
      if (it == name_to_param_map_.end()) {
        return error_at(start, "Parameter '" + std::string(name) +
                                   "' not found in the model");
      }
      test_set_->add_parameter(*(it->second));
      return true;
    }

    // Test <- TestValue (Sep _ TestValue)* Eol
    bool parse_test() {
      current_test_.clear();
      while (true) {
        if (!parse_test_value()) {
          return false;
        }
        if (!at_separator()) {
          break;
        }
        pos_ += separator_.size();
        skip_spaces();
      }
      if (!at_eol()) {
        // A value ends at a separator or at the end of a line, so this is
        // the end of the input.
        return syntax_error("Expected the end of the line");
      }
      if (current_test_.size() < test_set_->get_parameters().size()) {
        return syntax_error("Test has fewer values than parameter declarations");
      }
      eol();
      test_set_->get_list_of_tests().push_back(current_test_);
      return true;
    }

    // TestValue <- (!(Sep / SpaceChar / Eol) .) (!(Sep / Eol) .)*; appends
    // the value to the current test.
    bool parse_test_value() {
      if (pos_ >= input_.size() || at_separator() || is_space(pos_) ||
          at_eol()) {
        return syntax_error("Expected a value");
      }
      const std::size_t start = pos_;
      ++pos_;
      while (pos_ < input_.size() && !at_separator() && !at_eol()) {
        ++pos_;
      }
      return add_value(detail::trimmed(input_.substr(start, pos_ - start)),
                       start);
    }

    bool add_value(std::string_view value, std::size_t pos) {
      const std::size_t index = current_test_.size();
      if (index >= test_set_->get_parameters().size()) {
        return error_at(pos, "Test has more values than parameter declarations");
      }
      if (value == "*") {
        current_test_.push_back(-1);
        return true;
      }
      const parameter& param = test_set_->get_parameters()[index];
      int found_idx = -1;
      if (param.get_type() == parameter_type::BOOLEAN) {
        if (detail::iequals(value, "true")) {
          found_idx = bool_value_indices_[index][1];
        } else if (detail::iequals(value, "false")) {
          found_idx = bool_value_indices_[index][0];
        }
      } else if (param.get_type() == parameter_type::INTEGER) {
        // The whole value must be an integer (with an optional sign); one out
        // of range is not in the domain.
        int int_val = 0;
        if (detail::parse_int(value, int_val)) {
          const auto& indices = int_value_indices_[index];
          auto it = indices.find(int_val);
          if (it != indices.end()) {
            found_idx = it->second;
          }
        }
      } else if (param.get_type() == parameter_type::ENUM) {
        const auto& indices = enum_value_indices_[index];
        auto it = indices.find(value);
        if (it != indices.end()) {
          found_idx = it->second;
        }
      }
      if (found_idx == -1) {
        return error_at(pos, "Value '" + std::string(value) +
                                 "' is not in the domain of parameter '" +
                                 param.get_name() + "'");
      }
      current_test_.push_back(found_idx);
      return true;
    }

    // Per parameter of the test set: its values to their (first) index.
    void init_value_indices() {
      enum_value_indices_.clear();
      int_value_indices_.clear();
      bool_value_indices_.clear();
      for (const parameter& param : test_set_->get_parameters()) {
        enum_value_indices_.emplace_back();
        int_value_indices_.emplace_back();
        bool_value_indices_.push_back({-1, -1});
        const auto& param_values = param.get_values();
        for (std::size_t i = 0; i < param_values.size(); ++i) {
          const auto& var_val = param_values[i].get_variant_value();
          if (std::holds_alternative<std::string>(var_val)) {
            enum_value_indices_.back().emplace(std::get<std::string>(var_val),
                                               static_cast<int>(i));
          } else if (std::holds_alternative<int>(var_val)) {
            int_value_indices_.back().emplace(std::get<int>(var_val),
                                              static_cast<int>(i));
          } else if (std::holds_alternative<bool>(var_val)) {
            int& bool_index =
                bool_value_indices_.back()[std::get<bool>(var_val) ? 1 : 0];
            if (bool_index == -1) {
              bool_index = static_cast<int>(i);
            }
          }
        }
      }
    }

    bool is_space(std::size_t p) const {
      return p < input_.size() && (input_[p] == ' ' || input_[p] == '\t');
    }

    bool at_eol() const {
      return pos_ < input_.size() &&
             (input_[pos_] == '\r' || input_[pos_] == '\n');
    }

    bool at_separator() const {
      return input_.substr(pos_, separator_.size()) == separator_;
    }

    // Eol <- '\r\n' / '\n' / '\r'
    bool eol() {
      if (!at_eol()) {
        return false;
      }
      if (input_[pos_] == '\r' && pos_ + 1 < input_.size() &&
          input_[pos_ + 1] == '\n') {
        ++pos_;
      }
      ++pos_;
      ++line_;
      line_start_ = pos_;
      return true;
    }

    void skip_spaces() {
      while (is_space(pos_)) {
        ++pos_;
      }
    }

    void skip_spaces_and_eols() {
      while (is_space(pos_) || eol()) {
        if (is_space(pos_)) {
          ++pos_;
        }
      }
    }

    bool syntax_error(const std::string& msg) { return error_at(pos_, msg); }

    // Sets the error message for the given position (in the current line);
    // returns false.
    bool error_at(std::size_t pos, const std::string& msg) {
      std::ostringstream oss;
      oss << "Error in test set file at " << line_ << ":"
          << 1 + detail::count_codepoints(
                     input_.substr(line_start_, pos - line_start_))
          << " -> " << msg;
      error_message_ = oss.str();
      return false;
    }

    const model model_;
    std::string separator_;
    std::unordered_map<std::string, const parameter*, detail::string_hash,
                       std::equal_to<>>
        name_to_param_map_;
    std::vector<std::unordered_map<std::string, int, detail::string_hash,
                                   std::equal_to<>>>
        enum_value_indices_;
    std::vector<std::unordered_map<int, int>> int_value_indices_;
    // Per parameter: the indices of false and true (-1 if not a value).
    std::vector<std::array<int, 2>> bool_value_indices_;
    std::string_view input_;
    std::size_t pos_ = 0;
    std::size_t line_ = 1;
    std::size_t line_start_ = 0;
    test_set* test_set_ = nullptr;
    std::string error_message_;
    std::vector<int> current_test_;
};

test_set_parser::test_set_parser(const model& model, std::string_view separator)
    : impl_{std::make_unique<impl>(model, separator)} {}

test_set_parser::~test_set_parser() {}

bool test_set_parser::parse_test_set(std::string_view sv, test_set& t) {
  return impl_->parse_test_set(sv, t);
}

std::string_view test_set_parser::get_last_error_message() const {
  return impl_->get_last_error_message();
}

}  // namespace citcpp
