#ifndef SHARED_AUX_TYPES_HPP_
#define SHARED_AUX_TYPES_HPP_

#include "bitset.hpp"
#include "datatypes_config.hpp"

namespace citcpp {
namespace detail {

struct alignas(false_sharing_avoidance_alignment) aligned_array_wrapper {
    array_wrapper_uint64 value;
};

}  // namespace detail
}  // namespace citcpp

#endif /* SHARED_AUX_TYPES_HPP_ */
