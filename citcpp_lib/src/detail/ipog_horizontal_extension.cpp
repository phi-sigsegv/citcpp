#include "ipog_horizontal_extension.hpp"

#include <algorithm>

namespace {

inline int ipog_horizontal_select_best_value(
    unsigned int num_current_param_values,
    const std::vector<unsigned long long>& gain_per_value,
    int& last_picked_value, std::vector<int>& value_to_valid_options) {

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

// The coverage related operations of the horizontal extension, on the
// coverage maps of all relations involving the current parameter.
class ipog_horizontal_extension_ops {
  public:
    ipog_horizontal_extension_ops(
        unsigned int real_current_param_idx,
        unsigned int num_current_param_values,
        std::vector<std::pair<const citcpp::detail::internal_relation*,
                              citcpp::detail::ipog_coverage_map>>& relations,
        int& last_picked_value, std::vector<int>& value_to_valid_options,
        std::unordered_map<const citcpp::detail::internal_relation*,
                           unsigned long long>& num_covered_tuples)
        : real_current_param_idx_(real_current_param_idx),
          num_current_param_values_(num_current_param_values),
          relations_(relations),
          last_picked_value_(last_picked_value),
          value_to_valid_options_(value_to_valid_options),
          num_covered_tuples_(num_covered_tuples),
          gain_per_value_(num_current_param_values, 0) {}

    // Selects the value of the current parameter for the first test.
    int select_first(const citcpp::detail::test& test,
                     const citcpp::detail::bitset_uint64& valid_values) {
      // We first check whether the test already has a concrete value
      // for the current parameter, because if so, then there is no
      // point in evaluating a coverage gain.
      if (test.get_values()[real_current_param_idx_] >= 0) {
        last_picked_value_ = test.get_values()[real_current_param_idx_];
        value_to_valid_options_[last_picked_value_]--;

        return last_picked_value_;
      }

      add_gains(test, valid_values);
      return select();
    }

    // Updates the coverage by the previous test (whose value of the current
    // parameter has been set), and selects the value of the current parameter
    // for the given test.
    citcpp::detail::new_covered_tuples_and_selected_value update_and_select(
        const citcpp::detail::test& prev_test, const citcpp::detail::test& test,
        const citcpp::detail::bitset_uint64& prev_test_valid_values,
        const citcpp::detail::bitset_uint64& valid_values) {

      citcpp::detail::new_covered_tuples_and_selected_value res{0, 0};
      res.num_new_covered_tuples_ = update(prev_test, prev_test_valid_values);

      const int current_param_value =
          test.get_values()[real_current_param_idx_];
      if (current_param_value < 0) {
        add_gains(test, valid_values);
      } else {
        // The test already has a concrete value for the current parameter:
        // there is no point in evaluating a coverage gain; make sure this
        // value is selected.
        gain_per_value_[current_param_value]++;
      }

      res.selected_value_ = select();
      return res;
    }

    // Updates the coverage by the given test, if its value of the current
    // parameter has been set. Returns the number of newly covered tuples.
    unsigned long long update(const citcpp::detail::test& test,
                              const citcpp::detail::bitset_uint64& valid_values) {
      const int current_param_value =
          test.get_values()[real_current_param_idx_];
      unsigned long long total = 0;
      for (auto& [relation, cov_map] : relations_) {
        unsigned long long newly = 0;
        if (current_param_value >= 0) {
          newly = cov_map.cover(test, current_param_value, valid_values);
        }
        num_covered_tuples_[relation] += newly;
        total += newly;
      }
      return total;
    }

  private:
    void add_gains(const citcpp::detail::test& test,
                   const citcpp::detail::bitset_uint64& valid_values) {
      for (const auto& rel_and_cov_map : relations_) {
        rel_and_cov_map.second.add_gains(test, valid_values, gain_per_value_);
      }
    }

    int select() {
      const int selected_value = ipog_horizontal_select_best_value(
          num_current_param_values_, gain_per_value_, last_picked_value_,
          value_to_valid_options_);
      std::fill(gain_per_value_.begin(), gain_per_value_.end(), 0);
      return selected_value;
    }

    const unsigned int real_current_param_idx_;
    const unsigned int num_current_param_values_;
    std::vector<std::pair<const citcpp::detail::internal_relation*,
                          citcpp::detail::ipog_coverage_map>>& relations_;
    int& last_picked_value_;
    std::vector<int>& value_to_valid_options_;
    std::unordered_map<const citcpp::detail::internal_relation*,
                       unsigned long long>& num_covered_tuples_;
    std::vector<unsigned long long> gain_per_value_;
};

}  // namespace

namespace citcpp {
namespace detail {

ipog_horizontal_extension_result ipog_horizontal_extension(
    unsigned long long num_missing_combinations_to_cover,
    constraint_handler& constr_handler, internal_test_set& test_set,
    std::vector<std::pair<const internal_relation*, ipog_coverage_map>>&
        relations) {

  const unsigned int real_current_param_idx =
      relations[0].second.get_parameter_index_map()
          [relations[0].first->get_current_param_idx()];
  const unsigned int num_current_param_values =
      relations[0]
          .second.get_model()
          .get_parameter_num_values()[real_current_param_idx];

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

  int last_picked_value = 0;
  std::vector<int> value_to_valid_options(
      get_value_to_valid_options(num_current_param_values, valid_values));

  ipog_horizontal_extension_ops ops(
      real_current_param_idx, num_current_param_values, relations,
      last_picked_value, value_to_valid_options, result.num_new_covered_tuples);

  test* previous_test = nullptr;
  int selected_value = 0;
  unsigned long long num_new_covered_tuples = 0;
  std::size_t test_index = 0;
  for (test& t : test_set.get_list_of_tests()) {
    if (std::ranges::any_of(
            relations.begin(), relations.end(), [](const auto& p) {
              return p.first->get_current_interaction_strength() > 2;
            })) {
      last_picked_value = static_cast<int>(num_current_param_values - 1);
    }

    if (!previous_test) {
      selected_value = ops.select_first(t, valid_values[test_index]);
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
          ops.update_and_select(*previous_test, t, valid_values[test_index - 1],
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

    ops.update(*previous_test, valid_values[test_index - 1]);
  }

  return result;
}

}  // namespace detail
}  // namespace citcpp
