#ifndef DETAIL_COVERAGE_MAP_HPP_
#define DETAIL_COVERAGE_MAP_HPP_

#include <algorithm>
#include <new>
#include <vector>

#include "binom_coeff_table.hpp"
#include "bitset.hpp"
#include "datatypes_config.hpp"
#include "function_ref.hpp"
#include "internal_model.hpp"

namespace citcpp {
namespace detail {

class coverage_map_second_level : public bitset_uint64 {
  public:
    coverage_map_second_level() : bitset_uint64() {}

    coverage_map_second_level(size_type num_bits,
                              const param_vector &param_indices)
        : bitset_uint64(num_bits), param_indices_(param_indices) {}

    coverage_map_second_level(const coverage_map_second_level &other)
        : bitset_uint64(other), param_indices_(other.param_indices_) {}

    coverage_map_second_level(coverage_map_second_level &&other)
        : bitset_uint64(std::move(other)),
          param_indices_(std::move(other.param_indices_)) {}

    ~coverage_map_second_level() {}

    coverage_map_second_level &operator=(
        const coverage_map_second_level &other) {
      if (this != &other) {
        bitset_uint64::operator=(other);
        param_indices_ = other.param_indices_;
      }

      return *this;
    }

    coverage_map_second_level &operator=(coverage_map_second_level &&other) {
      if (this != &other) {
        bitset_uint64::operator=(std::move(other));
        param_indices_ = std::move(other.param_indices_);
      }

      return *this;
    }

    /**
     * Swaps this and the given other bitset.
     */
    void swap(coverage_map_second_level &other) {
      bitset_uint64::swap(other);
      std::swap(param_indices_, other.param_indices_);
    }

    const param_vector &get_parameter_indices() const { return param_indices_; }

  private:
    param_vector param_indices_;
};

/**
 * This is a quite central data structure in combinatorial testing tools.
 * It keeps track of the coverage of the parameter combinations and their cross
 * product of value combinations.
 * Since the number of value combinations for all t-way combinations of
 * parameters can be quite huge, this data structure is optimized for efficient
 * memory representation. At the same time, operations on the data structure
 * need to be lighting fast, again due to the vast amount of t-tuples whose
 * coverage to track.
 *
 * The coverage map is able to keep track of tuple coverage
 * of t-wise combinations from n parameters (indices [0, ... ,n-1]).
 * Depending on the value of the parameter \a fixed_last_parameter, the last
 * parameter is fixed. Or in other words: We select combinations of length t-1
 * from the parameters [0, ... ,n-2], and extend those combinations by always
 * appending parameter n-1 to them.
 */
class coverage_map_base {
  public:
    typedef std::vector<coverage_map_second_level>::size_type size_type;
    typedef coverage_map_second_level second_level_type;

    coverage_map_base(unsigned int n, unsigned int t, const model &model,
                      const std::vector<unsigned int> &parameter_index_map,
                      const binom_coeff_table &binomial_coeffs,
                      bool fixed_last_parameter);

    coverage_map_base(const coverage_map_base &other) = default;
    coverage_map_base(coverage_map_base &&other) = default;

    ~coverage_map_base() = default;

    coverage_map_base &operator=(const coverage_map_base &other) = default;
    coverage_map_base &operator=(coverage_map_base &&other) = default;

    const model &get_model() const { return model_; }

    const std::vector<unsigned int> &get_parameter_index_map() const {
      return parameter_index_map_;
    }

    unsigned int get_number_of_parameters_to_select_from() const { return n_; }

    unsigned int get_number_of_parameters_to_select() const { return t_; }

    std::vector<coverage_map_second_level> &get_coverage_map() {
      return cov_map_;
    }

    const std::vector<coverage_map_second_level> &get_coverage_map() const {
      return cov_map_;
    }

    unsigned long long get_total_number_of_tuples() const {
      return total_num_tuples_;
    }

  protected:
    const unsigned long long size_;
    const model &model_;
    const std::vector<unsigned int> &parameter_index_map_;
    const unsigned int n_;
    const unsigned int t_;
    std::vector<coverage_map_second_level> cov_map_;
    unsigned long long total_num_tuples_;
};

template <class T_VISITOR>
class value_combination_iterator {
  public:
    typedef coverage_map_base::size_type size_type;

