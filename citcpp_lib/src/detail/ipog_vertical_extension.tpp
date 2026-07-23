#include <algorithm>

#include "citcpp_utils.hpp"
#include "ipog_vertical_extension.hpp"

namespace citcpp {
namespace detail {

class ipog_vertical_extension_functor {
  public:
    ipog_vertical_extension_functor(
        unsigned int strength, const internal_model& model,
        constraint_handler& constr_handler, internal_test_set& test_set,
        ipog_horizontal_extension_result&
            partitioning_of_tests_according_to_current_values,
        list_intrusive<test_list_intrusive_integ>& modified_tests,
        unsigned long long num_missing_combinations_to_cover)
        : model_(model),
          constr_handler_(constr_handler),
          test_set_(test_set),
          partitioning_of_tests_according_to_current_values_(
              partitioning_of_tests_according_to_current_values),
          modified_tests_(modified_tests),
          value_indices_(strength),
          weights_(strength),
          scratch_test_(model.get_parameter_num_values().size(), -1),
          num_missing_combinations_to_cover_(num_missing_combinations_to_cover),
          num_checked_tuples_(0),
          num_new_covered_tuples_(0) {}

    bool operator()(ipog_coverage_map::second_level_type& value_combinations) {
      if (!value_combinations.all_covered()) {
        ipog_vertical_extension_func(value_combinations);

        reset_scratch_test(value_combinations);
      }

      return num_checked_tuples_ < num_missing_combinations_to_cover_;
    }

    bool operator()(const value_vector& value_indices,
                    ipog_coverage_map::second_level_type::size_type bitpos,
                    ipog_coverage_map::second_level_type& value_combinations) {

      ipog_vertical_extension_value_combo_func(value_indices, bitpos,
                                               value_combinations);

      return num_checked_tuples_ < num_missing_combinations_to_cover_;
    }

    unsigned long long get_num_checked_tuples() const {
      return num_checked_tuples_;
    }

    unsigned long long get_num_new_covered_tuples() const {
      return num_new_covered_tuples_;
    }

    void reset() {
      num_checked_tuples_ = 0;
      num_new_covered_tuples_ = 0;
    }

  private:
    void ipog_vertical_extension_func(
        ipog_coverage_map::second_level_type& value_combinations) {

      const param_vector& param_indices =
          value_combinations.get_parameter_indices();

      // Pre-calculate weights for index computation
      ipog_coverage_map::second_level_type::size_type weight = 1;
      for (int i = static_cast<int>(param_indices.size() - 1); i >= 0; --i) {
        weights_[i] = weight;
        weight *= static_cast<ipog_coverage_map::second_level_type::size_type>(
            model_.get_parameter_num_values()[param_indices[i]]);
      }

      for (test_list_intrusive_integ& t : modified_tests_) {
        // Here we compute an index into the bitset. To do so, we treat the
        // number of values of each parameter as a kind of radix. Consider
        // three parameters p_0, p_1, p_2. Now say that v_i is the number of
        // values for p_i. If we now have values x_0, x_1, x_2, then the
        // index is x_0 * v_1 * v_2 + x_1 * v_2 + x_2.
        ipog_coverage_map::second_level_type::size_type index = 0;
        bool index_valid = true;
        for (std::size_t i = 0; i < param_indices.size(); ++i) {
          const int param_value = t.get_test().get_values()[param_indices[i]];

          if (param_value < 0) {
            // We have found a don't care value for that combination in
            // the considered test.
            index_valid = false;
            break;
          }

          index += static_cast<ipog_coverage_map::second_level_type::size_type>(
                       param_value) *
                   weights_[i];
        }

        if (index_valid) {
          if (!value_combinations.test_and_set_covered(index)) {
            num_checked_tuples_++;
            num_new_covered_tuples_++;
            value_combinations.set_valid(index);
          }
        }
      }

      // constr_handler_.mark_valid_tuples(value_combinations, param_indices);

      visit_all_value_combos_of_param_combo(
          model_, param_indices, value_indices_, *this, value_combinations);
    }

