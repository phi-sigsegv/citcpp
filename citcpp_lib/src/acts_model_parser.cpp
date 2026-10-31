#include <citcpp/acts_model_parser.hpp>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "parser_utils.hpp"

namespace {

// Thrown by a semantic error, which stops the parsing.
struct semantic_error {};

}  // namespace

namespace citcpp {

/**
 * The parser of the ACTS model format. It implements the following PEG,
 * with one function per rule (each function gives the rules it implements):
 *
 *   Root              <- __ SystemSection ParameterSection RelationSection?
 *                        ConstraintSection?
 *
 *   SystemSection     <- '[' ___ 'System'i ___ ']' _ LineEnd __
 *                        SystemNameRule
 *   SystemNameRule    <- 'name'i ___ ':' _ SystemName __
 *   SystemName        <- (!Eol .)*
 *
 *   ParameterSection  <- '[' ___ 'Parameter'i ___ ']' _ LineEnd __
 *                        (ParameterRule __)+
 *   ParameterRule     <- (BooleanParam / EnumParam / IntParam)
 *
 *   BooleanParam      <- ParamName ___ BooleanType ___ ':' ___
 *                        BooleanValueList SpaceChar* ParamDelcEnd
 *   EnumParam         <- ParamName ___ EnumType ___ ':' ___ EnumValueList
 *                        ParamDelcEnd
 *   IntParam          <- ParamName ___ IntType ___ ':' ___ IntegerValueList
 *                        SpaceChar* ParamDelcEnd
 *
 *   ParamName         <- Identifier
 *
 *   BooleanType       <- '(' ___ ('boolean'i / 'bool'i) ___ ')'
 *   EnumType          <- '(' ___ 'enum'i ___ ')'
 *   IntType           <- '(' ___ ('integer'i / 'int'i) ___ ')'
 *
 *   ParamDelcEnd      <- LineEnd
 *
 *   BooleanValueList  <- BooleanValue (___ ',' ___ BooleanValue)*
 *   BooleanValue      <- 'true'i / 'false'i
 *
 *   EnumValueList     <- EnumValue (___ ',' ___ EnumValue)*
 *   EnumValue         <- (!(',' / ';' / SpaceChar / Eol) .)
 *                        (!(',' / ';' / Eol) .)*
 *
 *   IntegerValueList  <- IntegerValue (___ ',' ___ IntegerValue)*
 *   IntegerValue      <- [+-]? [0-9]+
 *
 *   RelationSection   <- '[' ___ 'Relation'i ___ ']' _ LineEnd __
 *                        RelationRule*
 *   RelationRule      <- RelationName ___ ':' ___ '(' ___ RelParamNameList
 *                        ___ ',' ___ RelStrength ___ ')' _ LineEnd __
 *
 *   RelationName      <- Identifier
 *   RelParamNameList  <- RelParamName (___ ',' ___ RelParamName)*
 *   RelParamName      <- Identifier
 *   RelStrength       <- [0-9]+
 *
 *   ConstraintSection <- '[' ___ 'Constraint'i ___ ']' _ LineEnd __
 *                        ConstraintRule*
 *   ConstraintRule    <- Implication _ ConstraintEnd __
 *   ConstraintEnd     <- LineEnd
 *   Implication       <- OrExpr (_ '=>' _ OrExpr)?
 *   OrExpr            <- AndExpr (_ '||' _ AndExpr)*
 *   AndExpr           <- Operand (_ '&&' _ Operand)*
 *   Operand           <- '(' _ Implication _ ')' / AtomicProp
 *   AtomicProp        <- ConstrParamName _ RelOp _ ConstrValue
 *   ConstrParamName   <- Identifier
 *   RelOp             <- '=' / '!=' / '>=' / '<=' / '>' / '<'
 *   ConstrValue       <- ConstrBoolValue / ConstrIntValue / ConstrEnumValue
 *   ConstrBoolValue   <- 'true'i / 'false'i
 *   ConstrIntValue    <- [+-]? [0-9]+
 *   ConstrEnumValue   <- '"' < (!(',' / ';' / '"' / SpaceChar / Eol) .)
 *                        (!(',' / ';' / '"' / Eol) .)* > '"'
 *
 *   Identifier        <- [a-zA-Z_] [a-zA-Z0-9_]*
 *
 *   __                <- ([ \t\r\n] / LineComment)*
 *   ___               <- [ \t\r\n]*
 *   _                 <- [ \t]*
 *   SpaceChar         <- ' ' / '\t'
 *   Eol               <- '\r\n' / '\n' / '\r'
 *   LineEnd           <- Eol / !.
 *   LineComment       <- '--' (!Eol .)* &LineEnd
 *
 * So section headers, declarations, relations and constraints end with
 * their lines, and comments are lines of their own ('--' in a system name or
 * an enum value belongs to it). The whole input has to match. '.' is a UTF-8
 * code point. The system name and the enum values of parameters are
 * trimmed. Integers have to fit into an int.
 *
 * As in a PEG, a failed alternative, repetition or option resets the
 * position. A syntax error is reported at the farthest position the parsing
 * reached, with what was expected there. A semantic error (an integer out of
 * range, a reference to an undeclared parameter, an ordering operator with a
 * value that is not an integer) stops the parsing. Positions are
 * line:column, counting columns in code points.
 */
class acts_model_parser::impl {
  public:
    bool parse_input_model(std::string_view sv, model& model) {
      input_ = sv;
      pos_ = 0;
      model_ = &model;
      error_message_.clear();
      farthest_pos_ = 0;
      expected_.clear();
      param_indices_.clear();
      for (std::size_t i = 0; i < model.get_parameters().size(); ++i) {
        param_indices_.emplace(model.get_parameters()[i].get_name(), i);
      }

      try {
        if (parse_root()) {
          return true;
        }
      } catch (const semantic_error&) {
        return false;
      }
      report_syntax_error();
      return false;
    }

