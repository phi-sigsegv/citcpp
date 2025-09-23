#include "citcpp_algo_common.hpp"

namespace {

// Recursive helper function for combination generation and sum calculation
// This function will be called by each async task.
unsigned long long recursive_combine_and_sum(
    int start_idx_for_next, int current_level,
    unsigned long long current_prod_val,
    const std::vector<unsigned int> &factorLevels,
    const std::vector<unsigned int> &parameter_index_map) {
  unsigned long long partial_sum = 0;
  for (int j = start_idx_for_next; j >= current_level; --j) {
    if (current_level == 0) {
      partial_sum += current_prod_val * factorLevels[parameter_index_map[j]];
    } else {
      partial_sum += recursive_combine_and_sum(
          j - 1, current_level - 1,
          current_prod_val * factorLevels[parameter_index_map[j]], factorLevels,
          parameter_index_map);
    }
  }

  return partial_sum;
}

class compute_partial_sum_with_parameter_map_task
    : public citcpp::detail::thread_pool::Task {
    typedef citcpp::detail::thread_pool::Task base_type;
    typedef compute_partial_sum_with_parameter_map_task this_type;

  public:
    compute_partial_sum_with_parameter_map_task()
        : base_type(),
          start_idx_for_next_(0),
          current_level_(0),
          additional_factor_(0),
          factorLevels_(nullptr),
          parameter_index_map_(nullptr),
          num_combinations_(nullptr) {
      setCallable(*this);
    }

    compute_partial_sum_with_parameter_map_task(
        int start_idx_for_next, int current_level,
        unsigned long long additional_factor,
        const std::vector<unsigned int> *factorLevels,
        const std::vector<unsigned int> *parameter_index_map,
        std::atomic_ullong *num_combinations)
        : base_type(),
          start_idx_for_next_(start_idx_for_next),
          current_level_(current_level),
          additional_factor_(additional_factor),
          factorLevels_(factorLevels),
          parameter_index_map_(parameter_index_map),
          num_combinations_(num_combinations) {
      setCallable(*this);
    }

    compute_partial_sum_with_parameter_map_task(const this_type &) = delete;

    compute_partial_sum_with_parameter_map_task(this_type &&other)
        : base_type(std::move(other)),
          start_idx_for_next_(other.start_idx_for_next_),
          current_level_(other.current_level_),
          additional_factor_(other.additional_factor_),
          factorLevels_(other.factorLevels_),
          parameter_index_map_(other.parameter_index_map_),
          num_combinations_(other.num_combinations_) {
      setCallable(*this);
    }

    virtual ~compute_partial_sum_with_parameter_map_task() {}

    this_type &operator=(const this_type &) = delete;

    this_type &operator=(this_type &&other) {
      if (this != &other) {
        base_type::operator=(std::move(other));
        start_idx_for_next_ = other.start_idx_for_next_;
        current_level_ = other.current_level_;
        additional_factor_ = other.additional_factor_;
        factorLevels_ = other.factorLevels_;
        parameter_index_map_ = other.parameter_index_map_;
        num_combinations_ = other.num_combinations_;
        setCallable(*this);
      }

      return *this;
    }

    void operator()() {
      unsigned long long chunk_num_combos = recursive_combine_and_sum(
          start_idx_for_next_ - 1, current_level_ - 1,
          additional_factor_ *
              (*factorLevels_)[(*parameter_index_map_)[start_idx_for_next_]],
          *factorLevels_, *parameter_index_map_);
      num_combinations_->fetch_add(chunk_num_combos, std::memory_order_acq_rel);
    }

  private:
    int start_idx_for_next_;
    int current_level_;
    unsigned long long additional_factor_;
    const std::vector<unsigned int> *factorLevels_;
    const std::vector<unsigned int> *parameter_index_map_;
    std::atomic_ullong *num_combinations_;
};

}  // namespace

namespace citcpp {
namespace detail {

unsigned long long number_of_combinations_to_cover(
    unsigned int n, const model &model,
    const std::vector<unsigned int> &parameter_index_map, unsigned int t,
    bool fixed_last_parameter) {

  if (fixed_last_parameter) {
    const unsigned int real_last_param_idx = parameter_index_map[n - 1];
    const int num_last_param_values =
        model.get_parameters()[real_last_param_idx];

    if (t >= 2) {
      return recursive_combine_and_sum(n - 2, t - 2, num_last_param_values,
                                       model.get_parameters(),
                                       parameter_index_map);
    } else {
      // We have exactly one parameter to select, which is just the one we have
      // fixed. So we do not have to walk over combinations of parameters here.
      return num_last_param_values;
    }
  } else {
    return recursive_combine_and_sum(n - 1, t - 1, 1, model.get_parameters(),
                                     parameter_index_map);
  }
}

unsigned long long number_of_combinations_to_cover(
    unsigned int n, const model &model,
    const std::vector<unsigned int> &parameter_index_map, unsigned int t,
    bool fixed_last_parameter, thread_pool &tp) {

  if (t < 2 || fixed_last_parameter && t < 3) {
    return number_of_combinations_to_cover(n, model, parameter_index_map, t,
                                           fixed_last_parameter);
  }

  const unsigned int numFactors = n;

  // Distribute the initial choices among threads
  // Each thread will compute a partial sum which we aggregate.
  std::atomic_ullong num_combinations = 0;

  if (fixed_last_parameter) {
    const unsigned int real_last_param_idx = parameter_index_map[n - 1];
    const int num_last_param_values =
        model.get_parameters()[real_last_param_idx];

    task_group tg(tp.createTaskGroup());

    std::vector<compute_partial_sum_with_parameter_map_task> tasks(numFactors -
                                                                   t + 1);
    for (unsigned int i = numFactors - 2; i >= (t - 2); --i) {
      tasks[i - t + 2] = compute_partial_sum_with_parameter_map_task(
          i, t - 2, num_last_param_values, &model.get_parameters(),
          &parameter_index_map, &num_combinations);
      tg.spawn(i, &tasks[i - t + 2]);
    }
    tg.wait();
  } else {
    task_group tg(tp.createTaskGroup());

    std::vector<compute_partial_sum_with_parameter_map_task> tasks(numFactors -
                                                                   t + 1);
    for (unsigned int i = numFactors - 1; i >= (t - 1); --i) {
      tasks[i - t + 1] = compute_partial_sum_with_parameter_map_task(
          i, t - 1, 1, &model.get_parameters(), &parameter_index_map,
          &num_combinations);
      tg.spawn(i, &tasks[i - t + 1]);
    }
    tg.wait();
  }

  return num_combinations;
}

}  // namespace detail
}  // namespace citcpp
