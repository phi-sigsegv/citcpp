#ifndef CITCPP_UTILS_HPP_
#define CITCPP_UTILS_HPP_

#include <citcpp/input_model.hpp>
#include <citcpp/test_set.hpp>
#include <new>
#include <string>

#include "internal_model.hpp"
#include "internal_test_set.hpp"

namespace citcpp {
namespace detail {

struct alignas(std::hardware_destructive_interference_size) aligned_ull_value {
    unsigned long long value;
};

extern const std::string EMPTY_VALUE_SEPARATOR;
extern const std::string DEFAULT_VALUE_SEPARATOR;
extern const citcpp::parameter_value DONT_CARE_PARAMETER_VALUE;

internal_test_set create_internal_test_set(const input_model &input_model,
                                           const citcpp::test_set &tests);

void replace_dont_care_values(internal_test_set &test_set, const model &model);

}  // namespace detail
}  // namespace citcpp

#endif /* CITCPP_UTILS_HPP_ */
