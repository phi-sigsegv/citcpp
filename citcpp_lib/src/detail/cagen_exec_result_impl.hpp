#ifndef CAGEN_EXEC_RESULT_IMPL_HPP_
#define CAGEN_EXEC_RESULT_IMPL_HPP_

#include <citcpp/cagen_exec_handle.hpp>
#include <utility>

namespace citcpp {

class cagen_exec_result_impl : public cagen_exec_result {
  public:
    void set_result(const test_set& tests) { test_set_ = tests; }

    void set_result(test_set&& tests) { test_set_ = std::move(tests); }

    void set_result_code(cagen_result_code result_code) {
      result_code_ = result_code;
    }

    void set_error_message(std::string_view error_message) {
      error_message_ = error_message;
    }
};

}  // namespace citcpp

#endif /* CAGEN_EXEC_RESULT_IMPL_HPP_ */
