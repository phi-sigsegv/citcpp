#ifndef INPUT_MODEL_HPP_
#define INPUT_MODEL_HPP_

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
          parameter_name_map_(),
          relations_(other.relations_),
          constraints_(other.constraints_) {
      for (auto& param : parameters_) {
        parameter_name_map_[param.get_name()] = &param;
      }
    }
    model(model&& other)
        : name_(std::move(other.name_)),
          parameters_(std::move(other.parameters_)),
          parameter_name_map_(),
          relations_(std::move(other.relations_)),
          constraints_(std::move(other.constraints_)) {
      for (auto& param : parameters_) {
        parameter_name_map_[param.get_name()] = &param;
      }
    }

    ~model() = default;

    model& operator=(const model& other) {
      if (&other != this) {
        name_ = other.name_;
        parameters_ = other.parameters_;
        parameter_name_map_.clear();
        for (auto& param : parameters_) {
          parameter_name_map_[param.get_name()] = &param;
        }
        relations_ = other.relations_;
        constraints_ = other.constraints_;
      }

      return *this;
    }
    model& operator=(model&& other) {
      if (&other != this) {
        name_ = std::move(other.name_);
        parameters_ = std::move(other.parameters_);
        parameter_name_map_.clear();
        for (auto& param : parameters_) {
          parameter_name_map_[param.get_name()] = &param;
        }
        relations_ = std::move(other.relations_);
        constraints_ = std::move(other.constraints_);
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

    const std::vector<constraint_holder>& get_constraints() const {
      return constraints_;
    }

    void add_constraint(constraint_holder constraint) {
      constraints_.push_back(std::move(constraint));
    }

    void add_constraint(std::unique_ptr<constraint> constraint) {
      constraints_.push_back(std::move(constraint));
    }

  private:
    std::string name_;
    std::vector<parameter> parameters_;
    std::unordered_map<std::string, parameter*> parameter_name_map_;
    std::vector<relation> relations_;
    std::vector<constraint_holder> constraints_;
};

}  // namespace citcpp

#endif /* INPUT_MODEL_HPP_ */
