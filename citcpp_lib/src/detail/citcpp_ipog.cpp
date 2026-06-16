#include "citcpp_ipog.hpp"

#include <algorithm>
#include <chrono>
#include <numeric>
#include <thread>
#include <unordered_map>
#include <utility>

#include "binom_coeff_table.hpp"
#include "cagen_exec_handle_ipog_impl.hpp"
#include "cagen_exec_result_impl.hpp"
#include "citcpp_algo_common.hpp"
#include "citcpp_utils.hpp"
#include "constraint_handler.hpp"
#include "constraint_handler_concurrent.hpp"
#include "coverage_map.hpp"
#include "datatypes_config.hpp"
#include "functor_executor_thread_pool.hpp"
#include "ipog_all_value_combinations.hpp"
#include "ipog_horizontal_extension.hpp"
#include "ipog_measure_testset.hpp"
#include "ipog_vertical_extension.hpp"

namespace {

template <citcpp::detail::conc_is_void_functor_executor T_EXEC>
void main_ipog_loop_body(
    const citcpp::detail::internal_model& model,
    const std::vector<citcpp::detail::internal_relation>& relations,
    citcpp::detail::internal_test_set& test_set,
    citcpp::detail::constraint_handler& constr_handler,
    const bool is_extend_mode, const unsigned int real_current_param_idx,
    const bool with_mt,
    const citcpp::detail::binom_coeff_table& binomial_coeffs, T_EXEC& exec,
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

    unsigned long long reported_number_combos_to_cover = 0;
    for (const auto& relation : relations) {
      if (relation
              .get_parameter_index_map()[relation.get_current_param_idx()] ==
          real_current_param_idx) {

        const unsigned long long relation_number_combos_to_cover =
            with_mt
                ? number_of_combinations_to_cover(
                      relation.get_current_param_idx() + 1, model,
                      relation.get_parameter_index_map(),
                      relation.get_current_interaction_strength(), true, exec)
                : number_of_combinations_to_cover(
                      relation.get_current_param_idx() + 1, model,
                      relation.get_parameter_index_map(),
                      relation.get_current_interaction_strength(), true);

        // We only report the combinations as covered, after we have reached
        // the full interaction strength. This is because otherwise we could
        // count too many interactions.
        if (relation.get_current_interaction_strength() >=
            relation.get_specified_interaction_strength()) {
          reported_number_combos_to_cover += relation_number_combos_to_cover;
        }
      }
    }

    exec.suspend_workers();
    exec_handle.add_number_of_processed_combinations(
        reported_number_combos_to_cover);
    exec_handle.add_number_of_covered_combinations(
        reported_number_combos_to_cover);
  } else {
    std::vector<std::pair<const internal_relation*, ipog_coverage_map>>
        relation_cov_maps;
    unsigned long long number_combos_to_process = 0;
    for (const auto& relation : relations) {
      if (relation
              .get_parameter_index_map()[relation.get_current_param_idx()] ==
          real_current_param_idx) {

        relation_cov_maps.push_back(std::make_pair(
            &relation,
            ipog_coverage_map(relation.get_current_param_idx() + 1,
                              relation.get_current_interaction_strength(),
                              model, relation.get_parameter_index_map(),
                              binomial_coeffs, true)));
        number_combos_to_process +=
            relation_cov_maps.back().second.get_total_number_of_tuples();
      }
    }

    if (is_extend_mode) {
      auto measure_coverage_res =
          with_mt
              ? ipog_measure_testset(model, test_set, relation_cov_maps, exec)
              : ipog_measure_testset(model, test_set, relation_cov_maps);

      for (const auto& relation_cov_result :
           measure_coverage_res.num_covered_tuples) {

        number_combos_to_process -= relation_cov_result.second;

        // We only report the combinations as covered, after we have reached
        // the full interaction strength. This is because otherwise we could
        // count too many interactions.
        if (relation_cov_result.first->get_current_interaction_strength() >=
            relation_cov_result.first->get_specified_interaction_strength()) {

          exec_handle.add_number_of_processed_combinations(
              relation_cov_result.second);
          exec_handle.add_number_of_covered_combinations(
              relation_cov_result.second);
        }
      }
    }

    auto horizontal_ext_res =
        with_mt ? ipog_horizontal_extension(number_combos_to_process,
                                            constr_handler, test_set,
                                            relation_cov_maps, exec)
                : ipog_horizontal_extension(number_combos_to_process,
                                            constr_handler, test_set,
                                            relation_cov_maps);

    exec.suspend_workers();

    for (const auto& relation_cov_result :
         horizontal_ext_res.num_new_covered_tuples) {

      number_combos_to_process -= relation_cov_result.second;

      // We only report the combinations as covered, after we have reached
      // the full interaction strength. This is because otherwise we could
      // count too many interactions.
      if (relation_cov_result.first->get_current_interaction_strength() >=
          relation_cov_result.first->get_specified_interaction_strength()) {

        exec_handle.add_number_of_processed_combinations(
            relation_cov_result.second);
        exec_handle.add_number_of_covered_combinations(
            relation_cov_result.second);
      }
    }

    exec_handle.set_testset_size(test_set.get_list_of_tests().size());

    if (exec_handle.is_job_aborted()) {
      return;
    }

    if (number_combos_to_process > 0) {
      auto vertical_ext_res = ipog_vertical_extension(
          number_combos_to_process, constr_handler, horizontal_ext_res,
          test_set, relation_cov_maps);

      for (const auto& relation_cov_result : vertical_ext_res) {

        number_combos_to_process -=
            relation_cov_result.second.num_checked_tuples;

        // We only report the combinations as covered, after we have reached
        // the full interaction strength. This is because otherwise we could
        // count too many interactions.
        if (relation_cov_result.first->get_current_interaction_strength() >=
            relation_cov_result.first->get_specified_interaction_strength()) {

          exec_handle.add_number_of_processed_combinations(
              relation_cov_result.second.num_checked_tuples);
          exec_handle.add_number_of_covered_combinations(
              relation_cov_result.second.num_new_covered_tuples);
        }
      }

      exec_handle.set_testset_size(test_set.get_list_of_tests().size());
    }
  }
}

