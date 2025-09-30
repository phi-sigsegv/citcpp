#include "ipog_otf_horizontal_extension.hpp"

#include <new>

#include "citcpp_utils.hpp"
#include "param_combo_iteration.hpp"

namespace {

class ipog_horizontal_select_best_value_per_param_combo_functor {
  public:
    ipog_horizontal_select_best_value_per_param_combo_functor(
        const unsigned int real_current_param_idx,
        const citcpp::detail::test &test, unsigned int current_test_index,
        const citcpp::detail::internal_test_set &test_set, bool is_extend_mode,
        std::vector<unsigned long long> &gain_per_value)
        : real_current_param_idx_(real_current_param_idx),
          test_(test),
          current_test_index_(current_test_index),
          test_set_(test_set),
          is_extend_mode_(is_extend_mode),
          param_combo_gain_per_value_(gain_per_value.size()),
          gain_per_value_(gain_per_value) {}

    bool operator()(const citcpp::detail::param_vector &param_indices) {
      using namespace citcpp::detail;

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
      for (const test &t : test_set_.get_list_of_tests()) {
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

        if (!is_extend_mode_ && test_index >= current_test_index_) {
          // If we are not extending an already existing testset, then
          // we know for sure that all tests beyond our current one
          // have a don't care value for the current parameter.
          // That means our coverage gain would not change by also
          // inspecting the remaining tests for the current parameter
          // combination.
          // If we are extending an already existing testset however,
          // then we have to walk over all tests, since a later test
          // might cover the same tuple.
          break;
        }

        ++test_index;
      }

      for (std::vector<unsigned int>::size_type i = 0;
           i < param_combo_gain_per_value_.size(); ++i) {

        gain_per_value_[i] += param_combo_gain_per_value_[i];
      }

      return true;
    }

  private:
    const unsigned int real_current_param_idx_;
    const citcpp::detail::test &test_;
    const unsigned int current_test_index_;
    const citcpp::detail::internal_test_set &test_set_;
    const bool is_extend_mode_;
    std::vector<unsigned int> param_combo_gain_per_value_;
    std::vector<unsigned long long> &gain_per_value_;
};

class ipog_horizontal_select_best_value_per_param_combo_functor_parallel {
  public:
    ipog_horizontal_select_best_value_per_param_combo_functor_parallel(
        const unsigned int real_current_param_idx,
        const unsigned int num_current_param_values,
        const citcpp::detail::test &test, unsigned int current_test_index,
        const citcpp::detail::internal_test_set &test_set, bool is_extend_mode,
        citcpp::detail::thread_local_vector<
            citcpp::detail::aligned_vector<unsigned long long>> &gain_per_value,
        const citcpp::detail::param_combo_parallel_iterator &param_combo_it)
        : real_current_param_idx_(real_current_param_idx),
          test_(test),
          current_test_index_(current_test_index),
          test_set_(test_set),
          is_extend_mode_(is_extend_mode),
          param_combo_gain_per_value_(
              param_combo_it.get_num_workers(),
              citcpp::detail::aligned_vector<unsigned int>(
                  num_current_param_values, 0)),
          gain_per_value_(gain_per_value),
          param_combo_it_(param_combo_it) {}

    bool operator()(const citcpp::detail::param_vector &param_indices) {
      using namespace citcpp::detail;

      std::vector<unsigned int> &thread_local_param_combo_gain_per_value =
          param_combo_gain_per_value_[param_combo_it_.get_worker_id()].value;
      std::vector<unsigned long long> &thread_local_gain_per_value =
          gain_per_value_[param_combo_it_.get_worker_id()].value;

      // We first check whether the test already has a concrete value
      // for the current parameter, because if so, then there is no
      // point in evaluating a coverage gain.
      if (test_.get_values()[real_current_param_idx_] >= 0) {
        thread_local_gain_per_value
            [test_.get_values()[real_current_param_idx_]] += 1;

        return true;
      }

      for (std::vector<unsigned int>::size_type i = 0;
           i < thread_local_param_combo_gain_per_value.size(); ++i) {

        thread_local_param_combo_gain_per_value[i] = 1;
      }

      unsigned int test_index = 0;
      for (const test &t : test_set_.get_list_of_tests()) {
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
            for (unsigned int value = 0;
                 value < thread_local_param_combo_gain_per_value.size();
                 ++value) {

              const unsigned int param_idx =
                  param_indices[param_indices.size() - 1];
              const int other_param_value = t.get_values()[param_idx];
              if (value == other_param_value) {
                thread_local_param_combo_gain_per_value[value] = 0;
              }
            }
          }
        }

