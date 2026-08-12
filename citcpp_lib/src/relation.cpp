#include <citcpp/relation.hpp>

namespace citcpp {

const std::string& relation::get_name() const { return name_; }

void relation::set_name(std::string_view name) { name_ = name; }

const std::vector<parameter_reference>& relation::get_parameters() const {
  return parameters_;
}

std::vector<parameter_reference>& relation::get_parameters() {
  return parameters_;
}

void relation::add_parameter(const parameter& parameter) {
  parameters_.push_back(parameter);
}

void relation::add_parameter(parameter_reference parameter) {
  parameters_.push_back(std::move(parameter));
}

unsigned int relation::get_interaction_strength() const {
  return interaction_strength_;
}

void relation::set_interaction_strength(unsigned int interaction_strength) {
  interaction_strength_ = interaction_strength;
}

bool relation::operator==(const relation& other) const {
  return name_ == other.name_ && parameters_ == other.parameters_ &&
         interaction_strength_ == other.interaction_strength_;
}

std::ostream& operator<<(std::ostream& os, const relation& rel) {
  os << rel.get_name() << ": (";
  for (const auto& param : rel.get_parameters()) {
    os << param << ", ";
  }
  os << rel.get_interaction_strength();
  os << ")";

  return os;
}

}  // namespace citcpp
