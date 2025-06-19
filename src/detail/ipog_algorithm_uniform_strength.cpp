#include <ranges>
#include <functional>
#include "ipog_algorithm_uniform_strength.hpp"
#include "for_each_cross_product_elem.hpp"

namespace
{
  void
  recursive_combine_with_cov_map_index (
      unsigned int start_idx_for_next,
      unsigned int current_count,
      const unsigned int current_param_idx,
      const unsigned int strength,
      const citcpp::detail::binom_coeff_table &binomial_coeffs,
      std::vector<unsigned int> &param_indices,
      citcpp::detail::coverage_map::size_type cov_map_first_level_index,
      const std::function<
	  void
	  (const std::vector<unsigned int>&,
	   citcpp::detail::coverage_map::size_type)> &callback)
  {
    using namespace citcpp::detail;

    if (current_count == strength)
      {
	callback (param_indices, cov_map_first_level_index);
	return;
      }

    for (unsigned int j = start_idx_for_next; j < current_param_idx; ++j)
      {
	param_indices[current_count] = j;
	recursive_combine_with_cov_map_index (
	    j + 1,
	    current_count + 1,
	    current_param_idx,
	    strength,
	    binomial_coeffs,
	    param_indices,
	    cov_map_first_level_index
		+ binomial_coeffs.get_coefficient (j, current_count + 1),
	    callback);
      }
  }

  unsigned int
  ipog_horizontal_select_best_value (
      unsigned int current_param_idx, unsigned int strength,
      const citcpp::detail::model &model,
      const std::vector<unsigned int> &parameter_index_map,
      const citcpp::detail::binom_coeff_table &binomial_coeffs,
      const citcpp::detail::test &test,
      const citcpp::detail::coverage_map &cov_map, tf::Executor *executor)
  {
    using namespace citcpp::detail;

    // This is an array containing the coverage gain per value of the current parameter.
    std::vector<unsigned long long> gain_per_value (
	model.get_parameters ()[parameter_index_map[current_param_idx]]);
    std::vector<unsigned int> param_indices (strength);

    auto func_computing_gain_per_value =
	[&model, &parameter_index_map, &test, &cov_map, &gain_per_value]
	(const std::vector<unsigned int> &param_indices,
	 coverage_map::size_type cov_map_first_level_index)
	   {
	     const coverage_map::second_level_type & value_combinations = cov_map.get_coverage_map()[cov_map_first_level_index];
	     if (value_combinations.size() == 0)
	       {
		 // The bitset tracking the value combinations has not been initialized yet, which
		 // means the coverage gain is 1 for each of the values of the parameter.
		 for (unsigned long long & gain : gain_per_value)
		   {
		     ++gain;
		   }
	       }
	     else if (!value_combinations.all())
	       {
		 // We have a valid bitset and we have uncovered value combinations left in it.
		 // Thus we have to walk through it concerning all possible value
		 // combinations.
		 bitset_uint64::size_type base_index = 0;
		 bool found_dont_care = false;
		 for (std::vector<unsigned int>::size_type i = 0; i < param_indices.size(); ++i)
		   {
		     const unsigned int param_idx = param_indices[i];
		     const unsigned int real_param_idx = parameter_index_map[param_idx];
		     const int param_value = test.get_values()[real_param_idx];

		     if (param_value < 0)
		       {
			 // We have found a don't care value for that combination in
			 // the considered test.
			 found_dont_care = true;
			 break;
		       }

		     bitset_uint64::size_type addend = param_value;
		     for (std::vector<unsigned int>::size_type j = 0; j <= i; ++j)
		       {
			 addend *= model.get_parameters ()[real_param_idx];
		       }
		     base_index += addend;
		   }

		 if (!found_dont_care)
		   {
		     // If we have found a don't care value in one of the [0, ... ,current_param_idx - 1]
		     // parameters, then we skip the combination in the coverage gain computation.
		     for (unsigned int value = 0; value < gain_per_value.size (); ++value)
		       {
			 if (!value_combinations.test(base_index + value))
			   {
			     gain_per_value[value] += 1;
			   }
		       }
		   }
	       }
	   }
	 ;

    recursive_combine_with_cov_map_index (0, 0, current_param_idx, strength - 1,
					  binomial_coeffs, param_indices, 0,
					  func_computing_gain_per_value);

    unsigned int value_with_max_gain = 0;
    unsigned long long max_gain = 0;
    for (unsigned int value = 0; value < gain_per_value.size (); ++value)
      {
	if (gain_per_value[value] > max_gain)
	  {
	    value_with_max_gain = value;
	    max_gain = gain_per_value[value];
	  }
      }

    return value_with_max_gain;
  }

