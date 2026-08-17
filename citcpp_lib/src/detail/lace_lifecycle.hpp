#ifndef DETAIL_LACE_LIFECYCLE_HPP_
#define DETAIL_LACE_LIFECYCLE_HPP_

#include <cstddef>

namespace citcpp {
namespace detail {

/**
 * Start Lace with <n_workers> workers and a a task deque size of <dqsize>
 * per worker. If <n_workers> is set to 0, automatically detects available
 * cores. If <dqsize> is est to 0, uses a reasonable default value.
 *
 * This method keep track of a global state regarding the number of times
 * that this method has been called. If lace has already been started,
 * then calling this method just updates the internal usage counter.
 * This method can be called concurrently, and also release_lace and
 * this method can be called concurrently.
 */
void lace_init(unsigned int n_workers, std::size_t dqsize);

/**
 * This method shall be called whenever a client that has called init_lace
 * before is now done with using lace.
 * The last one closes the door and shuts down the lace framework.
 * This method can be called concurrently, and also init_lace and
 * this method can be called concurrently.
 */
void lace_quit();

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_LACE_LIFECYCLE_HPP_ */