    std::string_view get_last_error_message() const { return error_message_; }

  private:
    // Root <- __ SystemSection ParameterSection RelationSection?
    //         ConstraintSection?
    bool parse_root() {
      skip_spacing();
      if (!parse_system_section() || !parse_parameter_section()) {
        return false;
      }
      attempt([this] { return parse_relation_section(); });
      attempt([this] { return parse_constraint_section(); });
      if (pos_ < input_.size()) {
        return expected("the end of the input");
      }
      return true;
    }

    // SystemSection  <- '[' ___ 'System'i ___ ']' _ LineEnd __ SystemNameRule
    // SystemNameRule <- 'name'i ___ ':' _ SystemName __
    // SystemName     <- (!Eol .)*
    bool parse_system_section() {
      if (!parse_section_header("System") || !literal_i("name")) {
        return false;
      }
      skip_whitespace();
      if (!literal(":")) {
        return false;
      }
      skip_spaces();
      const std::size_t start = pos_;
      while (any_char_except("\n\r")) {
      }
      model_->set_name(
          std::string(detail::trimmed(input_.substr(start, pos_ - start))));
      skip_spacing();
      return true;
    }

    // ParameterSection <- '[' ___ 'Parameter'i ___ ']' _ LineEnd __
    //                     (ParameterRule __)+
    bool parse_parameter_section() {
      if (!parse_section_header("Parameter") || !parse_parameter_rule()) {
        return false;
      }
      skip_spacing();
      while (attempt([this] { return parse_parameter_rule(); })) {
        skip_spacing();
      }
      return true;
    }

    // ParameterRule <- (BooleanParam / EnumParam / IntParam)
    bool parse_parameter_rule() {
      return attempt([this] {
               return parse_parameter(parameter_type::BOOLEAN);
             }) ||
             attempt([this] { return parse_parameter(parameter_type::ENUM); }) ||
             parse_parameter(parameter_type::INTEGER);
    }

