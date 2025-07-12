#include <algorithm>
#include "coverage_map.hpp"

namespace
{
  unsigned long long
  recursively_initialize_coverage_map (
      unsigned int start_idx_for_next, unsigned int current_level,
      unsigned long long num_value_combinations,
      citcpp::detail::coverage_map_base &cov_map,
      const citcpp::detail::model &model,
      const std::vector<unsigned int> &parameter_index_map,
      const unsigned int n, const unsigned int k,
      citcpp::detail::coverage_map_base::size_type &cov_map_first_level_index)
  {
    using namespace citcpp::detail;

    if (current_level == k)
      {
	cov_map.get_coverage_map ()[cov_map_first_level_index] =
	    coverage_map_base::second_level_type (num_value_combinations);

	++cov_map_first_level_index;

	return num_value_combinations;
      }

    unsigned long long partial_sum = 0;
    for (unsigned int j = start_idx_for_next; j <= n - k + current_level; ++j)
      {
	partial_sum += recursively_initialize_coverage_map (
	    j + 1,
	    current_level + 1,
	    num_value_combinations
		* model.get_parameters ()[parameter_index_map[j]],
	    cov_map, model, parameter_index_map, n, k,
	    cov_map_first_level_index);
      }

    return partial_sum;
  }
}

namespace citcpp
{
  namespace detail
  {
    coverage_map_base::coverage_map_base (
	unsigned int n, unsigned int t, const model &model,
	const std::vector<unsigned int> &parameter_index_map,
	const binom_coeff_table &binomial_coeffs, bool fixed_last_parameter) :
	size_ (
	    fixed_last_parameter ?
		binomial_coeffs.get_coefficient (n, t - 1) :
		binomial_coeffs.get_coefficient (n, t)), model_ (model), parameter_index_map_ (
	    parameter_index_map), n_ (n), t_ (t), cov_map_ (size_), total_num_tuples_ (
	    0)
    {
      coverage_map_base::size_type cov_map_first_level_index = 0;

      if (fixed_last_parameter)
	{
	  const unsigned int real_last_param_idx = parameter_index_map[n_ - 1];
	  const int num_last_param_values =
	      model.get_parameters ()[real_last_param_idx];

	  total_num_tuples_ = recursively_initialize_coverage_map (
	      0, 0, num_last_param_values, *this, model, parameter_index_map,
	      n_ - 1, t_ - 1, cov_map_first_level_index);
	}
      else
	{
	  total_num_tuples_ = recursively_initialize_coverage_map (
	      0, 0, 1, *this, model, parameter_index_map, n_, t_,
	      cov_map_first_level_index);
	}
    }

    const model&
    coverage_map_base::get_model () const
    {
      return model_;
    }

    std::vector<bitset_with_num_ones>&
    coverage_map_base::get_coverage_map ()
    {
      return cov_map_;
    }

    const std::vector<bitset_with_num_ones>&
    coverage_map_base::get_coverage_map () const
    {
      return cov_map_;
    }

    unsigned long long
    coverage_map_base::get_total_number_of_tuples () const
    {
      return total_num_tuples_;
    }
  }
}
