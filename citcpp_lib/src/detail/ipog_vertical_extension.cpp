#include "ipog_vertical_extension.hpp"

#include "citcpp_utils.hpp"

namespace {

class ipog_vertical_extension_tuple_functor {
  public:
    ipog_vertical_extension_tuple_functor(
        const citcpp::detail::internal_model& model,
        citcpp::detail::internal_test_set& test_set,
        citcpp::detail::ipog_horizontal_extension_result&
            partitioning_of_tests_according_to_current_values,
        citcpp::detail::list_intrusive<
            citcpp::detail::test_list_intrusive_integ>& modified_tests,
        const unsigned long long num_missing_combinations_to_cover)
        : model_(model),
          test_set_(test_set),
          partitioning_of_tests_according_to_current_values_(
              partitioning_of_tests_according_to_current_values),
          modified_tests_(modified_tests),
          num_missing_combinations_to_cover_(num_missing_combinations_to_cover),
          num_new_covered_tuples_(0) {}

    bool operator()(
        const citcpp::detail::value_vector& value_indices,
        citcpp::detail::coverage_map_base::size_type bitpos,
        citcpp::detail::coverage_map::second_level_type& value_combinations) {
      using namespace citcpp::detail;

      ipog_vertical_extension_func(value_indices, bitpos, value_combinations);

      return num_new_covered_tuples_ < num_missing_combinations_to_cover_;
    }

    unsigned long long get_num_new_covered_tuples() const {
      return num_new_covered_tuples_;
    }

  private:
    void ipog_vertical_extension_func(
        const citcpp::detail::value_vector& value_indices,
        citcpp::detail::coverage_map_base::size_type bitpos,
        citcpp::detail::coverage_map::second_level_type& value_combinations) {
      using namespace citcpp::detail;

      // First we check whether the value combination is covered, because if it
      // is not, then there is no point try to fit it into some test.
      const param_vector& param_indices =
          value_combinations.get_parameter_indices();

      if (value_combinations.test_and_set(bitpos)) {
        return;
      }

      ++num_new_covered_tuples_;

      // Now we iterate over all tests trying to fit the value combination.
      // However, we do not iterate over the entire test test, but instead
      // leverage upon its partition according to the value of the current
      // parameter as set by the horizontal extension for a test.
      // First we determine the value of the current parameter in the value
      // combination we want to cover.
      const int current_param_value_to_cover = value_indices.back();

      // Iterate over the tests with the same value for the current parameter
      // value we have to cover.
      for (test_list_intrusive_integ& t :
           partitioning_of_tests_according_to_current_values_
               .value_to_row_mapping[current_param_value_to_cover]) {

        if (ipog_vertical_extension_try_inject_value_combo(
                param_indices, value_indices, t.get_test())) {
          // Remember that we have modified the test and need to
          // evaluate it again whether also other combinations are covered.
          if (!t.get_test()
                   .get_vertical_extension_intrusive_list_node()
                   .is_linked_up()) {
            modified_tests_.push_back(
                t.get_test().get_vertical_extension_intrusive_list_node());
          }

          // Return, since we have found a test and injected the value
          // combination.
          return;
        }
      }

      // Now we iterate over the tests with a don't care value for the current
      // parameter.
      for (auto it = partitioning_of_tests_according_to_current_values_
                         .rows_with_current_parameter_dont_care_value.begin();
           it != partitioning_of_tests_according_to_current_values_
                     .rows_with_current_parameter_dont_care_value.end();
           ++it) {
        test& t = it->get_test();

        if (ipog_vertical_extension_try_inject_value_combo(param_indices,
                                                           value_indices, t)) {
          if (!t.get_vertical_extension_intrusive_list_node().is_linked_up()) {
            // Remember that we have modified the test and need to
            // evaluate it again whether also other combinations are covered.
            modified_tests_.push_back(
                t.get_vertical_extension_intrusive_list_node());
          }

          // Since we have successfully injected the combination, the test must
          // be moved to a different partition for looking it up when trying to
          // inject other combinations with the same value for the current
          // parameter.
          partitioning_of_tests_according_to_current_values_
              .rows_with_current_parameter_dont_care_value.erase(it);
          partitioning_of_tests_according_to_current_values_
              .value_to_row_mapping[current_param_value_to_cover]
              .push_back(t.get_value_partition_intrusive_list_node());

          // Return, since we have found a test and injected the value
          // combination.
          return;
        }
      }

      // If we have reached this point, then we did not find a matching test.
      // Thus, we have to add a new one with the value combination.
      // Initialize all values of the test with don't care.
      test t(model_.get_parameter_num_values().size(), -1);

      for (unsigned int i = 0; i < param_indices.size(); ++i) {
        const unsigned int param_idx = param_indices[i];
        const int param_value_to_cover = value_indices[i];
        t.get_values()[param_idx] = param_value_to_cover;
      }

      test_set_.get_list_of_tests().push_back(std::move(t));

      // Update the mapping from values of the current parameter to the tests.
      partitioning_of_tests_according_to_current_values_
          .value_to_row_mapping[current_param_value_to_cover]
          .push_back(test_set_.get_list_of_tests()
                         .back()
                         .get_value_partition_intrusive_list_node());
    }

