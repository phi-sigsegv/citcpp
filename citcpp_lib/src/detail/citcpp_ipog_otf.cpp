#include "citcpp_ipog_otf.hpp"

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
#include "datatypes_config.hpp"
#include "ipog_all_value_combinations.hpp"
#include "ipog_otf_horizontal_extension.hpp"
#include "ipog_otf_vertical_extension.hpp"

namespace {

void main_ipog_loop_body(
    const citcpp::detail::internal_model& model,
    const std::vector<citcpp::detail::internal_relation>& relations,
    citcpp::detail::internal_test_set& test_set, const bool is_extend_mode,
    const unsigned int real_current_param_idx, const bool with_mt,
    citcpp::detail::thread_pool& tp,
    citcpp::detail::cagen_exec_handle_ipog_impl& exec_handle) {
  using namespace citcpp::detail;

  unsigned long long number_combos_to_cover = 0;
  unsigned long long reported_number_combos_to_cover = 0;
  for (const auto& relation : relations) {
    if (relation.get_parameter_index_map()[relation.get_current_param_idx()] ==
        real_current_param_idx) {

      const unsigned long long relation_number_combos_to_cover =
          with_mt ? number_of_combinations_to_cover(
                        relation.get_current_param_idx() + 1, model,
                        relation.get_parameter_index_map(),
                        relation.get_current_interaction_strength(), true, tp)
                  : number_of_combinations_to_cover(
                        relation.get_current_param_idx() + 1, model,
                        relation.get_parameter_index_map(),
                        relation.get_current_interaction_strength(), true);

      number_combos_to_cover += relation_number_combos_to_cover;

      // We only report the combinations as covered, after we have reached
      // the full interaction strength. This is because otherwise we could
      // count too many interactions.
      if (relation.get_current_interaction_strength() >=
          relation.get_specified_interaction_strength()) {
        reported_number_combos_to_cover += relation_number_combos_to_cover;
      }
    }
  }

  tp.stop_workers();

  if (model.get_parameter_num_values()[real_current_param_idx] <= 1) {
    // If the current parameter only has only value, then
    // we can treat this situation much simpler: We just have
    // to add that particular value to each test and update
    // the number of covered combinations.
    for (test& t : test_set.get_list_of_tests()) {
      t.get_values()[real_current_param_idx] = 0;
    }

    exec_handle.add_number_of_covered_combinations(
        reported_number_combos_to_cover);
  } else {
    auto horizontal_ext_res =
        with_mt
            ? ipog_horizontal_extension(number_combos_to_cover, test_set, model,
                                        relations, is_extend_mode, tp)
            : ipog_horizontal_extension(number_combos_to_cover, test_set, model,
                                        relations, is_extend_mode);
    tp.stop_workers();

    for (const auto& relation_cov_result :
         horizontal_ext_res.num_new_covered_tuples) {

      number_combos_to_cover -= relation_cov_result.second;

      // We only report the combinations as covered, after we have reached
      // the full interaction strength. This is because otherwise we could
      // count too many interactions.
      if (relation_cov_result.first->get_current_interaction_strength() >=
          relation_cov_result.first->get_specified_interaction_strength()) {

        exec_handle.add_number_of_covered_combinations(
            relation_cov_result.second);
      }
    }

    exec_handle.set_testset_size(test_set.get_list_of_tests().size());

    if (exec_handle.is_job_aborted()) {
      return;
    }

    if (number_combos_to_cover > 0) {
      auto vertical_ext_res =
          ipog_vertical_extension(number_combos_to_cover, horizontal_ext_res,
                                  test_set, model, relations);

      for (const auto& relation_cov_result :
           vertical_ext_res.num_new_covered_tuples) {

        number_combos_to_cover -= relation_cov_result.second;

        // We only report the combinations as covered, after we have reached
        // the full interaction strength. This is because otherwise we could
        // count too many interactions.
        if (relation_cov_result.first->get_current_interaction_strength() >=
            relation_cov_result.first->get_specified_interaction_strength()) {

          exec_handle.add_number_of_covered_combinations(
              relation_cov_result.second);
        }
      }

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

  std::vector<unsigned int> parameter_index_map(
      citcpp_ipog_base::create_parameter_index_map(relations, model));

  // First we compute the number of combination we have to cover, as well as
  // some key properties of the relations.
  std::unordered_map<const internal_relation*, unsigned long long>
      relation_to_combos_to_cover;
  unsigned long long number_combos_to_cover = 0;
  unsigned int maximum_required_strength = 0;
  unsigned int maximum_prefix_length = 0;
  for (const auto& relation : relations) {
    const unsigned long long relation_number_combos_to_cover =
        with_mt ? number_of_combinations_to_cover(
                      relation.get_parameter_index_map().size(), model,
                      relation.get_parameter_index_map(),
                      relation.get_specified_interaction_strength(), false, tp)
                : number_of_combinations_to_cover(
                      relation.get_parameter_index_map().size(), model,
                      relation.get_parameter_index_map(),
                      relation.get_specified_interaction_strength(), false);
    number_combos_to_cover += relation_number_combos_to_cover;
    relation_to_combos_to_cover[&relation] = relation_number_combos_to_cover;
    maximum_required_strength =
        std::max(maximum_required_strength,
                 relation.get_specified_interaction_strength());
    maximum_prefix_length = std::max(
        maximum_prefix_length, citcpp_ipog_base::length_of_common_param_prefix(
                                   relation, parameter_index_map));
  }

  tp.stop_workers();
  exec_handle.set_number_of_combinations_to_cover(number_combos_to_cover);
  exec_handle.set_number_of_parameters_to_process(parameter_index_map.size());

  if (exec_handle.is_job_aborted()) {
    return;
  }

  const unsigned int first_param_idx =
      std::min(maximum_required_strength, maximum_prefix_length);
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
          relation.get_parameter_index_map().size()) {

        // The relation will already be fully covered during the
        // initialization phase of the testset.
        relation_it = relations.erase(relation_it);
        exec_handle.add_number_of_covered_combinations(
            relation_to_combos_to_cover[&relation]);
      } else {
        ++relation_it;
      }
    }
  }

