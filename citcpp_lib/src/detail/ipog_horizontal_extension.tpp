#include <algorithm>

#include "citcpp_utils.hpp"
#include "ipog_horizontal_extension.hpp"
#include "shared_constants.hpp"

namespace citcpp {
namespace detail {

class ipog_horizontal_select_best_value_per_param_combo_functor {
  public:
    ipog_horizontal_select_best_value_per_param_combo_functor(
        const internal_model& model, const test& test,
        const bitset_uint64& valid_values,
        const unsigned int num_current_param_values)
        : model_(model),
          test_(test),
          valid_values_(valid_values),
          gain_per_value_(num_current_param_values) {}

    bool operator()(const coverage_map::second_level_type& value_combinations) {
      const param_vector& param_indices =
          value_combinations.get_parameter_indices();

      if (!value_combinations.all()) {
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
        coverage_map::second_level_type::size_type base_index = 0;
        for (std::vector<unsigned int>::size_type i = 0;
             i < param_indices.size() - 1; ++i) {
          const unsigned int param_idx = param_indices[i];
          const int param_value = test_.get_values()[param_idx];

          if (param_value < 0) {
            // We have found a don't care value for that combination in
            // the considered test.
            return true;
          }

          coverage_map::second_level_type::size_type addend = param_value;
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
          if (!value_combinations.test(base_index + value) &&
              valid_values_.test(value)) {

            gain_per_value_[value] += 1;
          }
        }
      }

      return true;
    }

    const std::vector<unsigned long long>& get_gain_per_value() const {
      return gain_per_value_;
    }

    std::vector<unsigned long long>& get_gain_per_value() {
      return gain_per_value_;
    }

  private:
    const internal_model& model_;
    const test& test_;
    const bitset_uint64& valid_values_;
    std::vector<unsigned long long> gain_per_value_;
};

template <conc_is_void_functor_executor T_EXEC>
class ipog_horizontal_select_best_value_per_param_combo_functor_parallel {
  public:
    ipog_horizontal_select_best_value_per_param_combo_functor_parallel(
        const internal_model& model, const test& test,
        const bitset_uint64& valid_values,
        const unsigned int num_current_param_values, const T_EXEC& exec)
        : thread_local_functors_(
              exec.get_num_workers(),
              {model, test, valid_values, num_current_param_values}),
          exec_(exec),
          num_current_param_values_(num_current_param_values) {}

    bool operator()(coverage_map::second_level_type& value_combinations) {
      auto& thread_local_functor =
          thread_local_functors_[exec_.get_worker_id()];

      return thread_local_functor(value_combinations);
    }

    std::vector<unsigned long long> get_gain_per_value() const {
      std::vector<unsigned long long> gain_per_value(num_current_param_values_);

      for (const auto& thread_local_functor : thread_local_functors_) {
        for (int i = 0; i < gain_per_value.size(); ++i) {
          gain_per_value[i] += thread_local_functor.get_gain_per_value()[i];
        }
      }

      return gain_per_value;
    }

