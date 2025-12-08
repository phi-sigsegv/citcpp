#ifndef DURATION_WRAPPER_HPP_
#define DURATION_WRAPPER_HPP_

#include <chrono>
#include <iostream>

namespace citcpp {

/**
 * This is just a thin wrapper around an instance of a std::chrono::duration
 * object, which can be used in order to print the duration to an output stream
 * in terms of seconds truncated to 3 decimal points. This should be sufficient
 * for the purpose of logging time durations.
 */
class duration_wrapper {
  public:
    template <class Rep, class Period>
    duration_wrapper(const std::chrono::duration<Rep, Period> d)
        : duration_in_sec_(d) {}

    friend std::ostream& operator<<(std::ostream& os,
                                    const duration_wrapper& d) {

      // Truncate at 3 decimal points.
      double secs_truncated =
          ((unsigned int)(d.duration_in_sec_.count() * 1000.0)) / 1000.0;
      os << secs_truncated << "s";

      return os;
    }

  private:
    const std::chrono::duration<double> duration_in_sec_;
};

}  // namespace citcpp

#endif /* DURATION_WRAPPER_HPP_ */
