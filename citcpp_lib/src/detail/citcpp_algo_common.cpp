#include "citcpp_algo_common.hpp"

namespace {

class num_combos_per_param_combo_functor {
  public:
    num_combos_per_param_combo_functor(
        const citcpp::detail::internal_model& model,
        const citcpp::detail::internal_test_set& test_set,
        const unsigned int param_combo_sizes,
        const citcpp::detail::bitset_uint64::size_type
            bitset_backing_array_size)
        : model_(model),
          test_set_(test_set),
          weights_(param_combo_sizes),
          values_combo_bitset_(bitset_backing_array_size),
          num_combos_{0, 0} {}

    bool operator()(const citcpp::detail::param_vector& param_indices) {
      using namespace citcpp::detail;

      bitset_uint64::size_type bitset_size = 1;
      for (const uint16_t p : param_indices) {
        bitset_size *= model_.get_parameter_num_values()[p];
      }
      values_combo_bitset_.reset_with_new_size(bitset_size);

      // Pre-calculate weights for index computation.
      // Those are used to compute an index into the bitset. To do so, we treat
      // the number of values of each parameter as a kind of radix. Consider
      // three parameters p_0, p_1, p_2. Now say that v_i is the number of
      // values for p_i. If we now have values x_0, x_1, x_2, then the index
      // is x_0 * v_1 * v_2 + x_1 * v_2 + x_2.
      bitset_uint64::size_type weight = 1;
      for (int i = static_cast<int>(param_indices.size() - 1); i >= 0; --i) {
        weights_[i] = weight;
        weight *= model_.get_parameter_num_values()[param_indices[i]];
      }

      num_combos_.num_combos_to_cover += bitset_size;

      for (const auto& test : test_set_.get_list_of_tests()) {
        bitset_uint64::size_type index = 0;
        bool found_dont_care = false;
        for (std::size_t i = 0; i < param_indices.size(); ++i) {
          const int param_value = test.get_values()[param_indices[i]];

          if (param_value < 0) {
            // We have found a don't care value for that combination in
            // the considered test in one of the parameters.
            // There is nothing to be updated concerning the
            // coverage.
            found_dont_care = true;
            break;
          }

          index += param_value * weights_[i];
        }

        if (!found_dont_care) {
          if (!values_combo_bitset_.test_and_set(index)) {
            num_combos_.num_covered_combos++;
          }
        }
      }

      return true;
    }

    const citcpp::detail::number_of_combinations& get_number_of_combos() const {
      return num_combos_;
    }

  private:
    const citcpp::detail::internal_model& model_;
    const citcpp::detail::internal_test_set& test_set_;
    std::vector<citcpp::detail::bitset_uint64::size_type> weights_;
    citcpp::detail::bitset_uint64 values_combo_bitset_;
    citcpp::detail::number_of_combinations num_combos_;
};

}  // namespace

namespace citcpp {
namespace detail {

number_of_combinations get_number_of_combinations(
    unsigned int n, const internal_model& model,
    const std::vector<unsigned int>& parameter_index_map, unsigned int t,
    bool fixed_last_parameter, const internal_test_set& test_set) {

  const unsigned int product_of_max_parameter_sizes =
      get_product_of_max_n_parameter_sizes(
          static_cast<unsigned int>(parameter_index_map.size()), t, model,
          parameter_index_map);

  num_combos_per_param_combo_functor per_param_combo_functor(
      model, test_set, t, product_of_max_parameter_sizes);

  param_combo_iterator param_combo_it(n, t, parameter_index_map,
                                      fixed_last_parameter);
  param_combo_it.visit_all_parameter_combinations(per_param_combo_functor);

  return per_param_combo_functor.get_number_of_combos();
}

}  // namespace detail
}  // namespace citcpp