    // BooleanParam <- ParamName ___ BooleanType ___ ':' ___ BooleanValueList
    //                 SpaceChar* ParamDelcEnd
    // EnumParam    <- ParamName ___ EnumType ___ ':' ___ EnumValueList
    //                 ParamDelcEnd
    // IntParam     <- ParamName ___ IntType ___ ':' ___ IntegerValueList
    //                 SpaceChar* ParamDelcEnd
    // ParamName    <- Identifier
    // ParamDelcEnd <- LineEnd
    bool parse_parameter(parameter_type type) {
      std::string_view name;
      if (!parse_identifier(name)) {
        return false;
      }
      skip_whitespace();
      if (!parse_parameter_type(type)) {
        return false;
      }
      skip_whitespace();
      if (!literal(":")) {
        return false;
      }
      skip_whitespace();

      parameter param;
      param.set_name(name);
      param.set_type(type);
      switch (type) {
        case parameter_type::BOOLEAN:
          if (!parse_list([&] { return parse_boolean_value(param); })) {
            return false;
          }
          skip_spaces();
          break;
        case parameter_type::ENUM:
          if (!parse_list([&] { return parse_enum_value(param); })) {
            return false;
          }
          break;
        case parameter_type::INTEGER:
          if (!parse_list([&] { return parse_integer_value(param); })) {
            return false;
          }
          skip_spaces();
          break;
      }
      if (!line_end()) {
        return false;
      }

      param_indices_.emplace(param.get_name(), model_->get_parameters().size());
      model_->add_parameter(std::move(param));
      return true;
    }

    // BooleanType <- '(' ___ ('boolean'i / 'bool'i) ___ ')'
    // EnumType    <- '(' ___ 'enum'i ___ ')'
    // IntType     <- '(' ___ ('integer'i / 'int'i) ___ ')'
    bool parse_parameter_type(parameter_type type) {
      if (!literal("(")) {
        return false;
      }
      skip_whitespace();
      bool matched = false;
      switch (type) {
        case parameter_type::BOOLEAN:
          matched = literal_i("boolean") || literal_i("bool");
          break;
        case parameter_type::ENUM:
          matched = literal_i("enum");
          break;
        case parameter_type::INTEGER:
          matched = literal_i("integer") || literal_i("int");
          break;
      }
      if (!matched) {
        return false;
      }
      skip_whitespace();
      return literal(")");
    }

    // BooleanValueList <- BooleanValue (___ ',' ___ BooleanValue)*
    // EnumValueList    <- EnumValue (___ ',' ___ EnumValue)*
    // IntegerValueList <- IntegerValue (___ ',' ___ IntegerValue)*
    // RelParamNameList <- RelParamName (___ ',' ___ RelParamName)*
    template <typename F>
    bool parse_list(F parse_element) {
      if (!parse_element()) {
        return false;
      }
      while (attempt([&] {
        skip_whitespace();
        if (!literal(",")) {
          return false;
        }
        skip_whitespace();
        return parse_element();
      })) {
      }
      return true;
    }

    // BooleanValue <- 'true'i / 'false'i
    bool parse_boolean_value(parameter& param) {
      if (literal_i("true")) {
        param.add_value(true);
      } else if (literal_i("false")) {
        param.add_value(false);
      } else {
        return false;
      }
      return true;
    }

    // EnumValue <- (!(',' / ';' / SpaceChar / Eol) .) (!(',' / ';' / Eol) .)*
    bool parse_enum_value(parameter& param) {
      const std::size_t start = pos_;
      if (!any_char_except(",; \t\n\r")) {
        return expected("an enum value");
      }
      while (any_char_except(",;\n\r")) {
      }
      param.add_value(
          std::string(detail::trimmed(input_.substr(start, pos_ - start))));
      return true;
    }

    // IntegerValue <- [+-]? [0-9]+
    bool parse_integer_value(parameter& param) {
      int value = 0;
      if (!parse_integer(value)) {
        return false;
      }
      param.add_value(value);
      return true;
    }

    // RelationSection <- '[' ___ 'Relation'i ___ ']' _ LineEnd __
    //                    RelationRule*
    bool parse_relation_section() {
      if (!parse_section_header("Relation")) {
        return false;
      }
      while (attempt([this] { return parse_relation_rule(); })) {
      }
      return true;
    }

