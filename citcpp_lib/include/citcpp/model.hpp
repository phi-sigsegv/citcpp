#ifndef INPUT_MODEL_HPP_
#define INPUT_MODEL_HPP_

#include <ostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "constraints.hpp"
#include "parameter.hpp"

namespace citcpp {

/**
 * Represents an input model consisting of a list of parameters.
 */
class model {
  public:
    model() = default;
    model(const model& other)
        : name_(other.name_),
          parameters_(other.parameters_),
          relations_(other.relations_),
          constraints_(other.constraints_) {
      for (auto& c : constraints_) {
        c = c->create_copy();
      }
    }
    model(model&& other)
        : name_(std::move(other.name_)),
          parameters_(std::move(other.parameters_)),
          relations_(std::move(other.relations_)),
          constraints_(std::move(other.constraints_)) {}

    ~model() = default;

    model& operator=(const model& other) {
      if (&other != this) {
        name_ = other.name_;
        parameters_ = other.parameters_;
        relations_ = other.relations_;
        constraints_ = other.constraints_;
        for (auto& c : constraints_) {
          c = c->create_copy();
        }
      }

      return *this;
    }
    model& operator=(model&& other) {
      if (&other != this) {
        name_ = std::move(other.name_);
        parameters_ = std::move(other.parameters_);
        relations_ = std::move(other.relations_);
        constraints_ = std::move(other.constraints_);
      }

      return *this;
    }

    const std::string& get_name() const { return name_; }

    void set_name(std::string_view name) { name_ = name; }

    const std::vector<parameter>& get_parameters() const { return parameters_; }

    std::vector<parameter>& get_parameters() { return parameters_; }

    void add_parameter(const parameter& param) { parameters_.push_back(param); }

    void add_parameter(parameter&& param) {
      parameters_.push_back(std::move(param));
    }

    const std::vector<relation>& get_relations() const { return relations_; }

    std::vector<relation>& get_relations() { return relations_; }

    void add_relation(const relation& r) { relations_.push_back(r); }

    void add_relation(relation&& r) { relations_.push_back(std::move(r)); }

    const std::vector<std::shared_ptr<constraint>>& get_constraints() const {
      return constraints_;
    }

    std::vector<std::shared_ptr<constraint>>& get_constraints() {
      return constraints_;
    }

    void add_constraint(std::shared_ptr<constraint> constraint) {
      constraints_.push_back(std::move(constraint));
    }

    friend std::ostream& operator<<(std::ostream& os, const model& model);

  private:
    std::string name_;
    std::vector<parameter> parameters_;
    std::vector<relation> relations_;
    std::vector<std::shared_ptr<constraint>> constraints_;
};

}  // namespace citcpp

#endif /* INPUT_MODEL_HPP_ */
