#include <citcpp/citcpp_config.hpp>

#include "detail/citcpp_utils.hpp"

namespace citcpp {

covering_array_computation_config::covering_array_computation_config()
    : replace_dont_care_values_(true),
      multithreading_enabled_(false),
      value_seperator_(detail::DEFAULT_VALUE_SEPARATOR),
      algo_(covering_array_computation_algorithm::IPOG) {}

coverage_measurement_config::coverage_measurement_config()
    : multithreading_enabled_(false),
      value_seperator_(detail::DEFAULT_VALUE_SEPARATOR) {}

std::ostream& operator<<(std::ostream& os,
                         const covering_array_computation_algorithm& algo) {

  switch (algo) {
    case covering_array_computation_algorithm::IPOG:
      os << "IPOG";
      break;
    case covering_array_computation_algorithm::IPOG_OTF:
      os << "IPOG_OTF";
      break;
  }

  return os;
}

}  // namespace citcpp