    // RelationRule <- RelationName ___ ':' ___ '(' ___ RelParamNameList ___
    //                 ',' ___ RelStrength ___ ')' _ LineEnd __
    // RelationName <- Identifier
    // RelStrength  <- [0-9]+
    bool parse_relation_rule() {
      std::string_view name;
      if (!parse_identifier(name)) {
        return false;
      }
      skip_whitespace();
      if (!literal(":")) {
        return false;
      }
      skip_whitespace();
      if (!literal("(")) {
        return false;
      }
      skip_whitespace();

      relation rel;
      rel.set_name(name);
      if (!parse_list([&] { return parse_relation_parameter(rel); })) {
        return false;
      }
      skip_whitespace();
      if (!literal(",")) {
        return false;
      }
      skip_whitespace();
      const std::size_t start = pos_;
      while (pos_ < input_.size() && detail::is_digit(input_[pos_])) {
        ++pos_;
      }
      if (pos_ == start) {
        return expected("an interaction strength");
      }
      rel.set_interaction_strength(to_int(start));
      skip_whitespace();
      if (!literal(")")) {
        return false;
      }
      skip_spaces();
      if (!line_end()) {
        return false;
      }
      skip_spacing();

      model_->add_relation(std::move(rel));
      return true;
    }

    // RelParamName <- Identifier
    bool parse_relation_parameter(relation& rel) {
      const std::size_t start = pos_;
      std::string_view name;
      if (!parse_identifier(name)) {
        return false;
      }
      const auto it = param_indices_.find(name);
      if (it == param_indices_.end()) {
        semantic_error_at(start, "relation",
                          "Cannot find declaration of parameter " +
                              std::string(name));
      }
      rel.add_parameter(model_->get_parameters()[it->second]);
      return true;
    }

    // ConstraintSection <- '[' ___ 'Constraint'i ___ ']' _ LineEnd __
    //                      ConstraintRule*
    bool parse_constraint_section() {
      if (!parse_section_header("Constraint")) {
        return false;
      }
      while (attempt([this] { return parse_constraint_rule(); })) {
      }
      return true;
    }

    // ConstraintRule <- Implication _ ConstraintEnd __
    // ConstraintEnd  <- LineEnd
    bool parse_constraint_rule() {
      std::shared_ptr<constraint> constr;
      if (!parse_implication(constr)) {
        return false;
      }
      skip_spaces();
      if (!line_end()) {
        return false;
      }
      skip_spacing();
      model_->add_constraint(std::move(constr));
      return true;
    }

    // Implication <- OrExpr (_ '=>' _ OrExpr)?
    bool parse_implication(std::shared_ptr<constraint>& result) {
      if (!parse_or_expr(result)) {
        return false;
      }
      std::shared_ptr<constraint> rhs;
      if (attempt([&] {
            skip_spaces();
            if (!literal("=>")) {
              return false;
            }
            skip_spaces();
            return parse_or_expr(rhs);
          })) {
        result = std::make_shared<implication>(std::move(result),
                                               std::move(rhs));
      }
      return true;
    }

    // OrExpr <- AndExpr (_ '||' _ AndExpr)*
    bool parse_or_expr(std::shared_ptr<constraint>& result) {
      return parse_operands<or_expression>(
          "||",
          [this](std::shared_ptr<constraint>& operand) {
            return parse_and_expr(operand);
          },
          result);
    }

    // AndExpr <- Operand (_ '&&' _ Operand)*
    bool parse_and_expr(std::shared_ptr<constraint>& result) {
      return parse_operands<and_expression>(
          "&&",
          [this](std::shared_ptr<constraint>& operand) {
            return parse_operand(operand);
          },
          result);
    }

