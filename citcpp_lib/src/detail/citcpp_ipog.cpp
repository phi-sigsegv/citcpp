#include "citcpp_ipog.hpp"

#include <algorithm>
#include <chrono>
#include <numeric>
#include <thread>
#include <utility>

#include "binom_coeff_table.hpp"
#include "cagen_exec_handle_ipog_impl.hpp"
#include "cagen_exec_result_impl.hpp"
#include "citcpp_algo_common.hpp"
#include "citcpp_utils.hpp"
#include "coverage_map.hpp"
#include "datatypes_config.hpp"
#include "ipog_all_value_combinations.hpp"
#include "ipog_horizontal_extension.hpp"
#include "ipog_measure_testset.hpp"
#include "ipog_vertical_extension.hpp"

namespace {

void main_ipog_loop_body(
    const citcpp::detail::internal_model& model,
    std::vector<citcpp::detail::internal_relation>& relations,
    citcpp::detail::internal_test_set& test_set, bool is_extend_mode,
    unsigned int real_current_param_idx, const bool with_mt,
    const citcpp::detail::binom_coeff_table& binomial_coeffs,
    citcpp::detail::thread_pool& tp,
    citcpp::detail::cagen_exec_handle_ipog_impl& exec_handle) {
  using namespace citcpp::detail;

  if (model.get_parameter_num_values()[real_current_param_idx] <= 1) {
    // If the current parameter only has only value, then
    // we can treat this situation much simpler: We just have
    // to add that particular value to each test and update
    // the number of covered combinations.
    for (test& t : test_set.get_list_of_tests()) {
      t.get_values()[real_current_param_idx] = 0;
    }

    unsigned long long number_combos_to_cover = 0;
    for (const auto& relation : relations) {
      if (relation
              .get_parameter_index_map()[relation.get_current_param_idx()] ==
          real_current_param_idx) {

        number_combos_to_cover +=
            with_mt ? number_of_combinations_to_cover(
                          relation.get_current_param_idx() + 1, model,
                          relation.get_parameter_index_map(),
                          relation.get_current_interaction_strength(), true, tp)
                    : number_of_combinations_to_cover(
                          relation.get_current_param_idx() + 1, model,
                          relation.get_parameter_index_map(),
                          relation.get_current_interaction_strength(), true);
      }
    }

    tp.stop_workers();
    exec_handle.add_number_of_covered_combinations(number_combos_to_cover);
  } else {
    std::vector<std::pair<internal_relation, coverage_map>> relation_cov_maps;
    unsigned long long number_combos_to_cover = 0;
    for (const auto& relation : relations) {
      if (relation
              .get_parameter_index_map()[relation.get_current_param_idx()] ==
          real_current_param_idx) {

        relation_cov_maps.push_back(std::make_pair(
            relation, coverage_map(relation.get_current_param_idx() + 1,
                                   relation.get_current_interaction_strength(),
                                   model, relation.get_parameter_index_map(),
                                   binomial_coeffs, true)));
        number_combos_to_cover +=
            relation_cov_maps.back().second.get_total_number_of_tuples();
      }
    }

    if (is_extend_mode) {
      auto measure_coverage_res =
          with_mt ? ipog_measure_testset(model, test_set, relation_cov_maps, tp)
                  : ipog_measure_testset(model, test_set, relation_cov_maps);

      number_combos_to_cover -= measure_coverage_res.num_covered_tuples;
    }

    auto horizontal_ext_res =
        with_mt ? ipog_horizontal_extension(number_combos_to_cover, test_set,
                                            relation_cov_maps, tp)
                : ipog_horizontal_extension(number_combos_to_cover, test_set,
                                            relation_cov_maps);
    tp.stop_workers();

    number_combos_to_cover -= horizontal_ext_res.num_new_covered_tuples;
    exec_handle.add_number_of_covered_combinations(
        horizontal_ext_res.num_new_covered_tuples);
    exec_handle.set_testset_size(test_set.get_list_of_tests().size());

    if (exec_handle.is_job_aborted()) {
      return;
    }

    if (number_combos_to_cover > 0) {
      auto vertical_ext_res =
          ipog_vertical_extension(number_combos_to_cover, horizontal_ext_res,
                                  test_set, relation_cov_maps);

      number_combos_to_cover -= vertical_ext_res.num_new_covered_tuples;
      exec_handle.add_number_of_covered_combinations(
          vertical_ext_res.num_new_covered_tuples);
      exec_handle.set_testset_size(test_set.get_list_of_tests().size());
    }
  }
}

void main_ipog_loop(const citcpp::detail::internal_model& model,
                    std::vector<citcpp::detail::internal_relation>& relations,
                    citcpp::detail::internal_test_set& test_set,
                    const citcpp::covering_array_computation_config config,
                    citcpp::detail::cagen_exec_handle_ipog_impl& exec_handle) {
  using namespace citcpp::detail;

  unsigned int num_threads = std::thread::hardware_concurrency();
  if (num_threads == 0) {
    num_threads = 4;
  }

  thread_pool tp(num_threads);

  const bool with_mt = config.multithreading_enabled();

  // First we compute the number of combination we have to cover.
  unsigned int minimum_required_strength =
      model.get_parameter_num_values().size();
  unsigned long long number_combos_to_cover = 0;
  for (const auto& relation : relations) {
    number_combos_to_cover +=
        with_mt ? number_of_combinations_to_cover(
                      relation.get_parameter_index_map().size(), model,
                      relation.get_parameter_index_map(),
                      relation.get_specified_interaction_strength(), false, tp)
                : number_of_combinations_to_cover(
                      relation.get_parameter_index_map().size(), model,
                      relation.get_parameter_index_map(),
                      relation.get_specified_interaction_strength(), false);
    minimum_required_strength =
        std::min(minimum_required_strength,
                 relation.get_specified_interaction_strength());
  }

  tp.stop_workers();
  exec_handle.set_number_of_combinations_to_cover(number_combos_to_cover);

  if (exec_handle.is_job_aborted()) {
    return;
  }

  std::vector<unsigned int> parameter_index_map(
      citcpp_ipog_base::create_parameter_index_map(relations, model));

  unsigned int first_param_idx = 0;
  for (first_param_idx = 0; first_param_idx < minimum_required_strength;
       ++first_param_idx) {

    const unsigned int real_current_param_idx =
        parameter_index_map[first_param_idx];

    for (auto& relation : relations) {
      if (relation
              .get_parameter_index_map()[relation.get_current_param_idx()] !=
          real_current_param_idx) {

        // The current parameter is not contained in at least one of the
        // relations. Thus, it shall not be part of the initialization of the
        // testset.
        break;
      }
    }

    // If we reach this point, then the current parameter is part of all
    // relations. Therefore it will be part of the initialization of the
    // testset, and we have to increment the parameter indices of the relations.
    for (auto& relation : relations) {
      relation.set_current_param_idx(relation.get_current_param_idx() + 1);
    }
  }

  {
    // Step 1: Initialize for the first t parameters.
    auto initial_step_res = create_all_value_combinations(
        first_param_idx, model, parameter_index_map, test_set);
    exec_handle.add_number_of_covered_combinations(
        initial_step_res.num_created_combinations);
    exec_handle.set_testset_size(test_set.get_list_of_tests().size());
    exec_handle.set_number_of_processed_parameters(first_param_idx);
  }

  // Here is the main IPOG loop.
  const binom_coeff_table binomial_coeffs(parameter_index_map.size());

  for (unsigned int current_param_idx = first_param_idx;
       current_param_idx < parameter_index_map.size(); ++current_param_idx) {

    if (exec_handle.is_job_aborted()) {
      return;
    }

    const unsigned int real_current_param_idx =
        parameter_index_map[current_param_idx];

    main_ipog_loop_body(model, relations, test_set, false,
                        real_current_param_idx, with_mt, binomial_coeffs, tp,
                        exec_handle);

    exec_handle.set_number_of_processed_parameters(current_param_idx + 1);

    for (auto& relation : relations) {
      if (relation
              .get_parameter_index_map()[relation.get_current_param_idx()] ==
          real_current_param_idx) {

        relation.set_current_param_idx(relation.get_current_param_idx() + 1);
      }
    }
  }
}

void main_ipog_loop_extend_test_set(
    const citcpp::detail::internal_model& model,
    std::vector<citcpp::detail::internal_relation>& relations,
    citcpp::detail::internal_test_set& test_set,
    const citcpp::covering_array_computation_config config,
    citcpp::detail::cagen_exec_handle_ipog_impl& exec_handle) {
  using namespace citcpp::detail;

  unsigned int num_threads = std::thread::hardware_concurrency();
  if (num_threads == 0) {
    num_threads = 4;
  }

  thread_pool tp(num_threads);

  const bool with_mt = config.multithreading_enabled();

  // First we compute the number of combination we have to cover.
  unsigned long long number_combos_to_cover = 0;
  for (const auto& relation : relations) {
    number_combos_to_cover +=
        with_mt ? number_of_combinations_to_cover(
                      relation.get_parameter_index_map().size(), model,
                      relation.get_parameter_index_map(),
                      relation.get_specified_interaction_strength(), false, tp)
                : number_of_combinations_to_cover(
                      relation.get_parameter_index_map().size(), model,
                      relation.get_parameter_index_map(),
                      relation.get_specified_interaction_strength(), false);
  }

  tp.stop_workers();
  exec_handle.set_number_of_combinations_to_cover(number_combos_to_cover);

  if (exec_handle.is_job_aborted()) {
    return;
  }

  std::vector<unsigned int> parameter_index_map(
      citcpp_ipog_base::create_parameter_index_map(relations, model));

  // Here is the main IPOG loop.
  const binom_coeff_table binomial_coeffs(parameter_index_map.size());

  for (unsigned int current_param_idx = 0;
       current_param_idx < parameter_index_map.size(); ++current_param_idx) {

    if (exec_handle.is_job_aborted()) {
      return;
    }

    const unsigned int real_current_param_idx =
        parameter_index_map[current_param_idx];

    main_ipog_loop_body(model, relations, test_set, true,
                        real_current_param_idx, with_mt, binomial_coeffs, tp,
                        exec_handle);

    exec_handle.set_number_of_processed_parameters(current_param_idx + 1);

    for (auto& relation : relations) {
      if (relation
              .get_parameter_index_map()[relation.get_current_param_idx()] ==
          real_current_param_idx) {

        relation.set_current_param_idx(relation.get_current_param_idx() + 1);
      }
    }
  }
}

}  // namespace

