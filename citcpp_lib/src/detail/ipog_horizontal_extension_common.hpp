#ifndef IPOG_HORIZONTAL_EXTENSION_COMMON_HPP_
#define IPOG_HORIZONTAL_EXTENSION_COMMON_HPP_

#include "internal_test_set.hpp"

namespace citcpp {
namespace detail {

struct ipog_horizontal_extension_result {
    std::vector<list_intrusive<test>> value_to_row_mapping;
    list_intrusive<test> rows_with_current_parameter_dont_care_value;
    unsigned long long num_new_covered_tuples;
};

}  // namespace detail
}  // namespace citcpp

#endif /* IPOG_HORIZONTAL_EXTENSION_COMMON_HPP_ */
