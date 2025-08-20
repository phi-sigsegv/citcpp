#include "citcpp_covm.hpp"

#include <algorithm>
#include <chrono>
#include <citcpp/coverage_measurement.hpp>
#include <numeric>
#include <thread>
#include <unordered_map>

#include "binom_coeff_table.hpp"
#include "citcpp_utils.hpp"
#include "coverage_map.hpp"
#include "covm_algorithm_uniform_strength.hpp"
#include "covm_exec_handle_impl.hpp"
#include "datatypes_config.hpp"

namespace {

citcpp::detail::test_set create_internal_test_set(
    const citcpp::input_model input_model, const citcpp::test_set &tests) {
  using namespace citcpp::detail;

  struct ParamValueHash {
      std::size_t operator()(const citcpp::parameter_value &v) const {
        std::size_t h = std::hash<std::variant<bool, std::string, int>>{}(
            v.get_variant_value());
        return h;
      }
  };

  std::vector<std::unordered_map<citcpp::parameter_value, int, ParamValueHash>>
      all_param_value_mappings(
          input_model.get_parameters().size(),
          std::unordered_map<citcpp::parameter_value, int, ParamValueHash>());

  {
    int model_param_index = 0;
    for (const citcpp::parameter &param : input_model.get_parameters()) {
      auto &param_value_mappings = all_param_value_mappings[model_param_index];

      int param_value_index = 0;
      for (const citcpp::parameter_value &param_value : param.get_values()) {
        param_value_mappings[param_value] = param_value_index;

        ++param_value_index;
      }

      ++model_param_index;
    }
  }

  std::vector<int> param_mapping(tests.get_parameters().size(), -1);

  {
    int test_param_index = 0;
    for (const citcpp::parameter_def &param_def : tests.get_parameters()) {
      int model_param_index = 0;
      for (const citcpp::parameter &param : input_model.get_parameters()) {
        if (param.get_name() == param_def.get_name() &&
            param.get_type() == param_def.get_type()) {

          param_mapping[test_param_index] = model_param_index;
          break;
        }

        ++model_param_index;
      }

      ++test_param_index;
    }
  }

  citcpp::detail::test_set internal_test_set;

  {
    for (const std::vector<citcpp::parameter_value> &values :
         tests.get_list_of_tests()) {

      citcpp::detail::test internal_test(all_param_value_mappings.size(), -2);
      int test_param_index = 0;
      for (const citcpp::parameter_value &param_value : values) {
        if (param_mapping[test_param_index] >= 0) {
          int model_param_index = param_mapping[test_param_index];
          const auto &param_value_mappings =
              all_param_value_mappings[model_param_index];

          auto param_value_idx_it = param_value_mappings.find(param_value);
          if (param_value_idx_it != param_value_mappings.end()) {
            internal_test.get_values()[model_param_index] =
                param_value_idx_it->second;
          } else if (param_value == DONT_CARE_PARAMETER_VALUE) {
            // Cannot find value. If it is a don't care, then set its index to
            // -1.
            internal_test.get_values()[model_param_index] = -1;
          }
        }

        ++test_param_index;
      }

      internal_test_set.get_list_of_tests().push_back(std::move(internal_test));
    }
  }

  return internal_test_set;
}

void main_covm_loop(const citcpp::detail::model &model,
                    const citcpp::detail::test_set &test_set,
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

citcpp_covm::citcpp_covm(const input_model &input_model,
                         const citcpp::test_set &tests,
                         const coverage_measurement_config &config)
    : config_(config),
      input_model_(input_model),
      model_(input_model_),
      input_tests_(tests),
      tests_(create_internal_test_set(input_model_, input_tests_)),
      strength_(1) {}

citcpp_covm::citcpp_covm(input_model &&input_model, citcpp::test_set &&tests,
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
  exec_handle.set_coverage_measurement(std::move(covm));
}

}  // namespace detail
}  // namespace citcpp
