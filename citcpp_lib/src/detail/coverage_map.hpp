#ifndef DETAIL_COVERAGE_MAP_HPP_
#define DETAIL_COVERAGE_MAP_HPP_

#include <algorithm>
#include <citcpp/function_ref.hpp>
#include <vector>

#include "binom_coeff_table.hpp"
#include "bitset.hpp"
#include "datatypes_config.hpp"
#include "functor_executor.hpp"
#include "internal_model.hpp"
#include "shared_constants.hpp"

namespace citcpp {
namespace detail {

class coverage_map_second_level : public bitset_uint64 {
  public:
    coverage_map_second_level() : bitset_uint64() {}

    coverage_map_second_level(size_type num_bits,
                              const param_vector& param_indices)
        : bitset_uint64(num_bits), param_indices_(param_indices) {}

    coverage_map_second_level(const coverage_map_second_level& other)
        : bitset_uint64(other), param_indices_(other.param_indices_) {}

    coverage_map_second_level(coverage_map_second_level&& other)
        : bitset_uint64(std::move(other)),
          param_indices_(std::move(other.param_indices_)) {}

    ~coverage_map_second_level() {}

    coverage_map_second_level& operator=(
        const coverage_map_second_level& other) {
      if (this != &other) {
        bitset_uint64::operator=(other);
        param_indices_ = other.param_indices_;
      }

      return *this;
    }

    coverage_map_second_level& operator=(coverage_map_second_level&& other) {
      if (this != &other) {
        bitset_uint64::operator=(std::move(other));
        param_indices_ = std::move(other.param_indices_);
      }

      return *this;
    }

    /**
     * Swaps this and the given other bitset.
     */
    void swap(coverage_map_second_level& other) {
      bitset_uint64::swap(other);
      std::swap(param_indices_, other.param_indices_);
    }

    const param_vector& get_parameter_indices() const { return param_indices_; }

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

    coverage_map_base(unsigned int n, unsigned int t,
                      const internal_model& model,
                      const std::vector<unsigned int>& parameter_index_map,
                      const binom_coeff_table& binomial_coeffs,
                      bool fixed_last_parameter);

    coverage_map_base(const coverage_map_base& other) = default;
    coverage_map_base(coverage_map_base&& other) = default;

    ~coverage_map_base() = default;

    coverage_map_base& operator=(const coverage_map_base& other) = default;
    coverage_map_base& operator=(coverage_map_base&& other) = default;

    const internal_model& get_model() const { return model_; }

    const std::vector<unsigned int>& get_parameter_index_map() const {
      return parameter_index_map_;
    }

    unsigned int get_number_of_parameters_to_select_from() const { return n_; }

    unsigned int get_number_of_parameters_to_select() const { return t_; }

    std::vector<coverage_map_second_level>& get_coverage_map() {
      return cov_map_;
    }

    const std::vector<coverage_map_second_level>& get_coverage_map() const {
      return cov_map_;
    }

    unsigned long long get_total_number_of_tuples() const {
      return total_num_tuples_;
    }

  protected:
    const unsigned long long size_;
    const internal_model& model_;
    const std::vector<unsigned int>& parameter_index_map_;
    const unsigned int n_;
    const unsigned int t_;
    std::vector<coverage_map_second_level> cov_map_;
    unsigned long long total_num_tuples_;
};

class coverage_map_iterator {
  public:
    coverage_map_iterator(coverage_map_base& cov_map) : cov_map_(cov_map) {}

    coverage_map_iterator(const coverage_map_iterator& other) = default;
    coverage_map_iterator(coverage_map_iterator&& other) = default;

    ~coverage_map_iterator() = default;

    coverage_map_iterator& operator=(const coverage_map_iterator& other) =
        default;
    coverage_map_iterator& operator=(coverage_map_iterator&& other) = default;

    template <class T_VISITOR>
    void visit_all_parameter_combinations(T_VISITOR& visitor) {
      for (coverage_map_base::second_level_type& value_combinations :
           cov_map_.get_coverage_map()) {
        if (!visitor(value_combinations)) {
          return;
        }
      }
    }

  private:
    coverage_map_base& cov_map_;
};

template <conc_is_void_functor_executor T_EXEC>
class coverage_map_parallel_iterator {
  public:
    coverage_map_parallel_iterator() = default;

