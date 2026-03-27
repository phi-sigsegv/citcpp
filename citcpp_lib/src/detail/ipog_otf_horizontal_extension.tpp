#include <algorithm>

#include "citcpp_utils.hpp"
#include "ipog_otf_horizontal_extension.hpp"
#include "param_combo_iteration.hpp"
#include "shared_constants.hpp"

namespace citcpp {
namespace detail {

class ipog_otf_horizontal_select_best_value_per_param_combo_functor {
  public:
    ipog_otf_horizontal_select_best_value_per_param_combo_functor(
        const unsigned int real_current_param_idx, const test& test,
        const bitset_uint64& valid_values, unsigned int current_test_index,
        const internal_test_set& test_set, bool is_extend_mode,
        const unsigned int num_current_param_values)
        : real_current_param_idx_(real_current_param_idx),
          test_(test),
          valid_values_(valid_values),
          current_test_index_(current_test_index),
          test_set_(test_set),
          is_extend_mode_(is_extend_mode),
          param_combo_gain_per_value_(num_current_param_values),
          gain_per_value_(num_current_param_values) {}

    bool operator()(const param_vector& param_indices) {
      const int current_param_value =
          test_.get_values()[real_current_param_idx_];

      for (std::vector<unsigned int>::size_type i = 0;
           i < param_combo_gain_per_value_.size(); ++i) {

        param_combo_gain_per_value_[i] = 1;
        if (current_param_value >= 0 && current_param_value != i) {
          // We check whether the test already has a concrete value
          // for the current parameter, because if so, then we force the gain
          // computation to only have a gain for that particular value.
          // We cannot skip the gain computation altogether, since we
          // use that number to track the number of covered tuples.
          param_combo_gain_per_value_[i] = 0;
        }
      }

      unsigned int test_index = 0;
      for (const test& t : test_set_.get_list_of_tests()) {
        if (test_index != current_test_index_) {
          bool is_same_tuple_prefix = true;

          for (std::vector<unsigned int>::size_type i = 0;
               i < param_indices.size() - 1; ++i) {

            const unsigned int param_idx = param_indices[i];
            const int param_value = test_.get_values()[param_idx];
            const int other_param_value = t.get_values()[param_idx];

            if (param_value < 0) {
              // We have found a don't care value for that combination in
              // the considered test.
              return true;
            }

            if (param_value != other_param_value) {
              is_same_tuple_prefix = false;
              break;
            }
          }

          if (is_same_tuple_prefix) {
            const unsigned int param_idx =
                param_indices[param_indices.size() - 1];
            const int other_param_value = t.get_values()[param_idx];

            for (unsigned int value = 0;
                 value < param_combo_gain_per_value_.size(); ++value) {
              if (value == other_param_value) {
                param_combo_gain_per_value_[value] = 0;
              }
            }
          }
        }

        if (test_index >= current_test_index_ &&
            (!is_extend_mode_ || current_param_value >= 0)) {
          // If we are not extending an already existing testset, then
          // we know for sure that all tests beyond our current one
          // have a don't care value for the current parameter.
          // That means our coverage gain would not change by also
          // inspecting the remaining tests for the current parameter
          // combination.
          //
          // If we are extending an already existing testset however,
          // then we have to walk over all tests, since a later test
          // might cover the same tuple. So replacing the don't care value
          // at the current parameter would not result in a coverage gain.
          // If however the test has a concrete value at the current parameter,
          // then we must only check the tests preceding the current one.
          // This is because we need an accurate number for the coverage
          // gain, since we use that number to track the number of covered
          // tuples. While in principle coverage of the tuple could be
          // accounted for by the current test and anther one, we always
          // associate coverage of a tuple with the test that comes first.
          // Therefore, we can abort the loop at this point.
          break;
        }

        ++test_index;
      }

      for (std::vector<unsigned int>::size_type i = 0;
           i < param_combo_gain_per_value_.size(); ++i) {

        if (valid_values_.test(i)) {
          gain_per_value_[i] += param_combo_gain_per_value_[i];
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

    void reset() {
      for (int i = 0; i < gain_per_value_.size(); ++i) {
        gain_per_value_[i] = 0;
      }
    }

  private:
    const unsigned int real_current_param_idx_;
    const test& test_;
    const bitset_uint64& valid_values_;
    const unsigned int current_test_index_;
    const internal_test_set& test_set_;
    const bool is_extend_mode_;
    std::vector<unsigned int> param_combo_gain_per_value_;
    std::vector<unsigned long long> gain_per_value_;
};

template <conc_is_void_functor_executor T_EXEC>
class ipog_otf_horizontal_select_best_value_per_param_combo_functor_parallel {
  public:
    ipog_otf_horizontal_select_best_value_per_param_combo_functor_parallel(
        const unsigned int real_current_param_idx, const test& test,
        const bitset_uint64& valid_values, unsigned int current_test_index,
        const internal_test_set& test_set, bool is_extend_mode,
        const unsigned int num_current_param_values, const T_EXEC& exec)
        : thread_local_functors_(
              exec.get_num_workers(),
              {real_current_param_idx, test, valid_values, current_test_index,
               test_set, is_extend_mode, num_current_param_values}),
          exec_(exec),
          num_current_param_values_(num_current_param_values) {}

    bool operator()(const param_vector& param_indices) {
      auto& thread_local_functor =
          thread_local_functors_[exec_.get_worker_id()];

      return thread_local_functor(param_indices);
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

    void reset() {
      for (auto& thread_local_functor : thread_local_functors_) {
        thread_local_functor.reset();
      }
    }

  private:
    alignas(false_sharing_avoidance_alignment) thread_local_vector<
        ipog_otf_horizontal_select_best_value_per_param_combo_functor> thread_local_functors_;
    const T_EXEC& exec_;
    const unsigned int num_current_param_values_;
};

template <typename T_PARAM_COMBO_IT>
new_covered_tuples_and_selected_value ipog_otf_horizontal_select_best_value(
    const unsigned int num_current_param_values,
    const std::vector<std::vector<unsigned long long>>& relation_gain_per_value,
    const std::vector<std::pair<const internal_relation*, T_PARAM_COMBO_IT>>&
        param_combo_its,
    unsigned int& last_picked_value, std::vector<int>& value_to_valid_options,
    std::unordered_map<const internal_relation*, unsigned long long>&
        num_covered_tuples) {

  new_covered_tuples_and_selected_value res{0, 0};

  int value_with_max_gain = -1;
  unsigned long long max_gain = 0;
  for (unsigned int v_index = 0; v_index < num_current_param_values;
       ++v_index) {

    unsigned int value =
        (v_index + last_picked_value + 1) % num_current_param_values;

    unsigned long long value_gain = 0;
    for (int relation_idx = 0; relation_idx < relation_gain_per_value.size();
         relation_idx++) {

      value_gain += relation_gain_per_value[relation_idx][value];
    }

    if (value_gain > max_gain) {
      value_with_max_gain = value;
      max_gain = value_gain;
    } else if (value_gain == max_gain) {
      // We use a simple tie breaking strategy: We do not favor one value
      // over the other. If two values have the same gain, then we pick the
      // one which we have picked less so far. Since also this could be a
      // tie (we have picked the value the same number of times, we remember
      // the value we have picked before, and choose the next one in this
      // case.
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

    for (int relation_idx = 0; relation_idx < relation_gain_per_value.size();
         relation_idx++) {

      num_covered_tuples[param_combo_its[relation_idx].first] +=
          relation_gain_per_value[relation_idx][value_with_max_gain];
    }
  }

  res.num_new_covered_tuples_ = max_gain;
  res.selected_value_ = value_with_max_gain;

  return res;
}

inline new_covered_tuples_and_selected_value
ipog_otf_horizontal_select_best_value(
    const unsigned int real_current_param_idx,
    const unsigned int num_current_param_values,
    const bitset_uint64& valid_values, const test& test,
    unsigned int current_test_index, const internal_test_set& test_set,
    bool is_extend_mode,
    std::vector<std::pair<const internal_relation*, param_combo_iterator>>&
        param_combo_its,
    unsigned int& last_picked_value, std::vector<int>& value_to_valid_options,
    std::unordered_map<const internal_relation*, unsigned long long>&
        num_covered_tuples) {

  new_covered_tuples_and_selected_value res{0, 0};

  // In the following vector we track for each relation, the coverage
  // gain per value of the current parameter.
  std::vector<std::vector<unsigned long long>> relation_gain_per_value(
      param_combo_its.size());

  ipog_otf_horizontal_select_best_value_per_param_combo_functor
      per_param_combo_functor(real_current_param_idx, test, valid_values,
                              current_test_index, test_set, is_extend_mode,
                              num_current_param_values);

  for (int relation_idx = 0; relation_idx < param_combo_its.size();
       relation_idx++) {

    per_param_combo_functor.reset();
    param_combo_its[relation_idx].second.visit_all_parameter_combinations(
        per_param_combo_functor);
    relation_gain_per_value[relation_idx] =
        per_param_combo_functor.get_gain_per_value();
  }

  return ipog_otf_horizontal_select_best_value(
      num_current_param_values, relation_gain_per_value, param_combo_its,
      last_picked_value, value_to_valid_options, num_covered_tuples);
}

template <conc_is_void_functor_executor T_EXEC>
new_covered_tuples_and_selected_value ipog_otf_horizontal_select_best_value(
    const unsigned int real_current_param_idx,
    const unsigned int num_current_param_values,
    const bitset_uint64& valid_values, const test& test,
    unsigned int current_test_index, const internal_test_set& test_set,
    bool is_extend_mode,
    std::vector<std::pair<const internal_relation*,
                          param_combo_parallel_iterator<T_EXEC>>>&
        param_combo_its,
    const T_EXEC& exec, unsigned int& last_picked_value,
    std::vector<int>& value_to_valid_options,
    std::unordered_map<const internal_relation*, unsigned long long>&
        num_covered_tuples) {

  new_covered_tuples_and_selected_value res{0, 0};

  // In the following vector we track for each relation, the coverage
  // gain per value of the current parameter.
  std::vector<std::vector<unsigned long long>> relation_gain_per_value(
      param_combo_its.size());

  ipog_otf_horizontal_select_best_value_per_param_combo_functor_parallel<T_EXEC>
      per_param_combo_functor(real_current_param_idx, test, valid_values,
                              current_test_index, test_set, is_extend_mode,
                              num_current_param_values, exec);

  for (int relation_idx = 0; relation_idx < param_combo_its.size();
       relation_idx++) {

    per_param_combo_functor.reset();
    param_combo_its[relation_idx].second.visit_all_parameter_combinations(
        per_param_combo_functor);
    relation_gain_per_value[relation_idx] =
        std::move(per_param_combo_functor.get_gain_per_value());
  }

  return ipog_otf_horizontal_select_best_value(
      num_current_param_values, relation_gain_per_value, param_combo_its,
      last_picked_value, value_to_valid_options, num_covered_tuples);
}

inline ipog_horizontal_extension_result ipog_otf_horizontal_extension(
    const unsigned long long num_missing_combinations_to_cover,
    const constraint_handler& constr_handler, internal_test_set& test_set,
    const internal_model& model,
    const std::vector<internal_relation>& relations, bool is_extend_mode) {

  const unsigned int real_current_param_idx =
      relations[0]
          .get_parameter_index_map()[relations[0].get_current_param_idx()];
  const int num_current_param_values =
      model.get_parameter_num_values()[real_current_param_idx];

  // First initialize the result object.
  ipog_horizontal_extension_result result{
      std::vector<list_intrusive<test_list_intrusive_integ>>(
          num_current_param_values),
      list_intrusive<test_list_intrusive_integ>(),
      std::unordered_map<const internal_relation*, unsigned long long>()};

  std::vector<std::pair<const internal_relation*, param_combo_iterator>>
      param_combo_its;
  for (auto& rel : relations) {
    param_combo_its.emplace_back(
        &rel, param_combo_iterator(rel.get_current_param_idx() + 1,
                                   rel.get_current_interaction_strength(),
                                   rel.get_parameter_index_map(), true));
  }

  // Call the constraint handler and ask for a mapping from tests to possible
  // extension values.
  std::vector<bitset_uint64> valid_values(
      constr_handler.get_valid_parameter_assignments(test_set,
                                                     real_current_param_idx));

  unsigned int last_picked_value = 0;
  std::vector<int> value_to_valid_options(
      get_value_to_valid_options(num_current_param_values, valid_values));

  unsigned int test_index = 0;
  unsigned long long num_new_covered_tuples = 0;
  for (test& t : test_set.get_list_of_tests()) {
    if (std::ranges::any_of(relations.begin(), relations.end(),
                            [](const auto& p) {
                              return p.get_current_interaction_strength() > 2;
                            })) {
      last_picked_value = num_current_param_values - 1;
    }

    new_covered_tuples_and_selected_value res =
        ipog_otf_horizontal_select_best_value(
            real_current_param_idx, num_current_param_values,
            valid_values[test_index], t, test_index, test_set, is_extend_mode,
            param_combo_its, last_picked_value, value_to_valid_options,
            result.num_new_covered_tuples);

    if (res.selected_value_ >= 0) {
      // We might not have selected any value. This can happen, if no matter
      // which value we would pick, the coverage gain would be 0.
      // If so, our best option is to keep it as don't care, in order for
      // later vertical extension steps to exploit that don't care value.
      // If we have selected a value however with most coverage, then we set
      // it in the test accordingly.
      t.get_values()[real_current_param_idx] = res.selected_value_;
      // Maintain a mapping from values of the current parameter to the
      // tests.
      result.value_to_row_mapping[res.selected_value_].push_back(
          t.get_value_partition_intrusive_list_node());
    } else {
      // Maintain a mapping from values of the current parameter to the
      // tests.
      result.rows_with_current_parameter_dont_care_value.push_back(
          t.get_value_partition_intrusive_list_node());
    }

    // Keep track of how many tuples we have covered in addition.
    num_new_covered_tuples += res.num_new_covered_tuples_;

    if (num_new_covered_tuples >= num_missing_combinations_to_cover) {
      return result;
    }

    ++test_index;
  }

  return result;
}

template <conc_is_void_functor_executor T_EXEC>
ipog_horizontal_extension_result ipog_otf_horizontal_extension(
    const unsigned long long num_missing_combinations_to_cover,
    const constraint_handler& constr_handler, internal_test_set& test_set,
    const internal_model& model,
    const std::vector<internal_relation>& relations, bool is_extend_mode,
    T_EXEC& exec) {

  const unsigned int real_current_param_idx =
      relations[0]
          .get_parameter_index_map()[relations[0].get_current_param_idx()];
  const int num_current_param_values =
      model.get_parameter_num_values()[real_current_param_idx];

  // First initialize the result object.
  ipog_horizontal_extension_result result{
      std::vector<list_intrusive<test_list_intrusive_integ>>(
          num_current_param_values),
      list_intrusive<test_list_intrusive_integ>(),
      std::unordered_map<const internal_relation*, unsigned long long>()};

  std::vector<std::pair<const internal_relation*,
                        param_combo_parallel_iterator<T_EXEC>>>
      param_combo_its;
  for (auto& rel : relations) {
    param_combo_its.emplace_back(
        &rel, param_combo_parallel_iterator<T_EXEC>(
                  rel.get_current_param_idx() + 1,
                  rel.get_current_interaction_strength(),
                  rel.get_parameter_index_map(), true, exec));
  }

  // Call the constraint handler and ask for a mapping from tests to possible
  // extension values.
  std::vector<bitset_uint64> valid_values(
      constr_handler.get_valid_parameter_assignments(test_set,
                                                     real_current_param_idx));

  unsigned int last_picked_value = 0;
  std::vector<int> value_to_valid_options(
      get_value_to_valid_options(num_current_param_values, valid_values));

  unsigned int test_index = 0;
  unsigned long long num_new_covered_tuples = 0;
  for (test& t : test_set.get_list_of_tests()) {
    if (std::ranges::any_of(relations.begin(), relations.end(),
                            [](const auto& p) {
                              return p.get_current_interaction_strength() > 2;
                            })) {
      last_picked_value = num_current_param_values - 1;
    }

    new_covered_tuples_and_selected_value res =
        ipog_otf_horizontal_select_best_value(
            real_current_param_idx, num_current_param_values,
            valid_values[test_index], t, test_index, test_set, is_extend_mode,
            param_combo_its, exec, last_picked_value, value_to_valid_options,
            result.num_new_covered_tuples);

    if (res.selected_value_ >= 0) {
      // We might not have selected any value. This can happen, if no matter
      // which value we would pick, the coverage gain would be 0.
      // If so, our best option is to keep it as don't care, in order for
      // later vertical extension steps to exploit that don't care value.
      // If we have selected a value however with most coverage, then we set
      // it in the test accordingly.
      t.get_values()[real_current_param_idx] = res.selected_value_;
      // Maintain a mapping from values of the current parameter to the
      // tests.
      result.value_to_row_mapping[res.selected_value_].push_back(
          t.get_value_partition_intrusive_list_node());
    } else {
      // Maintain a mapping from values of the current parameter to the
      // tests.
      result.rows_with_current_parameter_dont_care_value.push_back(
          t.get_value_partition_intrusive_list_node());
    }

    // Keep track of how many tuples we have covered in addition.
    num_new_covered_tuples += res.num_new_covered_tuples_;

    if (num_new_covered_tuples >= num_missing_combinations_to_cover) {
      return result;
    }

    ++test_index;
  }

  return result;
}

}  // namespace detail
}  // namespace citcpp