namespace citcpp {
namespace detail {

citcpp_ipog::citcpp_ipog(const model& input_model,
                         const covering_array_computation_config& config)
    : citcpp_ipog_base(),
      config_(config),
      input_model_(input_model),
      model_(input_model_),
      input_tests_(),
      strength_(1) {}

citcpp_ipog::citcpp_ipog(model&& input_model,
                         const covering_array_computation_config& config)
    : citcpp_ipog_base(),
      config_(config),
      input_model_(std::move(input_model)),
      model_(input_model_),
      input_tests_(),
      strength_(1) {}

citcpp_ipog::citcpp_ipog(const model& input_model,
                         const citcpp::test_set& tests,
                         const covering_array_computation_config& config)
    : citcpp_ipog_base(),
      config_(config),
      input_model_(input_model),
      model_(input_model_),
      input_tests_(create_internal_test_set(input_model_, tests)),
      strength_(1) {}

citcpp_ipog::citcpp_ipog(model&& input_model, test_set&& tests,
                         const covering_array_computation_config& config)
    : citcpp_ipog_base(),
      config_(config),
      input_model_(std::move(input_model)),
      model_(input_model_),
      input_tests_(create_internal_test_set(input_model_, tests)),
      strength_(1) {}

void citcpp_ipog::set_interaction_strength(int t) { strength_ = t; }

void citcpp_ipog::entry_point(cagen_exec_handle_ipog_impl& exec_handle) {
  const auto t_start = std::chrono::high_resolution_clock::now();

  std::vector<internal_relation> relations(
      create_relations(input_model_, model_, strength_));

  internal_test_set tests(input_tests_);
  if (tests.get_list_of_tests().empty()) {
    main_ipog_loop(model_, relations, tests, config_, exec_handle);
  } else {
    main_ipog_loop_extend_test_set(model_, relations, tests, config_,
                                   exec_handle);
  }
  if (config_.replace_dont_care_values()) {
    replace_dont_care_values(tests, model_);
  }
  test_set ts(
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
