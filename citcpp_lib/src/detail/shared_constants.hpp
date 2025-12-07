#ifndef SHARED_CONSTANTS_HPP_
#define SHARED_CONSTANTS_HPP_

#include <cstddef>

namespace citcpp {
namespace detail {

inline constexpr std::size_t false_sharing_avoidance_alignment = 64;

}  // namespace detail
}  // namespace citcpp

#endif /* SHARED_CONSTANTS_HPP_ */