    // X (_ op _ X)* for the operands X of an OrExpr or an AndExpr; more than
    // one become an expression of type T.
    template <typename T, typename F>
    bool parse_operands(std::string_view op, F parse_one,
                        std::shared_ptr<constraint>& result) {
      if (!parse_one(result)) {
        return false;
      }
      std::vector<std::shared_ptr<constraint>> operands;
      std::shared_ptr<constraint> operand;
      while (attempt([&] {
        skip_spaces();
        if (!literal(op)) {
          return false;
        }
        skip_spaces();
        return parse_one(operand);
      })) {
        if (operands.empty()) {
          operands.push_back(std::move(result));
        }
        operands.push_back(std::move(operand));
      }
      if (!operands.empty()) {
        result = std::make_shared<T>(std::move(operands));
      }
      return true;
    }

    // Operand <- '(' _ Implication _ ')' / AtomicProp
    bool parse_operand(std::shared_ptr<constraint>& result) {
      return attempt([&] {
               if (!literal("(")) {
                 return false;
               }
               skip_spaces();
               if (!parse_implication(result)) {
                 return false;
               }
               skip_spaces();
               return literal(")");
             }) ||
             parse_atomic_prop(result);
    }

    // AtomicProp      <- ConstrParamName _ RelOp _ ConstrValue
    // ConstrParamName <- Identifier
    // ConstrValue     <- ConstrBoolValue / ConstrIntValue / ConstrEnumValue
    // ConstrBoolValue <- 'true'i / 'false'i
    // ConstrIntValue  <- [+-]? [0-9]+
    bool parse_atomic_prop(std::shared_ptr<constraint>& result) {
      const std::size_t start = pos_;
      std::string_view name;
      if (!parse_identifier(name)) {
        return false;
      }
      if (!param_indices_.contains(name)) {
        semantic_error_at(start, "constraint",
                          "Cannot find declaration of parameter " +
                              std::string(name));
      }
      skip_spaces();
      relational_operator op = relational_operator::EQ;
      if (!parse_relational_operator(op)) {
        return false;
      }
      skip_spaces();

      const parameter_reference param{std::string(name)};
      int int_value = 0;
      std::string enum_value;
      const bool is_true = literal_i("true");
      if (is_true || literal_i("false")) {
        check_unordered(op, start);
        result = std::make_shared<boolean_proposition>(
            param, op, parameter_value(is_true));
      } else if (parse_integer(int_value)) {
        result = std::make_shared<int_proposition>(param, op,
                                                   parameter_value(int_value));
      } else if (parse_constr_enum_value(enum_value)) {
        check_unordered(op, start);
        result = std::make_shared<enum_proposition>(
            param, op, parameter_value(enum_value));
      } else {
        return false;
      }
      return true;
    }

    // RelOp <- '=' / '!=' / '>=' / '<=' / '>' / '<'
    bool parse_relational_operator(relational_operator& op) {
      if (literal("=")) {
        op = relational_operator::EQ;
      } else if (literal("!=")) {
        op = relational_operator::NEQ;
      } else if (literal(">=")) {
        op = relational_operator::GE;
      } else if (literal("<=")) {
        op = relational_operator::LE;
      } else if (literal(">")) {
        op = relational_operator::GT;
      } else if (literal("<")) {
        op = relational_operator::LT;
      } else {
        return false;
      }
      return true;
    }

    // ConstrEnumValue <- '"' < (!(',' / ';' / '"' / SpaceChar / Eol) .)
    //                    (!(',' / ';' / '"' / Eol) .)* > '"'
    bool parse_constr_enum_value(std::string& value) {
      const std::size_t start = pos_;
      if (!literal("\"")) {
        return false;
      }
      if (!any_char_except(",;\" \t\n\r")) {
        expected("an enum value");
        pos_ = start;
        return false;
      }
      while (any_char_except(",;\"\n\r")) {
      }
      const std::size_t end = pos_;
      if (!literal("\"")) {
        pos_ = start;
        return false;
      }
      value = input_.substr(start + 1, end - start - 1);
      return true;
    }

    // Values that are not integers have no order.
    void check_unordered(relational_operator op, std::size_t start) {
      switch (op) {
        case relational_operator::GE:
        case relational_operator::GT:
        case relational_operator::LE:
        case relational_operator::LT:
          semantic_error_at(start, "constraint",
                            "Cannot have operator for a non-integer parameter "
                            "that needs ordered values");
          break;
        default:
          break;
      }
    }

