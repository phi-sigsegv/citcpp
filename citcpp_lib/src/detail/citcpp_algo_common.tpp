#include <functional>

#include "bitset.hpp"
#include "citcpp_algo_common.hpp"
#include "citcpp_utils.hpp"
#include "param_combo_iteration.hpp"
#include "shared_constants.hpp"

namespace citcpp {
namespace detail {

// Recursive helper function for combination generation and sum calculation
// This function will be called by each async task.
inline unsigned long long recursive_combine_and_sum(
    unsigned int max_param_idx, unsigned int current_level,
    unsigned long long current_prod_val,
    const std::vector<unsigned int>& factor_levels,
    const std::vector<unsigned int>& parameter_index_map) {

  unsigned long long partial_sum = 0;
  for (unsigned int j = current_level; j <= max_param_idx; ++j) {
    if (current_level == 0) {
      partial_sum += current_prod_val * factor_levels[parameter_index_map[j]];
    } else {
      partial_sum += recursive_combine_and_sum(
          j - 1, current_level - 1,
          current_prod_val * factor_levels[parameter_index_map[j]],
          factor_levels, parameter_index_map);
    }
  }

  return partial_sum;
}

class alignas(false_sharing_avoidance_alignment) compute_partial_sum_task {
  public:
    compute_partial_sum_task() = default;

    compute_partial_sum_task(
        unsigned int max_param_idx, unsigned int min_param_idx,
        unsigned int num_params_to_select, unsigned long long additional_factor,
        const std::vector<unsigned int>* factor_levels,
        const std::vector<unsigned int>* parameter_index_map,
        std::atomic_ullong* num_combinations)
        : max_param_idx_(max_param_idx),
          min_param_idx_(min_param_idx),
          num_params_to_select_(num_params_to_select),
          additional_factor_(additional_factor),
          factor_levels_(factor_levels),
          parameter_index_map_(parameter_index_map),
          num_combinations_(num_combinations) {}

    void operator()() {
      const unsigned int current_level = num_params_to_select_ - 1;
      unsigned long long chunk_num_combos = 0;
      for (unsigned int j = min_param_idx_; j <= max_param_idx_; ++j) {
        if (current_level == 0) {
          chunk_num_combos += additional_factor_ *
                              (*factor_levels_)[(*parameter_index_map_)[j]];
        } else {
          chunk_num_combos += recursive_combine_and_sum(
              j - 1, current_level - 1,
              additional_factor_ *
                  (*factor_levels_)[(*parameter_index_map_)[j]],
              (*factor_levels_), (*parameter_index_map_));
        }
      }

      num_combinations_->fetch_add(chunk_num_combos, std::memory_order_acq_rel);
    }

  private:
    unsigned int max_param_idx_{0};
    unsigned int min_param_idx_{0};
    unsigned int num_params_to_select_{0};
    unsigned long long additional_factor_{0};
    const std::vector<unsigned int>* factor_levels_{nullptr};
    const std::vector<unsigned int>* parameter_index_map_{nullptr};
    std::atomic_ullong* num_combinations_{nullptr};
};

inline unsigned long long number_of_combinations_to_cover(
    unsigned int n, const internal_model& model,
    const std::vector<unsigned int>& parameter_index_map, unsigned int t,
    bool fixed_last_parameter) {

  if (fixed_last_parameter) {
    const unsigned int real_last_param_idx = parameter_index_map[n - 1];
    const unsigned int num_last_param_values =
        model.get_parameter_num_values()[real_last_param_idx];

    if (t >= 2) {
      return recursive_combine_and_sum(n - 2, t - 2, num_last_param_values,
                                       model.get_parameter_num_values(),
                                       parameter_index_map);
    } else {
      // We have exactly one parameter to select, which is just the one we
      // have fixed. So we do not have to walk over combinations of parameters
      // here.
      return num_last_param_values;
    }
  } else {
    return recursive_combine_and_sum(
        n - 1, t - 1, 1, model.get_parameter_num_values(), parameter_index_map);
  }
}

template <conc_is_void_functor_executor T_EXEC>
unsigned long long number_of_combinations_to_cover(
    unsigned int n, const internal_model& model,
    const std::vector<unsigned int>& parameter_index_map, unsigned int t,
    bool fixed_last_parameter, T_EXEC& exec) {

  std::atomic_ullong num_combinations = 0;

  if (fixed_last_parameter) {
    // Parallelization cannot really pay off if we have an interaction
    // strength
    // <= 2. So resort to the sequential implementation.
    if (t <= 2) {
      return number_of_combinations_to_cover(n, model, parameter_index_map, t,
                                             fixed_last_parameter);
    }

    const unsigned int real_last_param_idx = parameter_index_map[n - 1];
    const unsigned int num_last_param_values =
        model.get_parameter_num_values()[real_last_param_idx];

    thread_local_vector<compute_partial_sum_task> tasks(n - t + 1);

    {
      auto exec_scope(exec.create_execution_scope());
      // t > 2 holds, and thus array_offset > 0 holds.
      const unsigned int array_offset = t - 2;
      for (unsigned int i = n - 2; i >= array_offset; --i) {
        tasks[i - array_offset] =
            compute_partial_sum_task(i, i, t - 1, num_last_param_values,
                                     &model.get_parameter_num_values(),
                                     &parameter_index_map, &num_combinations);
        exec_scope.spawn_execution(tasks[i - array_offset]);
      }
    }
  } else {
    // Parallelization cannot really pay off if we have an interaction
    // strength <= 1. So resort to the sequential implementation.
    if (t <= 1) {
      return number_of_combinations_to_cover(n, model, parameter_index_map, t,
                                             fixed_last_parameter);
    }

    thread_local_vector<compute_partial_sum_task> tasks(n - t + 1);

    {
      auto exec_scope(exec.create_execution_scope());
      // t > 1 holds, and thus array_offset > 0 holds.
      const unsigned int array_offset = t - 1;
      for (unsigned int i = n - 1; i >= array_offset; --i) {
        tasks[i - array_offset] = compute_partial_sum_task(
            i, i, t, 1, &model.get_parameter_num_values(), &parameter_index_map,
            &num_combinations);
        exec_scope.spawn_execution(tasks[i - array_offset]);
      }
    }
  }

  return num_combinations;
}

}  // namespace detail
}  // namespace citcpp
