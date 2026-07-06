#ifndef CITCPP_PARAMETER_HPP_
#define CITCPP_PARAMETER_HPP_

#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>
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

      std::visit(
          [&os](auto arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, bool>) {
              if (arg) {
                os << "true";
              } else {
                os << "false";
              }
            } else {
              os << arg;
            }
          },
          param_value.value_);

      return os;
    }

  private:
    std::variant<bool, std::string, int> value_;
};

struct parameter_value_hash {
    std::size_t operator()(const parameter_value& value) const noexcept {
      return std::hash<std::variant<bool, std::string, int>>{}(
          value.get_variant_value());
    }
};

extern const parameter_value DONT_CARE_PARAMETER_VALUE;

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

    friend std::ostream& operator<<(std::ostream& os, const parameter& param);

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
    parameter_reference() = default;
    parameter_reference(const std::string& param_name)
        : param_name_(param_name) {}
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

    friend std::ostream& operator<<(std::ostream& os,
                                    const parameter_reference& param);

  private:
    std::string param_name_;
};

struct parameter_reference_hash {
    std::size_t operator()(const parameter_reference& param) const noexcept {
      return std::hash<std::string>{}(param.get_name());
    }
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

    friend std::ostream& operator<<(std::ostream& os, const relation& rel);

  private:
    std::string name_;
    std::vector<parameter_reference> parameters_;
    unsigned int interaction_strength_;
};

}  // namespace citcpp

#endif /* CITCPP_PARAMETER_HPP_ */
