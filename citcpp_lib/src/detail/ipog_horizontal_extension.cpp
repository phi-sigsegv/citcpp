#include "ipog_horizontal_extension.hpp"

#include <new>

#include "citcpp_utils.hpp"

namespace {

class ipog_horizontal_select_best_value_per_param_combo_functor {
  public:
    ipog_horizontal_select_best_value_per_param_combo_functor(
        const citcpp::detail::model &model, const citcpp::detail::test &test,
        std::vector<unsigned long long> &gain_per_value)
        : model_(model), test_(test), gain_per_value_(gain_per_value) {}

    bool operator()(const citcpp::detail::coverage_map::second_level_type
                        &value_combinations) {
      using namespace citcpp::detail;

      const param_vector &param_indices =
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
            addend *= model_.get_parameters()[param_indices[j]];
          }
          base_index += addend;
        }

        // If we have found a don't care value in one of the [0, ...
        // ,current_param_idx - 1] parameters, then we skip the combination in
        // the coverage gain computation.
        for (unsigned int value = 0; value < gain_per_value_.size(); ++value) {
          if (!value_combinations.test(base_index + value)) {
            gain_per_value_[value] += 1;
          }
        }
      }

      return true;
    }

  private:
    const citcpp::detail::model &model_;
    const citcpp::detail::test &test_;
    std::vector<unsigned long long> &gain_per_value_;
};

class ipog_horizontal_select_best_value_per_param_combo_functor_parallel {
  public:
    ipog_horizontal_select_best_value_per_param_combo_functor_parallel(
        const citcpp::detail::model &model, const citcpp::detail::test &test,
        citcpp::detail::thread_local_vector<
            std::vector<citcpp::detail::aligned_ull_value>> &gain_per_value,
        const citcpp::detail::coverage_map_parallel_iterator &cov_map_it)
        : model_(model),
          test_(test),
          gain_per_value_(gain_per_value),
          cov_map_it_(cov_map_it) {}

    bool operator()(
        citcpp::detail::coverage_map::second_level_type &value_combinations) {
      using namespace citcpp::detail;

      const param_vector &param_indices =
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
            addend *= model_.get_parameters()[param_indices[j]];
          }
          base_index += addend;
        }

        // If we have found a don't care value in one of the [0, ...
        // ,current_param_idx - 1] parameters, then we skip the combination in
        // the coverage gain computation.
        std::vector<citcpp::detail::aligned_ull_value>
            &thread_local_gain_per_value =
                gain_per_value_[cov_map_it_.get_worker_id()];
        for (unsigned int value = 0; value < thread_local_gain_per_value.size();
             ++value) {
          if (!value_combinations.test(base_index + value)) {
            thread_local_gain_per_value[value].value += 1;
          }
        }
      }

      return true;
    }

  private:
    const citcpp::detail::model &model_;
    const citcpp::detail::test &test_;
    citcpp::detail::thread_local_vector<
        std::vector<citcpp::detail::aligned_ull_value>> &gain_per_value_;
    const citcpp::detail::coverage_map_parallel_iterator &cov_map_it_;
};

int ipog_horizontal_select_best_value(
    const unsigned int real_current_param_idx,
    const unsigned int num_current_param_values,
    const citcpp::detail::model &model, const citcpp::detail::test &test,
    citcpp::detail::coverage_map_iterator &cov_map_it,
    unsigned int &last_picked_value,
    std::vector<unsigned int> &value_to_num_picked) {
  using namespace citcpp::detail;

  // We first check whether the test already has a concrete value
  // for the current parameter, because if so, then there is no
  // point in evaluating a coverage gain.
  if (test.get_values()[real_current_param_idx] >= 0) {
    last_picked_value = test.get_values()[real_current_param_idx];
    value_to_num_picked[last_picked_value]++;

    return last_picked_value;
  }

  // This is an array containing the coverage gain per value of the current
  // parameter.
  std::vector<unsigned long long> gain_per_value(num_current_param_values);

  ipog_horizontal_select_best_value_per_param_combo_functor
      per_param_combo_functor(model, test, gain_per_value);
  cov_map_it.visit_all_parameter_combinations(per_param_combo_functor);

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

  return value_with_max_gain;
}

