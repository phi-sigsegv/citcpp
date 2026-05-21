#include <algorithm>

#include "citcpp_utils.hpp"
#include "ipog_horizontal_extension.hpp"
#include "shared_constants.hpp"

namespace citcpp {
namespace detail {

class ipog_horizontal_select_best_value_per_param_combo_functor {
  public:
    ipog_horizontal_select_best_value_per_param_combo_functor(
        const internal_model& model,
        const unsigned int num_current_param_values)
        : model_(model), gain_per_value_(num_current_param_values) {}

    void operator()(
        const test& test, const bitset_uint64& valid_values,
        const ipog_coverage_map::second_level_type& value_combinations) {

      const param_vector& param_indices =
          value_combinations.get_parameter_indices();

      if (!value_combinations.all_covered()) {
        // We have a bitset and we have uncovered value combinations left in it.
        // Thus we have to walk through it concerning all possible value
        // combinations.
        // Here we compute an index into the bitset. To do so, we treat the
        // number of values of each parameter as a kind of radix. Consider three
        // parameters p_0, p_1, p_2. The last parameter is always the current
        // one processed by IPOG. Now say that v_i is the number of values for
        // p_i. If we now have values x_0, x_1, x_2, then the index is x_0 * v_1
        // * v_2 + x_1 * v_2 + x_2. In the base index we just compute x_0 * v_1
        // * v_2 + x_1 * v_2, since that expression is constant throughout all
        // different values of p_2 whose different coverage gains we want to
        // assess.
        ipog_coverage_map::second_level_type::size_type base_index = 0;
        for (std::vector<unsigned int>::size_type i = 0;
             i < param_indices.size() - 1; ++i) {
          const unsigned int param_idx = param_indices[i];
          const int param_value = test.get_values()[param_idx];

          if (param_value < 0) {
            // We have found a don't care value for that combination in
            // the considered test.
            return;
          }

          ipog_coverage_map::second_level_type::size_type addend = param_value;
          for (std::vector<unsigned int>::size_type j = i + 1;
               j < param_indices.size(); ++j) {
            addend *= model_.get_parameter_num_values()[param_indices[j]];
          }
          base_index += addend;
        }

        // If we have found a don't care value in one of the [0, ...
        // ,current_param_idx - 1] parameters, then we skip the combination in
        // the coverage gain computation.
        for (unsigned int value = 0; value < gain_per_value_.size(); ++value) {
          if (!value_combinations.is_marked_covered(base_index + value)) {
            if (valid_values.test(value)) {
              gain_per_value_[value] += 1;
            }
          }
        }
      }
    }

    const std::vector<unsigned long long>& get_gain_per_value() const {
      return gain_per_value_;
    }

    std::vector<unsigned long long>& get_gain_per_value() {
      return gain_per_value_;
    }

    void reset() {
      std::fill(gain_per_value_.begin(), gain_per_value_.end(), 0);
    }

  private:
    const internal_model& model_;
    std::vector<unsigned long long> gain_per_value_;
};

inline int ipog_horizontal_select_best_value(
    const unsigned int num_current_param_values,
    const std::vector<unsigned long long>& gain_per_value,
    unsigned int& last_picked_value, std::vector<int>& value_to_valid_options) {

  int value_with_max_gain = -1;
  unsigned long long max_gain = 0;
  for (unsigned int v_index = 0; v_index < num_current_param_values;
       ++v_index) {
    unsigned int value =
        (v_index + last_picked_value + 1) % num_current_param_values;
    if (gain_per_value[value] > max_gain) {
      value_with_max_gain = value;
      max_gain = gain_per_value[value];
    } else if (gain_per_value[value] == max_gain) {
      // We use a simple tie breaking strategy: We do not favor one value over
      // the other. If two values have the same gain, then we pick the one which
      // we have picked less so far. Since also this could be a tie (we have
      // picked the value the same number of times, we remember the value we
      // have picked before, and choose the next one in this case.
      if (value_with_max_gain >= 0 &&
          value_to_valid_options[value] >
              value_to_valid_options[value_with_max_gain]) {
        value_with_max_gain = value;
      }
    }
  }

  if (value_with_max_gain >= 0) {
    last_picked_value = value_with_max_gain;
    value_to_valid_options[value_with_max_gain]--;
  }

  return value_with_max_gain;
}

class ipog_horizontal_select_best_value_functor {
  public:
    ipog_horizontal_select_best_value_functor(
        const unsigned int real_current_param_idx,
        const unsigned int num_current_param_values,
        const internal_model& model,
        const std::vector<
            std::pair<const internal_relation*, ipog_coverage_map>>& relations,
        unsigned int& last_picked_value,
        std::vector<int>& value_to_valid_options)
        : real_current_param_idx_(real_current_param_idx),
          num_current_param_values_(num_current_param_values),
          per_param_combo_functor_(model, num_current_param_values),
          relations_(relations),
          last_picked_value_(last_picked_value),
          value_to_valid_options_(value_to_valid_options) {}

    int operator()(const test& test, const bitset_uint64& valid_values) {
      // We first check whether the test already has a concrete value
      // for the current parameter, because if so, then there is no
      // point in evaluating a coverage gain.
      if (test.get_values()[real_current_param_idx_] >= 0) {
        last_picked_value_ = test.get_values()[real_current_param_idx_];
        value_to_valid_options_[last_picked_value_]--;

        return last_picked_value_;
      }

      for (const auto& rel_and_cov_map : relations_) {
        for (const auto& value_combinations :
             rel_and_cov_map.second.get_coverage_map()) {
          per_param_combo_functor_(test, valid_values, value_combinations);
        }
      }

      // This is an array containing the coverage gain per value of the current
      // parameter.
      const std::vector<unsigned long long>& gain_per_value =
          per_param_combo_functor_.get_gain_per_value();

      const int selected_value = ipog_horizontal_select_best_value(
          num_current_param_values_, gain_per_value, last_picked_value_,
          value_to_valid_options_);

      per_param_combo_functor_.reset();

      return selected_value;
    }