    bool ipog_vertical_extension_try_inject_value_combo(
        const citcpp::detail::param_vector& param_indices,
        const citcpp::detail::value_vector& value_indices,
        citcpp::detail::test& t) {

      for (unsigned int i = 0; i < param_indices.size(); ++i) {
        const unsigned int param_idx = param_indices[i];
        const int param_value_to_cover = value_indices[i];
        const int param_value_in_test = t.get_values()[param_idx];

        if (param_value_in_test >= 0 &&
            param_value_to_cover != param_value_in_test) {
          // Cannot inject value combination in this test, moving on to the next
          // one.
          return false;
        }
      }

      for (unsigned int i = 0; i < param_indices.size(); ++i) {
        const unsigned int param_idx = param_indices[i];
        const int param_value_to_cover = value_indices[i];
        t.get_values()[param_idx] = param_value_to_cover;
      }

      return true;
    }

  private:
    const citcpp::detail::internal_model& model_;
    citcpp::detail::internal_test_set& test_set_;
    citcpp::detail::ipog_horizontal_extension_result&
        partitioning_of_tests_according_to_current_values_;
    citcpp::detail::list_intrusive<citcpp::detail::test_list_intrusive_integ>&
        modified_tests_;
    const unsigned long long num_missing_combinations_to_cover_;
    unsigned long long num_new_covered_tuples_;
};

class ipog_vertical_extension_functor {
  public:
    ipog_vertical_extension_functor(
        const unsigned int strength,
        const citcpp::detail::internal_model& model,
        citcpp::detail::internal_test_set& test_set,
        citcpp::detail::ipog_horizontal_extension_result&
            partitioning_of_tests_according_to_current_values,
        citcpp::detail::list_intrusive<
            citcpp::detail::test_list_intrusive_integ>& modified_tests,
        const unsigned long long num_missing_combinations_to_cover)
        : model_(model),
          test_set_(test_set),
          partitioning_of_tests_according_to_current_values_(
              partitioning_of_tests_according_to_current_values),
          modified_tests_(modified_tests),
          value_indices_(strength),
          num_missing_combinations_to_cover_(num_missing_combinations_to_cover),
          num_new_covered_tuples_(0) {}

    bool operator()(
        citcpp::detail::coverage_map::second_level_type& value_combinations) {

      using namespace citcpp::detail;

      if (!value_combinations.all()) {
        ipog_vertical_extension_func(value_combinations);
      }

      return num_new_covered_tuples_ < num_missing_combinations_to_cover_;
    }

    unsigned long long get_num_new_covered_tuples() const {
      return num_new_covered_tuples_;
    }

  private:
    void ipog_vertical_extension_func(
        citcpp::detail::coverage_map::second_level_type& value_combinations) {

      using namespace citcpp::detail;

      const param_vector& param_indices =
          value_combinations.get_parameter_indices();

      for (test_list_intrusive_integ& t : modified_tests_) {
        // Here we compute an index into the bitset. To do so, we treat the
        // number of values of each parameter as a kind of radix. Consider
        // three parameters p_0, p_1, p_2. Now say that v_i is the number of
        // values for p_i. If we now have values x_0, x_1, x_2, then the
        // index is x_0 * v_1 * v_2 + x_1 * v_2 + x_2.
        coverage_map::second_level_type::size_type index = 0;
        bool index_valid = true;
        for (std::vector<unsigned int>::size_type i = 0;
             i < param_indices.size(); ++i) {

          const unsigned int param_idx = param_indices[i];
          const int param_value = t.get_test().get_values()[param_idx];

          if (param_value < 0) {
            // We have found a don't care value for that combination in
            // the considered test.
            index_valid = false;
            break;
          }

          coverage_map::second_level_type::size_type addend = param_value;
          for (std::vector<unsigned int>::size_type j = i + 1;
               j < param_indices.size(); ++j) {
            addend *= model_.get_parameter_num_values()[param_indices[j]];
          }
          index += addend;
        }

        if (index_valid) {
          if (!value_combinations.test_and_set(index)) {
            num_new_covered_tuples_++;
          }
        }
      }

      ipog_vertical_extension_tuple_functor tuple_functor(
          model_, test_set_, partitioning_of_tests_according_to_current_values_,
          modified_tests_, num_missing_combinations_to_cover_);

      visit_all_value_combos_of_param_combo(model_, param_indices,
                                            value_indices_, tuple_functor,
                                            value_combinations);

      num_new_covered_tuples_ += tuple_functor.get_num_new_covered_tuples();
    }

  private:
    const citcpp::detail::internal_model& model_;
    citcpp::detail::internal_test_set& test_set_;
    citcpp::detail::ipog_horizontal_extension_result&
        partitioning_of_tests_according_to_current_values_;
    citcpp::detail::list_intrusive<citcpp::detail::test_list_intrusive_integ>&
        modified_tests_;
    citcpp::detail::value_vector value_indices_;
    const unsigned long long num_missing_combinations_to_cover_;
    unsigned long long num_new_covered_tuples_;
};

}  // namespace

namespace citcpp {
namespace detail {

ipog_vertical_extension_result ipog_vertical_extension(
    const unsigned long long num_missing_combinations_to_cover,
    ipog_horizontal_extension_result&
        partitioning_of_tests_according_to_current_values,
    internal_test_set& test_set,
    std::vector<std::pair<internal_relation, coverage_map>>& relations) {

  // First initialize the result object.
  ipog_vertical_extension_result result = {0};

  list_intrusive<test_list_intrusive_integ> modified_tests;

  for (auto& rel : relations) {
    coverage_map_iterator cov_map_it = rel.second.create_iterator();

    ipog_vertical_extension_functor functor(
        rel.second.get_number_of_parameters_to_select(), rel.second.get_model(),
        test_set, partitioning_of_tests_according_to_current_values,
        modified_tests, num_missing_combinations_to_cover);

    cov_map_it.visit_all_parameter_combinations(functor);

    result.num_new_covered_tuples += functor.get_num_new_covered_tuples();
  }

  return result;
}

}  // namespace detail
}  // namespace citcpp