int ipog_horizontal_select_best_value(
    const unsigned int real_current_param_idx,
    const unsigned int num_current_param_values,
    const citcpp::detail::model &model, const citcpp::detail::test &test,
    citcpp::detail::coverage_map_parallel_iterator &cov_map_it,
    unsigned int &last_picked_value,
    std::vector<unsigned int> &value_to_num_picked) {
  using namespace citcpp::detail;

  // We first check whether the test already has a concrete value
  // for the current parameter, because if so, then there is no
  // point in evaluating a coverage gain.
  if (test.get_values()[real_current_param_idx] >= 0) {
    last_picked_value = test.get_values()[real_current_param_idx];
    value_to_num_picked[last_picked_value]++;

    return last_picked_value;
  }

  // This is an array containing the coverage gain per value of the current
  // parameter.
  thread_local_vector<std::vector<citcpp::detail::aligned_ull_value>>
      gain_per_value(cov_map_it.get_num_workers(),
                     std::vector<citcpp::detail::aligned_ull_value>(
                         num_current_param_values));

  ipog_horizontal_select_best_value_per_param_combo_functor_parallel
      per_param_combo_functor(model, test, gain_per_value, cov_map_it);
  cov_map_it.visit_all_parameter_combinations(per_param_combo_functor);

  int value_with_max_gain = -1;
  unsigned long long max_gain = 0;
  for (unsigned int v_index = 0; v_index < num_current_param_values;
       ++v_index) {
    unsigned int value =
        (v_index + last_picked_value + 1) % num_current_param_values;

    unsigned long long value_gain = 0;
    for (std::vector<citcpp::detail::aligned_ull_value>
             &thread_local_gain_per_value : gain_per_value) {
      value_gain += thread_local_gain_per_value[value].value;
    }

    if (value_gain > max_gain) {
      value_with_max_gain = value;
      max_gain = value_gain;
    } else if (value_gain == max_gain) {
      // We use a simple tie breaking strategy: We do not favor one value over
      // the other. If two values have the same gain, then we pick the one which
      // we have picked less so far. Since also this could be a tie (we have
      // picked the value the same number of times, we remember the value we
      // have picked before, and choose the next one in this case.
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

  return value_with_max_gain;
}

class ipog_horizontal_update_coverage_map_per_param_combo_functor {
  public:
    ipog_horizontal_update_coverage_map_per_param_combo_functor(
        const citcpp::detail::model &model, const citcpp::detail::test &test,
        const int current_param_selected_value)
        : model_(model),
          test_(test),
          current_param_selected_value_(current_param_selected_value),
          num_new_covered_tuples_(0) {}

    bool operator()(
        citcpp::detail::coverage_map::second_level_type &value_combinations) {
      using namespace citcpp::detail;

      const param_vector &param_indices =
          value_combinations.get_parameter_indices();

      if (!value_combinations.all()) {
        // Here we compute an index into the bitset. To do so, we treat the
        // number of values of each parameter as a kind of radix. Consider three
        // parameters p_0, p_1, p_2. The last parameter is always the current
        // one processed by IPOG. Now say that v_i is the number of values for
        // p_i. If we now have values x_0, x_1, x_2, then the index is x_0 * v_1
        // * v_2 + x_1 * v_2 + x_2.
        coverage_map::second_level_type::size_type index =
            current_param_selected_value_;
        for (std::vector<unsigned int>::size_type i = 0;
             i < param_indices.size() - 1; ++i) {
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
            addend *= model_.get_parameters()[param_indices[j]];
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

  private:
    const citcpp::detail::model &model_;
    const citcpp::detail::test &test_;
    const int current_param_selected_value_;
    unsigned long long num_new_covered_tuples_;
};

class ipog_horizontal_update_coverage_map_per_param_combo_functor_parallel {
  public:
    ipog_horizontal_update_coverage_map_per_param_combo_functor_parallel(
        const citcpp::detail::model &model, const citcpp::detail::test &test,
        const int current_param_selected_value,
        const citcpp::detail::coverage_map_parallel_iterator &cov_map_it)
        : model_(model),
          test_(test),
          current_param_selected_value_(current_param_selected_value),
          num_new_covered_tuples_(cov_map_it.get_num_workers()),
          cov_map_it_(cov_map_it) {}

    bool operator()(
        citcpp::detail::coverage_map::second_level_type &value_combinations) {
      using namespace citcpp::detail;

      const param_vector &param_indices =
          value_combinations.get_parameter_indices();

      if (!value_combinations.all()) {
        // Here we compute an index into the bitset. To do so, we treat the
        // number of values of each parameter as a kind of radix. Consider three
        // parameters p_0, p_1, p_2. The last parameter is always the current
        // one processed by IPOG. Now say that v_i is the number of values for
        // p_i. If we now have values x_0, x_1, x_2, then the index is x_0 * v_1
        // * v_2 + x_1 * v_2 + x_2.
        coverage_map::second_level_type::size_type index =
            current_param_selected_value_;
        for (std::vector<unsigned int>::size_type i = 0;
             i < param_indices.size() - 1; ++i) {
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
            addend *= model_.get_parameters()[param_indices[j]];
          }
          index += addend;
        }

        if (!value_combinations.test_and_set(index)) {
          ++num_new_covered_tuples_[cov_map_it_.get_worker_id()].value;
        }
      }

      return true;
    }

    unsigned long long get_num_new_covered_tuples() const {
      unsigned long long ret = 0;
      for (const auto &i : num_new_covered_tuples_) {
        ret += i.value;
      }

      return ret;
    }

  private:
    const citcpp::detail::model &model_;
    const citcpp::detail::test &test_;
    const int current_param_selected_value_;
    citcpp::detail::thread_local_vector<citcpp::detail::aligned_ull_value>
        num_new_covered_tuples_;
    const citcpp::detail::coverage_map_parallel_iterator &cov_map_it_;
};

unsigned long long ipog_horizontal_update_coverage_map(
    const citcpp::detail::model &model, const citcpp::detail::test &test,
    const int current_param_selected_value,
    citcpp::detail::coverage_map_iterator &cov_map_it) {
  using namespace citcpp::detail;

  ipog_horizontal_update_coverage_map_per_param_combo_functor
      per_param_combo_functor(model, test, current_param_selected_value);
  cov_map_it.visit_all_parameter_combinations(per_param_combo_functor);

  return per_param_combo_functor.get_num_new_covered_tuples();
}

unsigned long long ipog_horizontal_update_coverage_map(
    const citcpp::detail::model &model, const citcpp::detail::test &test,
    const int current_param_selected_value,
    citcpp::detail::coverage_map_parallel_iterator &cov_map_it) {
  using namespace citcpp::detail;

  ipog_horizontal_update_coverage_map_per_param_combo_functor_parallel
      per_param_combo_functor(model, test, current_param_selected_value,
                              cov_map_it);
  cov_map_it.visit_all_parameter_combinations(per_param_combo_functor);

  return per_param_combo_functor.get_num_new_covered_tuples();
}

struct new_covered_tuples_and_selected_value {
    unsigned long long num_new_covered_tuples_;
    int selected_value_;
};

class
    ipog_horizontal_update_coverage_map_and_select_best_value_per_param_combo_functor {
  public:
    ipog_horizontal_update_coverage_map_and_select_best_value_per_param_combo_functor(
        const citcpp::detail::model &model,
        const citcpp::detail::test &prev_test, const citcpp::detail::test &test,
        std::vector<unsigned long long> &gain_per_value,
        const bool enable_gain_computation,
        const int current_param_selected_value_for_prev_test)
        : model_(model),
          prev_test_(prev_test),
          test_(test),
          gain_per_value_(gain_per_value),
          enable_gain_computation_(enable_gain_computation),
          current_param_selected_value_for_prev_test_(
              current_param_selected_value_for_prev_test),
          num_new_covered_tuples_(0) {}

    bool operator()(
        citcpp::detail::coverage_map::second_level_type &value_combinations) {
      if (current_param_selected_value_for_prev_test_ >= 0) {
        update_coverage(value_combinations);
      }
      if (enable_gain_computation_) {
        compute_gain_per_value(value_combinations);
      }

      return true;
    }

    void update_coverage(
        citcpp::detail::coverage_map::second_level_type &value_combinations) {
      using namespace citcpp::detail;

      const param_vector &param_indices =
          value_combinations.get_parameter_indices();

      if (!value_combinations.all()) {
        // Here we compute an index into the bitset. To do so, we treat the
        // number of values of each parameter as a kind of radix. Consider three
        // parameters p_0, p_1, p_2. The last parameter is always the current
        // one processed by IPOG. Now say that v_i is the number of values for
        // p_i. If we now have values x_0, x_1, x_2, then the index is x_0 * v_1
        // * v_2 + x_1 * v_2 + x_2.
        coverage_map::second_level_type::size_type index =
            current_param_selected_value_for_prev_test_;
        for (std::vector<unsigned int>::size_type i = 0;
             i < param_indices.size() - 1; ++i) {
          const unsigned int param_idx = param_indices[i];
          const int param_value = prev_test_.get_values()[param_idx];

          if (param_value < 0) {
            // We have found a don't care value for that combination in
            // the considered test in one of the [0, ... ,current_param_idx - 1]
            // parameters. There is nothing to be updated concerning the
            // coverage. This combination will be taken care of during the
            // vertical extension step.
            return;
          }

          coverage_map::second_level_type::size_type addend = param_value;
          for (std::vector<unsigned int>::size_type j = i + 1;
               j < param_indices.size(); ++j) {
            addend *= model_.get_parameters()[param_indices[j]];
          }
          index += addend;
        }

        if (!value_combinations.test_and_set(index)) {
          ++num_new_covered_tuples_;
        }
      }
    }

    void compute_gain_per_value(
        const citcpp::detail::coverage_map::second_level_type
            &value_combinations) {
      using namespace citcpp::detail;

      const param_vector &param_indices =
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
            return;
          }

          coverage_map::second_level_type::size_type addend = param_value;
          for (std::vector<unsigned int>::size_type j = i + 1;
               j < param_indices.size(); ++j) {
            addend *= model_.get_parameters()[param_indices[j]];
          }
          base_index += addend;
        }

        // If we have found a don't care value in one of the [0, ...
        // ,current_param_idx - 1] parameters, then we skip the combination in
        // the coverage gain computation.
        for (unsigned int value = 0; value < gain_per_value_.size(); ++value) {
          if (!value_combinations.test(base_index + value)) {
            gain_per_value_[value] += 1;
          }
        }
      }
    }

    unsigned long long get_num_new_covered_tuples() const {
      return num_new_covered_tuples_;
    }

  private:
    const citcpp::detail::model &model_;
    const citcpp::detail::test &prev_test_;
    const citcpp::detail::test &test_;
    std::vector<unsigned long long> &gain_per_value_;
    const bool enable_gain_computation_;
    const int current_param_selected_value_for_prev_test_;
    unsigned long long num_new_covered_tuples_;
};

class
    ipog_horizontal_update_coverage_map_and_select_best_value_per_param_combo_functor_parallel {
  public:
    ipog_horizontal_update_coverage_map_and_select_best_value_per_param_combo_functor_parallel(
        const citcpp::detail::model &model,
        const citcpp::detail::test &prev_test, const citcpp::detail::test &test,
        citcpp::detail::thread_local_vector<
            std::vector<citcpp::detail::aligned_ull_value>> &gain_per_value,
        const bool enable_gain_computation,
        const int current_param_selected_value_for_prev_test,
        const citcpp::detail::coverage_map_parallel_iterator &cov_map_it)
        : model_(model),
          prev_test_(prev_test),
          test_(test),
          gain_per_value_(gain_per_value),
          enable_gain_computation_(enable_gain_computation),
          current_param_selected_value_for_prev_test_(
              current_param_selected_value_for_prev_test),
          num_new_covered_tuples_(cov_map_it.get_num_workers()),
          cov_map_it_(cov_map_it) {}

    bool operator()(
        citcpp::detail::coverage_map::second_level_type &value_combinations) {
      if (current_param_selected_value_for_prev_test_ >= 0) {
        update_coverage(value_combinations);
      }
      if (enable_gain_computation_) {
        compute_gain_per_value(value_combinations);
      }

      return true;
    }

    void update_coverage(
        citcpp::detail::coverage_map::second_level_type &value_combinations) {
      using namespace citcpp::detail;

      const param_vector &param_indices =
          value_combinations.get_parameter_indices();

      if (!value_combinations.all()) {
        // Here we compute an index into the bitset. To do so, we treat the
        // number of values of each parameter as a kind of radix. Consider three
        // parameters p_0, p_1, p_2. The last parameter is always the current
        // one processed by IPOG. Now say that v_i is the number of values for
        // p_i. If we now have values x_0, x_1, x_2, then the index is x_0 * v_1
        // * v_2 + x_1 * v_2 + x_2.
        coverage_map::second_level_type::size_type index =
            current_param_selected_value_for_prev_test_;
        for (std::vector<unsigned int>::size_type i = 0;
             i < param_indices.size() - 1; ++i) {
          const unsigned int param_idx = param_indices[i];
          const int param_value = prev_test_.get_values()[param_idx];

          if (param_value < 0) {
            // We have found a don't care value for that combination in
            // the considered test in one of the [0, ... ,current_param_idx - 1]
            // parameters. There is nothing to be updated concerning the
            // coverage. This combination will be taken care of during the
            // vertical extension step.
            return;
          }

          coverage_map::second_level_type::size_type addend = param_value;
          for (std::vector<unsigned int>::size_type j = i + 1;
               j < param_indices.size(); ++j) {
            addend *= model_.get_parameters()[param_indices[j]];
          }
          index += addend;
        }

        if (!value_combinations.test_and_set(index)) {
          ++num_new_covered_tuples_[cov_map_it_.get_worker_id()].value;
        }
      }
    }

    void compute_gain_per_value(
        const citcpp::detail::coverage_map::second_level_type
            &value_combinations) {
      using namespace citcpp::detail;

      const param_vector &param_indices =
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
            return;
          }

          coverage_map::second_level_type::size_type addend = param_value;
          for (std::vector<unsigned int>::size_type j = i + 1;
               j < param_indices.size(); ++j) {
            addend *= model_.get_parameters()[param_indices[j]];
          }
          base_index += addend;
        }

        // If we have found a don't care value in one of the [0, ...
        // ,current_param_idx - 1] parameters, then we skip the combination in
        // the coverage gain computation.
        std::vector<citcpp::detail::aligned_ull_value>
            &thread_local_gain_per_value =
                gain_per_value_[cov_map_it_.get_worker_id()];
        for (unsigned int value = 0; value < thread_local_gain_per_value.size();
             ++value) {
          if (!value_combinations.test(base_index + value)) {
            thread_local_gain_per_value[value].value += 1;
          }
        }
      }
    }

    unsigned long long get_num_new_covered_tuples() const {
      unsigned long long ret = 0;
      for (const auto &i : num_new_covered_tuples_) {
        ret += i.value;
      }

      return ret;
    }

  private:
    const citcpp::detail::model &model_;
    const citcpp::detail::test &prev_test_;
    const citcpp::detail::test &test_;
    citcpp::detail::thread_local_vector<
        std::vector<citcpp::detail::aligned_ull_value>> &gain_per_value_;
    const bool enable_gain_computation_;
    const int current_param_selected_value_for_prev_test_;
    citcpp::detail::thread_local_vector<citcpp::detail::aligned_ull_value>
        num_new_covered_tuples_;
    const citcpp::detail::coverage_map_parallel_iterator &cov_map_it_;
};