  unsigned long long
  ipog_horizontal_update_coverage_map (
      unsigned int current_param_idx, unsigned int strength,
      const citcpp::detail::model &model,
      const std::vector<unsigned int> &parameter_index_map,
      const citcpp::detail::binom_coeff_table &binomial_coeffs,
      const citcpp::detail::test &test, citcpp::detail::coverage_map &cov_map)
  {
    using namespace citcpp::detail;

    int current_param_value =
	test.get_values ()[parameter_index_map[current_param_idx]];

    std::vector<unsigned int> param_indices (strength);
    unsigned long long num_new_covered_tuples = 0;

    auto func_updating_coverage =
	[&model, &parameter_index_map, &test, current_param_value, &cov_map,
	 &num_new_covered_tuples]
	(const std::vector<unsigned int> &param_indices,
	 coverage_map::size_type cov_map_first_level_index)
	   {
	     coverage_map::second_level_type & value_combinations = cov_map.get_coverage_map()[cov_map_first_level_index];
	     if (value_combinations.size() == 0)
	       {
		 bitset_uint64::size_type bitset_size = 1;
		 // The bitset tracking the value combinations has not been initialized yet, so
		 // we do that now.
		 for (std::vector<unsigned int>::size_type i = 0; i < param_indices.size(); ++i)
		   {
		     unsigned int param_idx = param_indices[i];
		     unsigned int real_param_idx = parameter_index_map[param_idx];
		     bitset_size *= model.get_parameters ()[real_param_idx];
		   }
		 value_combinations = coverage_map::second_level_type (bitset_size);
	       }

	     if (value_combinations.all())
	       {
		 // We do not have any uncovered value combinations left. Thus, there is no point
		 // in trying to possibly update its coverage.
		 return;
	       }

	     bitset_uint64::size_type index = current_param_value;
	     bool found_dont_care = false;
	     for (std::vector<unsigned int>::size_type i = 0; i < param_indices.size(); ++i)
	       {
		 const unsigned int param_idx = param_indices[i];
		 const unsigned int real_param_idx = parameter_index_map[param_idx];
		 const int param_value = test.get_values()[real_param_idx];

		 if (param_value < 0)
		   {
		     // We have found a don't care value for that combination in
		     // the considered test.
		     found_dont_care = true;
		     break;
		   }

		 bitset_uint64::size_type addend = param_value;
		 for (std::vector<unsigned int>::size_type j = 0; j <= i; ++j)
		   {
		     addend *= model.get_parameters ()[real_param_idx];
		   }
		 index += addend;
	       }

	     if (!found_dont_care)
	       {
		 // If we have found a don't care value in one of the [0, ... ,current_param_idx - 1]
		 // parameters, then there is nothing to be updated concerning the coverage.
		 // This combination will be taken of during the vertical extension step.
		 if (!value_combinations.test_and_set(index))
		   {
		     ++num_new_covered_tuples;
		   }
	       }
	   };

    recursive_combine_with_cov_map_index (0, 0, current_param_idx, strength - 1,
					  binomial_coeffs, param_indices, 0,
					  func_updating_coverage);

    return num_new_covered_tuples;
  }
}

