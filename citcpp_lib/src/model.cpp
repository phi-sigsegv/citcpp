#include <citcpp/model.hpp>

namespace citcpp {

std::ostream& operator<<(std::ostream& os, const parameter_reference& param) {
  os << param.get_name();

  return os;
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

std::ostream& operator<<(std::ostream& os, const relation& rel) {
  os << rel.get_name() << ": (";
  for (const auto& param : rel.get_parameters()) {
    os << param << ", ";
  }
  os << rel.get_interaction_strength();
  os << ")";

  return os;
}

std::ostream& operator<<(std::ostream& os, const model& model) {
  os << "[System]\n";
  os << "Name: " << model.get_name() << "\n\n";

  os << "[Parameter]\n";
  for (const auto& param : model.get_parameters()) {
    os << param << "\n";
  }
  os << "\n";

  if (!model.get_relations().empty()) {
    os << "[Relation]\n";
    for (const auto& rel : model.get_relations()) {
      os << rel << "\n";
    }
    os << "\n";
  }

  if (!model.get_constraints().empty()) {
    os << "[Constraint]\n";
    for (const auto& constr : model.get_constraints()) {
      os << *constr.get() << "\n";
    }
    os << "\n";
  }

  return os;
}

}  // namespace citcpp