  private:
    const unsigned int real_current_param_idx_;
    const unsigned int num_current_param_values_;
    ipog_horizontal_select_best_value_per_param_combo_functor
        per_param_combo_functor_;
    const std::vector<std::pair<const internal_relation*, ipog_coverage_map>>&
        relations_;
    unsigned int& last_picked_value_;
    std::vector<int>& value_to_valid_options_;
};

template <conc_is_void_functor_executor T_EXEC>
class ipog_horizontal_select_best_value_functor_parallel {
  public:
    ipog_horizontal_select_best_value_functor_parallel(
        const unsigned int real_current_param_idx,
        const unsigned int num_current_param_values,
        const internal_model& model,
        const std::vector<
            std::pair<const internal_relation*, ipog_coverage_map>>& relations,
        unsigned int& last_picked_value,
        std::vector<int>& value_to_valid_options, T_EXEC& exec)
        : thread_local_functors_(exec.get_num_workers() * 8,
                                 {model, num_current_param_values}),
          real_current_param_idx_(real_current_param_idx),
          num_current_param_values_(num_current_param_values),
          relations_(relations),
          last_picked_value_(last_picked_value),
          value_to_valid_options_(value_to_valid_options),
          exec_(exec) {}

    int operator()(const test& test, const bitset_uint64& valid_values) {
      // We first check whether the test already has a concrete value
      // for the current parameter, because if so, then there is no
      // point in evaluating a coverage gain.
      if (test.get_values()[real_current_param_idx_] >= 0) {
        last_picked_value_ = test.get_values()[real_current_param_idx_];
        value_to_valid_options_[last_picked_value_]--;

        return last_picked_value_;
      }

      unsigned long long max_num_tasks = 0;

      for (const auto& rel_and_cov_map : relations_) {
        const ipog_coverage_map* cov_map = &rel_and_cov_map.second;
        const unsigned long long total_param_combos =
            cov_map->get_coverage_map().size();
        const unsigned long long num_tasks =
            std::min((unsigned long long)thread_local_functors_.size(),
                     total_param_combos);
        const unsigned long long per_task_combos =
            total_param_combos / num_tasks;

        max_num_tasks = std::max(max_num_tasks, num_tasks);

        auto exec_scope(exec_.create_execution_scope());
        for (unsigned long long i = 0; i < num_tasks - 1; ++i) {
          auto& thread_local_func = thread_local_functors_[i];
          thread_local_func.set_task_parameters(&test, &valid_values, cov_map,
                                                per_task_combos * i,
                                                per_task_combos * (i + 1));
          exec_scope.spawn_execution(thread_local_func);
        }
        {
          auto& thread_local_func = thread_local_functors_[num_tasks - 1];
          thread_local_func.set_task_parameters(
              &test, &valid_values, cov_map, per_task_combos * (num_tasks - 1),
              total_param_combos);
          exec_scope.spawn_execution(thread_local_func);
        }
      }

      // This is an array containing the coverage gain per value of the current
      // parameter.
      std::vector<unsigned long long> gain_per_value(num_current_param_values_);

      for (unsigned long long i = 0; i < max_num_tasks; ++i) {
        auto& thread_local_functor = thread_local_functors_[i];
        for (int v = 0; v < gain_per_value.size(); ++v) {
          gain_per_value[v] += thread_local_functor.get_gain_per_value()[v];
        }

        thread_local_functor.reset();
      }

      const int selected_value = ipog_horizontal_select_best_value(
          num_current_param_values_, gain_per_value, last_picked_value_,
          value_to_valid_options_);

      return selected_value;
    }

  private:
    class alignas(false_sharing_avoidance_alignment) value_selection_task
        : public functor_task_base<value_selection_task> {

      private:
        typedef functor_task_base<value_selection_task> base_type;

      public:
        value_selection_task(const internal_model& model,
                             const unsigned int num_current_param_values)
            : per_param_combo_functor_(model, num_current_param_values),
              test_(nullptr),
              valid_values_(nullptr),
              cov_map_(nullptr),
              start_index_(0),
              end_index_(0) {}

        void operator()() {
          const auto& test = *test_;
          const auto& valid_values = *valid_values_;
          const auto& cov_map = cov_map_->get_coverage_map();

          for (unsigned long long i = start_index_; i < end_index_; ++i) {
            per_param_combo_functor_(test, valid_values, cov_map[i]);
          }
        }

        void set_task_parameters(const test* test,
                                 const bitset_uint64* valid_values,
                                 const ipog_coverage_map* cov_map,
                                 unsigned long long start_index,
                                 unsigned long long end_index) {
          base_type::reset();
          test_ = test;
          valid_values_ = valid_values;
          cov_map_ = cov_map;
          start_index_ = start_index;
          end_index_ = end_index;
        }

        const std::vector<unsigned long long>& get_gain_per_value() const {
          return per_param_combo_functor_.get_gain_per_value();
        }

        void reset() { per_param_combo_functor_.reset(); }

      private:
        ipog_horizontal_select_best_value_per_param_combo_functor
            per_param_combo_functor_;
        const test* test_;
        const bitset_uint64* valid_values_;
        const ipog_coverage_map* cov_map_;
        unsigned long long start_index_;
        unsigned long long end_index_;
    };