    // '[' ___ name'i ___ ']' _ LineEnd __ (the header of every section)
    bool parse_section_header(std::string_view name) {
      if (!literal("[")) {
        return false;
      }
      skip_whitespace();
      if (!literal_i(name)) {
        return false;
      }
      skip_whitespace();
      if (!literal("]")) {
        return false;
      }
      skip_spaces();
      if (!line_end()) {
        return false;
      }
      skip_spacing();
      return true;
    }

    // Identifier <- [a-zA-Z_] [a-zA-Z0-9_]*
    bool parse_identifier(std::string_view& identifier) {
      if (pos_ == input_.size() || !detail::is_identifier_start(input_[pos_])) {
        return expected("an identifier");
      }
      const std::size_t start = pos_;
      ++pos_;
      while (pos_ < input_.size() && detail::is_identifier_char(input_[pos_])) {
        ++pos_;
      }
      identifier = input_.substr(start, pos_ - start);
      return true;
    }

    // [+-]? [0-9]+ (IntegerValue, ConstrIntValue)
    bool parse_integer(int& value) {
      const std::size_t start = pos_;
      if (pos_ < input_.size() && (input_[pos_] == '+' || input_[pos_] == '-')) {
        ++pos_;
      }
      if (pos_ == input_.size() || !detail::is_digit(input_[pos_])) {
        expected("an integer");
        pos_ = start;
        return false;
      }
      while (pos_ < input_.size() && detail::is_digit(input_[pos_])) {
        ++pos_;
      }
      value = to_int(start);
      return true;
    }

    // The value of the integer from start to the current position.
    int to_int(std::size_t start) {
      int value = 0;
      if (!detail::parse_int(input_.substr(start, pos_ - start), value)) {
        semantic_error_at(start, "model file", "Integer value out of range");
      }
      return value;
    }

    // __ <- ([ \t\r\n] / LineComment)*
    void skip_spacing() {
      while (pos_ < input_.size()) {
        const char c = input_[pos_];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
          ++pos_;
        } else if (!skip_line_comment()) {
          return;
        }
      }
    }

    // LineComment <- '--' (!Eol .)* &LineEnd
    bool skip_line_comment() {
      if (input_.substr(pos_, 2) != "--") {
        return false;
      }
      const std::size_t start = pos_;
      pos_ += 2;
      while (any_char_except("\n\r")) {
      }
      if (pos_ < input_.size() && input_[pos_] != '\n' &&
          input_[pos_] != '\r') {
        // Not followed by an end of line (but by an invalid code point).
        pos_ = start;
        return false;
      }
      return true;
    }

    // ___ <- [ \t\r\n]*
    void skip_whitespace() {
      while (pos_ < input_.size() &&
             (input_[pos_] == ' ' || input_[pos_] == '\t' ||
              input_[pos_] == '\r' || input_[pos_] == '\n')) {
        ++pos_;
      }
    }

    // _ <- [ \t]*
    void skip_spaces() {
      while (pos_ < input_.size() &&
             (input_[pos_] == ' ' || input_[pos_] == '\t')) {
        ++pos_;
      }
    }

    // Eol <- '\r\n' / '\n' / '\r'
    bool eol() {
      if (pos_ < input_.size() && input_[pos_] == '\r') {
        ++pos_;
        if (pos_ < input_.size() && input_[pos_] == '\n') {
          ++pos_;
        }
        return true;
      }
      if (pos_ < input_.size() && input_[pos_] == '\n') {
        ++pos_;
        return true;
      }
      return false;
    }

    // LineEnd <- Eol / !.
    bool line_end() {
      if (eol() || pos_ == input_.size()) {
        return true;
      }
      return expected("the end of the line");
    }

    // (!(stop) .): skips one code point unless it is one of the (ASCII)
    // characters in stop, or the input has no complete one left.
    bool any_char_except(std::string_view stop) {
      if (pos_ == input_.size() || stop.find(input_[pos_]) != stop.npos) {
        return false;
      }
      const std::size_t len = detail::codepoint_length(input_.substr(pos_));
      if (len == 0) {
        return false;
      }
      pos_ += len;
      return true;
    }

