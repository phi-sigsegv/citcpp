#ifndef TEST_SET_HPP_
#define TEST_SET_HPP_

#include <list>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

#include "parameter.hpp"

namespace citcpp {

/**
 * This class represents a produced test set.
 */
class test_set {
  public:
    test_set(std::string_view value_separator);

    test_set();

    const std::string& get_value_separator() const { return value_separator_; }

    const std::vector<parameter>& get_parameters() const { return parameters_; }

    std::vector<parameter>& get_parameters() { return parameters_; }

    void add_parameter(const parameter& parameter) {
      parameters_.push_back(parameter);
    }

    void add_parameter(parameter&& parameter) {
      parameters_.push_back(std::move(parameter));
    }

    const std::list<std::vector<int>>& get_list_of_tests() const {
      return test_set_;
    }

    std::list<std::vector<int>>& get_list_of_tests() { return test_set_; }

    bool operator==(const test_set& other) const {
      return parameters_ == other.parameters_ && test_set_ == other.test_set_;
    }

    friend std::ostream& operator<<(std::ostream& os, const test_set& test_set);

  private:
    std::string value_separator_;
    std::vector<parameter> parameters_;
    std::list<std::vector<int>> test_set_;
};

}  // namespace citcpp

#endif /* TEST_SET_HPP_ */
