#include "acts_model_parser.hpp"

#include <peglib.h>

#include <sstream>
#include <string>
#include <string_view>

#include "parser_utils.hpp"

namespace {

class input_model_data_consumer {
  public:
    virtual void set_system_name(std::string_view name) = 0;
    virtual void set_param_identifier(std::string_view identifier) = 0;
    virtual void set_param_type(citcpp::parameter_type type) = 0;
    virtual void add_param_value(bool value) = 0;
    virtual void add_param_value(const std::string& value) = 0;
    virtual void add_param_value(int value) = 0;
    virtual void end_param_declaration() = 0;
    virtual void set_relation_identifier(std::string_view identifier) = 0;
    virtual void add_param_to_relation(std::string_view identifier, size_t line,
                                       size_t col) = 0;
    virtual void set_relation_strength(int strength) = 0;
    virtual void set_atomic_prop_param(const std::string& param_name, size_t line,
                                       size_t col) = 0;
    virtual void set_atomic_prop_rel_op(const std::string& rel_op) = 0;
    virtual void set_atomic_prop_value(int value) = 0;
    virtual void set_atomic_prop_value(const std::string& value, size_t line,
                                       size_t col) = 0;
    virtual void set_atomic_prop_value(bool value, size_t line,
                                       size_t col) = 0;
    virtual void end_constraint() = 0;
};

class system_name_consumer {
  public:
    system_name_consumer(input_model_data_consumer* consumer)
        : consumer_(consumer) {}

    void operator()(const peg::SemanticValues& vs) {
      consumer_->set_system_name(vs.token_to_string());
    }

  private:
    input_model_data_consumer* consumer_;
};

class param_identifier_consumer {
  public:
    param_identifier_consumer(input_model_data_consumer* consumer)
        : consumer_(consumer) {}

    void operator()(const peg::SemanticValues& vs) {
      consumer_->set_param_identifier(vs.token_to_string());
    }

  private:
    input_model_data_consumer* consumer_;
};

class param_boolean_type_consumer {
  public:
    param_boolean_type_consumer(input_model_data_consumer* consumer)
        : consumer_(consumer) {}

    void operator()(const peg::SemanticValues& vs) {
      consumer_->set_param_type(citcpp::parameter_type::BOOLEAN);
    }

  private:
    input_model_data_consumer* consumer_;
};

class param_enum_type_consumer {
  public:
    param_enum_type_consumer(input_model_data_consumer* consumer)
        : consumer_(consumer) {}

    void operator()(const peg::SemanticValues& vs) {
      consumer_->set_param_type(citcpp::parameter_type::ENUM);
    }

  private:
    input_model_data_consumer* consumer_;
};

class param_integer_type_consumer {
  public:
    param_integer_type_consumer(input_model_data_consumer* consumer)
        : consumer_(consumer) {}

    void operator()(const peg::SemanticValues& vs) {
      consumer_->set_param_type(citcpp::parameter_type::INTEGER);
    }

  private:
    input_model_data_consumer* consumer_;
};

class boolean_value_consumer {
  public:
    boolean_value_consumer(input_model_data_consumer* consumer)
        : consumer_(consumer) {}

    void operator()(const peg::SemanticValues& vs) {
      switch (vs.choice()) {
        case 0:
          consumer_->add_param_value(true);
          return;
        default:
          consumer_->add_param_value(false);
          return;
      }
    }

  private:
    input_model_data_consumer* consumer_;
};

class enum_value_consumer {
  public:
    enum_value_consumer(input_model_data_consumer* consumer)
        : consumer_(consumer) {}

    void operator()(const peg::SemanticValues& vs) {
      std::string str(vs.token_to_string());
      citcpp::detail::trim(str);
      consumer_->add_param_value(str);
    }

  private:
    input_model_data_consumer* consumer_;
};

class integer_value_consumer {
  public:
    integer_value_consumer(input_model_data_consumer* consumer)
        : consumer_(consumer) {}

    void operator()(const peg::SemanticValues& vs) {
      consumer_->add_param_value(vs.token_to_number<int>());
    }

  private:
    input_model_data_consumer* consumer_;
};

class param_declaration_end_consumer {
  public:
    param_declaration_end_consumer(input_model_data_consumer* consumer)
        : consumer_(consumer) {}

