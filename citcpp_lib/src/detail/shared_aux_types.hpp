#ifndef SHARED_AUX_TYPES_HPP_
#define SHARED_AUX_TYPES_HPP_

#include "bitset.hpp"
#include "datatypes_config.hpp"

namespace citcpp {
namespace detail {

struct aligned_array_wrapper : cache_aligned<array_wrapper_uint64> {};

}  // namespace detail
}  // namespace citcpp

#endif /* SHARED_AUX_TYPES_HPP_ */