  private:
    alignas(false_sharing_avoidance_alignment) thread_local_vector<
        ipog_horizontal_select_best_value_per_param_combo_functor> thread_local_functors_;
    const T_EXEC& exec_;
    const unsigned int num_current_param_values_;
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

inline int ipog_horizontal_select_best_value(
    const unsigned int real_current_param_idx,
    const unsigned int num_current_param_values, const internal_model& model,
    const bitset_uint64& valid_values, const test& test,
    std::vector<std::pair<const internal_relation*, coverage_map_iterator>>&
        relation_cov_map_its,
    unsigned int& last_picked_value, std::vector<int>& value_to_valid_options) {

  // We first check whether the test already has a concrete value
  // for the current parameter, because if so, then there is no
  // point in evaluating a coverage gain.
  if (test.get_values()[real_current_param_idx] >= 0) {
    last_picked_value = test.get_values()[real_current_param_idx];
    value_to_valid_options[last_picked_value]--;

    return last_picked_value;
  }

  ipog_horizontal_select_best_value_per_param_combo_functor
      per_param_combo_functor(model, test, valid_values,
                              num_current_param_values);
  for (auto& cov_map_it : relation_cov_map_its) {
    cov_map_it.second.visit_all_parameter_combinations(per_param_combo_functor);
  }

  // This is an array containing the coverage gain per value of the current
  // parameter.
  const std::vector<unsigned long long>& gain_per_value =
      per_param_combo_functor.get_gain_per_value();

  return ipog_horizontal_select_best_value(num_current_param_values,
                                           gain_per_value, last_picked_value,
                                           value_to_valid_options);
}

template <conc_is_void_functor_executor T_EXEC>
int ipog_horizontal_select_best_value(
    const unsigned int real_current_param_idx,
    const unsigned int num_current_param_values, const internal_model& model,
    const bitset_uint64& valid_values, const test& test,
    std::vector<std::pair<const internal_relation*,
                          coverage_map_parallel_iterator<T_EXEC>>>&
        relation_cov_map_its,
    const T_EXEC& exec, unsigned int& last_picked_value,
    std::vector<int>& value_to_valid_options) {

  // We first check whether the test already has a concrete value
  // for the current parameter, because if so, then there is no
  // point in evaluating a coverage gain.
  if (test.get_values()[real_current_param_idx] >= 0) {
    last_picked_value = test.get_values()[real_current_param_idx];
    value_to_valid_options[last_picked_value]--;

    return last_picked_value;
  }

  ipog_horizontal_select_best_value_per_param_combo_functor_parallel<T_EXEC>
      per_param_combo_functor(model, test, valid_values,
                              num_current_param_values, exec);
  for (auto& cov_map_it : relation_cov_map_its) {
    cov_map_it.second.visit_all_parameter_combinations(per_param_combo_functor);
  }

  // This is an array containing the coverage gain per value of the current
  // parameter.
  const std::vector<unsigned long long> gain_per_value(
      per_param_combo_functor.get_gain_per_value());

  return ipog_horizontal_select_best_value(num_current_param_values,
                                           gain_per_value, last_picked_value,
                                           value_to_valid_options);
}

class ipog_horizontal_update_coverage_map_per_param_combo_functor {
  public:
    ipog_horizontal_update_coverage_map_per_param_combo_functor(
        const internal_model& model, const test& test)
        : model_(model), test_(test), num_new_covered_tuples_(0) {}

    bool operator()(coverage_map::second_level_type& value_combinations) {
      const param_vector& param_indices =
          value_combinations.get_parameter_indices();

      if (!value_combinations.all()) {
        // Here we compute an index into the bitset. To do so, we treat the
        // number of values of each parameter as a kind of radix. Consider
        // three parameters p_0, p_1, p_2. Now say that v_i is the number of
        // values for p_i. If we now have values x_0, x_1, x_2, then the
        // index is x_0 * v_1 * v_2 + x_1 * v_2 + x_2.
        coverage_map::second_level_type::size_type index = 0;
        for (std::vector<unsigned int>::size_type i = 0;
             i < param_indices.size(); ++i) {
          const unsigned int param_idx = param_indices[i];
          const int param_value = test_.get_values()[param_idx];

          if (param_value < 0) {
            // We have found a don't care value for that combination in
            // the considered test in one of the [0, ... ,current_param_idx - 1]
            // parameters. There is nothing to be updated concerning the
            // coverage. This combination will be taken care of during the
            // vertical extension step.
            return true;
          }

          coverage_map::second_level_type::size_type addend = param_value;
          for (std::vector<unsigned int>::size_type j = i + 1;
               j < param_indices.size(); ++j) {
            addend *= model_.get_parameter_num_values()[param_indices[j]];
          }
          index += addend;
        }

        if (!value_combinations.test_and_set(index)) {
          ++num_new_covered_tuples_;
        }
      }

      return true;
    }

    unsigned long long get_num_new_covered_tuples() const {
      return num_new_covered_tuples_;
    }

    void reset_num_new_covered_tuples() { num_new_covered_tuples_ = 0; }

  private:
    const internal_model& model_;
    const test& test_;
    unsigned long long num_new_covered_tuples_;
};

template <conc_is_void_functor_executor T_EXEC>
class ipog_horizontal_update_coverage_map_per_param_combo_functor_parallel {
  public:
    ipog_horizontal_update_coverage_map_per_param_combo_functor_parallel(
        const internal_model& model, const test& test, const T_EXEC& exec)
        : thread_local_functors_(exec.get_num_workers(), {model, test}),
          exec_(exec) {}

