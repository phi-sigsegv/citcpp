#ifndef COVM_JSON_HPP_
#define COVM_JSON_HPP_

#include <citcpp/coverage_measurement.hpp>
#include <ostream>

namespace citcpp {
namespace detail {

class coverage_measurement_json {
  public:
    coverage_measurement_json(const coverage_measurement &covm);

    friend std::ostream &operator<<(std::ostream &os,
                                    const coverage_measurement_json &covm_json);

  private:
    const coverage_measurement &covm_;
};

}  // namespace detail
}  // namespace citcpp

#endif /* COVM_JSON_HPP_ */
