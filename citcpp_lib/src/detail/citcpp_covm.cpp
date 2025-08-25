#include "citcpp_covm.hpp"

#include <algorithm>
#include <chrono>
#include <citcpp/coverage_measurement.hpp>
#include <numeric>
#include <thread>

#include "binom_coeff_table.hpp"
#include "citcpp_utils.hpp"
#include "coverage_map.hpp"
#include "covm_algorithm_uniform_strength.hpp"
#include "covm_exec_handle_impl.hpp"
#include "covm_exec_result_impl.hpp"
#include "datatypes_config.hpp"

namespace {

void main_covm_loop(const citcpp::detail::model &model,
                    const citcpp::detail::internal_test_set &test_set,
                    const citcpp::coverage_measurement_config &config,
                    unsigned int strength, citcpp::coverage_measurement &covm,
                    citcpp::detail::covm_exec_handle_impl &exec_handle) {
  using namespace citcpp::detail;

  unsigned int num_threads = std::thread::hardware_concurrency();
  if (num_threads == 0) {
    num_threads = 4;
  }

  thread_pool tp(num_threads);

  const binom_coeff_table binomial_coeffs(model.get_parameters().size());
  std::vector<unsigned int> parameter_index_map(model.get_parameters().size());
  std::iota(parameter_index_map.begin(), parameter_index_map.end(), 0);

  coverage_map cov_map(model.get_parameters().size(), strength, model,
                       parameter_index_map, binomial_coeffs, false);

  unsigned long long number_combos_to_cover =
      cov_map.get_total_number_of_tuples();
  covm.set_number_of_param_combos_to_cover(cov_map.get_coverage_map().size());
  covm.set_number_of_combinations_to_cover(number_combos_to_cover);
  exec_handle.set_number_of_combinations_to_cover(number_combos_to_cover);

  if (exec_handle.is_job_aborted()) {
    return;
  }

  const bool with_mt = config.multithreading_enabled();

  if (with_mt) {
    measure_coverage(model, test_set, cov_map, exec_handle, covm, tp);
  } else {
    measure_coverage(model, test_set, cov_map, exec_handle, covm);
  }

  tp.stop_workers();
}

}  // namespace

namespace citcpp {
namespace detail {

citcpp_covm::citcpp_covm(const input_model &input_model, const test_set &tests,
                         const coverage_measurement_config &config)
    : config_(config),
      input_model_(input_model),
      model_(input_model_),
      input_tests_(tests),
      tests_(create_internal_test_set(input_model_, input_tests_)),
      strength_(1) {}

citcpp_covm::citcpp_covm(input_model &&input_model, test_set &&tests,
                         const coverage_measurement_config &config)
    : config_(config),
      input_model_(std::move(input_model)),
      model_(input_model_),
      input_tests_(std::move(tests)),
      tests_(create_internal_test_set(input_model_, input_tests_)),
      strength_(1) {}

void citcpp_covm::set_interaction_strength(unsigned int t) { strength_ = t; }

void citcpp_covm::entry_point(covm_exec_handle_impl &exec_handle) {
  const auto t_start = std::chrono::high_resolution_clock::now();

  citcpp::coverage_measurement covm;
  main_covm_loop(model_, tests_, config_, strength_, covm, exec_handle);

  const auto t_end = std::chrono::high_resolution_clock::now();
  const auto duration_in_milli_seconds =
      duration_cast<std::chrono::milliseconds>(t_end - t_start);
  exec_handle.set_duration_in_milli_seconds(duration_in_milli_seconds.count());

  // Set the generated result object.
  // This will also signal to the client that we are done.
  covm_exec_result_impl result;
  if (exec_handle.is_job_aborted()) {
    result.set_result_code(
        covm_exec_result::covm_result_code::COVERAGE_MEASUREMENT_ABORTED);
  } else {
    result.set_result_code(
        covm_exec_result::covm_result_code::COVERAGE_MEASUREMENT_COMPLETED);
  }
  result.set_error_message("");
  result.set_result(std::move(covm));
  exec_handle.set_coverage_measurement(std::move(result));
}

}  // namespace detail
}  // namespace citcpp
