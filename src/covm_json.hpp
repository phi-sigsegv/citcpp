#ifndef COVM_JSON_HPP_
#define COVM_JSON_HPP_

#include <citcpp/coverage_measurement.hpp>
#include <ostream>
#include <unordered_map>

namespace citcpp {
namespace detail {

class coverage_measurement_json {
  public:
    coverage_measurement_json(
        const std::unordered_map<std::string, coverage_measurement>& covm,
        const std::vector<std::size_t>& invalid_test_indices);

    friend std::ostream& operator<<(std::ostream& os,
                                    const coverage_measurement_json& covm_json);

  private:
    const std::unordered_map<std::string, coverage_measurement>& covm_;
    const std::vector<std::size_t>& invalid_test_indices_;
};

}  // namespace detail
}  // namespace citcpp

#endif /* COVM_JSON_HPP_ */
