#include <cstdint>
#include <vector>

#include "aligned_allocator.hpp"
#include "shared_constants.hpp"
#include "small_vector.hpp"
#include "spin_lock.hpp"

#ifndef DETAIL_COMPILE_TIME_SELECTED_DATATYPES_HPP_
#define DETAIL_COMPILE_TIME_SELECTED_DATATYPES_HPP_

namespace citcpp {
namespace detail {

// using param_vector = std::vector<std::uint16_t>;
// using value_vector = std::vector<int>;
template <typename T>
using small_vector = SmallVector<T, 8>;

using param_vector = small_vector<std::uint16_t>;
using value_vector = small_vector<int>;

template <typename T>
using thread_local_vector =
    std::vector<T, aligned_allocator<T, false_sharing_avoidance_alignment>>;

struct alignas(false_sharing_avoidance_alignment) aligned_ull_value {
    unsigned long long value;
};

template <typename T>
struct alignas(false_sharing_avoidance_alignment) cache_aligned : public T {
    // Lift all of T's constructors into the derive scope.
    using T::T;
};

template <typename T>
using aligned_vector = cache_aligned<std::vector<T>>;

using spin_lock = threads::SpinLock;

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_COMPILE_TIME_SELECTED_DATATYPES_HPP_ */
