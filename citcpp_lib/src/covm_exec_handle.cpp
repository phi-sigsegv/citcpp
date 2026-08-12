#include <citcpp/covm_exec_handle.hpp>

namespace citcpp {

const std::unordered_map<std::string, coverage_measurement>&
covm_exec_result::get_result() const {

  return result_;
}

void covm_exec_result::set_result(
    std::unordered_map<std::string, coverage_measurement> result) {

  result_ = std::move(result);
}

const std::vector<std::size_t>& covm_exec_result::get_invalid_test_indices()
    const {

  return invalid_test_indices_;
}

void covm_exec_result::set_invalid_test_indices(
    std::vector<std::size_t> invalid_test_indices) {

  invalid_test_indices_ = std::move(invalid_test_indices);
}

covm_exec_result::covm_result_code covm_exec_result::get_result_code() const {
  return result_code_;
}

void covm_exec_result::set_result_code(covm_result_code result_code) {
  result_code_ = result_code;
}

std::string_view covm_exec_result::get_error_message() const {
  return error_message_;
}

void covm_exec_result::set_error_message(std::string_view error_message) {
  error_message_ = error_message;
}

}  // namespace citcpp