  private:
    alignas(false_sharing_avoidance_alignment)
        thread_local_vector<value_selection_task> thread_local_functors_;
    const unsigned int real_current_param_idx_;
    const unsigned int num_current_param_values_;
    const std::vector<std::pair<const internal_relation*, ipog_coverage_map>>&
        relations_;
    unsigned int& last_picked_value_;
    std::vector<int>& value_to_valid_options_;
    T_EXEC& exec_;
};

class ipog_horizontal_update_coverage_map_per_param_combo_functor {
  public:
    ipog_horizontal_update_coverage_map_per_param_combo_functor(
        const internal_model& model)
        : model_(model), num_new_covered_tuples_(0) {}

    void operator()(const test& test, const bitset_uint64& valid_values,
                    ipog_coverage_map::second_level_type& value_combinations) {

      const param_vector& param_indices =
          value_combinations.get_parameter_indices();

      if (!value_combinations.all_covered()) {
        // Here we compute an index into the bitset. To do so, we treat the
        // number of values of each parameter as a kind of radix. Consider
        // three parameters p_0, p_1, p_2. Now say that v_i is the number of
        // values for p_i. If we now have values x_0, x_1, x_2, then the
        // index is x_0 * v_1 * v_2 + x_1 * v_2 + x_2. In the base index we just
        // compute x_0 * v_1 * v_2 + x_1 * v_2, since that expression is
        // constant throughout all different values of p_2.
        ipog_coverage_map::second_level_type::size_type base_index = 0;
        for (std::vector<unsigned int>::size_type i = 0;
             i < param_indices.size() - 1; ++i) {
          const unsigned int param_idx = param_indices[i];
          const int param_value = test.get_values()[param_idx];

          if (param_value < 0) {
            // We have found a don't care value for that combination in
            // the considered test in one of the [0, ... ,current_param_idx - 1]
            // parameters. There is nothing to be updated concerning the
            // coverage. This combination will be taken care of during the
            // vertical extension step.
            return;
          }

          ipog_coverage_map::second_level_type::size_type addend = param_value;
          for (std::vector<unsigned int>::size_type j = i + 1;
               j < param_indices.size(); ++j) {
            addend *= model_.get_parameter_num_values()[param_indices[j]];
          }
          base_index += addend;
        }

        const unsigned int real_current_param_idx =
            param_indices[param_indices.size() - 1];
        const int current_param_value =
            test.get_values()[real_current_param_idx];
        if (current_param_value < 0) {
          // We have found a don't care value for that combination in
          // the considered test in one of the [0, ... ,current_param_idx - 1]
          // parameters. There is nothing to be updated concerning the
          // coverage. This combination will be taken care of during the
          // vertical extension step.
          return;
        }

        if (!value_combinations.test_and_set_covered(base_index +
                                                     current_param_value)) {
          ++num_new_covered_tuples_;
        }

        for (unsigned int value = 0; value < valid_values.size(); ++value) {
          if (valid_values.test(value)) {
            value_combinations.set_valid(base_index + value);
          }
        }
      }
    }

    unsigned long long get_num_new_covered_tuples() const {
      return num_new_covered_tuples_;
    }

    void reset() { num_new_covered_tuples_ = 0; }

  private:
    const internal_model& model_;
    unsigned long long num_new_covered_tuples_;
};

class ipog_horizontal_update_coverage_map_functor {
  public:
    ipog_horizontal_update_coverage_map_functor(
        const unsigned int real_current_param_idx, const internal_model& model,
        std::vector<std::pair<const internal_relation*, ipog_coverage_map>>&
            relations,
        std::unordered_map<const internal_relation*, unsigned long long>&
            num_covered_tuples)
        : per_param_combo_functor_(model),
          real_current_param_idx_(real_current_param_idx),
          relations_(relations),
          num_covered_tuples_(num_covered_tuples) {}

    void operator()(const test& test, const bitset_uint64& valid_values) {
      // Only if the value at the current parameter is different from
      // don't care, we have a gain in coverage and thus need to
      // update the coverage map.
      if (test.get_values()[real_current_param_idx_] >= 0) {
        for (auto& rel_and_cov_map : relations_) {
          for (auto& value_combinations :
               rel_and_cov_map.second.get_coverage_map()) {
            per_param_combo_functor_(test, valid_values, value_combinations);
          }

          num_covered_tuples_[rel_and_cov_map.first] +=
              per_param_combo_functor_.get_num_new_covered_tuples();

          per_param_combo_functor_.reset();
        }
      }
    }

  private:
    ipog_horizontal_update_coverage_map_per_param_combo_functor
        per_param_combo_functor_;
    const unsigned int real_current_param_idx_;
    std::vector<std::pair<const internal_relation*, ipog_coverage_map>>&
        relations_;
    std::unordered_map<const internal_relation*, unsigned long long>&
        num_covered_tuples_;
};

template <conc_is_void_functor_executor T_EXEC>
class ipog_horizontal_update_coverage_map_functor_parallel {
  public:
    ipog_horizontal_update_coverage_map_functor_parallel(
        const unsigned int real_current_param_idx, const internal_model& model,
        std::vector<std::pair<const internal_relation*, ipog_coverage_map>>&
            relations,
        std::unordered_map<const internal_relation*, unsigned long long>&
            num_covered_tuples,
        T_EXEC& exec)
        : thread_local_functors_(exec.get_num_workers() * 8, {model}),
          real_current_param_idx_(real_current_param_idx),
          relations_(relations),
          num_covered_tuples_(num_covered_tuples),
          exec_(exec) {}

