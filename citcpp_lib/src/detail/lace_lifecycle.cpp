#include "lace_lifecycle.hpp"

#include <lace.h>

#include <mutex>

namespace {

std::mutex& get_global_lace_init_mutex() {
  static std::mutex mut;
  return mut;
}

int& get_lobal_lace_init_counter() {
  static int count = 0;
  return count;
}

}  // namespace

namespace citcpp {
namespace detail {

void lace_init(unsigned int n_workers, std::size_t dqsize) {
  std::lock_guard<std::mutex> lock(get_global_lace_init_mutex());

  int& instance_cnt = get_lobal_lace_init_counter();
  if (instance_cnt == 0) {
    lace_start(n_workers, dqsize);
  }

  instance_cnt++;
}

void lace_quit() {
  std::lock_guard<std::mutex> lock(get_global_lace_init_mutex());

  int& instance_cnt = get_lobal_lace_init_counter();
  instance_cnt--;
  if (instance_cnt == 0) {
    lace_stop();
  }
}

}  // namespace detail
}  // namespace citcpp
