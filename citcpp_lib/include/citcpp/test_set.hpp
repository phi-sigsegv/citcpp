#ifndef TEST_SET_HPP_
#define TEST_SET_HPP_

#include <list>
#include <ostream>
#include <vector>

#include "input_model.hpp"

namespace citcpp {

/**
 * This class represents the definition of a parameter of the testset.
 */
class parameter_def {
  public:
    const std::string &get_name() const { return name_; }

    std::string &get_name() { return name_; }

    void set_name(std::string_view name) { name_ = name; }

    parameter_def &name(std::string_view name) {
      name_ = name;
      return *this;
    }

    const parameter_type &get_type() const { return type_; }

    parameter_type &get_type() { return type_; }

    void set_type(parameter_type type) { type_ = type; }

    parameter_def &type(parameter_type type) {
      type_ = type;
      return *this;
    }

    bool operator==(const parameter_def &other) const {
      return name_ == other.name_ && type_ == other.type_;
    }

    friend std::ostream &operator<<(std::ostream &os,
                                    const parameter_def &param_def) {

      os << param_def.name_;

      return os;
    }

  private:
    std::string name_;
    parameter_type type_;
};

/**
 * This class represents a produced test set.
 */
class test_set {
  public:
    const std::vector<parameter_def> &get_parameters() const {
      return parameters_;
    }

    std::vector<parameter_def> &get_parameters() { return parameters_; }

    void add_parameter(const parameter_def &parameter) {
      parameters_.push_back(parameter);
    }

    void add_parameter(parameter_def &&parameter) {
      parameters_.push_back(std::move(parameter));
    }

    const std::list<std::vector<parameter_value>> &get_list_of_tests() const {
      return test_set_;
    }

    std::list<std::vector<parameter_value>> &get_list_of_tests() {
      return test_set_;
    }

    friend std::ostream &operator<<(std::ostream &os,
                                    const test_set &test_set) {

      const char *empty_sep = "";
      const char *real_sep = ", ";

      const char *sep = empty_sep;
      for (const auto &param : test_set.get_parameters()) {
        os << sep << param;
        sep = real_sep;
      }
      os << "\n";

      for (const auto &test : test_set.get_list_of_tests()) {
        sep = empty_sep;
        for (const auto &pv : test) {
          os << sep << pv;
          sep = real_sep;
        }
        os << "\n";
      }

      return os;
    }

  private:
    std::vector<parameter_def> parameters_;
    std::list<std::vector<parameter_value>> test_set_;
};

}  // namespace citcpp

#endif /* TEST_SET_HPP_ */
