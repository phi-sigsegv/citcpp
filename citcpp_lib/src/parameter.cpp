#include <citcpp/parameter.hpp>
#include <type_traits>

namespace citcpp {

parameter_value::parameter_value() : value_("") {}

parameter_value::operator bool() const { return std::get<bool>(value_); }

parameter_value::operator const std::string&() const {
  return std::get<std::string>(value_);
}

parameter_value::operator int() const { return std::get<int>(value_); }

const std::variant<bool, std::string, int>& parameter_value::get_variant_value()
    const {

  return value_;
}

bool parameter_value::operator==(const parameter_value& other) const {
  return value_ == other.value_;
}

std::ostream& operator<<(std::ostream& os, const parameter_value& param_value) {
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

const parameter_value DONT_CARE_PARAMETER_VALUE{"*"};

const std::string& parameter::get_name() const { return name_; }

std::string& parameter::get_name() { return name_; }

void parameter::set_name(std::string_view name) { name_ = name; }

parameter& parameter::name(std::string_view name) {
  name_ = name;
  return *this;
}

const parameter_type& parameter::get_type() const { return type_; }

parameter_type& parameter::get_type() { return type_; }

void parameter::set_type(parameter_type type) { type_ = type; }

parameter& parameter::type(parameter_type type) {
  type_ = type;
  return *this;
}

const std::vector<parameter_value>& parameter::get_values() const {
  return values_;
}

std::vector<parameter_value>& parameter::get_values() { return values_; }

void parameter::set_values(const std::vector<parameter_value>& values) {
  values_ = values;
}

parameter& parameter::values(const std::vector<parameter_value>& values) {
  values_ = values;
  return *this;
}

void parameter::add_value(parameter_value value) {
  values_.push_back(std::move(value));
}

bool parameter::operator==(const parameter& other) const {
  return name_ == other.name_;
}

std::ostream& operator<<(std::ostream& os, const parameter& param) {
  os << param.get_name() << " ";
  switch (param.get_type()) {
    case parameter_type::BOOLEAN:
      os << "(boolean) : ";
      break;
    case parameter_type::ENUM:
      os << "(enum) : ";
      break;
    case parameter_type::INTEGER:
      os << "(int) : ";
      break;
  }

  const std::string EMPTY_SEP = "";
  const std::string REAL_SEP = ", ";
  const std::string* sep = &EMPTY_SEP;
  for (const auto& value : param.get_values()) {
    os << *sep << value;
    sep = &REAL_SEP;
  }

  return os;
}

parameter_reference::parameter_reference() = default;

parameter_reference::parameter_reference(const std::string& param_name)
    : param_name_(param_name) {}

parameter_reference::parameter_reference(const parameter& param)
    : param_name_(param.get_name()) {}

const std::string& parameter_reference::get_name() const { return param_name_; }

bool parameter_reference::operator==(const parameter_reference& other) const {
  return get_name() == other.get_name();
}

std::ostream& operator<<(std::ostream& os, const parameter_reference& param) {
  os << param.get_name();

  return os;
}

}  // namespace citcpp
