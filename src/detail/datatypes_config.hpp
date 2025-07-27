/*
 * compile_time_selected_datatypes.hpp
 *
 *  Created on: Jul 2, 2025
 *      Author: philipp
 */
#include <vector>
#include "small_vector.hpp"
#include "threading_lib.hpp"

#ifndef DETAIL_COMPILE_TIME_SELECTED_DATATYPES_HPP_
#define DETAIL_COMPILE_TIME_SELECTED_DATATYPES_HPP_

namespace citcpp
{
  namespace detail
  {
    template<class T>
      //  using strength_vector = std::vector<T>;
      using strength_vector = SmallVector<T, 6>;

    using thread_pool = threads::WorkStealingThreadPool<32, 64>;
    using task_group = thread_pool::TaskGroup;
  }
}

#endif /* DETAIL_COMPILE_TIME_SELECTED_DATATYPES_HPP_ */