template <citcpp::detail::conc_is_void_functor_executor T_EXEC>
void main_ipog_loop(const citcpp::detail::internal_model& model,
                    std::vector<citcpp::detail::internal_relation>& relations,
                    citcpp::detail::internal_test_set& test_set,
                    citcpp::detail::constraint_handler& constr_handler,
                    const citcpp::covering_array_computation_config config,
                    T_EXEC& exec,
                    citcpp::detail::cagen_exec_handle_ipog_impl& exec_handle) {
  using namespace citcpp::detail;

  const bool with_mt = exec.get_num_workers() > 1;

  std::vector<unsigned int> parameter_index_map(
      citcpp_ipog_base::create_parameter_index_map(relations, model));

  for (auto& relation : relations) {
    relation.sort_parameters(parameter_index_map);
  }

  // First we compute the number of combination we have to cover, as well as
  // some key properties of the relations.
  std::unordered_map<const internal_relation*, unsigned long long>
      relation_to_combos_to_cover;
  unsigned long long number_combos_to_process = 0;
  unsigned int maximum_required_strength = 0;
  unsigned int maximum_prefix_length = 0;
  for (const auto& relation : relations) {
    const unsigned long long relation_number_combos_to_cover =
        with_mt
            ? number_of_combinations_to_cover(
                  relation.get_parameter_index_map().size(), model,
                  relation.get_parameter_index_map(),
                  relation.get_specified_interaction_strength(), false, exec)
            : number_of_combinations_to_cover(
                  relation.get_parameter_index_map().size(), model,
                  relation.get_parameter_index_map(),
                  relation.get_specified_interaction_strength(), false);
    number_combos_to_process += relation_number_combos_to_cover;
    relation_to_combos_to_cover[&relation] = relation_number_combos_to_cover;
    maximum_required_strength =
        std::max(maximum_required_strength,
                 relation.get_specified_interaction_strength());
    maximum_prefix_length = std::max(
        maximum_prefix_length, citcpp_ipog_base::length_of_common_param_prefix(
                                   relation, parameter_index_map));
  }

  exec.suspend_workers();
  exec_handle.set_number_of_combinations_to_process(number_combos_to_process);
  exec_handle.set_number_of_parameters_to_process(parameter_index_map.size());

  if (exec_handle.is_job_aborted()) {
    return;
  }

  const unsigned int first_param_idx =
      std::min(maximum_required_strength, maximum_prefix_length);
  {
    // Step 1: Initialize for the first t parameters.
    create_all_value_combinations(first_param_idx, model, parameter_index_map,
                                  constr_handler, test_set);

    // Cache the tests in the constraint handler.
    for (const auto& t : test_set.get_list_of_tests()) {
      constr_handler.cache_partial_test(&t);
    }

    exec_handle.set_testset_size(test_set.get_list_of_tests().size());
    exec_handle.set_number_of_processed_parameters(first_param_idx);
  }

  for (unsigned int param_idx = 0; param_idx < first_param_idx; ++param_idx) {
    const unsigned int real_current_param_idx = parameter_index_map[param_idx];

    auto relation_it = relations.begin();
    while (relation_it != relations.end()) {
      internal_relation& relation = *relation_it;

      if (relation
              .get_parameter_index_map()[relation.get_current_param_idx()] ==
          real_current_param_idx) {

        relation.set_current_param_idx(relation.get_current_param_idx() + 1);
      }

      if (relation.get_current_param_idx() >=
          relation.get_specified_interaction_strength()) {

        auto num_combos =
            with_mt
                ? get_number_of_combinations(
                      relation.get_current_param_idx(), model,
                      relation.get_parameter_index_map(),
                      relation.get_current_param_idx(), false, test_set, exec)
                : get_number_of_combinations(
                      relation.get_current_param_idx(), model,
                      relation.get_parameter_index_map(),
                      relation.get_current_param_idx(), false, test_set);
        exec_handle.add_number_of_processed_combinations(
            num_combos.num_combos_to_cover);
        exec_handle.add_number_of_covered_combinations(
            num_combos.num_covered_combos);
      }

      if (relation.get_current_param_idx() >=
          relation.get_parameter_index_map().size()) {

        // The relation will already be fully covered during the
        // initialization phase of the testset.
        relation_it = relations.erase(relation_it);
      } else {
        ++relation_it;
      }
    }
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

    main_ipog_loop_body(model, relations, test_set, constr_handler, false,
                        real_current_param_idx, with_mt, binomial_coeffs, exec,
                        exec_handle);

    exec_handle.set_number_of_processed_parameters(current_param_idx + 1);

    auto relation_it = relations.begin();
    while (relation_it != relations.end()) {
      internal_relation& relation = *relation_it;

      if (relation
              .get_parameter_index_map()[relation.get_current_param_idx()] ==
          real_current_param_idx) {

        relation.set_current_param_idx(relation.get_current_param_idx() + 1);
      }

      if (relation.get_current_param_idx() >=
          relation.get_parameter_index_map().size()) {

        // The relation is fully covered. So we do not have to deal with it
        // anymore.
        relation_it = relations.erase(relation_it);
      } else {
        ++relation_it;
      }
    }
  }
}

