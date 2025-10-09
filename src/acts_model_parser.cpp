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
    virtual void add_param_value(const std::string &value) = 0;
    virtual void add_param_value(int value) = 0;
    virtual void end_param_declaration() = 0;
    virtual void set_relation_identifier(std::string_view identifier) = 0;
    virtual void add_param_to_relation(std::string_view identifier, size_t line,
                                       size_t col) = 0;
    virtual void set_relation_strength(int strength) = 0;
};

class system_name_consumer {
  public:
    system_name_consumer(input_model_data_consumer *consumer)
        : consumer_(consumer) {}

    void operator()(const peg::SemanticValues &vs) {
      consumer_->set_system_name(vs.token_to_string());
    }

  private:
    input_model_data_consumer *consumer_;
};

class param_identifier_consumer {
  public:
    param_identifier_consumer(input_model_data_consumer *consumer)
        : consumer_(consumer) {}

    void operator()(const peg::SemanticValues &vs) {
      consumer_->set_param_identifier(vs.token_to_string());
    }

  private:
    input_model_data_consumer *consumer_;
};

class param_boolean_type_consumer {
  public:
    param_boolean_type_consumer(input_model_data_consumer *consumer)
        : consumer_(consumer) {}

    void operator()(const peg::SemanticValues &vs) {
      consumer_->set_param_type(citcpp::parameter_type::BOOLEAN);
    }

  private:
    input_model_data_consumer *consumer_;
};

class param_enum_type_consumer {
  public:
    param_enum_type_consumer(input_model_data_consumer *consumer)
        : consumer_(consumer) {}

    void operator()(const peg::SemanticValues &vs) {
      consumer_->set_param_type(citcpp::parameter_type::ENUM);
    }

  private:
    input_model_data_consumer *consumer_;
};

class param_integer_type_consumer {
  public:
    param_integer_type_consumer(input_model_data_consumer *consumer)
        : consumer_(consumer) {}

    void operator()(const peg::SemanticValues &vs) {
      consumer_->set_param_type(citcpp::parameter_type::INTEGER);
    }

  private:
    input_model_data_consumer *consumer_;
};

class boolean_value_consumer {
  public:
    boolean_value_consumer(input_model_data_consumer *consumer)
        : consumer_(consumer) {}

    void operator()(const peg::SemanticValues &vs) {
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
    input_model_data_consumer *consumer_;
};

class enum_value_consumer {
  public:
    enum_value_consumer(input_model_data_consumer *consumer)
        : consumer_(consumer) {}

    void operator()(const peg::SemanticValues &vs) {
      std::string str(vs.token_to_string());
      citcpp::detail::trim(str);
      consumer_->add_param_value(str);
    }

  private:
    input_model_data_consumer *consumer_;
};

class integer_value_consumer {
  public:
    integer_value_consumer(input_model_data_consumer *consumer)
        : consumer_(consumer) {}

    void operator()(const peg::SemanticValues &vs) {
      consumer_->add_param_value(vs.token_to_number<int>());
    }

  private:
    input_model_data_consumer *consumer_;
};

class param_declaration_end_consumer {
  public:
    param_declaration_end_consumer(input_model_data_consumer *consumer)
        : consumer_(consumer) {}

    void operator()(const peg::SemanticValues &vs) {
      consumer_->end_param_declaration();
    }

  private:
    input_model_data_consumer *consumer_;
};

class relation_identifier_consumer {
  public:
    relation_identifier_consumer(input_model_data_consumer *consumer)
        : consumer_(consumer) {}

    void operator()(const peg::SemanticValues &vs) {
      consumer_->set_relation_identifier(vs.token_to_string());
    }

  private:
    input_model_data_consumer *consumer_;
};

class relation_param_identifier_consumer {
  public:
    relation_param_identifier_consumer(input_model_data_consumer *consumer)
        : consumer_(consumer) {}

    void operator()(const peg::SemanticValues &vs) {
      auto line_info = vs.line_info();
      consumer_->add_param_to_relation(vs.token_to_string(), line_info.first,
                                       line_info.second);
    }

