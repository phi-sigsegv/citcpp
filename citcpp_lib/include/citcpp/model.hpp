#ifndef INPUT_MODEL_HPP_
#define INPUT_MODEL_HPP_

#include <ostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace citcpp {

/**
 * Represents a value of a parameter.
 */
class parameter_value {
  public:
    parameter_value() : value_("") {}

    template <typename T>
    parameter_value(const T& value) : value_(value) {}

    operator bool() const { return std::get<bool>(value_); }
    operator const std::string&() const {
      return std::get<std::string>(value_);
    }
    operator int() const { return std::get<int>(value_); }

    template <typename T>
    void set_value(const T& value) {
      value_ = value;
    }

    template <typename T>
    parameter_value& value(const T& value) {
      value_ = value;
      return *this;
    }

    const std::variant<bool, std::string, int>& get_variant_value() const {
      return value_;
    }

    bool operator==(const parameter_value& other) const {
      return value_ == other.value_;
    }

    friend std::ostream& operator<<(std::ostream& os,
                                    const parameter_value& param_value) {

      std::visit([&os](auto arg) { os << arg; }, param_value.value_);

      return os;
    }

  private:
    std::variant<bool, std::string, int> value_;
};

enum class parameter_type { BOOLEAN, ENUM, INTEGER };

/**
 * Represents a parameter, which has a set of possible values.
 */
class parameter {
  public:
    const std::string& get_name() const { return name_; }

    std::string& get_name() { return name_; }

    void set_name(std::string_view name) { name_ = name; }

    parameter& name(std::string_view name) {
      name_ = name;
      return *this;
    }

    const parameter_type& get_type() const { return type_; }

    parameter_type& get_type() { return type_; }

    void set_type(parameter_type type) { type_ = type; }

    parameter& type(parameter_type type) {
      type_ = type;
      return *this;
    }

    const std::vector<parameter_value>& get_values() const { return values_; }

    std::vector<parameter_value>& get_values() { return values_; }

    void set_values(const std::vector<parameter_value>& values) {
      values_ = values;
    }

    parameter& values(const std::vector<parameter_value>& values) {
      values_ = values;
      return *this;
    }

    void add_value(const parameter_value& value) { values_.push_back(value); }

    void add_value(parameter_value&& value) {
      values_.push_back(std::move(value));
    }

    bool operator==(const parameter& other) const {
      return name_ == other.name_;
    }

  private:
    std::string name_;
    parameter_type type_;
    std::vector<parameter_value> values_;
};

/**
 * This class models a non-owning reference to some parameter.
 */
class parameter_reference {
  public:
    parameter_reference(const parameter& param)
        : param_name_(param.get_name()) {}
    parameter_reference(const parameter_reference& other) = default;
    parameter_reference(parameter_reference&& other) = default;

    ~parameter_reference() = default;

    parameter_reference& operator=(const parameter_reference& other) = default;
    parameter_reference& operator=(parameter_reference&& other) = default;

    const std::string& get_name() const { return param_name_; }

    bool operator==(const parameter_reference& other) const {
      return get_name() == other.get_name();
    }

  private:
    std::string param_name_;
};

/**
 * Represents a relation, which is a set of parameters and an interaction
 * strength.
 */
class relation {
  public:
    const std::string& get_name() const { return name_; }

    void set_name(std::string_view name) { name_ = name; }

    const std::vector<parameter_reference>& get_parameters() const {
      return parameters_;
    }

    std::vector<parameter_reference>& get_parameters() { return parameters_; }

    void add_parameter(const parameter& parameter) {
      parameters_.push_back(parameter);
    }

    void add_parameter(const parameter_reference& parameter) {
      parameters_.push_back(parameter);
    }

    void add_parameter(parameter_reference&& parameter) {
      parameters_.push_back(std::move(parameter));
    }

    unsigned int get_interaction_strength() const {
      return interaction_strength_;
    }

    void set_interaction_strength(unsigned int interaction_strength) {
      interaction_strength_ = interaction_strength;
    }

    bool operator==(const relation& other) const {
      return name_ == other.name_ && parameters_ == other.parameters_ &&
             interaction_strength_ == other.interaction_strength_;
    }

  private:
    std::string name_;
    std::vector<parameter_reference> parameters_;
    unsigned int interaction_strength_;
};

/**
 * Represents an input model consisting of a list of parameters.
 */
class model {
  public:
    model() = default;
    model(const model& other)
        : name_(other.get_name()),
          parameters_(other.get_parameters()),
          parameter_name_map_(),
          relations_(other.get_relations()) {
      for (auto& param : parameters_) {
        parameter_name_map_[param.get_name()] = &param;
      }
    }
    model(model&& other)
        : name_(std::move(other.get_name())),
          parameters_(std::move(other.get_parameters())),
          parameter_name_map_(),
          relations_(std::move(other.get_relations())) {
      for (auto& param : parameters_) {
        parameter_name_map_[param.get_name()] = &param;
      }
    }

    ~model() = default;

    model& operator=(const model& other) {
      if (&other != this) {
        name_ = other.get_name();
        parameters_ = other.get_parameters();
        parameter_name_map_.clear();
        for (auto& param : parameters_) {
          parameter_name_map_[param.get_name()] = &param;
        }
        relations_ = other.get_relations();
      }

      return *this;
    }
    model& operator=(model&& other) {
      if (&other != this) {
        name_ = std::move(other.get_name());
        parameters_ = std::move(other.get_parameters());
        parameter_name_map_.clear();
        for (auto& param : parameters_) {
          parameter_name_map_[param.get_name()] = &param;
        }
        relations_ = std::move(other.get_relations());
      }

      return *this;
    }

    const std::string& get_name() const { return name_; }

    void set_name(std::string_view name) { name_ = name; }

    const std::vector<parameter>& get_parameters() const { return parameters_; }

    const parameter& get_parameter(const std::string& name) const {
      return *parameter_name_map_.at(name);
    }

    parameter& get_parameter(const std::string& name) {
      return *parameter_name_map_.at(name);
    }

    void add_parameter(const parameter& param) {
      parameters_.push_back(param);
      parameter_name_map_[param.get_name()] = &parameters_.back();
    }

    void add_parameter(parameter&& param) {
      parameters_.push_back(std::move(param));
      parameter_name_map_[param.get_name()] = &parameters_.back();
    }

    const std::vector<relation>& get_relations() const { return relations_; }

    void add_relation(const relation& r) { relations_.push_back(r); }

    void add_relation(relation&& r) { relations_.push_back(std::move(r)); }

    bool operator==(const model& other) const {
      return name_ == other.name_ && parameters_ == other.parameters_ &&
             relations_ == other.relations_;
    }

  private:
    std::string name_;
    std::vector<parameter> parameters_;
    std::unordered_map<std::string, parameter*> parameter_name_map_;
    std::vector<relation> relations_;
};

}  // namespace citcpp

#endif /* INPUT_MODEL_HPP_ */
