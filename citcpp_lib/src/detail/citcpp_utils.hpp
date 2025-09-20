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

template <typename T>
struct alignas(std::hardware_destructive_interference_size) aligned_vector {
    std::vector<T> value;
};

extern const std::string EMPTY_VALUE_SEPARATOR;
extern const std::string DEFAULT_VALUE_SEPARATOR;
extern const citcpp::parameter_value DONT_CARE_PARAMETER_VALUE;

internal_test_set create_internal_test_set(const input_model &input_model,
                                           const citcpp::test_set &tests);

void replace_dont_care_values(internal_test_set &test_set, const model &model);

unsigned int get_product_of_max_n_parameter_sizes(
    const unsigned int num_parameters, const unsigned int n,
    const citcpp::detail::model &model,
    const std::vector<unsigned int> &parameter_index_map);

}  // namespace detail
}  // namespace citcpp

#endif /* CITCPP_UTILS_HPP_ */