namespace citcpp
{
  namespace detail
  {
    create_all_value_combinations_result
    create_all_value_combinations (
	unsigned int strength, const model &model,
	const std::vector<unsigned int> &parameter_indices,
	citcpp::detail::test_set &test_set)
    {
      auto l_map_to_param_idx = [&parameter_indices]
      (int idx)
	{ return parameter_indices[idx];};

      auto l_map_to_num_param_values = [&model]
      (unsigned int param_idx)
	{ return model.get_parameters()[param_idx];};

      auto r_param_num_values = std::ranges::iota_view
	{ 0u, strength } | std::views::transform (l_map_to_param_idx)
	  | std::views::transform (l_map_to_num_param_values);

      std::vector<unsigned int> param_num_values (r_param_num_values.begin (),
						  r_param_num_values.end ());

      create_all_value_combinations_result result
	{ 0 };

      for_each_cross_product_elem (
	  param_num_values,
	  [&model, &parameter_indices, &test_set, &result]
	  (const std::vector<unsigned int> &next_cross_product_elem)
	    {
	      // Initialize all values of the test with don't care.
	      test t(model.get_parameters().size(), -1);
	      t.set_num_dont_care_values(0);

	      // Replace the first t elements with the cross product element.
	      for (unsigned int index = 0; index < next_cross_product_elem.size(); ++index)
		{
		  t.get_values()[parameter_indices[index]] = next_cross_product_elem[index];
		}

	      test_set.get_list_of_tests().push_back(std::move(t));

	      ++result.num_created_combinations;
	    });

      return result;
    }

    ipog_horizontal_extension_result
    ipog_horizontal_extension (
	unsigned int current_param_idx, unsigned int strength,
	const model &model,
	const std::vector<unsigned int> &parameter_index_map,
	const unsigned long long num_missing_combinations_to_cover,
	const binom_coeff_table &binomial_coeffs, test_set &test_set,
	coverage_map &cov_map, tf::Executor *executor)
    {
      // First initialize the result object.
      ipog_horizontal_extension_result result
	{ std::vector<list_intrusive<test>> (
	    model.get_parameters ()[parameter_index_map[current_param_idx]]), 0 };

      for (test &t : test_set.get_list_of_tests ())
	{
	  unsigned int selected_value = ipog_horizontal_select_best_value (
	      current_param_idx, strength, model, parameter_index_map,
	      binomial_coeffs, t, cov_map, executor);

// Now that we have selected the value with most coverage, we set it in the
// test accordingly.
	  t.get_values ()[parameter_index_map[current_param_idx]] =
	      selected_value;

	  // The last step consists in updating the coverage map.
	  unsigned long long num_new_covered_tuples =
	      ipog_horizontal_update_coverage_map (current_param_idx, strength,
						   model, parameter_index_map,
						   binomial_coeffs, t, cov_map);

	  // Keep track of how many tuples we have covered in addition.
	  result.num_new_covered_tuples += num_new_covered_tuples;

	  // Maintain a mapping from values of the current parameter to the tests.
	  result.value_to_row_mapping[selected_value].push_back (t);

	  if (result.num_new_covered_tuples
	      >= num_missing_combinations_to_cover)
	    {
	      break;
	    }
	}

      return result;
    }

    ipog_vertical_extension_result
    ipog_vertical_extension (
	unsigned int current_param_idx, unsigned int strength,
	const model &model,
	const std::vector<unsigned int> &parameter_index_map,
	const unsigned long long num_missing_combinations_to_cover,
	const binom_coeff_table &binomial_coeffs,
	std::vector<list_intrusive<test>> &value_to_row_mapping,
	test_set &test_set, coverage_map &cov_map, tf::Executor *executor)
    {
      // First initialize the result object.
      ipog_vertical_extension_result result =
	{ 0 };

      return result;
    }
  }
}
