#ifndef DETAIL_CITCPP_IPOG_BASE_HPP_
#define DETAIL_CITCPP_IPOG_BASE_HPP_

namespace citcpp {
namespace detail {

// Forward declaration of cagen_exec_handle_ipog_impl due to usage of
// citcpp_ipog_base by cagen_exec_handle_ipog_impl definition.
class cagen_exec_handle_ipog_impl;

/**
 * This class provides an implementation of the IPOG algorithm.
 */
class citcpp_ipog_base {
  public:
    virtual ~citcpp_ipog_base() {}

    /**
     * This is the entry point to be called by a thread.
     */
    virtual void entry_point(cagen_exec_handle_ipog_impl &exec_handle) = 0;
};

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_CITCPP_IPOG_BASE_HPP_ */