    void operator()(const peg::SemanticValues& vs) {
      consumer_->end_param_declaration();
    }

  private:
    input_model_data_consumer* consumer_;
};

class relation_identifier_consumer {
  public:
    relation_identifier_consumer(input_model_data_consumer* consumer)
        : consumer_(consumer) {}

    void operator()(const peg::SemanticValues& vs) {
      consumer_->set_relation_identifier(vs.token_to_string());
    }

  private:
    input_model_data_consumer* consumer_;
};

class relation_param_identifier_consumer {
  public:
    relation_param_identifier_consumer(input_model_data_consumer* consumer)
        : consumer_(consumer) {}

    void operator()(const peg::SemanticValues& vs) {
      auto line_info = vs.line_info();
      consumer_->add_param_to_relation(vs.token_to_string(), line_info.first,
                                       line_info.second);
    }

  private:
    input_model_data_consumer* consumer_;
};

class relation_strength_consumer {
  public:
    relation_strength_consumer(input_model_data_consumer* consumer)
        : consumer_(consumer) {}

    void operator()(const peg::SemanticValues& vs) {
      consumer_->set_relation_strength(vs.token_to_number<int>());
    }

  private:
    input_model_data_consumer* consumer_;
};

class constr_param_name_consumer {
  public:
    constr_param_name_consumer(input_model_data_consumer* consumer)
        : consumer_(consumer) {}

    void operator()(const peg::SemanticValues& vs) {
      auto line_info = vs.line_info();
      consumer_->set_atomic_prop_param(vs.token_to_string(), line_info.first,
                                       line_info.second);
    }

  private:
    input_model_data_consumer* consumer_;
};

class atomic_prop_rel_op_consumer {
  public:
    atomic_prop_rel_op_consumer(input_model_data_consumer* consumer)
        : consumer_(consumer) {}

    void operator()(const peg::SemanticValues& vs) {
      consumer_->set_atomic_prop_rel_op(vs.token_to_string());
    }

  private:
    input_model_data_consumer* consumer_;
};

class constr_int_value_consumer {
  public:
    constr_int_value_consumer(input_model_data_consumer* consumer)
        : consumer_(consumer) {}

    void operator()(const peg::SemanticValues& vs) {
      consumer_->set_atomic_prop_value(vs.token_to_number<int>());
    }

  private:
    input_model_data_consumer* consumer_;
};

class constr_bool_value_consumer {
  public:
    constr_bool_value_consumer(input_model_data_consumer* consumer)
        : consumer_(consumer) {}

    void operator()(const peg::SemanticValues& vs) {
      auto line_info = vs.line_info();
      switch (vs.choice()) {
        case 0:
          consumer_->set_atomic_prop_value(true, line_info.first,
                                           line_info.second);
          return;
        default:
          consumer_->set_atomic_prop_value(false, line_info.first,
                                           line_info.second);
          return;
      }
    }

  private:
    input_model_data_consumer* consumer_;
};

class constr_enum_value_consumer {
  public:
    constr_enum_value_consumer(input_model_data_consumer* consumer)
        : consumer_(consumer) {}

    void operator()(const peg::SemanticValues& vs) {
      auto line_info = vs.line_info();
      consumer_->set_atomic_prop_value(vs.token_to_string(), line_info.first,
                                       line_info.second);
    }

  private:
    input_model_data_consumer* consumer_;
};

class constraint_end_consumer {
  public:
    constraint_end_consumer(input_model_data_consumer* consumer)
        : consumer_(consumer) {}

    void operator()(const peg::SemanticValues& vs) {
      consumer_->end_constraint();
    }