        if (!is_extend_mode_ && test_index >= current_test_index_) {
          // If we are not extending an already existing testset, then
          // we know for sure that all tests beyond our current one
          // have a don't care value for the current parameter.
          // That means our coverage gain would not change by also
          // inspecting the remaining tests for the current parameter
          // combination.
          // If we are extending an already existing testset however,
          // then we have to walk over all tests, since a later test
          // might cover the same tuple.
          break;
        }

        ++test_index;
      }

      for (std::vector<unsigned int>::size_type i = 0;
           i < thread_local_param_combo_gain_per_value.size(); ++i) {

        thread_local_gain_per_value[i] +=
            thread_local_param_combo_gain_per_value[i];
      }

      return true;
    }

  private:
    const unsigned int real_current_param_idx_;
    const citcpp::detail::test &test_;
    const unsigned int current_test_index_;
    const citcpp::detail::internal_test_set &test_set_;
    const bool is_extend_mode_;
    citcpp::detail::thread_local_vector<
        citcpp::detail::aligned_vector<unsigned int>>
        param_combo_gain_per_value_;
    citcpp::detail::thread_local_vector<
        citcpp::detail::aligned_vector<unsigned long long>> &gain_per_value_;
    const citcpp::detail::param_combo_parallel_iterator &param_combo_it_;
};

struct new_covered_tuples_and_selected_value {
    unsigned long long num_new_covered_tuples_;
    int selected_value_;
};

new_covered_tuples_and_selected_value ipog_horizontal_select_best_value(
    const unsigned int real_current_param_idx,
    const unsigned int num_current_param_values,
    const citcpp::detail::test &test, unsigned int current_test_index,
    const citcpp::detail::internal_test_set &test_set, bool is_extend_mode,
    citcpp::detail::param_combo_iterator &param_combo_it,
    unsigned int &last_picked_value,
    std::vector<unsigned int> &value_to_num_picked) {
  using namespace citcpp::detail;

  new_covered_tuples_and_selected_value res;

  // This is an array containing the coverage gain per value of the current
  // parameter.
  std::vector<unsigned long long> gain_per_value(num_current_param_values);

  ipog_horizontal_select_best_value_per_param_combo_functor
      per_param_combo_functor(real_current_param_idx, test, current_test_index,
                              test_set, is_extend_mode, gain_per_value);
  param_combo_it.visit_all_parameter_combinations(per_param_combo_functor);

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
      // We use a simple tie breaking strategy: We do not favor one value
      // over the other. If two values have the same gain, then we pick the
      // one which we have picked less so far. Since also this could be a
      // tie (we have picked the value the same number of times, we remember
      // the value we have picked before, and choose the next one in this
      // case.
      if (value_with_max_gain >= 0 &&
          value_to_num_picked[value] <
              value_to_num_picked[value_with_max_gain]) {

        value_with_max_gain = value;
      }
    }
  }

  if (value_with_max_gain >= 0) {
    last_picked_value = value_with_max_gain;
    value_to_num_picked[value_with_max_gain]++;
  }

  res.num_new_covered_tuples_ = max_gain;
  res.selected_value_ = value_with_max_gain;

  return res;
}

new_covered_tuples_and_selected_value ipog_horizontal_select_best_value(
    const unsigned int real_current_param_idx,
    const unsigned int num_current_param_values,
    const citcpp::detail::test &test, unsigned int current_test_index,
    const citcpp::detail::internal_test_set &test_set, bool is_extend_mode,
    citcpp::detail::param_combo_parallel_iterator &param_combo_it,
    unsigned int &last_picked_value,
    std::vector<unsigned int> &value_to_num_picked) {
  using namespace citcpp::detail;

  new_covered_tuples_and_selected_value res;

  // This is an array containing the coverage gain per value of the current
  // parameter.
  thread_local_vector<aligned_vector<unsigned long long>> gain_per_value(
      param_combo_it.get_num_workers(),
      aligned_vector<unsigned long long>(num_current_param_values, 0));

  ipog_horizontal_select_best_value_per_param_combo_functor_parallel
      per_param_combo_functor(real_current_param_idx, num_current_param_values,
                              test, current_test_index, test_set,
                              is_extend_mode, gain_per_value, param_combo_it);
  param_combo_it.visit_all_parameter_combinations(per_param_combo_functor);

  int value_with_max_gain = -1;
  unsigned long long max_gain = 0;
  for (unsigned int v_index = 0; v_index < num_current_param_values;
       ++v_index) {

    unsigned int value =
        (v_index + last_picked_value + 1) % num_current_param_values;

    unsigned long long value_gain = 0;
    for (aligned_vector<unsigned long long> &thread_local_gain_per_value :
         gain_per_value) {

      value_gain += thread_local_gain_per_value.value[value];
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
          value_to_num_picked[value] <
              value_to_num_picked[value_with_max_gain]) {

        value_with_max_gain = value;
      }
    }
  }

  if (value_with_max_gain >= 0) {
    last_picked_value = value_with_max_gain;
    value_to_num_picked[value_with_max_gain]++;
  }

  res.num_new_covered_tuples_ = max_gain;
  res.selected_value_ = value_with_max_gain;

  return res;
}

}  // namespace