    bool literal(std::string_view s) {
      if (input_.substr(pos_, s.size()) != s) {
        return expected_at(pos_, s, true);
      }
      pos_ += s.size();
      return true;
    }

    // A literal matched ignoring the case.
    bool literal_i(std::string_view s) {
      if (!detail::iequals(input_.substr(pos_, s.size()), s)) {
        return expected_at(pos_, s, true);
      }
      pos_ += s.size();
      return true;
    }

    // Parses with the given function, resetting the position if it fails.
    template <typename F>
    bool attempt(F parse) {
      const std::size_t start = pos_;
      if (parse()) {
        return true;
      }
      pos_ = start;
      return false;
    }

    // Records what was expected at the current position, for the message of
    // a syntax error; returns false.
    bool expected(std::string_view what) {
      return expected_at(pos_, what, false);
    }

    bool expected_at(std::size_t pos, std::string_view what, bool is_literal) {
      if (pos > farthest_pos_) {
        farthest_pos_ = pos;
        expected_.clear();
      }
      if (pos == farthest_pos_) {
        const std::pair<std::string_view, bool> entry(what, is_literal);
        bool known = false;
        for (const auto& e : expected_) {
          known = known || e == entry;
        }
        if (!known) {
          expected_.push_back(entry);
        }
      }
      return false;
    }

    void report_syntax_error() {
      std::ostringstream oss;
      oss << "Error in model file at " << position(farthest_pos_)
          << " -> Expected ";
      for (std::size_t i = 0; i < expected_.size(); ++i) {
        if (i > 0) {
          oss << (i + 1 == expected_.size() ? " or " : ", ");
        }
        if (expected_[i].second) {
          oss << "'" << expected_[i].first << "'";
        } else {
          oss << expected_[i].first;
        }
      }
      error_message_ = oss.str();
    }

    [[noreturn]] void semantic_error_at(std::size_t pos, std::string_view where,
                                        const std::string& msg) {
      std::ostringstream oss;
      oss << "Error in " << where << " at " << position(pos) << " -> " << msg;
      error_message_ = oss.str();
      throw semantic_error();
    }

    // The line and the column (in code points) of the given position.
    std::string position(std::size_t pos) const {
      std::size_t line = 1;
      std::size_t line_start = 0;
      for (std::size_t i = 0; i < pos; ++i) {
        // A line ends with '\r\n', '\n' or '\r' (see eol()).
        if (input_[i] == '\n' ||
            (input_[i] == '\r' &&
             (i + 1 == input_.size() || input_[i + 1] != '\n'))) {
          ++line;
          line_start = i + 1;
        }
      }
      const std::size_t column =
          1 + detail::count_codepoints(
                  input_.substr(line_start, pos - line_start));
      return std::to_string(line) + ":" + std::to_string(column);
    }

    std::string_view input_;
    std::size_t pos_ = 0;
    model* model_ = nullptr;
    std::string error_message_;
    // The farthest position a match failed at, and what was expected there
    // (each with whether it is a literal).
    std::size_t farthest_pos_ = 0;
    std::vector<std::pair<std::string_view, bool>> expected_;
    // The index in the model of each declared parameter (the first one of a
    // name).
    std::unordered_map<std::string, std::size_t, detail::string_hash,
                       std::equal_to<>>
        param_indices_;
};

acts_model_parser::acts_model_parser() : impl_{std::make_unique<impl>()} {}

acts_model_parser::acts_model_parser(acts_model_parser&& other) noexcept =
    default;

acts_model_parser::~acts_model_parser() {}

acts_model_parser& acts_model_parser::operator=(
    acts_model_parser&& other) noexcept = default;

bool acts_model_parser::parse_input_model(std::string_view sv, model& model) {
  return impl_->parse_input_model(sv, model);
}

std::string_view acts_model_parser::get_last_error_message() const {
  return impl_->get_last_error_message();
}

}  // namespace citcpp