  private:
    input_model_data_consumer* consumer_;
};

peg::parser create_acts_model_parser() {
  using namespace peg;

  parser p(R"(
Root              <- __ SystemSection ParameterSection RelationSection? ConstraintSection?

SystemSection     <- '[' __ 'System'i __ ']' __ SystemNameRule
SystemNameRule    <- 'name'i __ ':' __ SystemName __
SystemName        <- (!Eol .)*

ParameterSection  <- '[' __ 'Parameter'i __ ']' __ ParameterRule+ __
ParameterRule     <- (BooleanParam / EnumParam / IntParam)

BooleanParam      <- ParamName __ BooleanType __ ':' __ BooleanValueList SpaceChar* ParamDelcEnd
EnumParam         <- ParamName __ EnumType __ ':' __ EnumValueList ParamDelcEnd
IntParam          <- ParamName __ IntType __ ':' __ IntegerValueList SpaceChar* ParamDelcEnd

ParamName         <- Identifier

BooleanType       <- '(' __ ('boolean'i / 'bool'i) __ ')'
EnumType          <- '(' __ 'enum'i __ ')'
IntType           <- '(' __ ('integer'i / 'int'i) __ ')'

ParamDelcEnd      <- Eol

BooleanValueList  <- BooleanValue (__ ',' __ BooleanValue)*
BooleanValue      <- 'true'i / 'false'i

EnumValueList     <- EnumValue (__ ',' __ EnumValue)*
EnumValue         <- (!(',' / ';' / SpaceChar / Eol) .) (!(',' / ';' / Eol) .)*

IntegerValueList  <- IntegerValue (__ ',' __ IntegerValue)*
IntegerValue      <- [+-]? [0-9]+

RelationSection   <- '[' __ 'Relation'i __ ']' __ RelationRule*
RelationRule      <- RelationName __ ':' __ '(' __ RelParamNameList __ ',' __ RelStrength __ ')' __

RelationName      <- Identifier
RelParamNameList  <- RelParamName (__ ',' __ RelParamName)*
RelParamName      <- Identifier
RelStrength       <- [0-9]+

ConstraintSection <- '[' __ 'Constraint'i __ ']' __ ConstraintRule*
ConstraintRule    <- Implication ConstraintEnd __
ConstraintEnd     <- Eol
Implication       <- OrExpr (_ '=>' _ OrExpr)?
OrExpr            <- AndExpr (_ '||' _ AndExpr)*
AndExpr           <- Operand (_ '&&' _ Operand)*
Operand           <- AtomicProp / ('(' _ Implication _ ')')
AtomicProp        <- ConstrParamName _ RelOp _ ConstrValue
ConstrParamName   <- Identifier
RelOp             <- '=' / '!=' / '>=' / '<=' / '>' / '<'
ConstrValue       <- ConstrBoolValue / ConstrIntValue / ConstrEnumValue
ConstrBoolValue   <- 'true'i / 'false'i
ConstrIntValue    <- [+-]? [0-9]+
ConstrEnumValue   <- '"' < (!(',' / ';' / '"' / SpaceChar / Eol) .) (!(',' / ';' / '"' / Eol) .)* > '"'

Identifier        <- [a-zA-Z_] [a-zA-Z0-9_]*

~__               <- (WhiteSpace / Eol)*
~_                <- SpaceChar*
WhiteSpace        <- SpaceChar / LineComment
SpaceChar         <- ' ' / '\t'
Eol               <- '\r\n' / '\n' / '\r'
LineComment       <- '--' (!Eol .)* &Eol
  )");

  return p;
}

}  // namespace

