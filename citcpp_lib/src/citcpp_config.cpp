#include <citcpp/citcpp_config.hpp>

#include "detail/citcpp_utils.hpp"

namespace citcpp {

covering_array_computation_config::covering_array_computation_config()
    : replace_dont_care_values_(true),
      number_of_threads_(1),
      c_handler_memory_limit_gib_(8),
      value_seperator_(detail::DEFAULT_VALUE_SEPARATOR),
      algo_(covering_array_computation_algorithm::IPOG) {}

bool covering_array_computation_config::replace_dont_care_values() const {
  return replace_dont_care_values_;
}

unsigned int covering_array_computation_config::number_of_threads() const {
  return number_of_threads_;
}

std::size_t
covering_array_computation_config::constraint_handler_memory_limit_gb() const {
  return c_handler_memory_limit_gib_;
}

const std::string& covering_array_computation_config::value_separator() const {
  return value_seperator_;
}

covering_array_computation_algorithm
covering_array_computation_config::algorithm() const {
  return algo_;
}

covering_array_computation_config&
covering_array_computation_config::with_replace_dont_care_values(
    bool replace_dont_care_values) {

  replace_dont_care_values_ = replace_dont_care_values;

  return *this;
}

covering_array_computation_config&
covering_array_computation_config::with_number_of_threads(
    unsigned int number_of_threads) {

  number_of_threads_ = number_of_threads;

  return *this;
}

covering_array_computation_config&
covering_array_computation_config::with_constraint_handler_memory_limit_gb(
    std::size_t c_handler_memory_limit_gib) {

  c_handler_memory_limit_gib_ = c_handler_memory_limit_gib;

  return *this;
}

covering_array_computation_config&
covering_array_computation_config::with_value_separator(
    std::string_view value_seperator) {

  value_seperator_ = value_seperator;

  return *this;
}

covering_array_computation_config&
covering_array_computation_config::with_algorithm(
    covering_array_computation_algorithm algo) {

  algo_ = algo;

  return *this;
}

coverage_measurement_config::coverage_measurement_config()
    : number_of_threads_(1),
      c_handler_memory_limit_gib_(8),
      value_seperator_(detail::DEFAULT_VALUE_SEPARATOR) {}

std::size_t coverage_measurement_config::number_of_threads() const {
  return number_of_threads_;
}

std::size_t coverage_measurement_config::constraint_handler_memory_limit_gb()
    const {

  return c_handler_memory_limit_gib_;
}

const std::string& coverage_measurement_config::value_separator() const {
  return value_seperator_;
}

coverage_measurement_config&
coverage_measurement_config::with_number_of_threads(
    std::size_t number_of_threads) {

  number_of_threads_ = number_of_threads;

  return *this;
}

coverage_measurement_config&
coverage_measurement_config::with_constraint_handler_memory_limit_gb(
    std::size_t c_handler_memory_limit_gib) {

  c_handler_memory_limit_gib_ = c_handler_memory_limit_gib;

  return *this;
}

coverage_measurement_config& coverage_measurement_config::with_value_separator(
    std::string_view value_seperator) {

  value_seperator_ = value_seperator;

  return *this;
}

std::ostream& operator<<(std::ostream& os,
                         const covering_array_computation_algorithm& algo) {

  switch (algo) {
    case covering_array_computation_algorithm::IPOG:
      os << "IPOG";
      break;
  }

  return os;
}

}  // namespace citcpp