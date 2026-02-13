#ifndef IPOG_VERTICAL_EXTENSION_COMMON_HPP_
#define IPOG_VERTICAL_EXTENSION_COMMON_HPP_

#include <unordered_map>

namespace citcpp {
namespace detail {

struct ipog_vertical_extension_result_2 {
    unsigned long long num_checked_tuples;
    unsigned long long num_new_covered_tuples;
};

typedef std::unordered_map<const internal_relation*,
                           ipog_vertical_extension_result_2>
    ipog_vertical_extension_result;

}  // namespace detail
}  // namespace citcpp

#endif /* IPOG_VERTICAL_EXTENSION_COMMON_HPP_ */