namespace citcpp {
namespace detail {

class acts_model_parser::impl : input_model_data_consumer {
  public:
    impl()
        : input_model_data_consumer(),
          system_name_consumer_(system_name_consumer(this)),
          param_identifier_consumer_(param_identifier_consumer(this)),
          param_boolean_type_consumer_(param_boolean_type_consumer(this)),
          param_enum_type_consumer_(param_enum_type_consumer(this)),
          param_integer_type_consumer_(param_integer_type_consumer(this)),
          boolean_value_consumer_(boolean_value_consumer(this)),
          enum_value_consumer_(enum_value_consumer(this)),
          integer_value_consumer_(integer_value_consumer(this)),
          param_declaration_end_consumer_(param_declaration_end_consumer(this)),
          relation_identifier_consumer_(relation_identifier_consumer(this)),
          relation_param_identifier_consumer_(
              relation_param_identifier_consumer(this)),
          relation_strength_consumer_(relation_strength_consumer(this)),
          constr_param_name_consumer_(constr_param_name_consumer(this)),
          atomic_prop_rel_op_consumer_(atomic_prop_rel_op_consumer(this)),
          constr_int_value_consumer_(constr_int_value_consumer(this)),
          constr_bool_value_consumer_(constr_bool_value_consumer(this)),
          constr_enum_value_consumer_(constr_enum_value_consumer(this)),
          constraint_end_consumer_(constraint_end_consumer(this)),
          current_param_(parameter()),
          current_relation_(relation()),
          constr_param_name_(),
          atomic_prop_rel_op_(relational_operator::EQ),
          parser_(create_acts_model_parser()),
          error_occurred_(false),
          error_message_(),
          model_(nullptr) {

      parser_["SystemName"] = system_name_consumer_;

      parser_["ParamName"] = param_identifier_consumer_;
      parser_["BooleanType"] = param_boolean_type_consumer_;
      parser_["EnumType"] = param_enum_type_consumer_;
      parser_["IntType"] = param_integer_type_consumer_;
      parser_["BooleanValue"] = boolean_value_consumer_;
      parser_["EnumValue"] = enum_value_consumer_;
      parser_["IntegerValue"] = integer_value_consumer_;
      parser_["ParamDelcEnd"] = param_declaration_end_consumer_;

      parser_["RelationName"] = relation_identifier_consumer_;
      parser_["RelParamName"] = relation_param_identifier_consumer_;
      parser_["RelStrength"] = relation_strength_consumer_;
      parser_["ConstrParamName"] = constr_param_name_consumer_;
      parser_["RelOp"] = atomic_prop_rel_op_consumer_;
      parser_["ConstrIntValue"] = constr_int_value_consumer_;
      parser_["ConstrBoolValue"] = constr_bool_value_consumer_;
      parser_["ConstrEnumValue"] = constr_enum_value_consumer_;
      parser_["ConstraintEnd"] = constraint_end_consumer_;

      parser_.set_logger([this](size_t line, size_t col,
                                const std::string& msg) {
        std::ostringstream oss;
        oss << "Error in model file at " << line << ":" << col << " -> " << msg;

        error_message_ = oss.str();
      });
    }

    void set_input_model(model* model) { model_ = model; }

    void set_system_name(std::string_view name) { model_->set_name(name); }

    void set_param_identifier(std::string_view identifier) {
      current_param_.set_name(identifier);
    }

    void set_param_type(citcpp::parameter_type type) {
      current_param_.set_type(type);
    }

    void add_param_value(bool value) { current_param_.add_value(value); }

    void add_param_value(const std::string& value) {
      current_param_.add_value(value);
    }

    void add_param_value(int value) { current_param_.add_value(value); }

    void end_param_declaration() {
      model_->add_parameter(current_param_);
      // Reset our parameter.
      current_param_.get_values().clear();
    }

    void set_relation_identifier(std::string_view identifier) {
      current_relation_.set_name(identifier);
    }

    void add_param_to_relation(std::string_view identifier, size_t line,
                               size_t col) {
      // Search for the parameter in the model
      for (const parameter& param : model_->get_parameters()) {
        if (identifier == param.get_name()) {
          current_relation_.add_parameter(param);
          return;
        }
      }

      // If we reach this point, then we did not find the reference parameter.
      error_occurred_ = true;
      std::ostringstream oss;
      oss << "Error in relation at " << line << ":" << col
          << " -> Cannot find declaration of parameter " << identifier;

      error_message_ = oss.str();
    }

    void set_relation_strength(int strength) {
      current_relation_.set_interaction_strength(strength);

      model_->add_relation(current_relation_);
      // Reset our relation.
      current_relation_.get_parameters().clear();
    }

    void set_atomic_prop_param(const std::string& param_name, size_t line,
                               size_t col) {

      // Search for the parameter in the model
      for (const parameter& param : model_->get_parameters()) {
        if (param_name == param.get_name()) {
          constr_param_name_ = param_name;
          return;
        }
      }

      // If we reach this point, then we did not find the reference parameter.
      error_occurred_ = true;
      std::ostringstream oss;
      oss << "Error in constraint at " << line << ":" << col
          << " -> Cannot find declaration of parameter " << param_name;

      error_message_ = oss.str();
    }

