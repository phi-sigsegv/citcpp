#include <cstdint>
#include <vector>

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
using param_vector = SmallVector<param_index, 6>;
using value_vector = SmallVector<int, 6>;

template <class T>
// using thread_local_vector = std::vector<T>;
using thread_local_vector = SmallVector<T, 32>;

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

template <class T_DERIVED>
using functor_task_base = thread_pool::FunctorTaskBase<T_DERIVED>;

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_COMPILE_TIME_SELECTED_DATATYPES_HPP_ */