template <citcpp::detail::conc_is_void_functor_executor T_EXEC>
void main_ipog_loop_extend_test_set(
    const citcpp::detail::internal_model& model,
    std::vector<citcpp::detail::internal_relation>& relations,
    citcpp::detail::internal_test_set& test_set,
    citcpp::detail::constraint_handler& constr_handler,
    const citcpp::covering_array_computation_config config, T_EXEC& exec,
    citcpp::detail::cagen_exec_handle_ipog_impl& exec_handle) {
  using namespace citcpp::detail;

  const bool with_mt = exec.get_num_workers() > 1;

  // First we compute the number of combination we have to cover.
  unsigned long long number_combos_to_process = 0;
  for (const auto& relation : relations) {
    number_combos_to_process +=
        with_mt
            ? number_of_combinations_to_cover(
                  relation.get_parameter_index_map().size(), model,
                  relation.get_parameter_index_map(),
                  relation.get_specified_interaction_strength(), false, exec)
            : number_of_combinations_to_cover(
                  relation.get_parameter_index_map().size(), model,
                  relation.get_parameter_index_map(),
                  relation.get_specified_interaction_strength(), false);
  }

  exec.suspend_workers();
  exec_handle.set_number_of_combinations_to_process(number_combos_to_process);

  if (exec_handle.is_job_aborted()) {
    return;
  }

  std::vector<unsigned int> parameter_index_map(
      citcpp_ipog_base::create_parameter_index_map(relations, model));

  for (auto& relation : relations) {
    relation.sort_parameters(parameter_index_map);
  }

  exec_handle.set_number_of_parameters_to_process(parameter_index_map.size());

  // Here is the main IPOG loop.
  const binom_coeff_table binomial_coeffs(parameter_index_map.size());

  // Cache the tests in the constraint handler.
  for (const auto& t : test_set.get_list_of_tests()) {
    constr_handler.cache_partial_test(&t);
  }

  for (unsigned int current_param_idx = 0;
       current_param_idx < parameter_index_map.size(); ++current_param_idx) {

    if (exec_handle.is_job_aborted()) {
      return;
    }

    const unsigned int real_current_param_idx =
        parameter_index_map[current_param_idx];

    main_ipog_loop_body(model, relations, test_set, constr_handler, true,
                        real_current_param_idx, with_mt, binomial_coeffs, exec,
                        exec_handle);

    exec_handle.set_number_of_processed_parameters(current_param_idx + 1);

    auto relation_it = relations.begin();
    while (relation_it != relations.end()) {
      internal_relation& relation = *relation_it;

      if (relation
              .get_parameter_index_map()[relation.get_current_param_idx()] ==
          real_current_param_idx) {

        relation.set_current_param_idx(relation.get_current_param_idx() + 1);
      }

      if (relation.get_current_param_idx() >=
          relation.get_parameter_index_map().size()) {

        // The relation is fully covered. So we do not have to deal with it
        // anymore.
        relation_it = relations.erase(relation_it);
      } else {
        ++relation_it;
      }
    }
  }
}

}  // namespace

