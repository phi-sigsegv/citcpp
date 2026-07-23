#ifndef COVM_EXEC_RESULT_IMPL_HPP_
#define COVM_EXEC_RESULT_IMPL_HPP_

#include <citcpp/covm_exec_handle.hpp>
#include <utility>

namespace citcpp {

class covm_exec_result_impl : public covm_exec_result {
  public:
    void set_result(
        std::unordered_map<std::string, coverage_measurement> result) {
      result_ = std::move(result);
    }

    void set_invalid_test_indices(
        std::vector<std::size_t> invalid_test_indices) {
      invalid_test_indices_ = std::move(invalid_test_indices);
    }

    void set_result_code(covm_result_code result_code) {
      result_code_ = result_code;
    }

    void set_error_message(std::string_view error_message) {
      error_message_ = error_message;
    }
};

}  // namespace citcpp

#endif /* COVM_EXEC_RESULT_IMPL_HPP_ */
