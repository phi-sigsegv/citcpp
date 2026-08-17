#include "ipog_horizontal_extension.hpp"

#include <algorithm>

namespace {

class ipog_horizontal_select_best_value_per_param_combo_functor {
  public:
    ipog_horizontal_select_best_value_per_param_combo_functor(
        const citcpp::detail::internal_model& model,
        unsigned int num_current_param_values)
        : model_(model), gain_per_value_(num_current_param_values) {}

    void operator()(const citcpp::detail::test& test,
                    const citcpp::detail::bitset_uint64& valid_values,
                    const citcpp::detail::ipog_coverage_map::second_level_type&
                        value_combinations) {

      using namespace citcpp::detail;

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
        for (std::size_t i = 0; i < param_indices.size() - 1; ++i) {
          const int param_value = test.get_values()[param_indices[i]];

          if (param_value < 0) {
            // We have found a don't care value for that combination in
            // the considered test.
            return;
          }

          ipog_coverage_map::second_level_type::size_type addend = param_value;
          for (std::size_t j = i + 1; j < param_indices.size(); ++j) {
            addend *= static_cast<decltype(addend)>(
                model_.get_parameter_num_values()[param_indices[j]]);
          }
          base_index += addend;
        }

        // If we have found a don't care value in one of the [0, ...
        // ,current_param_idx - 1] parameters, then we skip the combination in
        // the coverage gain computation.
        for (std::size_t value = 0; value < gain_per_value_.size(); ++value) {
          const auto value_idx =
              static_cast<ipog_coverage_map::second_level_type::size_type>(
                  value);
          if (!value_combinations.is_marked_covered(base_index + value_idx)) {
            if (valid_values.test(value_idx)) {
              gain_per_value_[value_idx] += 1;
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
    const citcpp::detail::internal_model& model_;
    std::vector<unsigned long long> gain_per_value_;
};

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

class ipog_horizontal_select_best_value_functor {
  public:
    ipog_horizontal_select_best_value_functor(
        unsigned int real_current_param_idx,
        unsigned int num_current_param_values,
        const citcpp::detail::internal_model& model,
        const std::vector<std::pair<const citcpp::detail::internal_relation*,
                                    citcpp::detail::ipog_coverage_map>>&
            relations,
        int& last_picked_value, std::vector<int>& value_to_valid_options)
        : real_current_param_idx_(real_current_param_idx),
          num_current_param_values_(num_current_param_values),
          per_param_combo_functor_(model, num_current_param_values),
          relations_(relations),
          last_picked_value_(last_picked_value),
          value_to_valid_options_(value_to_valid_options) {}

    int operator()(const citcpp::detail::test& test,
                   const citcpp::detail::bitset_uint64& valid_values) {
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
    const std::vector<std::pair<const citcpp::detail::internal_relation*,
                                citcpp::detail::ipog_coverage_map>>& relations_;
    int& last_picked_value_;
    std::vector<int>& value_to_valid_options_;
};

class ipog_horizontal_update_coverage_map_per_param_combo_functor {
  public:
    ipog_horizontal_update_coverage_map_per_param_combo_functor(
        const citcpp::detail::internal_model& model)
        : model_(model), num_new_covered_tuples_(0) {}

    void operator()(const citcpp::detail::test& test,
                    const citcpp::detail::bitset_uint64& valid_values,
                    citcpp::detail::ipog_coverage_map::second_level_type&
                        value_combinations) {

      using namespace citcpp::detail;

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
        for (std::size_t i = 0; i < param_indices.size() - 1; ++i) {
          const int param_value = test.get_values()[param_indices[i]];

          if (param_value < 0) {
            // We have found a don't care value for that combination in
            // the considered test in one of the [0, ... ,current_param_idx - 1]
            // parameters. There is nothing to be updated concerning the
            // coverage. This combination will be taken care of during the
            // vertical extension step.
            return;
          }

          ipog_coverage_map::second_level_type::size_type addend = param_value;
          for (std::size_t j = i + 1; j < param_indices.size(); ++j) {
            addend *= static_cast<decltype(addend)>(
                model_.get_parameter_num_values()[param_indices[j]]);
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

        for (std::size_t value = 0; value < valid_values.size(); ++value) {
          const auto value_idx =
              static_cast<ipog_coverage_map::second_level_type::size_type>(
                  value);
          if (valid_values.test(value_idx)) {
            value_combinations.set_valid(base_index + value_idx);
          }
        }
      }
    }

    unsigned long long get_num_new_covered_tuples() const {
      return num_new_covered_tuples_;
    }

    void reset() { num_new_covered_tuples_ = 0; }

  private:
    const citcpp::detail::internal_model& model_;
    unsigned long long num_new_covered_tuples_;
};

class ipog_horizontal_update_coverage_map_functor {
  public:
    ipog_horizontal_update_coverage_map_functor(
        unsigned int real_current_param_idx,
        const citcpp::detail::internal_model& model,
        std::vector<std::pair<const citcpp::detail::internal_relation*,
                              citcpp::detail::ipog_coverage_map>>& relations,
        std::unordered_map<const citcpp::detail::internal_relation*,
                           unsigned long long>& num_covered_tuples)
        : per_param_combo_functor_(model),
          real_current_param_idx_(real_current_param_idx),
          relations_(relations),
          num_covered_tuples_(num_covered_tuples) {}

    void operator()(const citcpp::detail::test& test,
                    const citcpp::detail::bitset_uint64& valid_values) {
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
    std::vector<std::pair<const citcpp::detail::internal_relation*,
                          citcpp::detail::ipog_coverage_map>>& relations_;
    std::unordered_map<const citcpp::detail::internal_relation*,
                       unsigned long long>& num_covered_tuples_;
};

class
    ipog_horizontal_update_coverage_map_and_select_best_value_per_param_combo_functor {
  public:
    ipog_horizontal_update_coverage_map_and_select_best_value_per_param_combo_functor(
        const citcpp::detail::internal_model& model,
        unsigned int num_current_param_values)
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
        unsigned int real_current_param_idx,
        unsigned int num_current_param_values,
        const citcpp::detail::internal_model& model,
        std::vector<std::pair<const citcpp::detail::internal_relation*,
                              citcpp::detail::ipog_coverage_map>>& relations,
        int& last_picked_value, std::vector<int>& value_to_valid_options,
        std::unordered_map<const citcpp::detail::internal_relation*,
                           unsigned long long>& num_covered_tuples)
        : real_current_param_idx_(real_current_param_idx),
          num_current_param_values_(num_current_param_values),
          per_param_combo_functor_(model, num_current_param_values),
          relations_(relations),
          last_picked_value_(last_picked_value),
          value_to_valid_options_(value_to_valid_options),
          num_covered_tuples_(num_covered_tuples) {}

    citcpp::detail::new_covered_tuples_and_selected_value operator()(
        const citcpp::detail::test& prev_test, const citcpp::detail::test& test,
        const citcpp::detail::bitset_uint64& prev_test_valid_values,
        const citcpp::detail::bitset_uint64& valid_values) {

      using namespace citcpp::detail;

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
    std::vector<std::pair<const citcpp::detail::internal_relation*,
                          citcpp::detail::ipog_coverage_map>>& relations_;
    int& last_picked_value_;
    std::vector<int>& value_to_valid_options_;
    std::unordered_map<const citcpp::detail::internal_relation*,
                       unsigned long long>& num_covered_tuples_;
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

  int last_picked_value = 0;
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
  std::size_t test_index = 0;
  for (test& t : test_set.get_list_of_tests()) {
    if (std::ranges::any_of(
            relations.begin(), relations.end(), [](const auto& p) {
              return p.first->get_current_interaction_strength() > 2;
            })) {
      last_picked_value = static_cast<int>(num_current_param_values - 1);
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
