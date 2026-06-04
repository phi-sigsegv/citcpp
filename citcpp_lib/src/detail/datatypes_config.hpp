#include <cstdint>
#include <vector>

#include "aligned_allocator.hpp"
#include "shared_constants.hpp"
#include "small_vector.hpp"
#include "threading_lib.hpp"

#ifndef DETAIL_COMPILE_TIME_SELECTED_DATATYPES_HPP_
#define DETAIL_COMPILE_TIME_SELECTED_DATATYPES_HPP_

namespace citcpp {
namespace detail {

typedef std::uint16_t param_index;

// using param_vector = std::vector<param_index>;
// using value_vector = std::vector<int>;
template <typename T>
using small_vector = SmallVector<T, 6>;

using param_vector = small_vector<param_index>;
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

using thread_pool = threads::WorkStealingThreadPool<32>;
using task_group = thread_pool::TaskGroup;

using spin_lock = threads::SpinLock;

template <class T_DERIVED>
using functor_task_base = thread_pool::FunctorTaskBase<T_DERIVED>;

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_COMPILE_TIME_SELECTED_DATATYPES_HPP_ */
