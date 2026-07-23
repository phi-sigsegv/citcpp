#include "citcpp_covm.hpp"

#include <algorithm>
#include <chrono>
#include <citcpp/coverage_measurement.hpp>
#include <numeric>
#include <thread>
#include <unordered_map>

#include "binom_coeff_table.hpp"
#include "citcpp_algo_common.hpp"
#include "citcpp_utils.hpp"
#include "constraint_evaluator.hpp"
#include "constraint_handler.hpp"
#include "covm_algorithm_uniform_strength.hpp"
#include "covm_exec_handle_impl.hpp"
#include "covm_exec_result_impl.hpp"
#include "datatypes_config.hpp"
#include "functor_executor_thread_pool.hpp"
#include "model_simplifier.hpp"

namespace {

std::vector<citcpp::detail::internal_relation> create_relations(
    const citcpp::model& input_model,
    const citcpp::detail::internal_model& model, int strength) {
  using namespace citcpp::detail;

  std::vector<unsigned int> model_parameter_index_map(
      input_model.get_parameters().size());
  std::unordered_map<std::string, unsigned int> param_name_to_index_map;
  {
    unsigned int param_index = 0;
    for (const auto& param : input_model.get_parameters()) {
      param_name_to_index_map[param.get_name()] = param_index;
      model_parameter_index_map[param_index] = param_index;
      ++param_index;
    }
  }

  std::vector<internal_relation> relations;

  if (strength >= 1) {
    relations.emplace_back(std::move(model_parameter_index_map), strength);
  } else {
    for (const auto& relation : input_model.get_relations()) {
      std::vector<unsigned int> parameter_index_map;

      // Find the indices of referenced parameters and add them to the relation.
      for (const auto& param_ref : relation.get_parameters()) {
        unsigned int param_idx = param_name_to_index_map[param_ref.get_name()];
        parameter_index_map.push_back(param_idx);
      }

      std::sort(
          parameter_index_map.begin(), parameter_index_map.end(),
          [&model](const unsigned int& index1, const unsigned int& index2) {
            const auto& param_num_values = model.get_parameter_num_values();
            if (param_num_values[index1] != param_num_values[index2]) {
              return param_num_values[index1] > param_num_values[index2];
            }
            return index1 < index2;
          });

      relations.emplace_back(std::move(parameter_index_map),
                             relation.get_interaction_strength());
    }
  }

  return relations;
}

template <citcpp::detail::conc_is_void_functor_executor T_EXEC>
std::unordered_map<std::string, citcpp::coverage_measurement> main_covm_loop(
    const citcpp::model& input_model,
    const citcpp::detail::internal_model& model,
    const citcpp::detail::internal_test_set& test_set,
    const citcpp::detail::constraint_handler& constr_handler, T_EXEC& exec,
    int strength, citcpp::detail::covm_exec_handle_impl& exec_handle) {
  using namespace citcpp;
  using namespace citcpp::detail;

  const bool with_mt = exec.get_num_workers() > 1;

  std::vector<internal_relation> relations(
      create_relations(input_model, model, strength));

  const binom_coeff_table binomial_coeffs(
      static_cast<unsigned int>(model.get_parameter_num_values().size()));

  std::unordered_map<std::string, coverage_measurement> covm_per_relation;

  if (strength >= 1) {
    const auto& int_relation = relations[0];
    const unsigned int n = static_cast<unsigned int>(
        int_relation.get_parameter_index_map().size());

    unsigned long long number_combos_to_cover =
        with_mt ? number_of_combinations_to_cover(
                      n, model, int_relation.get_parameter_index_map(),
                      strength, false, exec)
                : number_of_combinations_to_cover(
                      n, model, int_relation.get_parameter_index_map(),
                      strength, false);

    covm_per_relation[""].set_number_of_param_combos_to_cover(
        binomial_coeffs.get_coefficient(n, strength));
    covm_per_relation[""].set_number_of_combinations_to_cover(
        number_combos_to_cover);
    exec_handle.set_number_of_combinations_to_process(number_combos_to_cover);
  } else {
    for (decltype(relations.size()) i = 0; i < relations.size(); ++i) {
      const auto& relation = input_model.get_relations()[i];
      const auto& int_relation = relations[i];
      const unsigned int n = static_cast<unsigned int>(
          int_relation.get_parameter_index_map().size());

      unsigned long long number_combos_to_cover =
          with_mt
              ? number_of_combinations_to_cover(
                    n, model, int_relation.get_parameter_index_map(),
                    int_relation.get_specified_interaction_strength(), false,
                    exec)
              : number_of_combinations_to_cover(
                    n, model, int_relation.get_parameter_index_map(),
                    int_relation.get_specified_interaction_strength(), false);

      covm_per_relation[relation.get_name()]
          .set_number_of_param_combos_to_cover(binomial_coeffs.get_coefficient(
              n, int_relation.get_specified_interaction_strength()));
      covm_per_relation[relation.get_name()]
          .set_number_of_combinations_to_cover(number_combos_to_cover);
      exec_handle.add_number_of_combinations_to_process(number_combos_to_cover);
    }
  }

  if (exec_handle.is_job_aborted()) {
    return covm_per_relation;
  }

  if (strength >= 1) {
    const auto& int_relation = relations[0];

    if (with_mt) {
      measure_coverage(int_relation.get_specified_interaction_strength(), model,
                       int_relation.get_parameter_index_map(), test_set,
                       constr_handler, exec_handle, covm_per_relation[""],
                       exec);
    } else {
      measure_coverage(int_relation.get_specified_interaction_strength(), model,
                       int_relation.get_parameter_index_map(), test_set,
                       constr_handler, exec_handle, covm_per_relation[""]);
    }
  } else {
    for (decltype(relations.size()) i = 0; i < relations.size(); ++i) {
      const auto& relation = input_model.get_relations()[i];
      const auto& int_relation = relations[i];

      if (with_mt) {
        measure_coverage(int_relation.get_specified_interaction_strength(),
                         model, int_relation.get_parameter_index_map(),
                         test_set, constr_handler, exec_handle,
                         covm_per_relation[relation.get_name()], exec);
      } else {
        measure_coverage(int_relation.get_specified_interaction_strength(),
                         model, int_relation.get_parameter_index_map(),
                         test_set, constr_handler, exec_handle,
                         covm_per_relation[relation.get_name()]);
      }
    }
  }

  exec.suspend_workers();

  return covm_per_relation;
}

}  // namespace

