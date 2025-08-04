#include "citcpp_algo_common.hpp"

namespace {

// Recursive helper function for combination generation and sum calculation
// This function will be called by each async task.
unsigned long long recursive_combine_and_sum(
    int start_idx_for_next, int current_level,
    unsigned long long current_prod_val,
    const std::vector<unsigned int> &factorLevels) {
  unsigned long long partial_sum = 0;
  for (int j = start_idx_for_next; j >= current_level; --j) {
    if (current_level == 0) {
      partial_sum += current_prod_val * factorLevels[j];
    } else {
      partial_sum += recursive_combine_and_sum(
          j - 1, current_level - 1, current_prod_val * factorLevels[j],
          factorLevels);
    }
  }

  return partial_sum;
}

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

class compute_partial_sum_task : public citcpp::detail::thread_pool::Task {
    typedef citcpp::detail::thread_pool::Task base_type;
    typedef compute_partial_sum_task this_type;

  public:
    compute_partial_sum_task() = delete;

    compute_partial_sum_task(int start_idx_for_next, int current_level,
                             const std::vector<unsigned int> *factorLevels,
                             std::atomic_ullong *num_combinations)
        : base_type(),
          start_idx_for_next_(start_idx_for_next),
          current_level_(current_level),
          factorLevels_(factorLevels),
          num_combinations_(num_combinations) {
      setCallable(*this);
    }

    compute_partial_sum_task(const this_type &) = delete;

    compute_partial_sum_task(this_type &&other)
        : base_type(std::move(other)),
          start_idx_for_next_(other.start_idx_for_next_),
          current_level_(other.current_level_),
          factorLevels_(other.factorLevels_),
          num_combinations_(other.num_combinations_) {
      setCallable(*this);
    }

    virtual ~compute_partial_sum_task() {}

    this_type &operator=(const this_type &) = delete;

    this_type &operator=(this_type &&) = delete;

    void operator()() {
      unsigned long long chunk_num_combos = recursive_combine_and_sum(
          start_idx_for_next_ - 1, current_level_ - 1,
          (*factorLevels_)[start_idx_for_next_], *factorLevels_);
      num_combinations_->fetch_add(chunk_num_combos, std::memory_order_acq_rel);
    }

  private:
    int start_idx_for_next_;
    int current_level_;
    const std::vector<unsigned int> *factorLevels_;
    std::atomic_ullong *num_combinations_;
};

class compute_partial_sum_with_parameter_map_task
    : public citcpp::detail::thread_pool::Task {
    typedef citcpp::detail::thread_pool::Task base_type;
    typedef compute_partial_sum_with_parameter_map_task this_type;

  public:
    compute_partial_sum_with_parameter_map_task() = delete;

    compute_partial_sum_with_parameter_map_task(
        int start_idx_for_next, int current_level,
        const std::vector<unsigned int> *factorLevels,
        const std::vector<unsigned int> *parameter_index_map,
        std::atomic_ullong *num_combinations)
        : base_type(),
          start_idx_for_next_(start_idx_for_next),
          current_level_(current_level),
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
          factorLevels_(other.factorLevels_),
          parameter_index_map_(other.parameter_index_map_),
          num_combinations_(other.num_combinations_) {
      setCallable(*this);
    }

    virtual ~compute_partial_sum_with_parameter_map_task() {}

    this_type &operator=(const this_type &) = delete;

    this_type &operator=(this_type &&) = delete;

    void operator()() {
      unsigned long long chunk_num_combos =
          recursive_combine_and_sum(start_idx_for_next_ - 1, current_level_ - 1,
                                    (*factorLevels_)[start_idx_for_next_],
                                    *factorLevels_, *parameter_index_map_);
      num_combinations_->fetch_add(chunk_num_combos, std::memory_order_acq_rel);
    }

  private:
    int start_idx_for_next_;
    int current_level_;
    const std::vector<unsigned int> *factorLevels_;
    const std::vector<unsigned int> *parameter_index_map_;
    std::atomic_ullong *num_combinations_;
};

}  // namespace

namespace citcpp {
namespace detail {

unsigned long long number_of_combinations_to_cover(const model &model,
                                                   unsigned int t) {
  return recursive_combine_and_sum(model.get_parameters().size() - 1, t - 1, 1,
                                   model.get_parameters());
}

unsigned long long number_of_combinations_to_cover(thread_pool &tp,
                                                   const model &model,
                                                   unsigned int t) {
  if (t < 2) {
    return number_of_combinations_to_cover(model, t);
  }

  const unsigned int numFactors = model.get_parameters().size();

  // Distribute the initial choices among threads
  // Each thread will compute a partial sum which we aggregate.
  std::atomic_ullong num_combinations = 0;

  task_group tg(tp.createTaskGroup());

  std::vector<compute_partial_sum_task> tasks;
  for (unsigned int i = numFactors - 1; i >= (t - 1); --i) {
    tasks.emplace_back(i, t - 1, &model.get_parameters(), &num_combinations);
    tg.spawn(i, &tasks.back());
  }
  tg.wait();

  return num_combinations;
}

unsigned long long number_of_combinations_to_cover(
    unsigned int current_param_idx, const model &model,
    const std::vector<unsigned int> &parameter_index_map, unsigned int t) {
  return recursive_combine_and_sum(current_param_idx - 1, t - 1, 1,
                                   model.get_parameters(), parameter_index_map);
}

unsigned long long number_of_combinations_to_cover(
    thread_pool &tp, unsigned int current_param_idx, const model &model,
    const std::vector<unsigned int> &parameter_index_map, unsigned int t) {
  if (t < 2) {
    return number_of_combinations_to_cover(current_param_idx, model,
                                           parameter_index_map, t);
  }

  const unsigned int numFactors = current_param_idx;

  // Distribute the initial choices among threads
  // Each thread will compute a partial sum which we aggregate.
  std::atomic_ullong num_combinations = 0;

  task_group tg(tp.createTaskGroup());

  std::vector<compute_partial_sum_with_parameter_map_task> tasks;
  for (unsigned int i = numFactors - 1; i >= (t - 1); --i) {
    tasks.emplace_back(i, t - 1, &model.get_parameters(), &parameter_index_map,
                       &num_combinations);
    tg.spawn(i, &tasks.back());
  }
  tg.wait();

  return num_combinations;
}

}  // namespace detail
}  // namespace citcpp