    void ipog_vertical_extension_value_combo_func(
        const value_vector& value_indices,
        ipog_coverage_map::second_level_type::size_type bitpos,
        ipog_coverage_map::second_level_type& value_combinations) {

      // First we check whether the value combination is covered, because if it
      // is, then there is no point trying to fit it into some test.
      const param_vector& param_indices =
          value_combinations.get_parameter_indices();

      if (value_combinations.test_and_set_covered(bitpos)) {
        return;
      }

      ++num_checked_tuples_;

      if (!value_combinations.is_valid(bitpos)) {
        if (!is_valid_tuple(param_indices, value_indices)) {
          // Value tuple is invalid according to constraints.
          return;
        }
        // // If the tuple is not valid, then it cannot be covered at all.
        // // So there's nothing to do for us here.
        // return;
      }

      ++num_new_covered_tuples_;

      // Now we iterate over all tests trying to fit the value combination.
      // However, we do not iterate over the entire test test, but instead
      // leverage upon its partition according to the value of the current
      // parameter as set by the horizontal extension for a test.
      // First we determine the value of the current parameter in the value
      // combination we want to cover.
      const int current_param_value_to_cover = value_indices.back();

      // Try to inject the combination into a test with the same value for the
      // current parameter value we have to cover.
      {
        test_list_intrusive_integ* node =
            constr_handler_.get_first_test_valid_for_assignment(
                partitioning_of_tests_according_to_current_values_
                    .value_to_row_mapping[current_param_value_to_cover],
                param_indices, value_indices);

        if (node) {
          test& t = node->get_test();
          // Remember that we have modified the test and need to
          // evaluate it again whether also other combinations are covered.
          if (!t.get_vertical_extension_intrusive_list_node().is_linked_up()) {
            modified_tests_.push_back(
                t.get_vertical_extension_intrusive_list_node());
          }

          for (std::size_t i = 0; i < param_indices.size(); ++i) {
            const int param_value_to_cover = value_indices[i];
            t.get_values()[param_indices[i]] = param_value_to_cover;
          }

          // Update the state of the test as seen by constraint hander.
          constr_handler_.update_cached_partial_test(&t);

          // Return, since we have found a test and injected the value
          // combination.
          return;
        }
      }

      // Now we try to inject the combination into a test with a don't care
      // value for the current parameter. we have to cover.
      {
        test_list_intrusive_integ* node =
            constr_handler_.get_first_test_valid_for_assignment(
                partitioning_of_tests_according_to_current_values_
                    .rows_with_current_parameter_dont_care_value,
                param_indices, value_indices);

        if (node) {
          test& t = node->get_test();
          // Remember that we have modified the test and need to
          // evaluate it again whether also other combinations are covered.
          if (!t.get_vertical_extension_intrusive_list_node().is_linked_up()) {
            modified_tests_.push_back(
                t.get_vertical_extension_intrusive_list_node());
          }

          // Since we have successfully injected the combination, the test must
          // be moved to a different partition for looking it up when trying to
          // inject other combinations with the same value for the current
          // parameter.
          partitioning_of_tests_according_to_current_values_
              .rows_with_current_parameter_dont_care_value.erase(*node);
          partitioning_of_tests_according_to_current_values_
              .value_to_row_mapping[current_param_value_to_cover]
              .push_back(t.get_value_partition_intrusive_list_node());

          for (std::size_t i = 0; i < param_indices.size(); ++i) {
            const int param_value_to_cover = value_indices[i];
            t.get_values()[param_indices[i]] = param_value_to_cover;
          }

          // Update the state of the test as seen by constraint hander.
          constr_handler_.update_cached_partial_test(&t);

          // Return, since we have found a test and injected the value
          // combination.
          return;
        }
      }

      // If we have reached this point, then we did not find a matching test.
      // Thus, we have to add a new one with the value combination.
      // Initialize all values of the test with don't care.
      test t(model_.get_parameter_num_values().size(), -1);

      for (std::size_t i = 0; i < param_indices.size(); ++i) {
        const int param_value_to_cover = value_indices[i];
        t.get_values()[param_indices[i]] = param_value_to_cover;
      }

      test_set_.get_list_of_tests().push_back(std::move(t));

      // Update the mapping from values of the current parameter to the tests.
      partitioning_of_tests_according_to_current_values_
          .value_to_row_mapping[current_param_value_to_cover]
          .push_back(test_set_.get_list_of_tests()
                         .back()
                         .get_value_partition_intrusive_list_node());

      // Cache the test in the constraint handler.
      constr_handler_.cache_partial_test(&test_set_.get_list_of_tests().back());
    }

    bool is_valid_tuple(const param_vector& param_indices,
                        const value_vector& value_indices) {

      for (std::size_t i = 0; i < param_indices.size(); ++i) {
        const int param_value_to_cover = value_indices[i];
        scratch_test_.get_values()[param_indices[i]] = param_value_to_cover;
      }

      bool res = constr_handler_.is_valid_partial_test(scratch_test_);

      return res;
    }

    void reset_scratch_test(
        ipog_coverage_map::second_level_type& value_combinations) {

      const param_vector& param_indices =
          value_combinations.get_parameter_indices();

      for (std::size_t i = 0; i < param_indices.size(); ++i) {
        scratch_test_.get_values()[param_indices[i]] = -1;
      }
    }

  private:
    const internal_model& model_;
    constraint_handler& constr_handler_;
    internal_test_set& test_set_;
    ipog_horizontal_extension_result&
        partitioning_of_tests_according_to_current_values_;
    list_intrusive<test_list_intrusive_integ>& modified_tests_;
    value_vector value_indices_;
    std::vector<ipog_coverage_map::second_level_type::size_type> weights_;
    test scratch_test_;
    const unsigned long long num_missing_combinations_to_cover_;
    unsigned long long num_checked_tuples_;
    unsigned long long num_new_covered_tuples_;
};

inline ipog_vertical_extension_result ipog_vertical_extension(
    unsigned long long num_missing_combinations_to_cover,
    constraint_handler& constr_handler,
    ipog_horizontal_extension_result&
        partitioning_of_tests_according_to_current_values,
    internal_test_set& test_set,
    std::vector<std::pair<const internal_relation*, ipog_coverage_map>>&
        relations) {

  // First initialize the result object.
  ipog_vertical_extension_result result;

  list_intrusive<test_list_intrusive_integ> modified_tests;

  for (auto& rel_and_cov_map : relations) {
    ipog_vertical_extension_functor functor(
        rel_and_cov_map.second.get_number_of_parameters_to_select(),
        rel_and_cov_map.second.get_model(), constr_handler, test_set,
        partitioning_of_tests_according_to_current_values, modified_tests,
        num_missing_combinations_to_cover);

    for (auto& value_combinations : rel_and_cov_map.second.get_coverage_map()) {
      const bool cont = functor(value_combinations);

      if (!cont) {
        break;
      }
    }

    result[rel_and_cov_map.first].num_checked_tuples =
        functor.get_num_checked_tuples();
    result[rel_and_cov_map.first].num_new_covered_tuples =
        functor.get_num_new_covered_tuples();
  }

  return result;
}

}  // namespace detail
}  // namespace citcpp
