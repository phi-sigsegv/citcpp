#include "bitset.hpp"
#include "citcpp_algo_common.hpp"
#include "citcpp_utils.hpp"
#include "param_combo_iteration.hpp"
#include "shared_aux_types.hpp"
#include "shared_constants.hpp"

namespace citcpp {
namespace detail {

// Recursive helper function for combination generation and sum calculation
// This function will be called by each async task.
inline unsigned long long recursive_combine_and_sum(
    int start_idx, int current_level, unsigned long long current_prod_val,
    const std::vector<unsigned int>& factor_levels,
    const std::vector<unsigned int>& parameter_index_map) {

  unsigned long long partial_sum = 0;
  for (int j = start_idx; j >= current_level; --j) {
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

class alignas(false_sharing_avoidance_alignment) compute_partial_sum_task
    : public functor_task_base<compute_partial_sum_task> {

  private:
    typedef functor_task_base<compute_partial_sum_task> base_type;

  public:
    compute_partial_sum_task() = default;

    compute_partial_sum_task(
        int start_idx, int end_idx, int num_params_to_select,
        unsigned long long additional_factor,
        const std::vector<unsigned int>* factor_levels,
        const std::vector<unsigned int>* parameter_index_map,
        std::atomic_ullong* num_combinations)
        : base_type(),
          start_idx_(start_idx),
          end_idx_(end_idx),
          num_params_to_select_(num_params_to_select),
          additional_factor_(additional_factor),
          factor_levels_(factor_levels),
          parameter_index_map_(parameter_index_map),
          num_combinations_(num_combinations) {}

    virtual ~compute_partial_sum_task() {}

    void operator()() {
      const int current_level = num_params_to_select_ - 1;
      unsigned long long chunk_num_combos = 0;
      for (int j = start_idx_; j >= end_idx_; --j) {
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
    int start_idx_;
    int end_idx_;
    int num_params_to_select_;
    unsigned long long additional_factor_;
    const std::vector<unsigned int>* factor_levels_;
    const std::vector<unsigned int>* parameter_index_map_;
    std::atomic_ullong* num_combinations_;
};

class num_combos_per_param_combo_functor {
  public:
    num_combos_per_param_combo_functor(
        const internal_model& model, const internal_test_set& test_set,
        const unsigned int bitset_backing_array_size)
        : model_(model),
          test_set_(test_set),
          bitset_backing_array_(bitset_backing_array_size),
          num_combos_{0, 0} {}

    bool operator()(const param_vector& param_indices) {
      bitset_non_owning_uint64::size_type bitset_size = 1;
      for (auto p : param_indices) {
        bitset_size *= model_.get_parameter_num_values()[p];
      }
      bitset_non_owning_uint64 values_combo_bitset(bitset_size);
      values_combo_bitset.set_backing_array(bitset_backing_array_.get_array());
      values_combo_bitset.reset();

      num_combos_.num_combos_to_cover += bitset_size;

      for (const test& test : test_set_.get_list_of_tests()) {
        // Here we compute an index into the bitset. To do so, we treat the
        // number of values of each parameter as a kind of radix. Consider
        // three parameters p_0, p_1, p_2. Now say that v_i is the number of
        // values for p_i. If we now have values x_0, x_1, x_2, then the index
        // is x_0 * v_1 * v_2 + x_1 * v_2 + x_2.
        bitset_non_owning_uint64::size_type index = 0;
        bool found_dont_care = false;
        for (std::vector<unsigned int>::size_type i = 0;
             i < param_indices.size(); ++i) {
          const unsigned int param_idx = param_indices[i];
          const int param_value = test.get_values()[param_idx];

          if (param_value < 0) {
            // We have found a don't care value for that combination in
            // the considered test in one of the parameters.
            // There is nothing to be updated concerning the
            // coverage.
            found_dont_care = true;
            break;
          }

          bitset_non_owning_uint64::size_type addend = param_value;
          for (std::vector<unsigned int>::size_type j = i + 1;
               j < param_indices.size(); ++j) {
            addend *= model_.get_parameter_num_values()[param_indices[j]];
          }
          index += addend;
        }

        if (!found_dont_care) {
          if (!values_combo_bitset.test_and_set(index)) {
            num_combos_.num_covered_combos++;
          }
        }
      }

      return true;
    }

    number_of_combinations get_number_of_combos() { return num_combos_; }

  private:
    const internal_model& model_;
    const internal_test_set& test_set_;
    array_wrapper_uint64 bitset_backing_array_;
    number_of_combinations num_combos_;
};

struct alignas(false_sharing_avoidance_alignment)
    aligned_number_of_combinations {
    number_of_combinations value;
};

template <conc_is_void_functor_executor T_EXEC>
class num_combos_per_param_combo_functor_parallel {
  public:
    num_combos_per_param_combo_functor_parallel(
        const internal_model& model, const internal_test_set& test_set,
        const unsigned int bitset_backing_array_size,
        const param_combo_parallel_iterator<T_EXEC>& param_combo_it)
        : model_(model),
          test_set_(test_set),
          param_combo_it_(param_combo_it),
          bitset_backing_array_(param_combo_it.get_num_workers(),
                                {{bitset_backing_array_size}}),
          num_combos_(param_combo_it.get_num_workers(),
                      {number_of_combinations{0, 0}}) {}

    bool operator()(const param_vector& param_indices) {
      bitset_non_owning_uint64::size_type bitset_size = 1;
      for (auto p : param_indices) {
        bitset_size *= model_.get_parameter_num_values()[p];
      }
      bitset_non_owning_uint64 values_combo_bitset(bitset_size);
      values_combo_bitset.set_backing_array(
          bitset_backing_array_[param_combo_it_.get_worker_id()]
              .value.get_array());
      values_combo_bitset.reset();

      num_combos_[param_combo_it_.get_worker_id()].value.num_combos_to_cover +=
          bitset_size;

      for (const test& test : test_set_.get_list_of_tests()) {
        // Here we compute an index into the bitset. To do so, we treat the
        // number of values of each parameter as a kind of radix. Consider
        // three parameters p_0, p_1, p_2. Now say that v_i is the number of
        // values for p_i. If we now have values x_0, x_1, x_2, then the index
        // is x_0 * v_1 * v_2 + x_1 * v_2 + x_2.
        bitset_non_owning_uint64::size_type index = 0;
        bool found_dont_care = false;
        for (std::vector<unsigned int>::size_type i = 0;
             i < param_indices.size(); ++i) {
          const unsigned int param_idx = param_indices[i];
          const int param_value = test.get_values()[param_idx];

          if (param_value < 0) {
            // We have found a don't care value for that combination in
            // the considered test in one of the parameters.
            // There is nothing to be updated concerning the
            // coverage.
            found_dont_care = true;
            break;
          }

          bitset_non_owning_uint64::size_type addend = param_value;
          for (std::vector<unsigned int>::size_type j = i + 1;
               j < param_indices.size(); ++j) {
            addend *= model_.get_parameter_num_values()[param_indices[j]];
          }
          index += addend;
        }

        if (!found_dont_care) {
          if (!values_combo_bitset.test_and_set(index)) {
            auto& thread_local_num_combos =
                num_combos_[param_combo_it_.get_worker_id()];
            thread_local_num_combos.value.num_covered_combos++;
          }
        }
      }

      return true;
    }

    number_of_combinations get_number_of_combos() {
      number_of_combinations result{0, 0};

      for (const auto& thread_local_num_combos : num_combos_) {
        result.num_combos_to_cover +=
            thread_local_num_combos.value.num_combos_to_cover;
        result.num_covered_combos +=
            thread_local_num_combos.value.num_covered_combos;
      }

      return result;
    }

  private:
    const internal_model& model_;
    const internal_test_set& test_set_;
    const param_combo_parallel_iterator<T_EXEC>& param_combo_it_;
    alignas(false_sharing_avoidance_alignment)
        thread_local_vector<aligned_array_wrapper> bitset_backing_array_;
    alignas(false_sharing_avoidance_alignment)
        thread_local_vector<aligned_number_of_combinations> num_combos_;
};

inline unsigned long long number_of_combinations_to_cover(
    unsigned int n, const internal_model& model,
    const std::vector<unsigned int>& parameter_index_map, unsigned int t,
    bool fixed_last_parameter) {

  if (fixed_last_parameter) {
    const unsigned int real_last_param_idx = parameter_index_map[n - 1];
    const int num_last_param_values =
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
    const int num_last_param_values =
        model.get_parameter_num_values()[real_last_param_idx];

    std::vector<compute_partial_sum_task> tasks(n - t + 1);

    {
      auto exec_scope(exec.create_execution_scope());
      const int array_offset = t - 2;
      for (int i = n - 2; i >= array_offset; --i) {
        tasks[i - array_offset] =
            compute_partial_sum_task(i, i, t - 1, num_last_param_values,
                                     &model.get_parameter_num_values(),
                                     &parameter_index_map, &num_combinations);
        exec_scope.spawn_execution(tasks[i - array_offset]);
      }
    }
  } else {
    // Parallelization cannot really pay off if we have an interaction
    // strength
    // <= 1. So resort to the sequential implementation.
    if (t <= 1) {
      return number_of_combinations_to_cover(n, model, parameter_index_map, t,
                                             fixed_last_parameter);
    }

    std::vector<compute_partial_sum_task> tasks(n - t + 1);

    {
      auto exec_scope(exec.create_execution_scope());
      const int array_offset = t - 1;
      for (int i = n - 1; i >= array_offset; --i) {
        tasks[i - array_offset] = compute_partial_sum_task(
            i, i, t, 1, &model.get_parameter_num_values(), &parameter_index_map,
            &num_combinations);
        exec_scope.spawn_execution(tasks[i - array_offset]);
      }
    }
  }

  return num_combinations;
}

inline number_of_combinations get_number_of_combinations(
    unsigned int n, const internal_model& model,
    const std::vector<unsigned int>& parameter_index_map, unsigned int t,
    bool fixed_last_parameter, const internal_test_set& test_set) {

  const unsigned int product_of_max_parameter_sizes =
      get_product_of_max_n_parameter_sizes(parameter_index_map.size(), t, model,
                                           parameter_index_map);

  num_combos_per_param_combo_functor per_param_combo_functor(
      model, test_set, product_of_max_parameter_sizes);

  param_combo_iterator param_combo_it(n, t, parameter_index_map,
                                      fixed_last_parameter);
  param_combo_it.visit_all_parameter_combinations(per_param_combo_functor);

  return per_param_combo_functor.get_number_of_combos();
}

template <conc_is_void_functor_executor T_EXEC>
number_of_combinations get_number_of_combinations(
    unsigned int n, const internal_model& model,
    const std::vector<unsigned int>& parameter_index_map, unsigned int t,
    bool fixed_last_parameter, const internal_test_set& test_set,
    T_EXEC& exec) {

  const unsigned int product_of_max_parameter_sizes =
      get_product_of_max_n_parameter_sizes(parameter_index_map.size(), t, model,
                                           parameter_index_map);

  param_combo_parallel_iterator<T_EXEC> param_combo_it(
      n, t, parameter_index_map, fixed_last_parameter, exec);

  num_combos_per_param_combo_functor_parallel<T_EXEC> per_param_combo_functor(
      model, test_set, product_of_max_parameter_sizes, param_combo_it);

  param_combo_it.visit_all_parameter_combinations(per_param_combo_functor);

  return per_param_combo_functor.get_number_of_combos();
}

}  // namespace detail
}  // namespace citcpp