    void operator()(const test& test, const bitset_uint64& valid_values) {
      // Only if the value at the current parameter is different from
      // don't care, we have a gain in coverage and thus need to
      // update the coverage map.
      if (test.get_values()[real_current_param_idx_] >= 0) {
        for (auto& rel_and_cov_map : relations_) {
          ipog_coverage_map* cov_map = &rel_and_cov_map.second;
          const unsigned long long total_param_combos =
              cov_map->get_coverage_map().size();
          const unsigned long long num_tasks =
              std::min((unsigned long long)thread_local_functors_.size(),
                       total_param_combos);
          const unsigned long long per_task_combos =
              total_param_combos / num_tasks;

          {
            auto exec_scope(exec_.create_execution_scope());
            for (unsigned long long i = 0; i < num_tasks - 1; ++i) {
              auto& thread_local_func = thread_local_functors_[i];
              thread_local_func.set_task_parameters(
                  &test, &valid_values, cov_map, per_task_combos * i,
                  per_task_combos * (i + 1));
              exec_scope.spawn_execution(thread_local_func);
            }
            {
              auto& thread_local_func = thread_local_functors_[num_tasks - 1];
              thread_local_func.set_task_parameters(
                  &test, &valid_values, cov_map,
                  per_task_combos * (num_tasks - 1), total_param_combos);
              exec_scope.spawn_execution(thread_local_func);
            }
          }

          for (unsigned long long i = 0; i < num_tasks; ++i) {
            auto& thread_local_functor = thread_local_functors_[i];
            num_covered_tuples_[rel_and_cov_map.first] +=
                thread_local_functor.get_num_new_covered_tuples();

            thread_local_functor.reset();
          }
        }
      }
    }

  private:
    class alignas(false_sharing_avoidance_alignment) value_selection_task
        : public functor_task_base<value_selection_task> {

      private:
        typedef functor_task_base<value_selection_task> base_type;

      public:
        value_selection_task(const internal_model& model)
            : per_param_combo_functor_(model),
              test_(nullptr),
              valid_values_(nullptr),
              cov_map_(nullptr),
              start_index_(0),
              end_index_(0) {}

        void operator()() {
          const auto& test = *test_;
          const auto& valid_values = *valid_values_;
          auto& cov_map = cov_map_->get_coverage_map();

          for (unsigned long long i = start_index_; i < end_index_; ++i) {
            per_param_combo_functor_(test, valid_values, cov_map[i]);
          }
        }

        void set_task_parameters(const test* test,
                                 const bitset_uint64* valid_values,
                                 ipog_coverage_map* cov_map,
                                 unsigned long long start_index,
                                 unsigned long long end_index) {
          base_type::reset();
          test_ = test;
          valid_values_ = valid_values;
          cov_map_ = cov_map;
          start_index_ = start_index;
          end_index_ = end_index;
        }

        unsigned long long get_num_new_covered_tuples() const {
          return per_param_combo_functor_.get_num_new_covered_tuples();
        }

        void reset() { per_param_combo_functor_.reset(); }

      private:
        ipog_horizontal_update_coverage_map_per_param_combo_functor
            per_param_combo_functor_;
        const test* test_;
        const bitset_uint64* valid_values_;
        ipog_coverage_map* cov_map_;
        unsigned long long start_index_;
        unsigned long long end_index_;
    };

  private:
    alignas(false_sharing_avoidance_alignment)
        thread_local_vector<value_selection_task> thread_local_functors_;
    const unsigned int real_current_param_idx_;
    std::vector<std::pair<const internal_relation*, ipog_coverage_map>>&
        relations_;
    std::unordered_map<const internal_relation*, unsigned long long>&
        num_covered_tuples_;
    T_EXEC& exec_;
};

class
    ipog_horizontal_update_coverage_map_and_select_best_value_per_param_combo_functor {
  public:
    ipog_horizontal_update_coverage_map_and_select_best_value_per_param_combo_functor(
        const internal_model& model,
        const unsigned int num_current_param_values)
        : covm_update_func_(model),
          best_value_selection_func_(model, num_current_param_values) {}

    const ipog_horizontal_update_coverage_map_per_param_combo_functor&
    get_coverage_update_functor() const {
      return covm_update_func_;
    }

    ipog_horizontal_update_coverage_map_per_param_combo_functor&
    get_coverage_update_functor() {
      return covm_update_func_;
    }

    const ipog_horizontal_select_best_value_per_param_combo_functor&
    get_select_best_value_functor() const {
      return best_value_selection_func_;
    }

    ipog_horizontal_select_best_value_per_param_combo_functor&
    get_select_best_value_functor() {
      return best_value_selection_func_;
    }

  private:
    ipog_horizontal_update_coverage_map_per_param_combo_functor
        covm_update_func_;
    ipog_horizontal_select_best_value_per_param_combo_functor
        best_value_selection_func_;
};

class ipog_horizontal_update_coverage_map_and_select_best_value_functor {
  public:
    ipog_horizontal_update_coverage_map_and_select_best_value_functor(
        const unsigned int real_current_param_idx,
        const unsigned int num_current_param_values,
        const internal_model& model,
        std::vector<std::pair<const internal_relation*, ipog_coverage_map>>&
            relations,
        unsigned int& last_picked_value,
        std::vector<int>& value_to_valid_options,
        std::unordered_map<const internal_relation*, unsigned long long>&
            num_covered_tuples)
        : real_current_param_idx_(real_current_param_idx),
          num_current_param_values_(num_current_param_values),
          per_param_combo_functor_(model, num_current_param_values),
          relations_(relations),
          last_picked_value_(last_picked_value),
          value_to_valid_options_(value_to_valid_options),
          num_covered_tuples_(num_covered_tuples) {}

