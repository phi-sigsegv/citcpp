#include "ipog_otf_horizontal_extension.hpp"

#include <algorithm>

#include "citcpp_utils.hpp"
#include "constraint_handler_concurrent.hpp"
#include "param_combo_iteration.hpp"
#include "shared_constants.hpp"

namespace {

class ipog_horizontal_select_best_value_per_param_combo_functor {
  public:
    ipog_horizontal_select_best_value_per_param_combo_functor(
        const unsigned int real_current_param_idx,
        const citcpp::detail::test& test,
        const citcpp::detail::bitset_uint64& valid_values,
        unsigned int current_test_index,
        const citcpp::detail::internal_test_set& test_set, bool is_extend_mode,
        std::vector<unsigned long long>& gain_per_value)
        : real_current_param_idx_(real_current_param_idx),
          test_(test),
          valid_values_(valid_values),
          current_test_index_(current_test_index),
          test_set_(test_set),
          is_extend_mode_(is_extend_mode),
          param_combo_gain_per_value_(gain_per_value.size()),
          gain_per_value_(gain_per_value) {}

    bool operator()(const citcpp::detail::param_vector& param_indices) {
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

  private:
    const unsigned int real_current_param_idx_;
    const citcpp::detail::test& test_;
    const citcpp::detail::bitset_uint64& valid_values_;
    const unsigned int current_test_index_;
    const citcpp::detail::internal_test_set& test_set_;
    const bool is_extend_mode_;
    std::vector<unsigned int> param_combo_gain_per_value_;
    std::vector<unsigned long long>& gain_per_value_;
};

class ipog_horizontal_select_best_value_per_param_combo_functor_parallel {
  public:
    ipog_horizontal_select_best_value_per_param_combo_functor_parallel(
        const unsigned int real_current_param_idx,
        const unsigned int num_current_param_values,
        const citcpp::detail::test& test,
        const citcpp::detail::bitset_uint64& valid_values,
        unsigned int current_test_index,
        const citcpp::detail::internal_test_set& test_set, bool is_extend_mode,
        citcpp::detail::thread_local_vector<
            citcpp::detail::aligned_vector<unsigned long long>>& gain_per_value,
        const citcpp::detail::thread_pool& tp)
        : real_current_param_idx_(real_current_param_idx),
          test_(test),
          valid_values_(valid_values),
          current_test_index_(current_test_index),
          test_set_(test_set),
          is_extend_mode_(is_extend_mode),
          tp_(tp),
          param_combo_gain_per_value_(
              tp.get_num_workers(),
              citcpp::detail::aligned_vector<unsigned int>(
                  num_current_param_values, 0)),
          gain_per_value_(gain_per_value) {}

    bool operator()(const citcpp::detail::param_vector& param_indices) {
      using namespace citcpp::detail;

      std::vector<unsigned int>& thread_local_param_combo_gain_per_value =
          param_combo_gain_per_value_[tp_.get_worker_id()].value;
      std::vector<unsigned long long>& thread_local_gain_per_value =
          gain_per_value_[tp_.get_worker_id()].value;

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

        if (valid_values_.test(i)) {
          thread_local_gain_per_value[i] +=
              thread_local_param_combo_gain_per_value[i];
        }
      }

      return true;
    }