  private:
    input_model_data_consumer *consumer_;
};

class relation_strength_consumer {
  public:
    relation_strength_consumer(input_model_data_consumer *consumer)
        : consumer_(consumer) {}

    void operator()(const peg::SemanticValues &vs) {
      consumer_->set_relation_strength(vs.token_to_number<int>());
    }

  private:
    input_model_data_consumer *consumer_;
};

peg::parser create_acts_model_parser() {
  using namespace peg;

  parser p(R"(
Root             <- _ SystemSection ParameterSection RelationSection?

SystemSection    <- '[' _ 'System'i _ ']' _ SystemNameRule
SystemNameRule   <- 'name'i _ ':' _ SystemName _
SystemName       <- (!Eol .)*

ParameterSection <- '[' _ 'Parameter'i _ ']' _ ParameterRule+ _
ParameterRule    <- (BooleanParam / EnumParam / IntParam)

BooleanParam     <- ParamName _ BooleanType _ ':' _ BooleanValueList SpaceChar* ParamDelcEnd
EnumParam        <- ParamName _ EnumType _ ':' _ EnumValueList ParamDelcEnd
IntParam         <- ParamName _ IntType _ ':' _ IntegerValueList SpaceChar* ParamDelcEnd

ParamName        <- Identifier

BooleanType      <- '(' _ ('boolean'i / 'bool'i) _ ')'
EnumType         <- '(' _ 'enum'i _ ')'
IntType          <- '(' _ ('integer'i / 'int'i) _ ')'

ParamDelcEnd     <- Eol

BooleanValueList <- BooleanValue (_ ',' _ BooleanValue)*
BooleanValue     <- 'true'i / 'false'i

EnumValueList    <- EnumValue (_ ',' _ EnumValue)*
EnumValue        <- (!(',' / ';' / SpaceChar / Eol) .) (!(',' / ';' / Eol) .)*

IntegerValueList <- IntegerValue (_ ',' _ IntegerValue)*
IntegerValue     <- [+-]? [0-9]+

RelationSection  <- '[' _ 'Relation'i _ ']' _ RelationRule*
RelationRule     <- RelationName _ ':' _ '(' _ RelParamNameList _ ',' _ RelStrength _ ')' _

RelationName     <- Identifier
RelParamNameList <- RelParamName (_ ',' _ RelParamName)*
RelParamName     <- Identifier
RelStrength      <- [0-9]+

Identifier       <- [a-zA-Z_] [a-zA-Z0-9_]*

~_               <- (WhiteSpace / Eol)*
WhiteSpace       <- SpaceChar / LineComment
SpaceChar        <- ' ' / '\t'
Eol              <- '\r\n' / '\n' / '\r'
LineComment      <- '--' (!Eol .)* &Eol
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
          parser_(create_acts_model_parser()),
          current_param_(parameter()),
          current_relation_(relation()),
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

      parser_.set_logger([this](size_t line, size_t col,
                                const std::string &msg) {
        std::ostringstream oss;
        oss << "Error in model file at " << line << ":" << col << " -> " << msg;

        error_message_ = oss.str();
      });
    }

    void set_input_model(model *model) { model_ = model; }

    void set_system_name(std::string_view name) { model_->set_name(name); }

    void set_param_identifier(std::string_view identifier) {
      current_param_.set_name(identifier);
    }

    void set_param_type(citcpp::parameter_type type) {
      current_param_.set_type(type);
    }

    void add_param_value(bool value) { current_param_.add_value(value); }

    void add_param_value(const std::string &value) {
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
      for (const parameter &param : model_->get_parameters()) {
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
    peg::parser parser_;
    bool error_occurred_;
    std::string error_message_;
    parameter current_param_;
    relation current_relation_;
    model *model_;
};

acts_model_parser::acts_model_parser() : impl_{std::make_unique<impl>()} {}

acts_model_parser::~acts_model_parser() {}

bool acts_model_parser::parse_input_model(std::string_view sv, model &model) {

  impl_->set_input_model(&model);
  return impl_->parse_input_model(sv);
}

std::string_view acts_model_parser::get_last_error_message() const {
  return impl_->get_last_error_message();
}

}  // namespace detail
}  // namespace citcpp
