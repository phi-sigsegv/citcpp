#include "ipog_otf_vertical_extension.hpp"

#include <functional>

#include "bitset.hpp"
#include "citcpp_utils.hpp"
#include "param_combo_iteration.hpp"

namespace {

template <class T_VISITOR>
bool recursively_visit_all_value_combos_of_param_combo(
    const citcpp::detail::model &model,
    const citcpp::detail::param_vector &param_indices,
    citcpp::detail::value_vector &value_indices, int current_index,
    citcpp::detail::bitset_non_owning_uint64::size_type partial_bit_pos,
    T_VISITOR &visitor) {
  using namespace citcpp::detail;

  // The current range goes from 0 to max_value[current_index]
  const unsigned int max_val =
      model.get_parameters()[param_indices[current_index]];

  bitset_non_owning_uint64::size_type bit_pos_value_factor = 1;
  for (std::vector<unsigned int>::size_type j = current_index + 1;
       j < param_indices.size(); ++j) {
    bit_pos_value_factor *= model.get_parameters()[param_indices[j]];
  }

  for (int i = max_val - 1; i >= 0; --i) {
    value_indices[current_index] = i;

    bool ret = true;

    if (current_index == 0) {
      bitset_non_owning_uint64::size_type bit_pos =
          partial_bit_pos + i * bit_pos_value_factor;
      // Call the visitor.
      ret = visitor(param_indices, value_indices, bit_pos);
    } else {
      ret = recursively_visit_all_value_combos_of_param_combo(
          model, param_indices, value_indices, current_index - 1,
          partial_bit_pos + i * bit_pos_value_factor, visitor);
    }

    if (!ret) {
      return false;
    }
  }

  return true;
}

template <class T_VISITOR>
void visit_all_value_combos_of_param_combo(
    const citcpp::detail::model &model,
    const citcpp::detail::param_vector &param_indices,
    citcpp::detail::value_vector &value_indices, T_VISITOR &visitor) {
  using namespace citcpp::detail;

  recursively_visit_all_value_combos_of_param_combo(
      model, param_indices, value_indices, value_indices.size() - 1, 0,
      visitor);
}

class ipog_vertical_extension_tuple_functor {
  public:
    ipog_vertical_extension_tuple_functor(
        const unsigned int current_param_idx,
        const citcpp::detail::model &model,
        citcpp::detail::internal_test_set &test_set,
        citcpp::detail::ipog_horizontal_extension_result
            &partitioning_of_tests_according_to_current_values,
        citcpp::detail::bitset_non_owning_uint64 &values_combo_bitset,
        const unsigned long long num_missing_combinations_to_cover)
        : current_param_idx_(current_param_idx),
          model_(model),
          test_set_(test_set),
          partitioning_of_tests_according_to_current_values_(
              partitioning_of_tests_according_to_current_values),
          values_combo_bitset_(values_combo_bitset),
          num_missing_combinations_to_cover_(num_missing_combinations_to_cover),
          num_new_covered_tuples_(0) {}

    bool operator()(
        const citcpp::detail::param_vector &param_indices,
        citcpp::detail::value_vector &value_indices,
        citcpp::detail::bitset_non_owning_uint64::size_type bit_pos) {
      using namespace citcpp::detail;

      ipog_vertical_extension_func(param_indices, value_indices, bit_pos);

      return num_new_covered_tuples_ < num_missing_combinations_to_cover_;
    }

    void ipog_vertical_extension_func(
        const citcpp::detail::param_vector &param_indices,
        citcpp::detail::value_vector &value_indices,
        citcpp::detail::bitset_non_owning_uint64::size_type bit_pos) {
      using namespace citcpp::detail;

      if (values_combo_bitset_.test_and_set(bit_pos)) {
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
      for (test &t : partitioning_of_tests_according_to_current_values_
                         .value_to_row_mapping[current_param_value_to_cover]) {
        // We also skip tests which do not have at least one don't care
        // value.
        // This is safe to do here, since at the callsite of this functor,
        // we have previously walked over all tests for the current parameter
        // combination, and calculated its current coverage. Thus, there is no
        // possibility for the vertical extension having covered a tuple by
        // chance.
        if (t.get_num_dont_care_values() == 0) {
          continue;
        }

        if (ipog_vertical_extension_try_inject_value_combo(param_indices,
                                                           value_indices, t)) {
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
        test &t = *it;

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
              .push_back(t);

          // Return, since we have found a test and injected the value
          // combination.
          return;
        }
      }

      // If we have reached this point, then we did not find a matching test.
      // Thus, we have to add a new one with the value combination.
      // Initialize all values of the test with don't care.
      test t(model_.get_parameters().size(), -1);
      t.set_num_dont_care_values((current_param_idx_ + 1) -
                                 param_indices.size());

      for (unsigned int i = 0; i < param_indices.size(); ++i) {
        const unsigned int param_idx = param_indices[i];
        const int param_value_to_cover = value_indices[i];
        t.get_values()[param_idx] = param_value_to_cover;
      }

      test_set_.get_list_of_tests().push_back(std::move(t));

      // Update the mapping from values of the current parameter to the tests.
      partitioning_of_tests_according_to_current_values_
          .value_to_row_mapping[current_param_value_to_cover]
          .push_back(test_set_.get_list_of_tests().back());
    }

    unsigned long long get_num_new_covered_tuples() const {
      return num_new_covered_tuples_;
    }

  private:
    bool ipog_vertical_extension_try_inject_value_combo(
        const citcpp::detail::param_vector &param_indices,
        const citcpp::detail::value_vector &value_indices,
        citcpp::detail::test &t) {

      int overwritten_dont_cares = 0;
      for (unsigned int i = 0; i < param_indices.size(); ++i) {
        const unsigned int param_idx = param_indices[i];
        const int param_value_to_cover = value_indices[i];
        const int param_value_in_test = t.get_values()[param_idx];

        if (param_value_in_test >= 0 &&
            param_value_to_cover != param_value_in_test) {
          // Cannot inject value combination in this test, moving on to the
          // next one.
          return false;
        }

        if (param_value_in_test < 0) {
          ++overwritten_dont_cares;
        }
      }

      for (unsigned int i = 0; i < param_indices.size(); ++i) {
        const unsigned int param_idx = param_indices[i];
        const int param_value_to_cover = value_indices[i];
        t.get_values()[param_idx] = param_value_to_cover;
      }

      t.set_num_dont_care_values(t.get_num_dont_care_values() -
                                 overwritten_dont_cares);

      return true;
    }

  private:
    const unsigned int current_param_idx_;
    const citcpp::detail::model &model_;
    citcpp::detail::internal_test_set &test_set_;
    citcpp::detail::ipog_horizontal_extension_result
        &partitioning_of_tests_according_to_current_values_;
    citcpp::detail::bitset_non_owning_uint64 &values_combo_bitset_;
    const unsigned long long num_missing_combinations_to_cover_;
    unsigned long long num_new_covered_tuples_;
};

class ipog_vertical_extension_functor {
  public:
    ipog_vertical_extension_functor(
        const unsigned int current_param_idx, const unsigned int strength,
        const citcpp::detail::model &model,
        citcpp::detail::internal_test_set &test_set,
        citcpp::detail::ipog_horizontal_extension_result
            &partitioning_of_tests_according_to_current_values,
        citcpp::detail::array_wrapper_uint64 &bitset_backing_array,
        const unsigned long long num_missing_combinations_to_cover)
        : current_param_idx_(current_param_idx),
          model_(model),
          test_set_(test_set),
          partitioning_of_tests_according_to_current_values_(
              partitioning_of_tests_according_to_current_values),
          bitset_backing_array_(bitset_backing_array),
          value_indices_(strength),
          num_missing_combinations_to_cover_(num_missing_combinations_to_cover),
          num_new_covered_tuples_(0) {}

