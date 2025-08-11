#include <citcpp/citcpp_config.hpp>

#include "detail/citcpp_utils.hpp"

namespace citcpp {

covering_array_computation_config::covering_array_computation_config()
    : replace_dont_care_values_(true),
      multithreading_enabled_(false),
      value_seperator_(detail::DEFAULT_VALUE_SEPARATOR) {}

}  // namespace citcpp