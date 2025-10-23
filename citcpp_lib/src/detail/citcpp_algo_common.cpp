#include "citcpp_algo_common.hpp"

#include <new>

namespace {

// Recursive helper function for combination generation and sum calculation
// This function will be called by each async task.
unsigned long long recursive_combine_and_sum(
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

class alignas(std::hardware_destructive_interference_size)
    compute_partial_sum_task : public citcpp::detail::thread_pool::Task {
    typedef citcpp::detail::thread_pool::Task base_type;
    typedef compute_partial_sum_task this_type;

  public:
    compute_partial_sum_task()
        : base_type(),
          start_idx_(0),
          end_idx_(0),
          num_params_to_select_(0),
          additional_factor_(0),
          factor_levels_(nullptr),
          parameter_index_map_(nullptr),
          num_combinations_(nullptr) {
      setCallable(*this);
    }

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
          num_combinations_(num_combinations) {
      setCallable(*this);
    }

    compute_partial_sum_task(const this_type&) = delete;

    compute_partial_sum_task(this_type&& other)
        : base_type(std::move(other)),
          start_idx_(other.start_idx_),
          end_idx_(other.end_idx_),
          num_params_to_select_(other.num_params_to_select_),
          additional_factor_(other.additional_factor_),
          factor_levels_(other.factor_levels_),
          parameter_index_map_(other.parameter_index_map_),
          num_combinations_(other.num_combinations_) {
      setCallable(*this);
    }

    virtual ~compute_partial_sum_task() {}

    this_type& operator=(const this_type&) = delete;

    this_type& operator=(this_type&& other) {
      if (this != &other) {
        base_type::operator=(std::move(other));
        start_idx_ = other.start_idx_;
        end_idx_ = other.end_idx_;
        num_params_to_select_ = other.num_params_to_select_;
        additional_factor_ = other.additional_factor_;
        factor_levels_ = other.factor_levels_;
        parameter_index_map_ = other.parameter_index_map_;
        num_combinations_ = other.num_combinations_;
        setCallable(*this);
      }

      return *this;
    }

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

}  // namespace

namespace citcpp {
namespace detail {

unsigned long long number_of_combinations_to_cover(
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
      // We have exactly one parameter to select, which is just the one we have
      // fixed. So we do not have to walk over combinations of parameters here.
      return num_last_param_values;
    }
  } else {
    return recursive_combine_and_sum(
        n - 1, t - 1, 1, model.get_parameter_num_values(), parameter_index_map);
  }
}

unsigned long long number_of_combinations_to_cover(
    unsigned int n, const internal_model& model,
    const std::vector<unsigned int>& parameter_index_map, unsigned int t,
    bool fixed_last_parameter, thread_pool& tp) {

  std::atomic_ullong num_combinations = 0;

  if (fixed_last_parameter) {
    const unsigned int real_last_param_idx = parameter_index_map[n - 1];
    const int num_last_param_values =
        model.get_parameter_num_values()[real_last_param_idx];

    if (t >= 2) {
      task_group tg(tp.createTaskGroup());

      std::vector<compute_partial_sum_task> tasks(n - t + 1);
      for (unsigned int i = n - 2; i >= (t - 2); --i) {
        tasks[i - t + 2] =
            compute_partial_sum_task(i, i, t - 1, num_last_param_values,
                                     &model.get_parameter_num_values(),
                                     &parameter_index_map, &num_combinations);
        tg.spawn(i, &tasks[i - t + 2]);
      }
      tg.wait();
    } else {
      // We have exactly one parameter to select, which is just the one we have
      // fixed. So we do not have to walk over combinations of parameters here.
      return num_last_param_values;
    }
  } else {
    task_group tg(tp.createTaskGroup());

    std::vector<compute_partial_sum_task> tasks(n - t + 1);
    for (unsigned int i = n - 1; i >= (t - 1); --i) {
      tasks[i - t + 1] = compute_partial_sum_task(
          i, i, t, 1, &model.get_parameter_num_values(), &parameter_index_map,
          &num_combinations);
      tg.spawn(i, &tasks[i - t + 1]);
    }
    tg.wait();
  }

  return num_combinations;
}

}  // namespace detail
}  // namespace citcpp
