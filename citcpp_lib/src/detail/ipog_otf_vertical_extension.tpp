#include <functional>

#include "bitset.hpp"
#include "citcpp_utils.hpp"
#include "ipog_otf_vertical_extension.hpp"
#include "param_combo_iteration.hpp"

namespace citcpp {
namespace detail {

class ipog_otf_vertical_extension_functor {
  public:
    ipog_otf_vertical_extension_functor(
        const unsigned int strength, const internal_model& model,
        const constraint_handler& constr_handler, internal_test_set& test_set,
        ipog_horizontal_extension_result&
            partitioning_of_tests_according_to_current_values,
        array_wrapper_uint64& bitset_backing_array,
        const unsigned long long num_missing_combinations_to_cover)
        : model_(model),
          constr_handler_(constr_handler),
          test_set_(test_set),
          partitioning_of_tests_according_to_current_values_(
              partitioning_of_tests_according_to_current_values),
          bitset_backing_array_(bitset_backing_array),
          value_indices_(strength),
          scratch_test_(model.get_parameter_num_values().size(), -1),
          num_missing_combinations_to_cover_(num_missing_combinations_to_cover),
          num_checked_tuples_(0),
          num_new_covered_tuples_(0) {}

    bool operator()(const param_vector& param_indices) {
      ipog_vertical_extension_func(param_indices);

      reset_scratch_test(param_indices);

      return num_checked_tuples_ < num_missing_combinations_to_cover_;
    }

    bool operator()(value_vector& value_indices,
                    bitset_non_owning_uint64::size_type bit_pos,
                    const param_vector& param_indices,
                    bitset_non_owning_uint64& values_combo_bitset) {

      ipog_vertical_extension_value_combo_func(
          value_indices, bit_pos, param_indices, values_combo_bitset);

      return num_checked_tuples_ < num_missing_combinations_to_cover_;
    }

    unsigned long long get_num_checked_tuples() const {
      return num_checked_tuples_;
    }

    unsigned long long get_num_new_covered_tuples() const {
      return num_new_covered_tuples_;
    }

  private:
    void ipog_vertical_extension_func(const param_vector& param_indices) {
      bitset_non_owning_uint64::size_type bitset_size = 1;
      for (auto p : param_indices) {
        bitset_size *= model_.get_parameter_num_values()[p];
      }
      bitset_non_owning_uint64 values_combo_bitset(bitset_size);
      values_combo_bitset.set_backing_array(bitset_backing_array_.get_array());
      values_combo_bitset.reset();

      for (test& t : test_set_.get_list_of_tests()) {
        // Here we compute an index into the bitset. To do so, we treat the
        // number of values of each parameter as a kind of radix. Consider
        // three parameters p_0, p_1, p_2. Now say that v_i is the number of
        // values for p_i. If we now have values x_0, x_1, x_2, then the
        // index is x_0 * v_1 * v_2 + x_1 * v_2 + x_2.
        bitset_non_owning_uint64::size_type base_index = 0;
        bool index_valid = true;
        for (std::vector<unsigned int>::size_type i = 0;
             i < param_indices.size(); ++i) {

          const unsigned int param_idx = param_indices[i];
          const int param_value = t.get_values()[param_idx];

          if (param_value < 0) {
            // We have found a don't care value for that combination in
            // the considered test.
            index_valid = false;
            break;
          }

          bitset_non_owning_uint64::size_type addend = param_value;
          for (std::vector<unsigned int>::size_type j = i + 1;
               j < param_indices.size(); ++j) {
            addend *= model_.get_parameter_num_values()[param_indices[j]];
          }
          base_index += addend;
        }

        if (index_valid) {
          values_combo_bitset.set(base_index);
        }
      }

      visit_all_value_combos_of_param_combo(model_, param_indices,
                                            value_indices_, *this,
                                            param_indices, values_combo_bitset);
    }

