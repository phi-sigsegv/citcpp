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
      const std::vector<unsigned int> &parameter_index_map,
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
	param_indices[current_count] = parameter_index_map[j];
	recursive_combine_with_cov_map_index (
	    j + 1,
	    current_count + 1,
	    current_param_idx,
	    strength,
	    parameter_index_map,
	    binomial_coeffs,
	    param_indices,
	    cov_map_first_level_index
		+ binomial_coeffs.get_coefficient (j, current_count + 1),
	    callback);
      }
  }

  void
  recursive_cross_product_with_bitset_index (
      const citcpp::detail::model &model,
      const std::vector<unsigned int> &param_indices,
      const unsigned int num_current_param_values,
      unsigned int current_index,
      std::vector<int> &values,
      citcpp::detail::coverage_map::second_level_type::size_type cov_map_second_level_index,
      const std::function<
	  void
	  (const std::vector<int>&,
	   citcpp::detail::coverage_map::second_level_type::size_type)> &callback)
  {
    using namespace citcpp::detail;

    if (current_index == param_indices.size ())
      {
	// We still have to iterate through all values of the current parameter,
	// which we handle here.
	for (unsigned int i = 0; i < num_current_param_values; ++i)
	  {
	    values[current_index] = i;
	    callback (values, cov_map_second_level_index + i);
	  }

	return;
      }

    // The current range goes from 0 to max_value[current_index]
    unsigned int max_val = model.get_parameters ()[param_indices[current_index]];

    for (unsigned int i = 0; i < max_val; ++i)
      {
	values[current_index] = i;
	// Here we compute an index into the bitset. To do so, we treat the number of values
	// of each parameter as a kind of radix. Consider three parameters p_0, p_1, p_2.
	// Now say that v_i is the number of values for p_i. If we now have values
	// x_0, x_1, x_2, then the index is x_0 * v_1 * v_2 + x_1 * v_2 + x_2.
	// In the recursion we just compute a base index. We just compute
	// x_0 * v_1 * v_2 + x_1 * v_2.
	coverage_map::second_level_type::size_type addend = i;
	for (std::vector<unsigned int>::size_type j = current_index + 1;
	    j < param_indices.size (); ++j)
	  {
	    addend *= model.get_parameters ()[param_indices[j]];
	  }
	addend *= num_current_param_values;

	recursive_cross_product_with_bitset_index (
	    model, param_indices, num_current_param_values, current_index + 1,
	    values, cov_map_second_level_index + addend, callback);
      }
  }

  unsigned int
  ipog_horizontal_select_best_value (
      unsigned int current_param_idx, unsigned int strength,
      const citcpp::detail::model &model,
      const std::vector<unsigned int> &parameter_index_map,
      const citcpp::detail::binom_coeff_table &binomial_coeffs,
      const citcpp::detail::test &test,
      const citcpp::detail::coverage_map &cov_map,
      std::vector<unsigned int> &value_to_num_picked, tf::Executor *executor)
  {
    using namespace citcpp::detail;

    const unsigned int num_current_param_values =
	model.get_parameters ()[parameter_index_map[current_param_idx]];
    // This is an array containing the coverage gain per value of the current parameter.
    std::vector<unsigned long long> gain_per_value (num_current_param_values);
    std::vector<unsigned int> param_indices (strength - 1);

    auto func_computing_gain_per_value =
	[&model, &test, &cov_map, num_current_param_values, &gain_per_value]
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
		 // Here we compute an index into the bitset. To do so, we treat the number of values
		 // of each parameter as a kind of radix. Consider three parameters p_0, p_1, p_2.
		 // The last parameter is always the current one processed by IPOG.
		 // Now say that v_i is the number of values for p_i. If we now have values
		 // x_0, x_1, x_2, then the index is x_0 * v_1 * v_2 + x_1 * v_2 + x_2.
		 // In the base index we just compute x_0 * v_1 * v_2 + x_1 * v_2, since
		 // that expression is constant throughout all different values of p_2 whose
		 // different coverage gains we want to assess.
		 coverage_map::second_level_type::size_type base_index = 0;
		 bool found_dont_care = false;
		 for (std::vector<unsigned int>::size_type i = 0; i < param_indices.size(); ++i)
		   {
		     const unsigned int param_idx = param_indices[i];
		     const int param_value = test.get_values()[param_idx];

		     if (param_value < 0)
		       {
			 // We have found a don't care value for that combination in
			 // the considered test.
			 found_dont_care = true;
			 break;
		       }

		     coverage_map::second_level_type::size_type addend = param_value;
		     for (std::vector<unsigned int>::size_type j = i + 1; j < param_indices.size(); ++j)
		       {
			 addend *= model.get_parameters ()[param_indices[j]];
		       }
		     addend *= num_current_param_values;
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
					  parameter_index_map, binomial_coeffs,
					  param_indices, 0,
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
	else if (gain_per_value[value] == max_gain)
	  {
	    // We use a simple tie breaking strategy: We do not favor one value over the
	    // other. If two values have the same gain, then we pick the one which we
	    // have picked less so far.
	    if (value_to_num_picked[value]
		< value_to_num_picked[value_with_max_gain])
	      {
		value_with_max_gain = value;
	      }
	  }
      }

    value_to_num_picked[value_with_max_gain]++;

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

    const unsigned int num_current_param_values =
	model.get_parameters ()[parameter_index_map[current_param_idx]];
    const int current_param_value =
	test.get_values ()[parameter_index_map[current_param_idx]];

    std::vector<unsigned int> param_indices (strength - 1);
    unsigned long long num_new_covered_tuples = 0;

    auto func_updating_coverage =
	[&model, &test, &cov_map, num_current_param_values, current_param_value,
	 &num_new_covered_tuples]
	(const std::vector<unsigned int> &param_indices,
	 coverage_map::size_type cov_map_first_level_index)
	   {
	     coverage_map::second_level_type & value_combinations = cov_map.get_coverage_map()[cov_map_first_level_index];
	     if (value_combinations.size() == 0)
	       {
		 coverage_map::second_level_type::size_type bitset_size = num_current_param_values;
		 // The bitset tracking the value combinations has not been initialized yet, so
		 // we do that now.
		 for (std::vector<unsigned int>::size_type i = 0; i < param_indices.size(); ++i)
		   {
		     unsigned int param_idx = param_indices[i];
		     bitset_size *= model.get_parameters ()[param_idx];
		   }
		 value_combinations = coverage_map::second_level_type (bitset_size);
	       }

	     if (value_combinations.all())
	       {
		 // We do not have any uncovered value combinations left. Thus, there is no point
		 // in trying to possibly update its coverage.
		 return;
	       }

	     // Here we compute an index into the bitset. To do so, we treat the number of values
	     // of each parameter as a kind of radix. Consider three parameters p_0, p_1, p_2.
	     // The last parameter is always the current one processed by IPOG.
	     // Now say that v_i is the number of values for p_i. If we now have values
	     // x_0, x_1, x_2, then the index is x_0 * v_1 * v_2 + x_1 * v_2 + x_2.
	     coverage_map::second_level_type::size_type index = current_param_value;
	     bool found_dont_care = false;
	     for (std::vector<unsigned int>::size_type i = 0; i < param_indices.size(); ++i)
	       {
		 const unsigned int param_idx = param_indices[i];
		 const int param_value = test.get_values()[param_idx];

		 if (param_value < 0)
		   {
		     // We have found a don't care value for that combination in
		     // the considered test.
		     found_dont_care = true;
		     break;
		   }

		 coverage_map::second_level_type::size_type addend = param_value;
		 for (std::vector<unsigned int>::size_type j = i + 1; j < param_indices.size(); ++j)
		   {
		     addend *= model.get_parameters ()[param_indices[j]];
		   }
		 addend *= num_current_param_values;
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
					  parameter_index_map, binomial_coeffs,
					  param_indices, 0,
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
	const unsigned int current_param_idx, const unsigned int strength,
	const model &model,
	const std::vector<unsigned int> &parameter_index_map,
	const unsigned long long num_missing_combinations_to_cover,
	const binom_coeff_table &binomial_coeffs, test_set &test_set,
	coverage_map &cov_map, tf::Executor *executor)
    {
      const unsigned int real_current_param_idx =
	  parameter_index_map[current_param_idx];
      const int num_current_param_values =
	  model.get_parameters ()[real_current_param_idx];

      // First initialize the result object.
      ipog_horizontal_extension_result result
	{ std::vector<list_intrusive<test>> (num_current_param_values), 0 };

      std::vector<unsigned int> value_to_num_picked (num_current_param_values);

      for (test &t : test_set.get_list_of_tests ())
	{
	  unsigned int selected_value = ipog_horizontal_select_best_value (
	      current_param_idx, strength, model, parameter_index_map,
	      binomial_coeffs, t, cov_map, value_to_num_picked, executor);

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
	const unsigned int current_param_idx, const unsigned int strength,
	const model &model,
	const std::vector<unsigned int> &parameter_index_map,
	const unsigned long long num_missing_combinations_to_cover,
	const binom_coeff_table &binomial_coeffs,
	std::vector<list_intrusive<test>> &value_to_row_mapping,
	test_set &test_set, coverage_map &cov_map)
    {
      // First initialize the result object.
      ipog_vertical_extension_result result =
	{ 0 };

      const unsigned int real_current_param_idx =
	  parameter_index_map[current_param_idx];
      const int num_current_param_values =
	  model.get_parameters ()[real_current_param_idx];

      std::vector<unsigned int> param_indices (strength - 1);
      unsigned long long num_new_covered_tuples = 0;
      std::vector<int> values (strength);

      auto func_find_suitable_row_and_extend =
	  [current_param_idx, &model, num_missing_combinations_to_cover,
	   &value_to_row_mapping, &test_set, &cov_map, real_current_param_idx,
	   num_current_param_values, &num_new_covered_tuples, &values]
	  (const std::vector<unsigned int> &param_indices,
	   coverage_map::size_type cov_map_first_level_index)
	     {
	       coverage_map::second_level_type & value_combinations = cov_map.get_coverage_map()[cov_map_first_level_index];

	       if (value_combinations.all())
		 {
		   // We do not have any uncovered value combinations left. Thus, there is no point
		   // in trying to match any test for the parameter combination.
		   return;
		 }

	       auto nested_func = [current_param_idx, &model, num_missing_combinations_to_cover, &value_to_row_mapping, &test_set, real_current_param_idx, &num_new_covered_tuples, &param_indices, &value_combinations]
	       (const std::vector<int> &values_to_cover, coverage_map::second_level_type::size_type cov_map_second_level_index)
		 {
		   // First we check whether the value combination is covered, because if it is not,
		   // then there is no point try to fit it into some test.
		   if (value_combinations.test_and_set(cov_map_second_level_index))
		     {
		       return;
		     }

		   ++num_new_covered_tuples;

		   // Now we iterate over all tests trying to fit the value combination.
		   // However, we do not iterate over the entire test test, but instead
		   // leverage upon its partition according to the value of the current
		   // parameter as set by the horizontal extension for a test.
		   // For that parameter, after horizontal extension, the value is guaranteed
		   // to not be a don't care.
		   // First we determine the value of the current parameter in the value
		   // combination we want to cover.
		   const int current_param_value_to_cover = values_to_cover.back();

		   // Iterative over the tests we have to check
		   for (test & t : value_to_row_mapping[current_param_value_to_cover])
		     {
		       // We also skip tests which do not have at least one don't care
		       // value.
		       if (t.get_num_dont_care_values() == 0)
			 {
			   continue;
			 }

		       bool can_inject_value_combination = true;
		       int overwritten_dont_cares = 0;
		       for (unsigned int i = 0; i < param_indices.size(); ++i)
			 {
			   const unsigned int param_idx = param_indices[i];
			   const int param_value_to_cover = values_to_cover[i];
			   const int param_value_in_test = t.get_values()[param_idx];

			   if (param_value_in_test >= 0 && param_value_to_cover != param_value_in_test)
			     {
			       // Cannot inject value combination in this test, moving on to the next
			       // one.
			       can_inject_value_combination = false;
			       break;
			     }

			   if (param_value_in_test < 0 )
			     {
			       ++overwritten_dont_cares;
			     }
			 }

		       if (can_inject_value_combination)
			 {
			   for (unsigned int i = 0; i < param_indices.size(); ++i)
			     {
			       const unsigned int param_idx = param_indices[i];
			       const int param_value_to_cover = values_to_cover[i];
			       t.get_values()[param_idx] = param_value_to_cover;
			     }

			   t.set_num_dont_care_values(t.get_num_dont_care_values() - overwritten_dont_cares);

			   // Return, since we have found a test and injected the value combination.
			   return;
			 }
		     }

		   // If we have reached this point, then we did not find a matching test.
		   // Thus, we have to add a new one with the value combination.
		   // Initialize all values of the test with don't care.
		   test t(model.get_parameters().size(), -1);
		   t.set_num_dont_care_values((current_param_idx + 1) - (param_indices.size() + 1));

		   for (unsigned int i = 0; i < param_indices.size(); ++i)
		     {
		       const unsigned int param_idx = param_indices[i];
		       const int param_value_to_cover = values_to_cover[i];
		       t.get_values()[param_idx] = param_value_to_cover;
		     }
		   t.get_values()[real_current_param_idx] = current_param_value_to_cover;

		   test_set.get_list_of_tests().push_back(std::move(t));

		   // Update the mapping from values of the current parameter to the tests.
		   value_to_row_mapping[current_param_value_to_cover].push_back (test_set.get_list_of_tests().back());
		 };

	       recursive_cross_product_with_bitset_index(model, param_indices, num_current_param_values, 0, values, 0, nested_func);
	     };

      recursive_combine_with_cov_map_index (0, 0, current_param_idx,
					    strength - 1, parameter_index_map,
					    binomial_coeffs, param_indices, 0,
					    func_find_suitable_row_and_extend);

      result.num_new_covered_tuples = num_new_covered_tuples;

      return result;
    }
  }
}