namespace citcpp {
namespace detail {

citcpp_ipog::citcpp_ipog(model input_model,
                         const covering_array_computation_config& config)
    : citcpp_ipog_base(),
      config_(config),
      input_model_(std::move(input_model)),
      model_(input_model_),
      input_tests_(),
      strength_(1) {}

citcpp_ipog::citcpp_ipog(model input_model, test_set tests,
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

  unsigned int num_threads = config_.number_of_threads();
  if (num_threads == 0) {
    num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) {
      num_threads = 1;
    }
  }

  thread_pool tp(num_threads);
  functor_executor_thread_pool exec(tp);

  std::vector<internal_relation> relations(
      create_relations(input_model_, model_, strength_));

  exec_handle.set_execution_phase(
      cagen_exec_handle::phase::CONSTRAINT_HANDLER_INIT);

  std::shared_ptr<constraint_handler> constr_handler_impl =
      constraint_handler::create_constraint_handler(
          model_, num_threads,
          exec_handle.get_constraint_handler_init_progress());
  std::shared_ptr<constraint_handler> constr_handler =
      (constr_handler_impl->is_thread_safe() && num_threads > 1)
          ? std::make_shared<
                concurrent_constraint_handler<functor_executor_thread_pool>>(
                *constr_handler_impl, exec)
          : constr_handler_impl;

  exec_handle.set_execution_phase(
      cagen_exec_handle::phase::COVERING_ARRAY_CONSTRUCTION);

  internal_test_set tests(input_tests_);
  if (tests.get_list_of_tests().empty()) {
    main_ipog_loop(model_, relations, tests, *constr_handler, config_, exec,
                   exec_handle);
  } else {
    main_ipog_loop_extend_test_set(model_, relations, tests, *constr_handler,
                                   config_, exec, exec_handle);
  }
  if (config_.replace_dont_care_values()) {
    constr_handler->replace_dont_care_values(tests);
  }
  test_set ts(
      model_.create_from_internal_test_set(tests, config_.value_separator()));

  const auto t_end = std::chrono::high_resolution_clock::now();
  const auto duration_in_milli_seconds =
      std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start);
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