    value_combination_iterator(const coverage_map_base &cov_map,
                               bool skip_fully_covered_param_combo,
                               T_VISITOR &visitor)
        : cov_map_(cov_map),
          skip_fully_covered_param_combo_(skip_fully_covered_param_combo),
          value_indices_(cov_map.get_number_of_parameters_to_select()),
          bit_pos_(0),
          visitor_(visitor) {}

    bool operator()(coverage_map_base::second_level_type &value_combinations) {
      if (!skip_fully_covered_param_combo_ || !value_combinations.all()) {
        bit_pos_ = 0;

        return recursively_visit_all_value_combos_of_param_combo(
            value_combinations, value_indices_.size() - 1, 0);
      }

      return true;
    }

    const value_vector &get_value_indices() const { return value_indices_; }

    size_type get_bitpos() const { return bit_pos_; }

  private:
    bool recursively_visit_all_value_combos_of_param_combo(
        coverage_map_base::second_level_type &value_combinations,
        int current_index, size_type partial_bit_pos) {

      // The current range goes from 0 to max_value[current_index]
      const unsigned int max_val =
          cov_map_.get_model().get_parameters()
              [value_combinations.get_parameter_indices()[current_index]];

      const param_vector &param_indices =
          value_combinations.get_parameter_indices();
      size_type bit_pos_value_factor = 1;
      for (std::vector<unsigned int>::size_type j = current_index + 1;
           j < param_indices.size(); ++j) {
        bit_pos_value_factor *=
            cov_map_.get_model().get_parameters()[param_indices[j]];
      }

      for (int i = max_val - 1; i >= 0; --i) {
        value_indices_[current_index] = i;

        bool ret = true;

        if (current_index == 0) {
          bit_pos_ = partial_bit_pos + i * bit_pos_value_factor;
          // We assume that the visitor is a functor accepting a reference to
          // this iterator. In addition
          ret = visitor_(value_combinations, value_indices_, bit_pos_);
        } else {
          ret = recursively_visit_all_value_combos_of_param_combo(
              value_combinations, current_index - 1,
              partial_bit_pos + i * bit_pos_value_factor);
        }

        if (!ret) {
          return false;
        }
      }

      return true;
    }

  private:
    const coverage_map_base &cov_map_;
    bool skip_fully_covered_param_combo_;
    value_vector value_indices_;
    size_type bit_pos_;
    T_VISITOR &visitor_;
};

class coverage_map_iterator {
  public:
    coverage_map_iterator(coverage_map_base &cov_map) : cov_map_(cov_map) {}

    coverage_map_iterator(const coverage_map_iterator &other) = default;
    coverage_map_iterator(coverage_map_iterator &&other) = default;

    ~coverage_map_iterator() = default;

    coverage_map_iterator &operator=(const coverage_map_iterator &other) =
        default;
    coverage_map_iterator &operator=(coverage_map_iterator &&other) = default;

    template <class T_VISITOR>
    void visit_all_parameter_combinations(T_VISITOR &visitor) {
      for (coverage_map_base::second_level_type &value_combinations :
           cov_map_.get_coverage_map()) {
        if (!visitor(value_combinations)) {
          return;
        }
      }
    }

    template <class T_VISITOR>
    void visit_all_tuples(bool skip_fully_covered_param_combo,
                          T_VISITOR &visitor) {
      value_combination_iterator<T_VISITOR> value_combo_it(
          cov_map_, skip_fully_covered_param_combo, visitor);

      for (coverage_map_base::second_level_type &value_combinations :
           cov_map_.get_coverage_map()) {
        if (!value_combo_it(value_combinations)) {
          return;
        }
      }
    }

  private:
    coverage_map_base &cov_map_;
};

class coverage_map_parallel_iterator {
  public:
    coverage_map_parallel_iterator(coverage_map_base &cov_map, thread_pool &tp)
        : cov_map_(cov_map), tp_(tp), iterate_tasks_(), visitor_() {
      const unsigned long long total_param_combos =
          cov_map.get_coverage_map().size();
      const unsigned long long num_tasks = std::min(
          (unsigned long long)cov_map.get_number_of_parameters_to_select_from(),
          total_param_combos);
      const unsigned long long per_task_combos = total_param_combos / num_tasks;

      for (unsigned int i = 0; i < num_tasks - 1; ++i) {
        iterate_tasks_.emplace_back(this, per_task_combos * i,
                                    per_task_combos * (i + 1));
      }

      iterate_tasks_.emplace_back(this, per_task_combos * (num_tasks - 1),
                                  total_param_combos);
    }

