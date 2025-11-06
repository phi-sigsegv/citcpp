#ifndef IPOG_VERTICAL_EXTENSION_COMMON_HPP_
#define IPOG_VERTICAL_EXTENSION_COMMON_HPP_

#include <unordered_map>

namespace citcpp {
namespace detail {

struct ipog_vertical_extension_result {
    std::unordered_map<const internal_relation*, unsigned long long>
        num_new_covered_tuples;
};

}  // namespace detail
}  // namespace citcpp

#endif /* IPOG_VERTICAL_EXTENSION_COMMON_HPP_ */