new_covered_tuples_and_selected_value
ipog_horizontal_update_coverage_map_and_select_best_value(
    const unsigned int real_current_param_idx,
    const unsigned int num_current_param_values,
    const citcpp::detail::model &model, const citcpp::detail::test &prev_test,
    const int current_param_selected_value_for_prev_test,
    const citcpp::detail::test &test,
    citcpp::detail::coverage_map_iterator &cov_map_it,
    unsigned int &last_picked_value,
    std::vector<unsigned int> &value_to_num_picked) {
  using namespace citcpp::detail;

  // This is an array containing the coverage gain per value of the current
  // parameter.
  std::vector<unsigned long long> gain_per_value(num_current_param_values);

  // We first check whether the test already has a concrete value
  // for the current parameter, because if so, then there is no
  // point in evaluating a coverage gain.
  const int current_param_value = test.get_values()[real_current_param_idx];
  if (current_param_value >= 0) {
    last_picked_value = current_param_value;
    value_to_num_picked[current_param_value]++;

    // The coverage gain computation is disabled in the functor,
    // so that it won't modify this gain info.
    gain_per_value[current_param_value]++;
  }

  ipog_horizontal_update_coverage_map_and_select_best_value_per_param_combo_functor
      per_param_combo_functor(model, prev_test, test, gain_per_value,
                              current_param_value < 0,
                              current_param_selected_value_for_prev_test);
  cov_map_it.visit_all_parameter_combinations(per_param_combo_functor);

  new_covered_tuples_and_selected_value res;
  res.num_new_covered_tuples_ =
      per_param_combo_functor.get_num_new_covered_tuples();

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

  res.selected_value_ = value_with_max_gain;

  return res;
}