    bool operator()(coverage_map::second_level_type& value_combinations) {
      auto& thread_local_functor =
          thread_local_functors_[exec_.get_worker_id()];

      return thread_local_functor(value_combinations);
    }

    unsigned long long get_num_new_covered_tuples() const {
      unsigned long long res = 0;

      for (const auto& thread_local_functor : thread_local_functors_) {
        res += thread_local_functor.get_num_new_covered_tuples();
      }

      return res;
    }

    void reset_num_new_covered_tuples() {
      for (auto& thread_local_functor : thread_local_functors_) {
        thread_local_functor.reset_num_new_covered_tuples();
      }
    }

  private:
    alignas(false_sharing_avoidance_alignment) thread_local_vector<
        ipog_horizontal_update_coverage_map_per_param_combo_functor> thread_local_functors_;
    const T_EXEC& exec_;
};

inline void ipog_horizontal_update_coverage_map(
    const internal_model& model, const test& test,
    std::vector<std::pair<const internal_relation*, coverage_map_iterator>>&
        relation_cov_map_its,
    std::unordered_map<const internal_relation*, unsigned long long>&
        num_covered_tuples) {

  ipog_horizontal_update_coverage_map_per_param_combo_functor
      per_param_combo_functor(model, test);
  for (auto& cov_map_it : relation_cov_map_its) {
    per_param_combo_functor.reset_num_new_covered_tuples();
    cov_map_it.second.visit_all_parameter_combinations(per_param_combo_functor);
    num_covered_tuples[cov_map_it.first] +=
        per_param_combo_functor.get_num_new_covered_tuples();
  }
}

template <conc_is_void_functor_executor T_EXEC>
void ipog_horizontal_update_coverage_map(
    const internal_model& model, const test& test,
    std::vector<std::pair<const internal_relation*,
                          coverage_map_parallel_iterator<T_EXEC>>>&
        relation_cov_map_its,
    const T_EXEC& exec,
    std::unordered_map<const internal_relation*, unsigned long long>&
        num_covered_tuples) {

  ipog_horizontal_update_coverage_map_per_param_combo_functor_parallel<T_EXEC>
      per_param_combo_functor(model, test, exec);
  for (auto& cov_map_it : relation_cov_map_its) {
    per_param_combo_functor.reset_num_new_covered_tuples();
    cov_map_it.second.visit_all_parameter_combinations(per_param_combo_functor);
    num_covered_tuples[cov_map_it.first] +=
        per_param_combo_functor.get_num_new_covered_tuples();
  }
}

class
    ipog_horizontal_update_coverage_map_and_select_best_value_per_param_combo_functor {
  public:
    ipog_horizontal_update_coverage_map_and_select_best_value_per_param_combo_functor(
        const internal_model& model, const test& prev_test, const test& test,
        const bitset_uint64& valid_values,
        const unsigned int num_current_param_values,
        const bool enable_coverage_update, const bool enable_gain_computation)
        : covm_update_func_(model, prev_test),
          best_value_selection_func_(model, test, valid_values,
                                     num_current_param_values),
          enable_coverage_update_(enable_coverage_update),
          enable_gain_computation_(enable_gain_computation) {}

    bool operator()(coverage_map::second_level_type& value_combinations) {
      if (enable_coverage_update_) {
        covm_update_func_(value_combinations);
      }
      if (enable_gain_computation_) {
        best_value_selection_func_(value_combinations);
      }

      return true;
    }

    const std::vector<unsigned long long>& get_gain_per_value() const {
      return best_value_selection_func_.get_gain_per_value();
    }

    std::vector<unsigned long long>& get_gain_per_value() {
      return best_value_selection_func_.get_gain_per_value();
    }

    unsigned long long get_num_new_covered_tuples() const {
      return covm_update_func_.get_num_new_covered_tuples();
    }

    void reset_num_new_covered_tuples() {
      covm_update_func_.reset_num_new_covered_tuples();
    }

  private:
    ipog_horizontal_update_coverage_map_per_param_combo_functor
        covm_update_func_;
    ipog_horizontal_select_best_value_per_param_combo_functor
        best_value_selection_func_;
    const bool enable_coverage_update_;
    const bool enable_gain_computation_;
};

template <conc_is_void_functor_executor T_EXEC>
class
    ipog_horizontal_update_coverage_map_and_select_best_value_per_param_combo_functor_parallel {
  public:
    ipog_horizontal_update_coverage_map_and_select_best_value_per_param_combo_functor_parallel(
        const internal_model& model, const test& prev_test, const test& test,
        const bitset_uint64& valid_values,
        const unsigned int num_current_param_values,
        const bool enable_coverage_update, const bool enable_gain_computation,
        const T_EXEC& exec)
        : thread_local_functors_(
              exec.get_num_workers(),
              {model, prev_test, test, valid_values, num_current_param_values,
               enable_coverage_update, enable_gain_computation}),
          exec_(exec),
          num_current_param_values_(num_current_param_values) {}

    bool operator()(coverage_map::second_level_type& value_combinations) {
      auto& thread_local_functor =
          thread_local_functors_[exec_.get_worker_id()];

      return thread_local_functor(value_combinations);
    }

    std::vector<unsigned long long> get_gain_per_value() const {
      std::vector<unsigned long long> gain_per_value(num_current_param_values_);

      for (const auto& thread_local_functor : thread_local_functors_) {
        for (int i = 0; i < gain_per_value.size(); ++i) {
          gain_per_value[i] += thread_local_functor.get_gain_per_value()[i];
        }
      }

      return gain_per_value;
    }

    unsigned long long get_num_new_covered_tuples() const {
      unsigned long long res = 0;

      for (const auto& thread_local_functor : thread_local_functors_) {
        res += thread_local_functor.get_num_new_covered_tuples();
      }

      return res;
    }

    void reset_num_new_covered_tuples() {
      for (auto& thread_local_functor : thread_local_functors_) {
        thread_local_functor.reset_num_new_covered_tuples();
      }
    }

  private:
    alignas(false_sharing_avoidance_alignment) thread_local_vector<
        ipog_horizontal_update_coverage_map_and_select_best_value_per_param_combo_functor> thread_local_functors_;
    const T_EXEC& exec_;
    const unsigned int num_current_param_values_;
};

inline new_covered_tuples_and_selected_value
ipog_horizontal_update_coverage_map_and_select_best_value(
    const unsigned int real_current_param_idx,
    const unsigned int num_current_param_values, const internal_model& model,
    const bitset_uint64& valid_values, const test& prev_test, const test& test,
    std::vector<std::pair<const internal_relation*, coverage_map_iterator>>&
        relation_cov_map_its,
    unsigned int& last_picked_value, std::vector<int>& value_to_valid_options,
    std::unordered_map<const internal_relation*, unsigned long long>&
        num_covered_tuples) {

  const int current_param_value = test.get_values()[real_current_param_idx];

  new_covered_tuples_and_selected_value res{0, 0};

  ipog_horizontal_update_coverage_map_and_select_best_value_per_param_combo_functor
      per_param_combo_functor(
          model, prev_test, test, valid_values, num_current_param_values,
          prev_test.get_values()[real_current_param_idx] >= 0,
          current_param_value < 0);
  for (auto& cov_map_it : relation_cov_map_its) {
    per_param_combo_functor.reset_num_new_covered_tuples();
    cov_map_it.second.visit_all_parameter_combinations(per_param_combo_functor);
    const unsigned long long num_new_covered_tuples =
        per_param_combo_functor.get_num_new_covered_tuples();
    num_covered_tuples[cov_map_it.first] += num_new_covered_tuples;
    res.num_new_covered_tuples_ += num_new_covered_tuples;
  }

  // This is an array containing the coverage gain per value of the current
  // parameter.
  std::vector<unsigned long long>& gain_per_value =
      per_param_combo_functor.get_gain_per_value();

  // Check whether the test already has a concrete value
  // for the current parameter, because if so, then there is no
  // point in evaluating a coverage gain.
  if (current_param_value >= 0) {
    // The coverage gain computation is disabled in the functor,
    // so that it won't modify this gain info.
    gain_per_value[current_param_value]++;
  }

  res.selected_value_ = ipog_horizontal_select_best_value(
      num_current_param_values, gain_per_value, last_picked_value,
      value_to_valid_options);

  return res;
}

template <conc_is_void_functor_executor T_EXEC>
new_covered_tuples_and_selected_value
ipog_horizontal_update_coverage_map_and_select_best_value(
    const unsigned int real_current_param_idx,
    const unsigned int num_current_param_values, const internal_model& model,
    const bitset_uint64& valid_values, const test& prev_test, const test& test,
    std::vector<std::pair<const internal_relation*,
                          coverage_map_parallel_iterator<T_EXEC>>>&
        relation_cov_map_its,
    const T_EXEC& exec, unsigned int& last_picked_value,
    std::vector<int>& value_to_valid_options,
    std::unordered_map<const internal_relation*, unsigned long long>&
        num_covered_tuples) {

  const int current_param_value = test.get_values()[real_current_param_idx];

  new_covered_tuples_and_selected_value res{0, 0};

  ipog_horizontal_update_coverage_map_and_select_best_value_per_param_combo_functor_parallel<
      T_EXEC>
      per_param_combo_functor(
          model, prev_test, test, valid_values, num_current_param_values,
          prev_test.get_values()[real_current_param_idx] >= 0,
          current_param_value < 0, exec);
  for (auto& cov_map_it : relation_cov_map_its) {
    per_param_combo_functor.reset_num_new_covered_tuples();
    cov_map_it.second.visit_all_parameter_combinations(per_param_combo_functor);
    const unsigned long long num_new_covered_tuples =
        per_param_combo_functor.get_num_new_covered_tuples();
    num_covered_tuples[cov_map_it.first] += num_new_covered_tuples;
    res.num_new_covered_tuples_ += num_new_covered_tuples;
  }

  // This is an array containing the coverage gain per value of the current
  // parameter.
  std::vector<unsigned long long> gain_per_value(
      per_param_combo_functor.get_gain_per_value());

  // Check whether the test already has a concrete value
  // for the current parameter, because if so, then there is no
  // point in evaluating a coverage gain.
  if (current_param_value >= 0) {
    // The coverage gain computation is disabled in the functor,
    // so that it won't modify this gain info.
    gain_per_value[current_param_value]++;
  }

  res.selected_value_ = ipog_horizontal_select_best_value(
      num_current_param_values, gain_per_value, last_picked_value,
      value_to_valid_options);

  return res;
}

inline ipog_horizontal_extension_result ipog_horizontal_extension(
    const unsigned long long num_missing_combinations_to_cover,
    const constraint_handler& constr_handler, internal_test_set& test_set,
    std::vector<std::pair<const internal_relation*, coverage_map>>& relations) {

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

  std::vector<std::pair<const internal_relation*, coverage_map_iterator>>
      relation_cov_map_its;
  for (auto& rel : relations) {
    relation_cov_map_its.push_back(
        std::make_pair(rel.first, rel.second.create_iterator()));
  }

  // Call the constraint handler and ask for a mapping from tests to possible
  // extension values.
  std::vector<bitset_uint64> valid_values(
      constr_handler.get_valid_parameter_assignments(test_set,
                                                     real_current_param_idx));

  unsigned int last_picked_value = 0;
  std::vector<int> value_to_valid_options(
      get_value_to_valid_options(num_current_param_values, valid_values));

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
      selected_value = ipog_horizontal_select_best_value(
          real_current_param_idx, num_current_param_values, model,
          valid_values[test_index], t, relation_cov_map_its, last_picked_value,
          value_to_valid_options);
    } else {
      if (selected_value >= 0) {
        // We might not have selected any value. This can happen, if no matter
        // which value we would pick, the coverage gain would be 0.
        // If so, our best option is to keep it as don't care, in order for
        // later vertical extension steps to exploit that don't care value.
        // If we have selected a value however with most coverage, then we set
        // it in the test accordingly.
        previous_test->get_values()[real_current_param_idx] = selected_value;
        // Maintain a mapping from values of the current parameter to the tests.
        result.value_to_row_mapping[selected_value].push_back(
            previous_test->get_value_partition_intrusive_list_node());
      } else {
        // Maintain a mapping from values of the current parameter to the tests.
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
          ipog_horizontal_update_coverage_map_and_select_best_value(
              real_current_param_idx, num_current_param_values, model,
              valid_values[test_index], *previous_test, t, relation_cov_map_its,
              last_picked_value, value_to_valid_options,
              result.num_new_covered_tuples);

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
      // If we have selected a value however with most coverage, then we set it
      // in the test accordingly.
      previous_test->get_values()[real_current_param_idx] = selected_value;
      // Maintain a mapping from values of the current parameter to the tests.
      result.value_to_row_mapping[selected_value].push_back(
          previous_test->get_value_partition_intrusive_list_node());
    } else {
      // Maintain a mapping from values of the current parameter to the tests.
      result.rows_with_current_parameter_dont_care_value.push_back(
          previous_test->get_value_partition_intrusive_list_node());
    }

    // Reset the intrusive list node, such that the vertical extension can
    // assume both pointers to be nullptr.
    previous_test->get_vertical_extension_intrusive_list_node().prev_node_ =
        nullptr;
    previous_test->get_vertical_extension_intrusive_list_node().next_node_ =
        nullptr;

    if (selected_value >= 0) {
      ipog_horizontal_update_coverage_map(model, *previous_test,
                                          relation_cov_map_its,
                                          result.num_new_covered_tuples);
    }
  }