namespace citcpp {
namespace detail {

ipog_horizontal_extension_result ipog_horizontal_extension(
    const unsigned int current_param_idx, const unsigned int strength,
    const model &model, const std::vector<unsigned int> &parameter_index_map,
    const unsigned long long num_missing_combinations_to_cover,
    internal_test_set &test_set, bool is_extend_mode) {

  const unsigned int real_current_param_idx =
      parameter_index_map[current_param_idx];
  const int num_current_param_values =
      model.get_parameters()[real_current_param_idx];

  // First initialize the result object.
  ipog_horizontal_extension_result result{
      std::vector<list_intrusive<test_list_intrusive_integ>>(
          num_current_param_values),
      list_intrusive<test_list_intrusive_integ>(), 0};

  unsigned int last_picked_value = 0;
  std::vector<unsigned int> value_to_num_picked(num_current_param_values);
  param_combo_iterator param_combo_it(current_param_idx + 1, strength,
                                      parameter_index_map, true);

  unsigned int test_index = 0;
  for (test &t : test_set.get_list_of_tests()) {
    if (strength > 2) {
      last_picked_value = num_current_param_values - 1;
    }

    new_covered_tuples_and_selected_value res =
        ipog_horizontal_select_best_value(
            real_current_param_idx, num_current_param_values, t, test_index,
            test_set, is_extend_mode, param_combo_it, last_picked_value,
            value_to_num_picked);

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
      // Since we have not set a concrete value, we increase the number of
      // dont care values in the test.
      t.set_num_dont_care_values(t.get_num_dont_care_values() + 1);
    }

    // Keep track of how many tuples we have covered in addition.
    result.num_new_covered_tuples += res.num_new_covered_tuples_;

    if (result.num_new_covered_tuples >= num_missing_combinations_to_cover) {
      return result;
    }

    ++test_index;
  }

  return result;
}

ipog_horizontal_extension_result ipog_horizontal_extension(
    const unsigned int current_param_idx, const unsigned int strength,
    const model &model, const std::vector<unsigned int> &parameter_index_map,
    const unsigned long long num_missing_combinations_to_cover,
    internal_test_set &test_set, bool is_extend_mode, thread_pool &tp) {

  const unsigned int real_current_param_idx =
      parameter_index_map[current_param_idx];
  const int num_current_param_values =
      model.get_parameters()[real_current_param_idx];

  // First initialize the result object.
  ipog_horizontal_extension_result result{
      std::vector<list_intrusive<test_list_intrusive_integ>>(
          num_current_param_values),
      list_intrusive<test_list_intrusive_integ>(), 0};

  unsigned int last_picked_value = 0;
  std::vector<unsigned int> value_to_num_picked(num_current_param_values);
  param_combo_parallel_iterator param_combo_it(current_param_idx + 1, strength,
                                               parameter_index_map, true, tp);

  unsigned int test_index = 0;
  for (test &t : test_set.get_list_of_tests()) {
    if (strength > 2) {
      last_picked_value = num_current_param_values - 1;
    }

    new_covered_tuples_and_selected_value res =
        ipog_horizontal_select_best_value(
            real_current_param_idx, num_current_param_values, t, test_index,
            test_set, is_extend_mode, param_combo_it, last_picked_value,
            value_to_num_picked);

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
      // Since we have not set a concrete value, we increase the number of
      // dont care values in the test.
      t.set_num_dont_care_values(t.get_num_dont_care_values() + 1);
    }

    // Keep track of how many tuples we have covered in addition.
    result.num_new_covered_tuples += res.num_new_covered_tuples_;

    if (result.num_new_covered_tuples >= num_missing_combinations_to_cover) {
      return result;
    }

    ++test_index;
  }

  return result;
}

}  // namespace detail
}  // namespace citcpp