namespace citcpp {
namespace detail {

citcpp_covm::citcpp_covm(model input_model, test_set tests,
                         const coverage_measurement_config& config)
    : config_(config),
      input_model_(std::move(simplify_model(input_model))),
      model_(input_model_),
      tests_(create_internal_test_set(input_model_, tests)),
      strength_(1) {}

void citcpp_covm::set_interaction_strength(int t) { strength_ = t; }

void citcpp_covm::entry_point(covm_exec_handle_impl& exec_handle) {
  const auto t_start = std::chrono::high_resolution_clock::now();

  unsigned int num_threads =
      static_cast<unsigned int>(config_.number_of_threads());
  if (num_threads == 0) {
    num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) {
      num_threads = 1;
    }
  }

  thread_pool tp(num_threads);
  functor_executor_thread_pool exec(tp);

  exec_handle.set_execution_phase(
      covm_exec_handle_impl::phase::CONSTRAINT_HANDLER_INIT);

  std::shared_ptr<constraint_handler> constr_handler =
      constraint_handler::create_constraint_handler(
          model_, num_threads,
          config_.constraint_handler_memory_limit_gb() * 1024 * 1024 * 1024,
          exec_handle.get_constraint_handler_init_progress());

  // Filter out invalid tests.
  std::vector<std::size_t> invalid_test_indices;
  {
    constraint_evaluator constr_eval(input_model_.get_parameters());
    std::size_t test_idx = 0;
    auto test_it = tests_.get_list_of_tests().begin();
    while (test_it != tests_.get_list_of_tests().end()) {
      const auto& t = *test_it;
      const bool is_valid = t.has_dont_care_value()
                                ? constr_handler->is_valid_partial_test(t)
                                : constr_eval(t, input_model_);
      if (!is_valid) {
        test_it = tests_.get_list_of_tests().erase(test_it);
        invalid_test_indices.push_back(test_idx);
      } else {
        ++test_it;
      }
      ++test_idx;
    }
  }

  exec_handle.set_execution_phase(
      covm_exec_handle_impl::phase::COVERAGE_MEASUREMENT);

  std::unordered_map<std::string, coverage_measurement> covm_per_relation(
      main_covm_loop(input_model_, model_, tests_, *constr_handler, exec,
                     strength_, exec_handle));

  const auto t_end = std::chrono::high_resolution_clock::now();
  const auto duration_in_milli_seconds =
      duration_cast<std::chrono::milliseconds>(t_end - t_start);
  exec_handle.set_duration_in_milli_seconds(
      static_cast<std::size_t>(duration_in_milli_seconds.count()));

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
  result.set_result(std::move(covm_per_relation));
  result.set_invalid_test_indices(std::move(invalid_test_indices));
  exec_handle.set_coverage_measurement(std::move(result));
}

}  // namespace detail
}  // namespace citcpp