    void ipog_vertical_extension_value_combo_func(
        value_vector& value_indices,
        bitset_non_owning_uint64::size_type bit_pos,
        const param_vector& param_indices,
        bitset_non_owning_uint64& values_combo_bitset) {

      if (values_combo_bitset.test_and_set(bit_pos)) {
        return;
      }

      ++num_checked_tuples_;

      const bool valid_tuple = is_valid_tuple(param_indices, value_indices);
      if (!valid_tuple) {
        // Value tuple is invalid according to constraints.
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
        const param_vector& param_indices, const value_vector& value_indices,
        test& t) {

      bool covers_combo = true;
      for (unsigned int i = 0; i < param_indices.size(); ++i) {
        const unsigned int param_idx = param_indices[i];
        const int param_value_to_cover = value_indices[i];
        const int param_value_in_test = t.get_values()[param_idx];

        scratch_test_.get_values()[param_idx] = param_value_in_test;
        t.get_values()[param_idx] = param_value_to_cover;

        if (param_value_in_test >= 0 &&
            param_value_to_cover != param_value_in_test) {
          // Cannot inject value combination in this test, moving on to the next
          // one.
          covers_combo = false;
        }
      }

      covers_combo = covers_combo && constr_handler_.is_valid_partial_test(t);

      if (!covers_combo) {
        // We need to rollback the changes we did to the test.
        for (unsigned int i = 0; i < param_indices.size(); ++i) {
          const unsigned int param_idx = param_indices[i];
          t.get_values()[param_idx] = scratch_test_.get_values()[param_idx];
        }

        return false;
      }

      return true;
    }

    bool is_valid_tuple(const param_vector& param_indices,
                        const value_vector& value_indices) {

      for (unsigned int i = 0; i < param_indices.size(); ++i) {
        const unsigned int param_idx = param_indices[i];
        const int param_value_to_cover = value_indices[i];
        scratch_test_.get_values()[param_idx] = param_value_to_cover;
      }

      bool res = constr_handler_.is_valid_partial_test(scratch_test_);

      return res;
    }

    void reset_scratch_test(const param_vector& param_indices) {
      for (unsigned int i = 0; i < param_indices.size(); ++i) {
        const unsigned int param_idx = param_indices[i];
        scratch_test_.get_values()[param_idx] = -1;
      }
    }

  private:
    const internal_model& model_;
    const constraint_handler& constr_handler_;
    internal_test_set& test_set_;
    ipog_horizontal_extension_result&
        partitioning_of_tests_according_to_current_values_;
    array_wrapper_uint64& bitset_backing_array_;
    value_vector value_indices_;
    test scratch_test_;
    const unsigned long long num_missing_combinations_to_cover_;
    unsigned long long num_checked_tuples_;
    unsigned long long num_new_covered_tuples_;
};

ipog_vertical_extension_result ipog_vertical_extension(
    const unsigned long long num_missing_combinations_to_cover,
    const constraint_handler& constr_handler,
    ipog_horizontal_extension_result&
        partitioning_of_tests_according_to_current_values,
    internal_test_set& test_set, const internal_model& model,
    const std::vector<internal_relation>& relations) {

  // First initialize the result object.
  ipog_vertical_extension_result result;

  for (auto& rel : relations) {
    const unsigned int real_current_param_idx =
        rel.get_parameter_index_map()[rel.get_current_param_idx()];
    const int num_current_param_values =
        model.get_parameter_num_values()[real_current_param_idx];

    const unsigned int product_of_max_parameter_sizes =
        num_current_param_values *
        (rel.get_current_interaction_strength() > 1
             ? get_product_of_max_n_parameter_sizes(
                   rel.get_current_param_idx(),
                   rel.get_current_interaction_strength() - 1, model,
                   rel.get_parameter_index_map())
             : 1);

    array_wrapper_uint64 bitset_backing_array(product_of_max_parameter_sizes);

    param_combo_iterator param_combo_it(rel.get_current_param_idx() + 1,
                                        rel.get_current_interaction_strength(),
                                        rel.get_parameter_index_map(), true);
    ipog_otf_vertical_extension_functor functor(
        rel.get_current_interaction_strength(), model, constr_handler, test_set,
        partitioning_of_tests_according_to_current_values, bitset_backing_array,
        num_missing_combinations_to_cover);

    param_combo_it.visit_all_parameter_combinations(functor);

    result[&rel].num_checked_tuples = functor.get_num_checked_tuples();
    result[&rel].num_new_covered_tuples = functor.get_num_new_covered_tuples();
  }

  return result;
}

}  // namespace detail
}  // namespace citcpp
