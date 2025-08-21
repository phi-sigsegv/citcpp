#include "citcpp_ipog.hpp"

#include <algorithm>
#include <chrono>
#include <numeric>
#include <thread>

#include "binom_coeff_table.hpp"
#include "cagen_exec_handle_ipog_impl.hpp"
#include "cagen_exec_result_impl.hpp"
#include "citcpp_algo_common.hpp"
#include "coverage_map.hpp"
#include "datatypes_config.hpp"
#include "for_each_cross_product_elem.hpp"
#include "ipog_algorithm_uniform_strength.hpp"

namespace {

std::vector<unsigned int>
get_parameter_indices_ordered_by_number_of_values_desc(
    const std::vector<unsigned int> &params) {

  std::vector<unsigned int> parameter_index_map(params.size());
  std::iota(parameter_index_map.begin(), parameter_index_map.end(), 0);

  std::sort(parameter_index_map.begin(), parameter_index_map.end(),
            [&params](const unsigned int &index1, const unsigned int &index2) {
              return params[index1] > params[index2];
            });

  return parameter_index_map;
}

void main_ipog_loop(const citcpp::detail::model &model, unsigned int strength,
                    citcpp::detail::test_set &test_set,
                    const citcpp::covering_array_computation_config config,
                    citcpp::detail::cagen_exec_handle_ipog_impl &exec_handle) {
  using namespace citcpp::detail;

  unsigned int num_threads = std::thread::hardware_concurrency();
  if (num_threads == 0) {
    num_threads = 4;
  }

  thread_pool tp(num_threads);

  const bool with_mt = config.multithreading_enabled();

  // First we compute the number of combination we have to cover.
  unsigned long long number_combos_to_cover =
      with_mt ? number_of_combinations_to_cover(tp, model, strength)
              : number_of_combinations_to_cover(model, strength);
  tp.stop_workers();
  exec_handle.set_number_of_combinations_to_cover(number_combos_to_cover);

  if (exec_handle.is_job_aborted()) {
    return;
  }

  std::vector<unsigned int> parameter_index_map(
      get_parameter_indices_ordered_by_number_of_values_desc(
          model.get_parameters()));

  {
    // Step 1: Initialize for the first t parameters.
    auto initial_step_res = create_all_value_combinations(
        strength, model, parameter_index_map, test_set);
    exec_handle.add_number_of_covered_combinations(
        initial_step_res.num_created_combinations);
    exec_handle.set_testset_size_(test_set.get_list_of_tests().size());
    exec_handle.set_number_of_processed_parameters(strength);
  }

  {
    // Here is the main IPOG loop.
    const binom_coeff_table binomial_coeffs(parameter_index_map.size());

    for (unsigned int current_param_idx = strength;
         current_param_idx < parameter_index_map.size(); ++current_param_idx) {
      if (exec_handle.is_job_aborted()) {
        return;
      }

      if (model.get_parameters()[parameter_index_map[current_param_idx]] <= 1) {
        // If the current parameter only has only value, then
        // we can treat this situation much simpler: We just have
        // to add that particular value to each test and update
        // the number of covered combinations.
        for (test &t : test_set.get_list_of_tests()) {
          t.get_values()[parameter_index_map[current_param_idx]] = 0;
        }

        unsigned long long number_combos_to_cover =
            with_mt ? number_of_combinations_to_cover(
                          tp, current_param_idx, model, parameter_index_map,
                          strength - 1)
                    : number_of_combinations_to_cover(current_param_idx, model,
                                                      parameter_index_map,
                                                      strength - 1);
        tp.stop_workers();
        exec_handle.add_number_of_covered_combinations(number_combos_to_cover);
      } else {
        coverage_map cov_map(current_param_idx + 1, strength, model,
                             parameter_index_map, binomial_coeffs, true);

        unsigned long long number_combos_to_cover =
            cov_map.get_total_number_of_tuples();

        auto horizontal_ext_res =
            with_mt
                ? ipog_horizontal_extension(
                      current_param_idx, strength, model, parameter_index_map,
                      number_combos_to_cover, test_set, cov_map, tp)
                : ipog_horizontal_extension(
                      current_param_idx, strength, model, parameter_index_map,
                      number_combos_to_cover, test_set, cov_map);
        tp.stop_workers();

        number_combos_to_cover -= horizontal_ext_res.num_new_covered_tuples;
        exec_handle.add_number_of_covered_combinations(
            horizontal_ext_res.num_new_covered_tuples);
        exec_handle.set_testset_size_(test_set.get_list_of_tests().size());

        if (exec_handle.is_job_aborted()) {
          exec_handle.set_number_of_processed_parameters(current_param_idx + 1);
          return;
        }

        if (number_combos_to_cover > 0) {
          auto vertical_ext_res = ipog_vertical_extension(
              current_param_idx, model, number_combos_to_cover,
              horizontal_ext_res, test_set, cov_map);

          number_combos_to_cover -= vertical_ext_res.num_new_covered_tuples;
          exec_handle.add_number_of_covered_combinations(
              vertical_ext_res.num_new_covered_tuples);
          exec_handle.set_testset_size_(test_set.get_list_of_tests().size());
        }
      }

      exec_handle.set_number_of_processed_parameters(current_param_idx + 1);
    }
  }
}

void replace_dont_care_values(citcpp::detail::test_set &test_set,
                              const citcpp::detail::model &model) {
  using namespace citcpp::detail;

  for (test &t : test_set.get_list_of_tests()) {
    for (unsigned int i = 0; i < t.get_values().size(); ++i) {
      int &value = t.get_values()[i];
      if (value < 0) {
        // Found don't care value. We simply replace it with the
        // first value of the respective parameter.
        value = 0;
      }
    }
  }
}

}  // namespace

namespace citcpp {
namespace detail {

citcpp_ipog::citcpp_ipog(const input_model &input_model,
                         const covering_array_computation_config &config)
    : config_(config),
      input_model_(input_model),
      model_(input_model_),
      strength_(1) {}

citcpp_ipog::citcpp_ipog(input_model &&input_model,
                         const covering_array_computation_config &config)
    : config_(config),
      input_model_(std::move(input_model)),
      model_(input_model_),
      strength_(1) {}

void citcpp_ipog::set_interaction_strength(unsigned int t) { strength_ = t; }

void citcpp_ipog::entry_point(cagen_exec_handle_ipog_impl &exec_handle) {
  const auto t_start = std::chrono::high_resolution_clock::now();

  citcpp::detail::test_set tests;
  main_ipog_loop(model_, strength_, tests, config_, exec_handle);
  if (config_.replace_dont_care_values()) {
    replace_dont_care_values(tests, model_);
  }
  ::citcpp::test_set ts(
      model_.create_from_internal_test_set(tests, config_.value_separator()));

  const auto t_end = std::chrono::high_resolution_clock::now();
  const auto duration_in_milli_seconds =
      duration_cast<std::chrono::milliseconds>(t_end - t_start);
  exec_handle.set_duration_in_milli_seconds(duration_in_milli_seconds.count());

  // Set the generated test set.
  // This will also signal to the client that we are done.
  cagen_exec_result_impl result;
  if (exec_handle.is_job_aborted()) {
    result.set_result_code(cagen_exec_result::cagen_result_code::
                               COVERING_ARRAY_GENERATION_ABORTED);
  } else {
    result.set_result_code(cagen_exec_result::cagen_result_code::
                               COVERING_ARRAY_GENERATION_COMPLETED);
  }
  result.set_error_message("");
  result.set_result(std::move(ts));
  exec_handle.set_test_set(std::move(result));
}

}  // namespace detail
}  // namespace citcpp