  {
    // Step 1: Initialize for the first t parameters.
    create_all_value_combinations(first_param_idx, model, parameter_index_map,
                                  test_set);
    exec_handle.set_testset_size(test_set.get_list_of_tests().size());
    exec_handle.set_number_of_processed_parameters(first_param_idx);
  }

  for (const auto& relation : relations) {
    const unsigned long long number_of_covered_combos =
        (relation.get_current_param_idx() >=
         relation.get_specified_interaction_strength())
            ? number_of_combinations_to_cover(
                  relation.get_current_param_idx(), model,
                  relation.get_parameter_index_map(),
                  relation.get_current_param_idx(), false)
            : 0;
    exec_handle.add_number_of_covered_combinations(number_of_covered_combos);
  }

  // Here is the main IPOG loop.
  for (unsigned int current_param_idx = first_param_idx;
       current_param_idx < parameter_index_map.size(); ++current_param_idx) {

    if (exec_handle.is_job_aborted()) {
      return;
    }

    const unsigned int real_current_param_idx =
        parameter_index_map[current_param_idx];

    main_ipog_loop_body(model, relations, test_set, false,
                        real_current_param_idx, with_mt, tp, exec_handle);

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

  exec_handle.set_number_of_parameters_to_process(parameter_index_map.size());

  // Here is the main IPOG loop.
  for (unsigned int current_param_idx = 0;
       current_param_idx < parameter_index_map.size(); ++current_param_idx) {

    if (exec_handle.is_job_aborted()) {
      return;
    }

    const unsigned int real_current_param_idx =
        parameter_index_map[current_param_idx];

    main_ipog_loop_body(model, relations, test_set, true,
                        real_current_param_idx, with_mt, tp, exec_handle);

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

citcpp_ipog_otf::citcpp_ipog_otf(
    const model& input_model, const covering_array_computation_config& config)
    : citcpp_ipog_base(),
      config_(config),
      input_model_(input_model),
      model_(input_model_),
      input_tests_(),
      strength_(1) {}

citcpp_ipog_otf::citcpp_ipog_otf(
    model&& input_model, const covering_array_computation_config& config)
    : citcpp_ipog_base(),
      config_(config),
      input_model_(std::move(input_model)),
      model_(input_model_),
      input_tests_(),
      strength_(1) {}

citcpp_ipog_otf::citcpp_ipog_otf(
    const model& input_model, const citcpp::test_set& tests,
    const covering_array_computation_config& config)
    : citcpp_ipog_base(),
      config_(config),
      input_model_(input_model),
      model_(input_model_),
      input_tests_(create_internal_test_set(input_model_, tests)),
      strength_(1) {}

citcpp_ipog_otf::citcpp_ipog_otf(
    model&& input_model, test_set&& tests,
    const covering_array_computation_config& config)
    : citcpp_ipog_base(),
      config_(config),
      input_model_(std::move(input_model)),
      model_(input_model_),
      input_tests_(create_internal_test_set(input_model_, tests)),
      strength_(1) {}

void citcpp_ipog_otf::set_interaction_strength(int t) { strength_ = t; }

void citcpp_ipog_otf::entry_point(cagen_exec_handle_ipog_impl& exec_handle) {
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