  private:
    const unsigned int real_current_param_idx_;
    const citcpp::detail::test& test_;
    const citcpp::detail::bitset_uint64& valid_values_;
    const unsigned int current_test_index_;
    const citcpp::detail::internal_test_set& test_set_;
    const bool is_extend_mode_;
    const citcpp::detail::thread_pool& tp_;
    alignas(citcpp::detail::false_sharing_avoidance_alignment)
        citcpp::detail::thread_local_vector<citcpp::detail::aligned_vector<
            unsigned int>> param_combo_gain_per_value_;
    alignas(citcpp::detail::false_sharing_avoidance_alignment)
        citcpp::detail::thread_local_vector<citcpp::detail::aligned_vector<
            unsigned long long>>& gain_per_value_;
};

struct new_covered_tuples_and_selected_value {
    unsigned long long num_new_covered_tuples_;
    int selected_value_;
};

new_covered_tuples_and_selected_value ipog_horizontal_select_best_value(
    const unsigned int real_current_param_idx,
    const unsigned int num_current_param_values,
    const citcpp::detail::bitset_uint64& valid_values,
    const citcpp::detail::test& test, unsigned int current_test_index,
    const citcpp::detail::internal_test_set& test_set, bool is_extend_mode,
    std::vector<std::pair<const citcpp::detail::internal_relation*,
                          citcpp::detail::param_combo_iterator>>&
        param_combo_its,
    unsigned int& last_picked_value,
    std::vector<unsigned int>& value_to_num_picked,
    std::unordered_map<const citcpp::detail::internal_relation*,
                       unsigned long long>& num_covered_tuples) {
  using namespace citcpp::detail;

  new_covered_tuples_and_selected_value res{0, 0};

  // In the following vector we track for each relation, the coverage
  // gain per value of the current parameter.
  std::vector<std::vector<unsigned long long>> relation_gain_per_value(
      param_combo_its.size(),
      std::vector<unsigned long long>(num_current_param_values, 0));

  for (int relation_idx = 0; relation_idx < param_combo_its.size();
       relation_idx++) {

    ipog_horizontal_select_best_value_per_param_combo_functor
        per_param_combo_functor(real_current_param_idx, test, valid_values,
                                current_test_index, test_set, is_extend_mode,
                                relation_gain_per_value[relation_idx]);
    param_combo_its[relation_idx].second.visit_all_parameter_combinations(
        per_param_combo_functor);
  }

  int value_with_max_gain = -1;
  unsigned long long max_gain = 0;
  for (unsigned int v_index = 0; v_index < num_current_param_values;
       ++v_index) {

    unsigned int value =
        (v_index + last_picked_value + 1) % num_current_param_values;

    unsigned long long value_gain = 0;
    for (int relation_idx = 0; relation_idx < param_combo_its.size();
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
          value_to_num_picked[value] <
              value_to_num_picked[value_with_max_gain]) {

        value_with_max_gain = value;
      }
    }
  }

  if (value_with_max_gain >= 0) {
    last_picked_value = value_with_max_gain;
    value_to_num_picked[value_with_max_gain]++;

    for (int relation_idx = 0; relation_idx < param_combo_its.size();
         relation_idx++) {

      num_covered_tuples[param_combo_its[relation_idx].first] +=
          relation_gain_per_value[relation_idx][value_with_max_gain];
    }
  }

  res.num_new_covered_tuples_ = max_gain;
  res.selected_value_ = value_with_max_gain;

  return res;
}

