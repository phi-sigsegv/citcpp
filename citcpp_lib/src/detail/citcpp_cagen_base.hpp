#ifndef DETAIL_CITCPP_CAGEN_BASE_HPP_
#define DETAIL_CITCPP_CAGEN_BASE_HPP_

#include <vector>

#include "internal_model.hpp"

namespace citcpp {
namespace detail {

/**
 * This is a common base class for covering array generation algorithms.
 */
class citcpp_cagen_base {
  public:
    virtual ~citcpp_cagen_base() = default;
};

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_CITCPP_IPOG_BASE_HPP_ */