new_covered_tuples_and_selected_value
ipog_horizontal_update_coverage_map_and_select_best_value(
    const unsigned int real_current_param_idx,
    const unsigned int num_current_param_values,
    const citcpp::detail::model &model, const citcpp::detail::test &prev_test,
    const int current_param_selected_value_for_prev_test,
    const citcpp::detail::test &test,
    citcpp::detail::coverage_map_parallel_iterator &cov_map_it,
    unsigned int &last_picked_value,
    std::vector<unsigned int> &value_to_num_picked) {
  using namespace citcpp::detail;

  // This is an array containing the coverage gain per value of the current
  // parameter.
  thread_local_vector<std::vector<citcpp::detail::aligned_ull_value>>
      gain_per_value(cov_map_it.get_num_workers(),
                     std::vector<citcpp::detail::aligned_ull_value>(
                         num_current_param_values));

  // We first check whether the test already has a concrete value
  // for the current parameter, because if so, then there is no
  // point in evaluating a coverage gain.
  const int current_param_value = test.get_values()[real_current_param_idx];
  if (current_param_value >= 0) {
    last_picked_value = current_param_value;
    value_to_num_picked[current_param_value]++;

    // The coverage gain computation is disabled in the functor,
    // so that it won't modify this gain info.
    for (std::vector<citcpp::detail::aligned_ull_value>
             &thread_local_gain_per_value : gain_per_value) {
      thread_local_gain_per_value[current_param_value].value++;
    }
  }

  ipog_horizontal_update_coverage_map_and_select_best_value_per_param_combo_functor_parallel
      per_param_combo_functor(
          model, prev_test, test, gain_per_value, current_param_value < 0,
          current_param_selected_value_for_prev_test, cov_map_it);
  cov_map_it.visit_all_parameter_combinations(per_param_combo_functor);

  new_covered_tuples_and_selected_value res;
  res.num_new_covered_tuples_ =
      per_param_combo_functor.get_num_new_covered_tuples();

  int value_with_max_gain = -1;
  unsigned long long max_gain = 0;
  for (unsigned int v_index = 0; v_index < num_current_param_values;
       ++v_index) {
    unsigned int value =
        (v_index + last_picked_value + 1) % num_current_param_values;

    unsigned long long value_gain = 0;
    for (std::vector<citcpp::detail::aligned_ull_value>
             &thread_local_gain_per_value : gain_per_value) {
      value_gain += thread_local_gain_per_value[value].value;
    }

    if (value_gain > max_gain) {
      value_with_max_gain = value;
      max_gain = value_gain;
    } else if (value_gain == max_gain) {
      // We use a simple tie breaking strategy: We do not favor one value over
      // the other. If two values have the same gain, then we pick the one which
      // we have picked less so far. Since also this could be a tie (we have
      // picked the value the same number of times, we remember the value we
      // have picked before, and choose the next one in this case.
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

  res.selected_value_ = value_with_max_gain;

  return res;
}

}  // namespace

namespace citcpp {
namespace detail {

ipog_horizontal_extension_result ipog_horizontal_extension(
    const unsigned int current_param_idx,
    const unsigned long long num_missing_combinations_to_cover,
    internal_test_set &test_set, coverage_map &cov_map) {

  const unsigned int real_current_param_idx =
      cov_map.get_parameter_index_map()[current_param_idx];
  const int num_current_param_values =
      cov_map.get_model().get_parameters()[real_current_param_idx];

  // First initialize the result object.
  ipog_horizontal_extension_result result{
      std::vector<list_intrusive<test_list_intrusive_integ>>(
          num_current_param_values),
      list_intrusive<test_list_intrusive_integ>(), 0};

  unsigned int last_picked_value = 0;
  std::vector<unsigned int> value_to_num_picked(num_current_param_values);
  coverage_map_iterator cov_map_it = cov_map.create_iterator();

  test *previous_test = nullptr;
  int selected_value = 0;
  for (test &t : test_set.get_list_of_tests()) {
    if (cov_map.get_number_of_parameters_to_select() > 2) {
      last_picked_value = num_current_param_values - 1;
    }

    if (!previous_test) {
      selected_value = ipog_horizontal_select_best_value(
          real_current_param_idx, num_current_param_values, cov_map.get_model(),
          t, cov_map_it, last_picked_value, value_to_num_picked);
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
              real_current_param_idx, num_current_param_values,
              cov_map.get_model(), *previous_test, selected_value, t,
              cov_map_it, last_picked_value, value_to_num_picked);

      selected_value = res.selected_value_;

      // Keep track of how many tuples we have covered in addition.
      result.num_new_covered_tuples += res.num_new_covered_tuples_;

      if (result.num_new_covered_tuples >= num_missing_combinations_to_cover) {
        return result;
      }
    }

    previous_test = &t;
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

    unsigned long long num_new_covered_tuples =
        selected_value >= 0 ? ipog_horizontal_update_coverage_map(
                                  cov_map.get_model(), *previous_test,
                                  selected_value, cov_map_it)
                            : 0;

    // Keep track of how many tuples we have covered in addition.
    result.num_new_covered_tuples += num_new_covered_tuples;
  }

  return result;
}

ipog_horizontal_extension_result ipog_horizontal_extension(
    const unsigned int current_param_idx,
    const unsigned long long num_missing_combinations_to_cover,
    internal_test_set &test_set, coverage_map &cov_map, thread_pool &tp) {

  const unsigned int real_current_param_idx =
      cov_map.get_parameter_index_map()[current_param_idx];
  const int num_current_param_values =
      cov_map.get_model().get_parameters()[real_current_param_idx];

  // First initialize the result object.
  ipog_horizontal_extension_result result{
      std::vector<list_intrusive<test_list_intrusive_integ>>(
          num_current_param_values),
      list_intrusive<test_list_intrusive_integ>(), 0};

  unsigned int last_picked_value = 0;
  std::vector<unsigned int> value_to_num_picked(num_current_param_values);
  coverage_map_parallel_iterator cov_map_it =
      cov_map.create_parallel_iterator(tp);

  test *previous_test = nullptr;
  int selected_value = 0;
  for (test &t : test_set.get_list_of_tests()) {
    if (cov_map.get_number_of_parameters_to_select() > 2) {
      last_picked_value = num_current_param_values - 1;
    }

    if (!previous_test) {
      selected_value = ipog_horizontal_select_best_value(
          real_current_param_idx, num_current_param_values, cov_map.get_model(),
          t, cov_map_it, last_picked_value, value_to_num_picked);
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
              real_current_param_idx, num_current_param_values,
              cov_map.get_model(), *previous_test, selected_value, t,
              cov_map_it, last_picked_value, value_to_num_picked);

      selected_value = res.selected_value_;

      // Keep track of how many tuples we have covered in addition.
      result.num_new_covered_tuples += res.num_new_covered_tuples_;

      if (result.num_new_covered_tuples >= num_missing_combinations_to_cover) {
        return result;
      }
    }

    previous_test = &t;
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

    unsigned long long num_new_covered_tuples =
        selected_value >= 0 ? ipog_horizontal_update_coverage_map(
                                  cov_map.get_model(), *previous_test,
                                  selected_value, cov_map_it)
                            : 0;

    // Keep track of how many tuples we have covered in addition.
    result.num_new_covered_tuples += num_new_covered_tuples;
  }

  return result;
}

}  // namespace detail
}  // namespace citcpp