    coverage_map_parallel_iterator(
        const coverage_map_parallel_iterator &other) = default;
    coverage_map_parallel_iterator(coverage_map_parallel_iterator &&other) =
        default;

    ~coverage_map_parallel_iterator() = default;

    coverage_map_parallel_iterator &operator=(
        const coverage_map_parallel_iterator &other) = default;
    coverage_map_parallel_iterator &operator=(
        coverage_map_parallel_iterator &&other) = default;

    unsigned int get_num_workers() const { return tp_.get_num_workers(); }

    unsigned int get_worker_id() const { return tp_.get_worker_id(); }

    template <class T_VISITOR>
    void visit_all_parameter_combinations(T_VISITOR &visitor) {
      visitor_ = visitor;

      task_group tg(tp_.createTaskGroup());
      for (unsigned int i = 1; i < iterate_tasks_.size(); ++i) {
        iterate_task &task = iterate_tasks_[i];
        task.reset();
        tg.spawn(i, &task);
      }
      iterate_task &task = iterate_tasks_[0];
      task.reset();
      tg.spawn_and_wait(&task);
    }

  private:
    class alignas(std::hardware_destructive_interference_size) iterate_task
        : public thread_pool::Task {
      private:
        typedef thread_pool::Task base_type;
        typedef iterate_task this_type;

      public:
        iterate_task() = delete;

        iterate_task(coverage_map_parallel_iterator *iterator,
                     unsigned long long start_index,
                     unsigned long long end_index)
            : base_type(),
              iterator_(iterator),
              start_index_(start_index),
              end_index_(end_index) {
          setCallable(*this);
        }

        iterate_task(const this_type &) = delete;

        iterate_task(this_type &&other)
            : base_type(std::move(other)),
              iterator_(other.iterator_),
              start_index_(other.start_index_),
              end_index_(other.end_index_) {
          setCallable(*this);
        }

        virtual ~iterate_task() {}

        this_type &operator=(const this_type &) = delete;
        this_type &operator=(this_type &&) = delete;

        void operator()() {
          for (unsigned long long i = start_index_; i < end_index_; ++i) {
            coverage_map_base::second_level_type &value_combinations =
                iterator_->cov_map_.get_coverage_map()[i];

            if (!iterator_->visitor_(value_combinations)) {
              return;
            }
          }
        }

      private:
        coverage_map_parallel_iterator *iterator_;
        const unsigned long long start_index_;
        const unsigned long long end_index_;
    };

    friend class iterate_task;

  private:
    coverage_map_base &cov_map_;
    thread_pool &tp_;
    std::vector<iterate_task> iterate_tasks_;
    function_ref<bool(coverage_map_base::second_level_type &)> visitor_;
};

/**
 * This is a quite central data structure in combinatorial testing tools.
 * It keeps track of the coverage of the parameter combinations and their cross
 * product of value combinations.
 * Since the number of value combinations for all t-way combinations of
 * parameters can be quite huge, this data structure is optimized for efficient
 * memory representation. At the same time, operations on the data structure
 * need to be lighting fast, again due to the vast amount of t-tuples whose
 * coverage to track.
 *
 * The coverage map is able to keep track of tuple coverage
 * of t-wise combinations from n parameters (indices [0, ... ,n-1]).
 * Depending on the value of the parameter \a fixed_last_parameter, the last
 * parameter is fixed. Or in other words: We select combinations of length t-1
 * from the parameters [0, ... ,n-2], and extend those combinations by always
 * appending parameter n-1 to them.
 */
class coverage_map : public coverage_map_base {
    typedef coverage_map_base base_type;

  public:
    coverage_map(unsigned int n, unsigned int t, const model &model,
                 const std::vector<unsigned int> &parameter_index_map,
                 const binom_coeff_table &binomial_coeffs,
                 bool fixed_last_parameter)
        : base_type(n, t, model, parameter_index_map, binomial_coeffs,
                    fixed_last_parameter) {}

    coverage_map(const coverage_map &other) = default;
    coverage_map(coverage_map &&other) = default;

    ~coverage_map() = default;

    coverage_map &operator=(const coverage_map &other) = default;
    coverage_map &operator=(coverage_map &&other) = default;

    coverage_map_iterator create_iterator() {
      return coverage_map_iterator(*this);
    }

    coverage_map_parallel_iterator create_parallel_iterator(thread_pool &tp) {
      return coverage_map_parallel_iterator(*this, tp);
    }
};

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_COVERAGE_MAP_HPP_ */
