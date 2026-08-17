#include <citcpp/cagen_exec_handle.hpp>

namespace citcpp {

const test_set& cagen_exec_result::get_result() const { return test_set_; }

void cagen_exec_result::set_result(const test_set& tests) { test_set_ = tests; }

void cagen_exec_result::set_result(test_set&& tests) {
  test_set_ = std::move(tests);
}

cagen_exec_result::cagen_result_code cagen_exec_result::get_result_code()
    const {

  return result_code_;
}

void cagen_exec_result::set_result_code(
    cagen_exec_result::cagen_result_code result_code) {

  result_code_ = result_code;
}

std::string_view cagen_exec_result::get_error_message() const {
  return error_message_;
}

void cagen_exec_result::set_error_message(std::string_view error_message) {
  error_message_ = error_message;
}

}  // namespace citcpp