  return result;
}

template <conc_is_void_functor_executor T_EXEC>
ipog_horizontal_extension_result ipog_horizontal_extension(
    const unsigned long long num_missing_combinations_to_cover,
    const constraint_handler& constr_handler, internal_test_set& test_set,
    std::vector<std::pair<const internal_relation*, coverage_map>>& relations,
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

  std::vector<std::pair<const internal_relation*,
                        coverage_map_parallel_iterator<T_EXEC>>>
      relation_cov_map_its;
  for (auto& rel : relations) {
    relation_cov_map_its.emplace_back(
        rel.first, rel.second.create_parallel_iterator(exec));
  }

  // Call the constraint handler and ask for a mapping from tests to possible
  // extension values.
  std::vector<bitset_uint64> valid_values(
      constr_handler.get_valid_parameter_assignments(test_set,
                                                     real_current_param_idx));

  unsigned int last_picked_value = 0;
  std::vector<int> value_to_valid_options(
      get_value_to_valid_options(num_current_param_values, valid_values));

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
      selected_value = ipog_horizontal_select_best_value(
          real_current_param_idx, num_current_param_values, model,
          valid_values[test_index], t, relation_cov_map_its, exec,
          last_picked_value, value_to_valid_options);
    } else {
      if (selected_value >= 0) {
        // We might not have selected any value. This can happen, if no matter
        // which value we would pick, the coverage gain would be 0.
        // If so, our best option is to keep it as don't care, in order for
        // later vertical extension steps to exploit that don't care value.
        // If we have selected a value however with most coverage, then we set
        // it in the test accordingly.
        previous_test->get_values()[real_current_param_idx] = selected_value;
        // Maintain a mapping from values of the current parameter to the tests.
        result.value_to_row_mapping[selected_value].push_back(
            previous_test->get_value_partition_intrusive_list_node());
      } else {
        // Maintain a mapping from values of the current parameter to the tests.
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
          ipog_horizontal_update_coverage_map_and_select_best_value(
              real_current_param_idx, num_current_param_values, model,
              valid_values[test_index], *previous_test, t, relation_cov_map_its,
              exec, last_picked_value, value_to_valid_options,
              result.num_new_covered_tuples);

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
      // If we have selected a value however with most coverage, then we set it
      // in the test accordingly.
      previous_test->get_values()[real_current_param_idx] = selected_value;
      // Maintain a mapping from values of the current parameter to the tests.
      result.value_to_row_mapping[selected_value].push_back(
          previous_test->get_value_partition_intrusive_list_node());
    } else {
      // Maintain a mapping from values of the current parameter to the tests.
      result.rows_with_current_parameter_dont_care_value.push_back(
          previous_test->get_value_partition_intrusive_list_node());
    }

    // Reset the intrusive list node, such that the vertical extension can
    // assume both pointers to be nullptr.
    previous_test->get_vertical_extension_intrusive_list_node().prev_node_ =
        nullptr;
    previous_test->get_vertical_extension_intrusive_list_node().next_node_ =
        nullptr;

    if (selected_value >= 0) {
      ipog_horizontal_update_coverage_map(model, *previous_test,
                                          relation_cov_map_its, exec,
                                          result.num_new_covered_tuples);
    }
  }

  return result;
}

}  // namespace detail
}  // namespace citcpp