    bool operator()(const citcpp::detail::param_vector &param_indices) {
      using namespace citcpp::detail;

      ipog_vertical_extension_func(param_indices);

      return num_new_covered_tuples_ < num_missing_combinations_to_cover_;
    }

    unsigned long long get_num_new_covered_tuples() const {
      return num_new_covered_tuples_;
    }

  private:
    void ipog_vertical_extension_func(
        const citcpp::detail::param_vector &param_indices) {
      using namespace citcpp::detail;

      bitset_non_owning_uint64::size_type bitset_size = 1;
      for (auto p : param_indices) {
        bitset_size *= model_.get_parameters()[p];
      }
      bitset_non_owning_uint64 values_combo_bitset(bitset_size);
      values_combo_bitset.set_backing_array(bitset_backing_array_.get_array());
      values_combo_bitset.reset();

      for (test &t : test_set_.get_list_of_tests()) {
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
            addend *= model_.get_parameters()[param_indices[j]];
          }
          base_index += addend;
        }

        if (index_valid) {
          values_combo_bitset.set(base_index);
        }
      }

      ipog_vertical_extension_tuple_functor tuple_functor(
          current_param_idx_, model_, test_set_,
          partitioning_of_tests_according_to_current_values_,
          values_combo_bitset, num_missing_combinations_to_cover_);

      visit_all_value_combos_of_param_combo(model_, param_indices,
                                            value_indices_, tuple_functor);

      num_new_covered_tuples_ += tuple_functor.get_num_new_covered_tuples();
    }

  private:
    const unsigned int current_param_idx_;
    const citcpp::detail::model &model_;
    citcpp::detail::internal_test_set &test_set_;
    citcpp::detail::ipog_horizontal_extension_result
        &partitioning_of_tests_according_to_current_values_;
    citcpp::detail::array_wrapper_uint64 &bitset_backing_array_;
    citcpp::detail::value_vector value_indices_;
    const unsigned long long num_missing_combinations_to_cover_;
    unsigned long long num_new_covered_tuples_;
};

}  // namespace

namespace citcpp {
namespace detail {

ipog_vertical_extension_result ipog_vertical_extension(
    const unsigned int current_param_idx, const unsigned int strength,
    const model &model, const std::vector<unsigned int> &parameter_index_map,
    const unsigned long long num_missing_combinations_to_cover,
    ipog_horizontal_extension_result
        &partitioning_of_tests_according_to_current_values,
    internal_test_set &test_set) {

  const unsigned int real_current_param_idx =
      parameter_index_map[current_param_idx];
  const int num_current_param_values =
      model.get_parameters()[real_current_param_idx];

  const unsigned int product_of_max_parameter_sizes =
      num_current_param_values *
      get_product_of_max_n_parameter_sizes(current_param_idx, strength - 1,
                                           model, parameter_index_map);

  array_wrapper_uint64 bitset_backing_array(product_of_max_parameter_sizes);

  // First initialize the result object.
  ipog_vertical_extension_result result = {0};

  param_combo_iterator param_combo_it(current_param_idx + 1, strength,
                                      parameter_index_map, true);
  ipog_vertical_extension_functor functor(
      current_param_idx, strength, model, test_set,
      partitioning_of_tests_according_to_current_values, bitset_backing_array,
      num_missing_combinations_to_cover);

  param_combo_it.visit_all_parameter_combinations(functor);

  result.num_new_covered_tuples = functor.get_num_new_covered_tuples();

  return result;
}

}  // namespace detail
}  // namespace citcpp