    void set_atomic_prop_rel_op(const std::string& rel_op) {
      if (rel_op == "!=") {
        atomic_prop_rel_op_ = relational_operator::NEQ;
      } else if (rel_op == "<") {
        atomic_prop_rel_op_ = relational_operator::LT;
      } else if (rel_op == "<=") {
        atomic_prop_rel_op_ = relational_operator::LE;
      } else if (rel_op == ">") {
        atomic_prop_rel_op_ = relational_operator::GT;
      } else if (rel_op == ">=") {
        atomic_prop_rel_op_ = relational_operator::GE;
      } else {
        atomic_prop_rel_op_ = relational_operator::EQ;
      }
    }

    void set_atomic_prop_value(int value) {
      constraint_holder prop(std::make_unique<int_proposition>(
          parameter_reference(constr_param_name_), atomic_prop_rel_op_, value));
      // TODO: Add to parent constraint.
    }

    void set_atomic_prop_value(const std::string& value, size_t line,
                               size_t col) {

      switch (atomic_prop_rel_op_) {
        case relational_operator::GT:
        case relational_operator::GE:
        case relational_operator::LT:
        case relational_operator::LE: {
          // If we reach this point, then we did not find the reference parameter.
          error_occurred_ = true;
          std::ostringstream oss;
          oss << "Error in constraint at " << line << ":" << col
              << " -> Cannot have operator for a non-integer parameter that needs an order";

          error_message_ = oss.str();
          return;
        }
        default:
          break;
      }

      constraint_holder prop(std::make_unique<enum_proposition>(
          parameter_reference(constr_param_name_), atomic_prop_rel_op_, value));
      // TODO: Add to parent constraint.
    }

    void set_atomic_prop_value(bool value, size_t line, size_t col) {

      switch (atomic_prop_rel_op_) {
        case relational_operator::GT:
        case relational_operator::GE:
        case relational_operator::LT:
        case relational_operator::LE: {
          // If we reach this point, then we did not find the reference parameter.
          error_occurred_ = true;
          std::ostringstream oss;
          oss << "Error in constraint at " << line << ":" << col
              << " -> Cannot have operator for a non-integer parameter that needs an order";

          error_message_ = oss.str();
          return;
        }
        default:
          break;
      }

      constraint_holder prop(std::make_unique<boolean_proposition>(
          parameter_reference(constr_param_name_), atomic_prop_rel_op_, value));
      // TODO: Add to parent constraint.
    }

    void end_constraint() {}

    bool parse_input_model(std::string_view sv) {
      error_occurred_ = false;
      error_message_.clear();
      current_param_.get_values().clear();
      bool ret = parser_.parse(sv);

      return ret && !error_occurred_;
    }

    std::string_view get_last_error_message() const { return error_message_; }

  private:
    system_name_consumer system_name_consumer_;
    param_identifier_consumer param_identifier_consumer_;
    param_enum_type_consumer param_enum_type_consumer_;
    param_boolean_type_consumer param_boolean_type_consumer_;
    param_integer_type_consumer param_integer_type_consumer_;
    boolean_value_consumer boolean_value_consumer_;
    enum_value_consumer enum_value_consumer_;
    integer_value_consumer integer_value_consumer_;
    param_declaration_end_consumer param_declaration_end_consumer_;
    relation_identifier_consumer relation_identifier_consumer_;
    relation_param_identifier_consumer relation_param_identifier_consumer_;
    relation_strength_consumer relation_strength_consumer_;
    constr_param_name_consumer constr_param_name_consumer_;
    atomic_prop_rel_op_consumer atomic_prop_rel_op_consumer_;
    constr_int_value_consumer constr_int_value_consumer_;
    constr_bool_value_consumer constr_bool_value_consumer_;
    constr_enum_value_consumer constr_enum_value_consumer_;
    constraint_end_consumer constraint_end_consumer_;
    parameter current_param_;
    relation current_relation_;
    std::string constr_param_name_;
    relational_operator atomic_prop_rel_op_;
    peg::parser parser_;
    bool error_occurred_;
    std::string error_message_;
    model* model_;
};

acts_model_parser::acts_model_parser() : impl_{std::make_unique<impl>()} {}

acts_model_parser::~acts_model_parser() {}

bool acts_model_parser::parse_input_model(std::string_view sv, model& model) {

  impl_->set_input_model(&model);
  return impl_->parse_input_model(sv);
}

std::string_view acts_model_parser::get_last_error_message() const {
  return impl_->get_last_error_message();
}

}  // namespace detail
}  // namespace citcpp
