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

using param_vector = SmallVector<param_index, 6>;
using value_vector = SmallVector<int, 6>;

template <class T>
// using thread_local_vector = std::vector<T>;
using thread_local_vector = SmallVector<T, 32>;

class alignas(false_sharing_avoidance_alignment) aligned_param_vector {

  public:
    aligned_param_vector() : value() {}

    explicit aligned_param_vector(size_t Size, const param_index& Value = 0)
        : value(Size, Value) {}

    /**
     @brief constructs a vector with the contents of the range
     <tt>[S, E)</tt>
     */
    template <typename ItTy>
    aligned_param_vector(ItTy S, ItTy E) : value(S, E) {}

    aligned_param_vector(std::initializer_list<param_index> IL) : value(IL) {}

    aligned_param_vector(const aligned_param_vector& RHS) : value(RHS.value) {}

    aligned_param_vector(aligned_param_vector&& RHS)
        : value(std::move(RHS.value)) {}

    aligned_param_vector(const param_vector& RHS) : value(RHS) {}

    aligned_param_vector(param_vector&& RHS) : value(std::move(RHS)) {}

    param_vector value;
};

struct alignas(false_sharing_avoidance_alignment) aligned_ull_value {
    unsigned long long value;
};

template <typename T>
class alignas(false_sharing_avoidance_alignment) aligned_vector {
  public:
    aligned_vector() : value() {}

    explicit aligned_vector(size_t Size, const T& Value = T())
        : value(Size, Value) {}

    /**
     @brief constructs a vector with the contents of the range
     <tt>[S, E)</tt>
     */
    template <typename ItTy>
    aligned_vector(ItTy S, ItTy E) : value(S, E) {}

    aligned_vector(std::initializer_list<param_index> IL) : value(IL) {}

    aligned_vector(const aligned_vector& RHS) : value(RHS.value) {}

    aligned_vector(aligned_vector&& RHS) : value(std::move(RHS.value)) {}

    aligned_vector(const std::vector<T>& RHS) : value(RHS) {}

    aligned_vector(std::vector<T>&& RHS) : value(std::move(RHS)) {}

    aligned_vector& operator=(const aligned_vector& RHS) {
      if (&RHS != this) {
        value = RHS.value;
      }

      return *this;
    }

    aligned_vector& operator=(const std::vector<T>& RHS) {
      value = RHS;

      return *this;
    }

    aligned_vector& operator=(aligned_vector&& RHS) {
      if (&RHS != this) {
        value = std::move(RHS.value);
      }

      return *this;
    }

    aligned_vector& operator=(std::vector<T>&& RHS) {
      value = std::move(RHS);

      return *this;
    }

    std::vector<T> value;
};

using thread_pool = threads::WorkStealingThreadPool<32>;
using task_group = thread_pool::TaskGroup;

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_COMPILE_TIME_SELECTED_DATATYPES_HPP_ */
