#include "coverage_map.hpp"

#include <algorithm>

namespace {

unsigned long long recursively_initialize_coverage_map(
    int start_idx_for_next, int current_level,
    unsigned long long num_value_combinations,
    citcpp::detail::coverage_map_base &cov_map,
    const citcpp::detail::internal_model &model,
    const std::vector<unsigned int> &parameter_index_map,
    citcpp::detail::coverage_map_base::size_type &cov_map_first_level_index,
    citcpp::detail::param_vector &param_indices) {
  using namespace citcpp::detail;

  unsigned long long partial_sum = 0;
  for (int j = start_idx_for_next; j >= current_level; --j) {
    param_indices[current_level] = parameter_index_map[j];

    if (current_level == 0) {
      unsigned long long final_num_value_combinations =
          num_value_combinations *
          model.get_parameters()[parameter_index_map[j]];
      partial_sum += final_num_value_combinations;

      cov_map.get_coverage_map()[cov_map_first_level_index] =
          coverage_map_base::second_level_type(final_num_value_combinations,
                                               param_indices);

      ++cov_map_first_level_index;
    } else {
      partial_sum += recursively_initialize_coverage_map(
          j - 1, current_level - 1,
          num_value_combinations *
              model.get_parameters()[parameter_index_map[j]],
          cov_map, model, parameter_index_map, cov_map_first_level_index,
          param_indices);
    }
  }

  return partial_sum;
}

}  // namespace

namespace citcpp {
namespace detail {

coverage_map_base::coverage_map_base(
    unsigned int n, unsigned int t, const internal_model &model,
    const std::vector<unsigned int> &parameter_index_map,
    const binom_coeff_table &binomial_coeffs, bool fixed_last_parameter)
    : size_(fixed_last_parameter ? binomial_coeffs.get_coefficient(n - 1, t - 1)
                                 : binomial_coeffs.get_coefficient(n, t)),
      model_(model),
      parameter_index_map_(parameter_index_map),
      n_(n),
      t_(t),
      cov_map_(size_),
      total_num_tuples_(0) {
  coverage_map_base::size_type cov_map_first_level_index = 0;
  param_vector param_indices(t);

  if (fixed_last_parameter) {
    const unsigned int real_last_param_idx = parameter_index_map[n_ - 1];
    const int num_last_param_values =
        model.get_parameters()[real_last_param_idx];

    param_indices[t - 1] = real_last_param_idx;

    if (t_ >= 2) {
      total_num_tuples_ = recursively_initialize_coverage_map(
          n_ - 2, t_ - 2, num_last_param_values, *this, model,
          parameter_index_map, cov_map_first_level_index, param_indices);
    } else {
      // We have exactly one parameter to select, which is just the one we have
      // fixed. So we do not have to walk over combinations of parameters here.
      get_coverage_map()[0] = coverage_map_base::second_level_type(
          num_last_param_values, param_indices);
      total_num_tuples_ = num_last_param_values;
    }
  } else {
    total_num_tuples_ = recursively_initialize_coverage_map(
        n_ - 1, t_ - 1, 1, *this, model, parameter_index_map,
        cov_map_first_level_index, param_indices);
  }
}

}  // namespace detail
}  // namespace citcpp