    new_covered_tuples_and_selected_value operator()(
        const test& prev_test, const test& test,
        const bitset_uint64& prev_test_valid_values,
        const bitset_uint64& valid_values) {

      const int current_param_value =
          test.get_values()[real_current_param_idx_];

      // Only if the value at the current parameter is different from
      // don't care, we have a gain in coverage and thus need to
      // update the coverage map.
      const bool enable_coverage_update =
          prev_test.get_values()[real_current_param_idx_] >= 0;

      new_covered_tuples_and_selected_value res{0, 0};

      for (auto& rel_and_cov_map : relations_) {
        for (auto& value_combinations :
             rel_and_cov_map.second.get_coverage_map()) {

          if (enable_coverage_update) {
            per_param_combo_functor_.get_coverage_update_functor()(
                prev_test, prev_test_valid_values, value_combinations);
          }
          if (current_param_value < 0) {
            per_param_combo_functor_.get_select_best_value_functor()(
                test, valid_values, value_combinations);
          }
        }

        const unsigned long long num_new_covered_tuples =
            per_param_combo_functor_.get_coverage_update_functor()
                .get_num_new_covered_tuples();
        num_covered_tuples_[rel_and_cov_map.first] += num_new_covered_tuples;
        res.num_new_covered_tuples_ += num_new_covered_tuples;

        per_param_combo_functor_.get_coverage_update_functor().reset();
      }

      // This is an array containing the coverage gain per value of the current
      // parameter.
      std::vector<unsigned long long>& gain_per_value =
          per_param_combo_functor_.get_select_best_value_functor()
              .get_gain_per_value();

      // Check whether the test already has a concrete value
      // for the current parameter, because if so, then there is no
      // point in evaluating a coverage gain.
      if (current_param_value >= 0) {
        // The coverage gain computation is disabled in the functor,
        // so that it won't modify this gain info.
        gain_per_value[current_param_value]++;
      }

      res.selected_value_ = ipog_horizontal_select_best_value(
          num_current_param_values_, gain_per_value, last_picked_value_,
          value_to_valid_options_);

      per_param_combo_functor_.get_select_best_value_functor().reset();

      return res;
    }

