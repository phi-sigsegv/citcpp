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

class alignas(false_sharing_avoidance_alignment) compute_partial_sum_task
    : public functor_task_base<compute_partial_sum_task> {

  private:
    typedef functor_task_base<compute_partial_sum_task> base_type;

  public:
    compute_partial_sum_task() = default;

    compute_partial_sum_task(
        unsigned int max_param_idx, unsigned int min_param_idx,
        unsigned int num_params_to_select, unsigned long long additional_factor,
        const std::vector<unsigned int>* factor_levels,
        const std::vector<unsigned int>* parameter_index_map,
        std::atomic_ullong* num_combinations)
        : base_type(),
          max_param_idx_(max_param_idx),
          min_param_idx_(min_param_idx),
          num_params_to_select_(num_params_to_select),
          additional_factor_(additional_factor),
          factor_levels_(factor_levels),
          parameter_index_map_(parameter_index_map),
          num_combinations_(num_combinations) {}

    virtual ~compute_partial_sum_task() {}

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
    unsigned int max_param_idx_;
    unsigned int min_param_idx_;
    unsigned int num_params_to_select_;
    unsigned long long additional_factor_;
    const std::vector<unsigned int>* factor_levels_;
    const std::vector<unsigned int>* parameter_index_map_;
    std::atomic_ullong* num_combinations_;
};

class num_combos_per_param_combo_functor {
  public:
    num_combos_per_param_combo_functor(
        const internal_model& model, const internal_test_set& test_set,
        const unsigned int param_combo_sizes,
        const bitset_uint64::size_type bitset_backing_array_size)
        : model_(model),
          test_set_(test_set),
          weights_(param_combo_sizes),
          values_combo_bitset_(bitset_backing_array_size),
          num_combos_{0, 0} {}

    bool operator()(const param_vector& param_indices) {
      bitset_uint64::size_type bitset_size = 1;
      for (const uint16_t p : param_indices) {
        bitset_size *= model_.get_parameter_num_values()[p];
      }
      values_combo_bitset_.reset_with_new_size(bitset_size);

      // Pre-calculate weights for index computation.
      // Those are used to compute an index into the bitset. To do so, we treat
      // the number of values of each parameter as a kind of radix. Consider
      // three parameters p_0, p_1, p_2. Now say that v_i is the number of
      // values for p_i. If we now have values x_0, x_1, x_2, then the index
      // is x_0 * v_1 * v_2 + x_1 * v_2 + x_2.
      bitset_uint64::size_type weight = 1;
      for (int i = static_cast<int>(param_indices.size() - 1); i >= 0; --i) {
        weights_[i] = weight;
        weight *= model_.get_parameter_num_values()[param_indices[i]];
      }

      num_combos_.num_combos_to_cover += bitset_size;

      for (const auto& test : test_set_.get_list_of_tests()) {
        bitset_uint64::size_type index = 0;
        bool found_dont_care = false;
        for (std::size_t i = 0; i < param_indices.size(); ++i) {
          const int param_value = test.get_values()[param_indices[i]];

          if (param_value < 0) {
            // We have found a don't care value for that combination in
            // the considered test in one of the parameters.
            // There is nothing to be updated concerning the
            // coverage.
            found_dont_care = true;
            break;
          }

          index += param_value * weights_[i];
        }

        if (!found_dont_care) {
          if (!values_combo_bitset_.test_and_set(index)) {
            num_combos_.num_covered_combos++;
          }
        }
      }

      return true;
    }

    const number_of_combinations& get_number_of_combos() const {
      return num_combos_;
    }

  private:
    const internal_model& model_;
    const internal_test_set& test_set_;
    std::vector<bitset_uint64::size_type> weights_;
    bitset_uint64 values_combo_bitset_;
    number_of_combinations num_combos_;
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

inline number_of_combinations get_number_of_combinations(
    unsigned int n, const internal_model& model,
    const std::vector<unsigned int>& parameter_index_map, unsigned int t,
    bool fixed_last_parameter, const internal_test_set& test_set) {

  const unsigned int product_of_max_parameter_sizes =
      get_product_of_max_n_parameter_sizes(
          static_cast<unsigned int>(parameter_index_map.size()), t, model,
          parameter_index_map);

  num_combos_per_param_combo_functor per_param_combo_functor(
      model, test_set, t, product_of_max_parameter_sizes);

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
      get_product_of_max_n_parameter_sizes(
          static_cast<unsigned int>(parameter_index_map.size()), t, model,
          parameter_index_map);

  param_combo_functor_parallel_iterator<num_combos_per_param_combo_functor,
                                        T_EXEC>
      per_param_combo_functor_parallel(n, t, parameter_index_map,
                                       fixed_last_parameter, exec,
                                       std::cref(model), std::cref(test_set), t,
                                       product_of_max_parameter_sizes);

  per_param_combo_functor_parallel.visit_all_parameter_combinations();

  number_of_combinations num_combos{0, 0};
  per_param_combo_functor_parallel.visit_all_functors(
      [&num_combos](const num_combos_per_param_combo_functor& f) {
        const number_of_combinations& local_num_combos =
            f.get_number_of_combos();
        num_combos.num_combos_to_cover += local_num_combos.num_combos_to_cover;
        num_combos.num_covered_combos += local_num_combos.num_covered_combos;
      });

  return num_combos;
}

}  // namespace detail
}  // namespace citcpp
