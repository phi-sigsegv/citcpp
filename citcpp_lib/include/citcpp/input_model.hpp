#ifndef INPUT_MODEL_HPP_
#define INPUT_MODEL_HPP_

#include <ostream>
#include <string>
#include <string_view>
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
    parameter_value(const T &value) : value_(value) {}

    operator bool() { return std::get<bool>(value_); }
    operator const std::string &() { return std::get<std::string>(value_); }
    operator int() { return std::get<int>(value_); }

    template <typename T>
    void set_value(const T &value) {
      value_ = value;
    }

    template <typename T>
    parameter_value &value(const T &value) {
      value_ = value;
      return *this;
    }

    bool operator==(const parameter_value &other) const {
      return value_ == other.value_;
    }

    friend std::ostream &operator<<(std::ostream &os,
                                    const parameter_value &param_value) {

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
    const std::string &get_name() const { return name_; }

    std::string &get_name() { return name_; }

    void set_name(std::string_view name) { name_ = name; }

    parameter &name(std::string_view name) {
      name_ = name;
      return *this;
    }

    const parameter_type &get_type() const { return type_; }

    parameter_type &get_type() { return type_; }

    void set_type(parameter_type type) { type_ = type; }

    parameter &type(parameter_type type) {
      type_ = type;
      return *this;
    }

    const std::vector<parameter_value> &get_values() const { return values_; }

    std::vector<parameter_value> &get_values() { return values_; }

    void set_values(const std::vector<parameter_value> &values) {
      values_ = values;
    }

    parameter &values(const std::vector<parameter_value> &values) {
      values_ = values;
      return *this;
    }

    void add_value(const parameter_value &value) { values_.push_back(value); }

    void add_value(parameter_value &&value) {
      values_.push_back(std::move(value));
    }

    bool operator==(const parameter &other) const {
      return name_ == other.name_ && type_ == other.type_ &&
             values_ == other.values_;
    }

  private:
    std::string name_;
    parameter_type type_;
    std::vector<parameter_value> values_;
};

/**
 * Represents an input model consisting of a list of parameters.
 */
class input_model {
  public:
    const std::string &get_name() const { return name_; }

    void set_name(std::string_view name) { name_ = name; }

    const std::vector<parameter> &get_parameters() const { return parameters_; }

    std::vector<parameter> &get_parameters() { return parameters_; }

    void add_parameter(const parameter &parameter) {
      parameters_.push_back(parameter);
    }

    void add_parameter(parameter &&parameter) {
      parameters_.push_back(std::move(parameter));
    }

    bool operator==(const input_model &other) const {
      return name_ == other.name_ && parameters_ == other.parameters_;
    }

  private:
    std::string name_;
    std::vector<parameter> parameters_;
};

}  // namespace citcpp

#endif /* INPUT_MODEL_HPP_ */