  private:
    const unsigned int real_current_param_idx_;
    const unsigned int num_current_param_values_;
    ipog_horizontal_update_coverage_map_and_select_best_value_per_param_combo_functor
        per_param_combo_functor_;
    std::vector<std::pair<const internal_relation*, ipog_coverage_map>>&
        relations_;
    unsigned int& last_picked_value_;
    std::vector<int>& value_to_valid_options_;
    std::unordered_map<const internal_relation*, unsigned long long>&
        num_covered_tuples_;
};

template <conc_is_void_functor_executor T_EXEC>
class
    ipog_horizontal_update_coverage_map_and_select_best_value_functor_parallel {

  public:
    ipog_horizontal_update_coverage_map_and_select_best_value_functor_parallel(
        const unsigned int real_current_param_idx,
        const unsigned int num_current_param_values,
        const internal_model& model,
        std::vector<std::pair<const internal_relation*, ipog_coverage_map>>&
            relations,
        unsigned int& last_picked_value,
        std::vector<int>& value_to_valid_options,
        std::unordered_map<const internal_relation*, unsigned long long>&
            num_covered_tuples,
        T_EXEC& exec)
        : thread_local_functors_(exec.get_num_workers() * 8,
                                 {model, num_current_param_values}),
          real_current_param_idx_(real_current_param_idx),
          num_current_param_values_(num_current_param_values),
          relations_(relations),
          last_picked_value_(last_picked_value),
          value_to_valid_options_(value_to_valid_options),
          num_covered_tuples_(num_covered_tuples),
          exec_(exec) {}

    new_covered_tuples_and_selected_value operator()(
        const test& prev_test, const test& test,
        const bitset_uint64& prev_test_valid_values,
        const bitset_uint64& valid_values) {

      const int current_param_value =
          test.get_values()[real_current_param_idx_];

      // Only if the value at the current parameter is different from
      // don't care, we have a gain in coverage and thus need to
      // update the coverage map.
      const bool enable_coverage_update =
          prev_test.get_values()[real_current_param_idx_] >= 0;

      new_covered_tuples_and_selected_value res{0, 0};

      unsigned long long max_num_tasks = 0;

      for (auto& rel_and_cov_map : relations_) {
        ipog_coverage_map* cov_map = &rel_and_cov_map.second;
        const unsigned long long total_param_combos =
            cov_map->get_coverage_map().size();
        const unsigned long long num_tasks =
            std::min((unsigned long long)thread_local_functors_.size(),
                     total_param_combos);
        const unsigned long long per_task_combos =
            total_param_combos / num_tasks;

        max_num_tasks = std::max(max_num_tasks, num_tasks);

        {
          auto exec_scope(exec_.create_execution_scope());
          for (unsigned long long i = 0; i < num_tasks - 1; ++i) {
            auto& thread_local_func = thread_local_functors_[i];
            thread_local_func.set_task_parameters(
                enable_coverage_update ? &prev_test : nullptr,
                current_param_value < 0 ? &test : nullptr,
                &prev_test_valid_values, &valid_values, cov_map,
                per_task_combos * i, per_task_combos * (i + 1));
            exec_scope.spawn_execution(thread_local_func);
          }
          {
            auto& thread_local_func = thread_local_functors_[num_tasks - 1];
            thread_local_func.set_task_parameters(
                enable_coverage_update ? &prev_test : nullptr,
                current_param_value < 0 ? &test : nullptr,
                &prev_test_valid_values, &valid_values, cov_map,
                per_task_combos * (num_tasks - 1), total_param_combos);
            exec_scope.spawn_execution(thread_local_func);
          }
        }

        if (enable_coverage_update) {
          for (unsigned long long i = 0; i < num_tasks; ++i) {
            auto& thread_local_functor = thread_local_functors_[i];
            const unsigned long long num_new_covered_tuples =
                thread_local_functor.get_coverage_update_functor()
                    .get_num_new_covered_tuples();
            num_covered_tuples_[rel_and_cov_map.first] +=
                num_new_covered_tuples;
            res.num_new_covered_tuples_ += num_new_covered_tuples;

            thread_local_functor.get_coverage_update_functor().reset();
          }
        }
      }

      // This is an array containing the coverage gain per value of the
      // current parameter.
      std::vector<unsigned long long> gain_per_value(num_current_param_values_);

      // Check whether the test already has a concrete value
      // for the current parameter, because if so, then there is no
      // point in evaluating a coverage gain.
      if (current_param_value >= 0) {
        // The coverage gain computation was disabled in the functor execution,
        // so gain info is inconclusive here.
        // We simply increase the gain of the already
        // selected parameter value, to have the subsequent logic
        // do the correct modifications of the state of the value
        // selection heuristics.
        gain_per_value[current_param_value]++;
      } else {
        for (unsigned long long i = 0; i < max_num_tasks; ++i) {
          auto& thread_local_functor = thread_local_functors_[i];
          for (int v = 0; v < gain_per_value.size(); ++v) {
            gain_per_value[v] +=
                thread_local_functor.get_select_best_value_functor()
                    .get_gain_per_value()[v];
          }

          thread_local_functor.get_select_best_value_functor().reset();
        }
      }

      res.selected_value_ = ipog_horizontal_select_best_value(
          num_current_param_values_, gain_per_value, last_picked_value_,
          value_to_valid_options_);

      return res;
    }

  private:
    class alignas(false_sharing_avoidance_alignment) value_selection_task
        : public functor_task_base<value_selection_task> {

      private:
        typedef functor_task_base<value_selection_task> base_type;

      public:
        value_selection_task(const internal_model& model,
                             const unsigned int num_current_param_values)
            : per_param_combo_functor_(model, num_current_param_values),
              prev_test_(nullptr),
              test_(nullptr),
              prev_test_valid_values_(nullptr),
              valid_values_(nullptr),
              cov_map_(nullptr),
              start_index_(0),
              end_index_(0) {}

        void operator()() {
          auto& cov_map = cov_map_->get_coverage_map();

          for (unsigned long long i = start_index_; i < end_index_; ++i) {
            if (prev_test_) {
              per_param_combo_functor_.get_coverage_update_functor()(
                  *prev_test_, *prev_test_valid_values_, cov_map[i]);
            }
            if (test_) {
              per_param_combo_functor_.get_select_best_value_functor()(
                  *test_, *valid_values_, cov_map[i]);
            }
          }
        }

        void set_task_parameters(const test* prev_test, const test* test,
                                 const bitset_uint64* prev_test_valid_values,
                                 const bitset_uint64* valid_values,
                                 ipog_coverage_map* cov_map,
                                 unsigned long long start_index,
                                 unsigned long long end_index) {
          base_type::reset();
          prev_test_ = prev_test;
          test_ = test;
          prev_test_valid_values_ = prev_test_valid_values;
          valid_values_ = valid_values;
          cov_map_ = cov_map;
          start_index_ = start_index;
          end_index_ = end_index;
        }

        const ipog_horizontal_update_coverage_map_per_param_combo_functor&
        get_coverage_update_functor() const {
          return per_param_combo_functor_.get_coverage_update_functor();
        }

        ipog_horizontal_update_coverage_map_per_param_combo_functor&
        get_coverage_update_functor() {
          return per_param_combo_functor_.get_coverage_update_functor();
        }

        const ipog_horizontal_select_best_value_per_param_combo_functor&
        get_select_best_value_functor() const {
          return per_param_combo_functor_.get_select_best_value_functor();
        }

        ipog_horizontal_select_best_value_per_param_combo_functor&
        get_select_best_value_functor() {
          return per_param_combo_functor_.get_select_best_value_functor();
        }

      private:
        ipog_horizontal_update_coverage_map_and_select_best_value_per_param_combo_functor
            per_param_combo_functor_;
        const test* prev_test_;
        const test* test_;
        const bitset_uint64* prev_test_valid_values_;
        const bitset_uint64* valid_values_;
        ipog_coverage_map* cov_map_;
        unsigned long long start_index_;
        unsigned long long end_index_;
    };

  private:
    alignas(false_sharing_avoidance_alignment)
        thread_local_vector<value_selection_task> thread_local_functors_;
    const unsigned int real_current_param_idx_;
    const unsigned int num_current_param_values_;
    std::vector<std::pair<const internal_relation*, ipog_coverage_map>>&
        relations_;
    unsigned int& last_picked_value_;
    std::vector<int>& value_to_valid_options_;
    std::unordered_map<const internal_relation*, unsigned long long>&
        num_covered_tuples_;
    T_EXEC& exec_;
};

inline ipog_horizontal_extension_result ipog_horizontal_extension(
    const unsigned long long num_missing_combinations_to_cover,
    constraint_handler& constr_handler, internal_test_set& test_set,
    std::vector<std::pair<const internal_relation*, ipog_coverage_map>>&
        relations) {

  const unsigned int real_current_param_idx =
      relations[0].second.get_parameter_index_map()
          [relations[0].first->get_current_param_idx()];
  const int num_current_param_values =
      relations[0]
          .second.get_model()
          .get_parameter_num_values()[real_current_param_idx];
  const internal_model& model = relations[0].second.get_model();

  // First initialize the result object.
  ipog_horizontal_extension_result result{
      std::vector<list_intrusive<test_list_intrusive_integ>>(
          num_current_param_values),
      list_intrusive<test_list_intrusive_integ>(),
      std::unordered_map<const internal_relation*, unsigned long long>()};

  // Call the constraint handler and ask for a mapping from tests to
  // possible extension values.
  std::vector<bitset_uint64> valid_values(
      constr_handler.get_valid_parameter_assignments(test_set,
                                                     real_current_param_idx));

  unsigned int last_picked_value = 0;
  std::vector<int> value_to_valid_options(
      get_value_to_valid_options(num_current_param_values, valid_values));

  ipog_horizontal_select_best_value_functor select_best_value_functor(
      real_current_param_idx, num_current_param_values, model, relations,
      last_picked_value, value_to_valid_options);

  ipog_horizontal_update_coverage_map_and_select_best_value_functor
      update_cov_and_select_best_value_functor(
          real_current_param_idx, num_current_param_values, model, relations,
          last_picked_value, value_to_valid_options,
          result.num_new_covered_tuples);

  ipog_horizontal_update_coverage_map_functor update_cov_map_functor(
      real_current_param_idx, model, relations, result.num_new_covered_tuples);

  test* previous_test = nullptr;
  int selected_value = 0;
  unsigned long long num_new_covered_tuples = 0;
  unsigned int test_index = 0;
  for (test& t : test_set.get_list_of_tests()) {
    if (std::ranges::any_of(
            relations.begin(), relations.end(), [](const auto& p) {
              return p.first->get_current_interaction_strength() > 2;
            })) {
      last_picked_value = num_current_param_values - 1;
    }

    if (!previous_test) {
      selected_value = select_best_value_functor(t, valid_values[test_index]);
    } else {
      if (selected_value >= 0) {
        // We might not have selected any value. This can happen, if no
        // matter which value we would pick, the coverage gain would be 0.
        // If so, our best option is to keep it as don't care, in order for
        // later vertical extension steps to exploit that don't care value.
        // If we have selected a value however with most coverage, then we
        // set it in the test accordingly.
        previous_test->get_values()[real_current_param_idx] = selected_value;
        // Maintain a mapping from values of the current parameter to the
        // tests.
        result.value_to_row_mapping[selected_value].push_back(
            previous_test->get_value_partition_intrusive_list_node());
        // Update the state of the test as seen by constraint hander.
        constr_handler.update_cached_partial_test(
            previous_test, real_current_param_idx, selected_value);
      } else {
        // Maintain a mapping from values of the current parameter to the
        // tests.
        result.rows_with_current_parameter_dont_care_value.push_back(
            previous_test->get_value_partition_intrusive_list_node());
      }

      // Reset the intrusive list node, such that the vertical extension can
      // assume both pointers to be nullptr.
      previous_test->get_vertical_extension_intrusive_list_node().prev_node_ =
          nullptr;
      previous_test->get_vertical_extension_intrusive_list_node().next_node_ =
          nullptr;

      new_covered_tuples_and_selected_value res =
          update_cov_and_select_best_value_functor(*previous_test, t,
                                                   valid_values[test_index - 1],
                                                   valid_values[test_index]);

      selected_value = res.selected_value_;

      // Keep track of how many tuples we have covered in addition.
      num_new_covered_tuples += res.num_new_covered_tuples_;

      if (num_new_covered_tuples >= num_missing_combinations_to_cover) {
        return result;
      }
    }

    previous_test = &t;
    ++test_index;
  }

  // Update coverage regarding the last test.
  if (previous_test) {
    if (selected_value >= 0) {
      // We might not have selected any value. This can happen, if no matter
      // which value we would pick, the coverage gain would be 0.
      // If so, our best option is to keep it as don't care, in order for
      // later vertical extension steps to exploit that don't care value.
      // If we have selected a value however with most coverage, then we set
      // it in the test accordingly.
      previous_test->get_values()[real_current_param_idx] = selected_value;
      // Maintain a mapping from values of the current parameter to the
      // tests.
      result.value_to_row_mapping[selected_value].push_back(
          previous_test->get_value_partition_intrusive_list_node());
      // Update the state of the test as seen by constraint hander.
      constr_handler.update_cached_partial_test(
          previous_test, real_current_param_idx, selected_value);
    } else {
      // Maintain a mapping from values of the current parameter to the
      // tests.
      result.rows_with_current_parameter_dont_care_value.push_back(
          previous_test->get_value_partition_intrusive_list_node());
    }

    // Reset the intrusive list node, such that the vertical extension can
    // assume both pointers to be nullptr.
    previous_test->get_vertical_extension_intrusive_list_node().prev_node_ =
        nullptr;
    previous_test->get_vertical_extension_intrusive_list_node().next_node_ =
        nullptr;

    update_cov_map_functor(*previous_test, valid_values[test_index - 1]);
  }

  return result;
}

template <conc_is_void_functor_executor T_EXEC>
ipog_horizontal_extension_result ipog_horizontal_extension(
    const unsigned long long num_missing_combinations_to_cover,
    constraint_handler& constr_handler, internal_test_set& test_set,
    std::vector<std::pair<const internal_relation*, ipog_coverage_map>>&
        relations,
    T_EXEC& exec) {

  const unsigned int real_current_param_idx =
      relations[0].second.get_parameter_index_map()
          [relations[0].first->get_current_param_idx()];
  const int num_current_param_values =
      relations[0]
          .second.get_model()
          .get_parameter_num_values()[real_current_param_idx];
  const internal_model& model = relations[0].second.get_model();

  // First initialize the result object.
  ipog_horizontal_extension_result result{
      std::vector<list_intrusive<test_list_intrusive_integ>>(
          num_current_param_values),
      list_intrusive<test_list_intrusive_integ>(),
      std::unordered_map<const internal_relation*, unsigned long long>()};

  // Call the constraint handler and ask for a mapping from tests to
  // possible extension values.
  std::vector<bitset_uint64> valid_values(
      constr_handler.get_valid_parameter_assignments(test_set,
                                                     real_current_param_idx));

  unsigned int last_picked_value = 0;
  std::vector<int> value_to_valid_options(
      get_value_to_valid_options(num_current_param_values, valid_values));

  ipog_horizontal_select_best_value_functor_parallel<T_EXEC>
      select_best_value_functor(
          real_current_param_idx, num_current_param_values, model, relations,
          last_picked_value, value_to_valid_options, exec);

  ipog_horizontal_update_coverage_map_and_select_best_value_functor_parallel<
      T_EXEC>
      update_cov_and_select_best_value_functor(
          real_current_param_idx, num_current_param_values, model, relations,
          last_picked_value, value_to_valid_options,
          result.num_new_covered_tuples, exec);

  ipog_horizontal_update_coverage_map_functor_parallel<T_EXEC>
      update_cov_map_functor(real_current_param_idx, model, relations,
                             result.num_new_covered_tuples, exec);

  test* previous_test = nullptr;
  int selected_value = 0;
  unsigned long long num_new_covered_tuples = 0;
  unsigned int test_index = 0;
  for (test& t : test_set.get_list_of_tests()) {
    if (std::ranges::any_of(
            relations.begin(), relations.end(), [](const auto& p) {
              return p.first->get_current_interaction_strength() > 2;
            })) {
      last_picked_value = num_current_param_values - 1;
    }

    if (!previous_test) {
      selected_value = select_best_value_functor(t, valid_values[test_index]);
    } else {
      if (selected_value >= 0) {
        // We might not have selected any value. This can happen, if no
        // matter which value we would pick, the coverage gain would be 0.
        // If so, our best option is to keep it as don't care, in order for
        // later vertical extension steps to exploit that don't care value.
        // If we have selected a value however with most coverage, then we
        // set it in the test accordingly.
        previous_test->get_values()[real_current_param_idx] = selected_value;
        // Maintain a mapping from values of the current parameter to the
        // tests.
        result.value_to_row_mapping[selected_value].push_back(
            previous_test->get_value_partition_intrusive_list_node());
        // Update the state of the test as seen by constraint hander.
        constr_handler.update_cached_partial_test(
            previous_test, real_current_param_idx, selected_value);
      } else {
        // Maintain a mapping from values of the current parameter to the
        // tests.
        result.rows_with_current_parameter_dont_care_value.push_back(
            previous_test->get_value_partition_intrusive_list_node());
      }

      // Reset the intrusive list node, such that the vertical extension can
      // assume both pointers to be nullptr.
      previous_test->get_vertical_extension_intrusive_list_node().prev_node_ =
          nullptr;
      previous_test->get_vertical_extension_intrusive_list_node().next_node_ =
          nullptr;

      new_covered_tuples_and_selected_value res =
          update_cov_and_select_best_value_functor(*previous_test, t,
                                                   valid_values[test_index - 1],
                                                   valid_values[test_index]);

      selected_value = res.selected_value_;

      // Keep track of how many tuples we have covered in addition.
      num_new_covered_tuples += res.num_new_covered_tuples_;

      if (num_new_covered_tuples >= num_missing_combinations_to_cover) {
        return result;
      }
    }

    previous_test = &t;
    ++test_index;
  }

  // Update coverage regarding the last test.
  if (previous_test) {
    if (selected_value >= 0) {
      // We might not have selected any value. This can happen, if no matter
      // which value we would pick, the coverage gain would be 0.
      // If so, our best option is to keep it as don't care, in order for
      // later vertical extension steps to exploit that don't care value.
      // If we have selected a value however with most coverage, then we set
      // it in the test accordingly.
      previous_test->get_values()[real_current_param_idx] = selected_value;
      // Maintain a mapping from values of the current parameter to the
      // tests.
      result.value_to_row_mapping[selected_value].push_back(
          previous_test->get_value_partition_intrusive_list_node());
      // Update the state of the test as seen by constraint hander.
      constr_handler.update_cached_partial_test(
          previous_test, real_current_param_idx, selected_value);
    } else {
      // Maintain a mapping from values of the current parameter to the
      // tests.
      result.rows_with_current_parameter_dont_care_value.push_back(
          previous_test->get_value_partition_intrusive_list_node());
    }

    // Reset the intrusive list node, such that the vertical extension can
    // assume both pointers to be nullptr.
    previous_test->get_vertical_extension_intrusive_list_node().prev_node_ =
        nullptr;
    previous_test->get_vertical_extension_intrusive_list_node().next_node_ =
        nullptr;

    update_cov_map_functor(*previous_test, valid_values[test_index - 1]);
  }

  return result;
}

}  // namespace detail
}  // namespace citcpp
