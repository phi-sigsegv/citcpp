/*
 * compile_time_selected_datatypes.hpp
 *
 *  Created on: Jul 2, 2025
 *      Author: philipp
 */
#include <cstdint>
#include <vector>

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

using thread_pool = threads::WorkStealingThreadPool<32>;
using task_group = thread_pool::TaskGroup;

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_COMPILE_TIME_SELECTED_DATATYPES_HPP_ */
