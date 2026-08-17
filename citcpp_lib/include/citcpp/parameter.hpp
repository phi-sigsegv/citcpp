#ifndef CITCPP_PARAMETER_HPP_
#define CITCPP_PARAMETER_HPP_

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
    /**
     * Creates a parameter value of type string with the empty
     * string with a default-constructed string.
     */
    parameter_value();

    /**
     * Created a parameter value according to the type of the given
     * value (bool, std::string, or int).
     */
    template <typename T>
    parameter_value(const T& value) : value_(value) {}

    operator bool() const;
    operator const std::string&() const;
    operator int() const;

    /**
     * Sets the parameter value according to the type of the given
     * value (bool, std::string, or int).
     */
    template <typename T>
    void set_value(const T& value) {
      value_ = value;
    }

    /**
     * Sets the parameter value according to the type of the given
     * value (bool, std::string, or int), and returns this parameter
     * value object.
     */
    template <typename T>
    parameter_value& value(const T& value) {
      value_ = value;
      return *this;
    }

    /**
     * Returns a reference to the underlying std::variant of this parameter
     * value.
     */
    const std::variant<bool, std::string, int>& get_variant_value() const;

    /**
     * Compares this parameter value against a given other one for equility.
     */
    bool operator==(const parameter_value& other) const;

    friend std::ostream& operator<<(std::ostream& os,
                                    const parameter_value& param_value);

  private:
    std::variant<bool, std::string, int> value_;
};

/**
 * Implements a hash functor for parameter_value.
 */
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
    const std::string& get_name() const;

    std::string& get_name();

    void set_name(std::string_view name);

    parameter& name(std::string_view name);

    const parameter_type& get_type() const;

    parameter_type& get_type();

    void set_type(parameter_type type);

    parameter& type(parameter_type type);

    const std::vector<parameter_value>& get_values() const;

    std::vector<parameter_value>& get_values();

    void set_values(const std::vector<parameter_value>& values);

    parameter& values(const std::vector<parameter_value>& values);

    void add_value(parameter_value value);

    bool operator==(const parameter& other) const;

    friend std::ostream& operator<<(std::ostream& os, const parameter& param);

  private:
    std::string name_{};
    parameter_type type_{parameter_type::BOOLEAN};
    std::vector<parameter_value> values_{};
};

/**
 * This class models a non-owning reference to some parameter.
 */
class parameter_reference {
  public:
    parameter_reference();

    parameter_reference(const std::string& param_name);

    parameter_reference(const parameter& param);

    const std::string& get_name() const;

    bool operator==(const parameter_reference& other) const;

    friend std::ostream& operator<<(std::ostream& os,
                                    const parameter_reference& param);

  private:
    std::string param_name_;
};

/**
 * Implements a hash functor for parameter_reference.
 */
struct parameter_reference_hash {
    std::size_t operator()(const parameter_reference& param) const noexcept {
      return std::hash<std::string>{}(param.get_name());
    }
};

}  // namespace citcpp

#endif /* CITCPP_PARAMETER_HPP_ */
