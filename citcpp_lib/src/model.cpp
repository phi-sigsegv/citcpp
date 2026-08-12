#include <citcpp/model.hpp>

namespace citcpp {

model::model() = default;

model::model(const model& other)
    : name_(other.name_),
      parameters_(other.parameters_),
      relations_(other.relations_),
      constraints_() {
  constraints_.reserve(other.constraints_.size());
  for (const auto& c : other.constraints_) {
    constraints_.push_back(c->create_copy());
  }
}

model::model(model&& other) noexcept = default;

model::~model() = default;

model& model::operator=(const model& other) {
  if (&other != this) {
    name_ = other.name_;
    parameters_ = other.parameters_;
    relations_ = other.relations_;
    constraints_.clear();
    constraints_.reserve(other.constraints_.size());
    for (const auto& c : other.constraints_) {
      constraints_.push_back(c->create_copy());
    }
  }

  return *this;
}

model& model::operator=(model&& other) noexcept = default;

const std::string& model::get_name() const { return name_; }

void model::set_name(std::string_view name) { name_ = name; }

const std::vector<parameter>& model::get_parameters() const {
  return parameters_;
}

std::vector<parameter>& model::get_parameters() { return parameters_; }

void model::add_parameter(parameter param) {
  parameters_.push_back(std::move(param));
}

const std::vector<relation>& model::get_relations() const { return relations_; }

std::vector<relation>& model::get_relations() { return relations_; }

void model::add_relation(relation r) { relations_.push_back(std::move(r)); }

const std::vector<std::shared_ptr<constraint>>& model::get_constraints() const {
  return constraints_;
}

std::vector<std::shared_ptr<constraint>>& model::get_constraints() {
  return constraints_;
}

void model::add_constraint(std::shared_ptr<constraint> constraint) {
  constraints_.push_back(std::move(constraint));
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
