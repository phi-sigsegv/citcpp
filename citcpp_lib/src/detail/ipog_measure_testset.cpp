#include "ipog_measure_testset.hpp"

namespace {

inline void measure_coverage(
    citcpp::detail::ipog_coverage_map::second_level_type& value_combinations,
    const std::vector<
        citcpp::detail::ipog_coverage_map::second_level_type::size_type>&
        weights,
    const citcpp::detail::internal_test_set& test_set,
    std::size_t num_seeded_tests, unsigned long long& num_covered_tuples) {

  using namespace citcpp::detail;

  const param_vector& param_indices =
      value_combinations.get_parameter_indices();

  std::size_t test_index = 0;
  for (const test& test : test_set.get_list_of_tests()) {
    if (test_index >= num_seeded_tests) {
      break;
    }

    // Here we compute an index into the bitset. To do so, we treat the
    // number of values of each parameter as a kind of radix. Consider
    // three parameters p_0, p_1, p_2. Now say that v_i is the number of
    // values for p_i. If we now have values x_0, x_1, x_2, then the index
    // is x_0 * v_1 * v_2 + x_1 * v_2 + x_2.
    ipog_coverage_map::second_level_type::size_type index = 0;
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

      index += static_cast<ipog_coverage_map::second_level_type::size_type>(
                   param_value) *
               weights[i];
    }

    if (!found_dont_care) {
      value_combinations.set_valid(index);
      if (!value_combinations.test_and_set_covered(index)) {
        ++num_covered_tuples;
      }
    }

    ++test_index;
  }
}

class ipog_measure_per_param_combo_functor {
  public:
    ipog_measure_per_param_combo_functor(
        const citcpp::detail::internal_model& model,
        const citcpp::detail::internal_test_set& test_set,
        std::size_t num_seeded_tests, unsigned int max_strength)
        : model_(model),
          test_set_(test_set),
          num_seeded_tests_(num_seeded_tests),
          num_covered_tuples_(0),
          weights_(max_strength) {}

    void operator()(citcpp::detail::ipog_coverage_map::second_level_type&
                        value_combinations) {

      using namespace citcpp::detail;

      const param_vector& param_indices =
          value_combinations.get_parameter_indices();

      // Pre-calculate weights for index computation
      ipog_coverage_map::second_level_type::size_type weight = 1;
      for (int i = static_cast<int>(param_indices.size() - 1); i >= 0; --i) {
        weights_[i] = weight;
        weight *= static_cast<ipog_coverage_map::second_level_type::size_type>(
            model_.get_parameter_num_values()[param_indices[i]]);
      }

      measure_coverage(value_combinations, weights_, test_set_,
                       num_seeded_tests_, num_covered_tuples_);
    }

    unsigned long long get_num_covered_tuples() const {
      return num_covered_tuples_;
    }

    void reset() { num_covered_tuples_ = 0; }

  private:
    const citcpp::detail::internal_model& model_;
    const citcpp::detail::internal_test_set& test_set_;
    const std::size_t num_seeded_tests_;
    unsigned long long num_covered_tuples_;
    std::vector<citcpp::detail::ipog_coverage_map::second_level_type::size_type>
        weights_;
};

}  // namespace

namespace citcpp {
namespace detail {

ipog_measure_testset_result ipog_measure_testset(
    const internal_model& model, const internal_test_set& test_set,
    std::size_t num_seeded_tests,
    std::vector<std::pair<const internal_relation*, ipog_coverage_map>>&
        relations) {

  // First initialize the result object.
  ipog_measure_testset_result result;

  unsigned int max_strength = 0;
  for (const auto& rel_and_cov_map : relations) {
    max_strength =
        std::max(max_strength,
                 rel_and_cov_map.second.get_number_of_parameters_to_select());
  }

  ipog_measure_per_param_combo_functor per_param_combo_functor(
      model, test_set, num_seeded_tests, max_strength);

  for (auto& rel_and_cov_map : relations) {
    per_param_combo_functor.reset();
    for (auto& value_combinations : rel_and_cov_map.second.get_coverage_map()) {
      per_param_combo_functor(value_combinations);
    }

    result.num_covered_tuples[rel_and_cov_map.first] =
        per_param_combo_functor.get_num_covered_tuples();
  }

  return result;
}

}  // namespace detail
}  // namespace citcpp