    coverage_map_parallel_iterator(coverage_map_base& cov_map, T_EXEC& exec)
        : cov_map_(&cov_map), exec_(&exec), iterate_tasks_(), visitor_() {
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

    coverage_map_parallel_iterator(const coverage_map_parallel_iterator& other)
        : cov_map_(other.cov_map_),
          exec_(other.exec_),
          iterate_tasks_(other.iterate_tasks_),
          visitor_(other.visitor_) {

      for (auto& task : iterate_tasks_) {
        task.set_iterator(this);
      }
    }

    coverage_map_parallel_iterator(coverage_map_parallel_iterator&& other)
        : cov_map_(other.cov_map_),
          exec_(other.exec_),
          iterate_tasks_(std::move(other.iterate_tasks_)),
          visitor_(std::move(other.visitor_)) {

      for (auto& task : iterate_tasks_) {
        task.set_iterator(this);
      }
    }

    ~coverage_map_parallel_iterator() = default;

    coverage_map_parallel_iterator& operator=(
        const coverage_map_parallel_iterator& other) {

      if (this != &other) {
        cov_map_ = other.cov_map_;
        exec_ = other.exec_;
        iterate_tasks_ = other.iterate_tasks_;
        visitor_ = other.visitor_;

        for (auto& task : iterate_tasks_) {
          task.set_iterator(this);
        }
      }

      return *this;
    }

    coverage_map_parallel_iterator& operator=(
        coverage_map_parallel_iterator&& other) {

      if (this != &other) {
        cov_map_ = other.cov_map_;
        exec_ = other.exec_;
        iterate_tasks_ = std::move(other.iterate_tasks_);
        visitor_ = std::move(other.visitor_);

        for (auto& task : iterate_tasks_) {
          task.set_iterator(this);
        }
      }

      return *this;
    }

    unsigned int get_num_workers() const { return exec_->get_num_workers(); }

    unsigned int get_worker_id() const { return exec_->get_worker_id(); }

    template <class T_VISITOR>
    void visit_all_parameter_combinations(T_VISITOR& visitor) {
      visitor_ = visitor;

      auto exec_scope(exec_->create_execution_scope());
      exec_scope.spawn_execution(iterate_tasks_);
    }

  private:
    class alignas(false_sharing_avoidance_alignment) iterate_task
        : public functor_task_base<iterate_task> {

      private:
        typedef functor_task_base<iterate_task> base_type;

      public:
        iterate_task() = default;

        iterate_task(coverage_map_parallel_iterator* iterator,
                     unsigned long long start_index,
                     unsigned long long end_index)
            : base_type(),
              iterator_(iterator),
              start_index_(start_index),
              end_index_(end_index) {}

        virtual ~iterate_task() {}

        void operator()() {
          for (unsigned long long i = start_index_; i < end_index_; ++i) {
            coverage_map_base::second_level_type& value_combinations =
                iterator_->cov_map_->get_coverage_map()[i];

            if (!iterator_->visitor_(value_combinations)) {
              return;
            }
          }
        }

        void set_iterator(coverage_map_parallel_iterator* iterator) {
          iterator_ = iterator;
        }

      private:
        coverage_map_parallel_iterator* iterator_;
        unsigned long long start_index_;
        unsigned long long end_index_;
    };

    friend class iterate_task;

  private:
    coverage_map_base* cov_map_;
    T_EXEC* exec_;
    std::vector<iterate_task> iterate_tasks_;
    function_ref<bool(coverage_map_base::second_level_type&)> visitor_;
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
    coverage_map(unsigned int n, unsigned int t, const internal_model& model,
                 const std::vector<unsigned int>& parameter_index_map,
                 const binom_coeff_table& binomial_coeffs,
                 bool fixed_last_parameter)
        : base_type(n, t, model, parameter_index_map, binomial_coeffs,
                    fixed_last_parameter) {}

    coverage_map(const coverage_map& other) = default;
    coverage_map(coverage_map&& other) = default;

    ~coverage_map() = default;

    coverage_map& operator=(const coverage_map& other) = default;
    coverage_map& operator=(coverage_map&& other) = default;

    coverage_map_iterator create_iterator() {
      return coverage_map_iterator(*this);
    }

    template <conc_is_void_functor_executor T_EXEC>
    coverage_map_parallel_iterator<T_EXEC> create_parallel_iterator(
        T_EXEC& exec) {

      return coverage_map_parallel_iterator(*this, exec);
    }
};

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_COVERAGE_MAP_HPP_ */