new_covered_tuples_and_selected_value ipog_horizontal_select_best_value(
    const unsigned int real_current_param_idx,
    const unsigned int num_current_param_values,
    const citcpp::detail::bitset_uint64& valid_values,
    const citcpp::detail::test& test, unsigned int current_test_index,
    const citcpp::detail::internal_test_set& test_set, bool is_extend_mode,
    std::vector<std::pair<const citcpp::detail::internal_relation*,
                          citcpp::detail::param_combo_parallel_iterator>>&
        param_combo_its,
    const citcpp::detail::thread_pool& tp, unsigned int& last_picked_value,
    std::vector<unsigned int>& value_to_num_picked,
    std::unordered_map<const citcpp::detail::internal_relation*,
                       unsigned long long>& num_covered_tuples) {
  using namespace citcpp::detail;

  new_covered_tuples_and_selected_value res{0, 0};

  // In the following vector we track the same information, but specific
  // to the relation.
  // In the following vector we track for each relation, the thread-specific
  // coverage gain per value of the current parameter.
  std::vector<thread_local_vector<aligned_vector<unsigned long long>>>
      relation_gain_per_value(
          param_combo_its.size(),
          thread_local_vector<aligned_vector<unsigned long long>>(
              tp.get_num_workers(),
              aligned_vector<unsigned long long>(num_current_param_values, 0)));

  for (int relation_idx = 0; relation_idx < param_combo_its.size();
       relation_idx++) {

    ipog_horizontal_select_best_value_per_param_combo_functor_parallel
        per_param_combo_functor(real_current_param_idx,
                                num_current_param_values, test, valid_values,
                                current_test_index, test_set, is_extend_mode,
                                relation_gain_per_value[relation_idx], tp);
    param_combo_its[relation_idx].second.visit_all_parameter_combinations(
        per_param_combo_functor);
  }

  int value_with_max_gain = -1;
  unsigned long long max_gain = 0;
  for (unsigned int v_index = 0; v_index < num_current_param_values;
       ++v_index) {

    unsigned int value =
        (v_index + last_picked_value + 1) % num_current_param_values;

    unsigned long long value_gain = 0;
    for (int relation_idx = 0; relation_idx < param_combo_its.size();
         relation_idx++) {

      for (const aligned_vector<unsigned long long>&
               thread_local_gain_per_value :
           relation_gain_per_value[relation_idx]) {

        value_gain += thread_local_gain_per_value.value[value];
      }
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

    for (int relation_idx = 0; relation_idx < param_combo_its.size();
         relation_idx++) {

      for (const aligned_vector<unsigned long long>&
               thread_local_gain_per_value :
           relation_gain_per_value[relation_idx]) {

        num_covered_tuples[param_combo_its[relation_idx].first] +=
            thread_local_gain_per_value.value[value_with_max_gain];
      }
    }
  }

  res.num_new_covered_tuples_ = max_gain;
  res.selected_value_ = value_with_max_gain;

  return res;
}

}  // namespace

namespace citcpp {
namespace detail {

ipog_horizontal_extension_result ipog_horizontal_extension(
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

  unsigned int last_picked_value = 0;
  std::vector<unsigned int> value_to_num_picked(num_current_param_values);
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
        ipog_horizontal_select_best_value(
            real_current_param_idx, num_current_param_values,
            valid_values[test_index], t, test_index, test_set, is_extend_mode,
            param_combo_its, last_picked_value, value_to_num_picked,
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

ipog_horizontal_extension_result ipog_horizontal_extension(
    const unsigned long long num_missing_combinations_to_cover,
    const constraint_handler& constr_handler, internal_test_set& test_set,
    const internal_model& model,
    const std::vector<internal_relation>& relations, bool is_extend_mode,
    thread_pool& tp) {

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

  unsigned int last_picked_value = 0;
  std::vector<unsigned int> value_to_num_picked(num_current_param_values);
  std::vector<
      std::pair<const internal_relation*, param_combo_parallel_iterator>>
      param_combo_its;
  for (auto& rel : relations) {
    param_combo_its.emplace_back(
        &rel,
        param_combo_parallel_iterator(rel.get_current_param_idx() + 1,
                                      rel.get_current_interaction_strength(),
                                      rel.get_parameter_index_map(), true, tp));
  }

  // Call the constraint handler and ask for a mapping from tests to possible
  // extension values.
  std::vector<bitset_uint64> valid_values(
      constr_handler.is_thread_safe()
          ? concurrent_constraint_handler(constr_handler)
                .get_valid_parameter_assignments(test_set,
                                                 real_current_param_idx, tp)
          : constr_handler.get_valid_parameter_assignments(
                test_set, real_current_param_idx));

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
        ipog_horizontal_select_best_value(
            real_current_param_idx, num_current_param_values,
            valid_values[test_index], t, test_index, test_set, is_extend_mode,
            param_combo_its, tp, last_picked_value, value_to_num_picked,
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
