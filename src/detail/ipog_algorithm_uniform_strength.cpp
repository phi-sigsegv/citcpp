#include <ranges>
#include <taskflow/taskflow.hpp>
#include <taskflow/algorithm/for_each.hpp>
#include "ipog_algorithm_uniform_strength.hpp"
#include "for_each_cross_product_elem.hpp"
#include "datatypes_config.hpp"

namespace
{
  unsigned long long
  ipog_loop_init_recursion (
      unsigned int start_idx_for_next, unsigned int current_count,
      unsigned long long current_prod_val, const unsigned int current_param_idx,
      const unsigned int strength, const citcpp::detail::model &model,
      const std::vector<unsigned int> &parameter_index_map,
      const citcpp::detail::binom_coeff_table &binomial_coeffs,
      citcpp::detail::coverage_map &cov_map,
      citcpp::detail::coverage_map::size_type &cov_map_first_level_index)
  {
    using namespace citcpp::detail;

    if (current_count == strength)
      {
	cov_map.get_coverage_map ()[cov_map_first_level_index] =
	    coverage_map::second_level_type (current_prod_val);

	++cov_map_first_level_index;

	return current_prod_val;
      }

    unsigned long long partial_sum = 0;
    for (unsigned int j = start_idx_for_next; j < current_param_idx; ++j)
      {
	partial_sum += ipog_loop_init_recursion (
	    j + 1, current_count + 1,
	    current_prod_val * model.get_parameters ()[parameter_index_map[j]],
	    current_param_idx, strength, model, parameter_index_map,
	    binomial_coeffs, cov_map, cov_map_first_level_index);
      }

    return partial_sum;
  }

  void
  ipog_horizontal_select_best_value_recursion (
      unsigned int start_idx_for_next, unsigned int current_count,
      const unsigned int current_param_idx, const citcpp::detail::model &model,
      const std::vector<unsigned int> &parameter_index_map,
      const citcpp::detail::binom_coeff_table &binomial_coeffs,
      const citcpp::detail::test &test,
      const citcpp::detail::coverage_map &cov_map,
      const unsigned int num_current_param_values,
      std::vector<unsigned long long> &gain_per_value,
      citcpp::detail::strength_vector<unsigned int> &param_indices,
      citcpp::detail::coverage_map::size_type &cov_map_first_level_index)
  {
    using namespace citcpp::detail;

    if (current_count == param_indices.size ())
      {
	const coverage_map::second_level_type &value_combinations =
	    cov_map.get_coverage_map ()[cov_map_first_level_index];

	++cov_map_first_level_index;

	if (!value_combinations.all ())
	  {
	    // We have a bitset and we have uncovered value combinations left in it.
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
	    for (std::vector<unsigned int>::size_type i = 0;
		i < param_indices.size (); ++i)
	      {
		const unsigned int param_idx = param_indices[i];
		const int param_value = test.get_values ()[param_idx];

		if (param_value < 0)
		  {
		    // We have found a don't care value for that combination in
		    // the considered test.
		    found_dont_care = true;
		    break;
		  }

		coverage_map::second_level_type::size_type addend = param_value;
		for (std::vector<unsigned int>::size_type j = i + 1;
		    j < param_indices.size (); ++j)
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
		for (unsigned int value = 0; value < gain_per_value.size ();
		    ++value)
		  {
		    if (!value_combinations.test (base_index + value))
		      {
			gain_per_value[value] += 1;
		      }
		  }
	      }
	  }

	return;
      }

    for (unsigned int j = start_idx_for_next; j < current_param_idx; ++j)
      {
	param_indices[current_count] = parameter_index_map[j];
	ipog_horizontal_select_best_value_recursion (j + 1, current_count + 1,
						     current_param_idx, model,
						     parameter_index_map,
						     binomial_coeffs, test,
						     cov_map,
						     num_current_param_values,
						     gain_per_value,
						     param_indices,
						     cov_map_first_level_index);
      }
  }

  int
  ipog_horizontal_select_best_value (
      unsigned int current_param_idx, unsigned int strength,
      const citcpp::detail::model &model,
      const std::vector<unsigned int> &parameter_index_map,
      const citcpp::detail::binom_coeff_table &binomial_coeffs,
      const citcpp::detail::test &test,
      const citcpp::detail::coverage_map &cov_map,
      citcpp::detail::strength_vector<unsigned int> &param_indices,
      unsigned int &last_picked_value,
      std::vector<unsigned int> &value_to_num_picked, tf::Executor *executor)
  {
    using namespace citcpp::detail;

    const unsigned int num_current_param_values =
	model.get_parameters ()[parameter_index_map[current_param_idx]];

    if (!executor)
      {
	// This is an array containing the coverage gain per value of the current parameter.
	std::vector<unsigned long long> gain_per_value (
	    num_current_param_values);
	coverage_map::size_type cov_map_first_level_index = 0;

	ipog_horizontal_select_best_value_recursion (0, 0, current_param_idx,
						     model, parameter_index_map,
						     binomial_coeffs, test,
						     cov_map,
						     num_current_param_values,
						     gain_per_value,
						     param_indices,
						     cov_map_first_level_index);

	int value_with_max_gain = -1;
	unsigned long long max_gain = 0;
	for (unsigned int v_index = 0; v_index < gain_per_value.size ();
	    ++v_index)
	  {
	    unsigned int value = (v_index + last_picked_value + 1)
		% gain_per_value.size ();
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
		// Since also this could be a tie (we have picked the value the same number
		// of times, we remember the value we have picked before, and choose
		// the next one in this case.
		if (value_with_max_gain >= 0
		    && value_to_num_picked[value]
			< value_to_num_picked[value_with_max_gain])
		  {
		    value_with_max_gain = value;
		  }
	      }
	  }

	if (value_with_max_gain >= 0)
	  {
	    last_picked_value = value_with_max_gain;
	    value_to_num_picked[value_with_max_gain]++;
	  }

	return value_with_max_gain;
      }
    else
      {
	// This is an array containing the coverage gain per value of the current parameter.
	std::vector<std::atomic_ullong> gain_per_value (
	    num_current_param_values);

	// In the sequential case, we iterate over C(current_param_idx, strength - 1)
	// combinations, since we have to choose strength - 1 parameters among parameters
	// less than the current one (we index parameters beginning at 0).
	// In the parallel case, we loop over fixations of the first selected parameter. Each
	// fixed first parameter constitutes a job that can be executed concurrently.
	// However, for each such job, we need an offset into the first-level array
	// of the coverage map. This index is the number of parameter combinations
	// processed by the selection with a lower index of the first parameter,
	// since combinations are stored in lexicographic oder.
	// For first param index = 0, we have an offset 0. For first param index = 1,
	// we have an offset of C(current_param_idx - 1, strength - 2),
	// since strength - 2 is the number of choices left to be made (since we have fixed
	// the first parameter), and we can only choose these from (current_param_idx - 1) parameters
	// if the have fixed the first parameter index to be 0.
	// For first param index = 2, we have an offset of
	// C(current_param_idx - 1, strength - 2) + C(current_param_idx - 2, strength - 2),
	// since strength - 2 is the number of choices left to be made (since we have fixed
	// the first parameter), and we can only choose these from (current_param_idx - 2)
	// parameters if we have fixed the first parameter index to be 1.
	// In addition, we have to add the offset of the job where the first param
	// index was fixed to 1. This continues in this way. So for the job with first param
	// index = i, we need the following offset: sum over j from 1 to i of C(current_param_idx - j, strength - 2).
	//
	// Now while we could compute this in a loop, there is a formula for computing
	// the sum over n from k to n_max of C(n, k),
	// which is called Hockey-stick Identity.
	// This sum is given by C(n_max + 1 , k + 1).
	// We can leverage upon that. What we need to compute is:
	// sum over n from (strength - 2) to (current_param_idx - 1) of C(n, strength - 2) -
	// sum over n from (strength - 2) to (current_param_idx - 1 - i) of C(n , strength - 2)
	// According to the formula, this is given by:
	// C(current_param_idx, strength - 1) - C(current_param_idx - i, strength - 1)
	const unsigned long long first_level_array_offset_part1 =
	    binomial_coeffs.get_coefficient (current_param_idx, strength - 1);

	tf::Taskflow taskflow;
	taskflow.for_each_index (
	    0u,
	    current_param_idx,
	    1u,
	    [current_param_idx, strength, &model, &parameter_index_map,
	     &binomial_coeffs, &test, &cov_map, &gain_per_value,
	     num_current_param_values, first_level_array_offset_part1]
	    (unsigned int i)
	      {
		std::vector<unsigned long long> local_gain_per_value (
		    num_current_param_values);
		strength_vector<unsigned int> param_indices (strength - 1);
		param_indices[0] = parameter_index_map[i];
		coverage_map::size_type cov_map_first_level_index = first_level_array_offset_part1 -
		binomial_coeffs.get_coefficient (current_param_idx - i,
		    strength - 1);

		ipog_horizontal_select_best_value_recursion (i + 1, 1, current_param_idx,
		    model, parameter_index_map,
		    binomial_coeffs, test,
		    cov_map,
		    num_current_param_values,
		    local_gain_per_value,
		    param_indices,
		    cov_map_first_level_index);

		for (unsigned int j = 0; j < local_gain_per_value.size(); ++j)
		  {
		    gain_per_value[j].fetch_add(local_gain_per_value[j], std::memory_order_acq_rel);
		  }
	      });

	executor->run (taskflow).wait ();

	int value_with_max_gain = -1;
	unsigned long long max_gain = 0;
	for (unsigned int v_index = 0; v_index < gain_per_value.size ();
	    ++v_index)
	  {
	    unsigned int value = (v_index + last_picked_value + 1)
		% gain_per_value.size ();
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
		// Since also this could be a tie (we have picked the value the same number
		// of times, we remember the value we have picked before, and choose
		// the next one in this case.
		if (value_with_max_gain >= 0
		    && value_to_num_picked[value]
			< value_to_num_picked[value_with_max_gain])
		  {
		    value_with_max_gain = value;
		  }
	      }
	  }

	if (value_with_max_gain >= 0)
	  {
	    last_picked_value = value_with_max_gain;
	    value_to_num_picked[value_with_max_gain]++;
	  }

	return value_with_max_gain;
      }
  }

  void
  ipog_horizontal_update_coverage_map_recursion (
      unsigned int start_idx_for_next, unsigned int current_count,
      const unsigned int current_param_idx, const citcpp::detail::model &model,
      const std::vector<unsigned int> &parameter_index_map,
      const citcpp::detail::binom_coeff_table &binomial_coeffs,
      const citcpp::detail::test &test, citcpp::detail::coverage_map &cov_map,
      const unsigned int num_current_param_values,
      const int current_param_value,
      citcpp::detail::strength_vector<unsigned int> &param_indices,
      citcpp::detail::coverage_map::size_type &cov_map_first_level_index,
      unsigned long long &num_new_covered_tuples)
  {
    using namespace citcpp::detail;

    if (current_count == param_indices.size ())
      {
	coverage_map::second_level_type &value_combinations =
	    cov_map.get_coverage_map ()[cov_map_first_level_index];

	++cov_map_first_level_index;

	if (value_combinations.all ())
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
	for (std::vector<unsigned int>::size_type i = 0;
	    i < param_indices.size (); ++i)
	  {
	    const unsigned int param_idx = param_indices[i];
	    const int param_value = test.get_values ()[param_idx];

	    if (param_value < 0)
	      {
		// We have found a don't care value for that combination in
		// the considered test.
		found_dont_care = true;
		break;
	      }

	    coverage_map::second_level_type::size_type addend = param_value;
	    for (std::vector<unsigned int>::size_type j = i + 1;
		j < param_indices.size (); ++j)
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
	    if (!value_combinations.test_and_set (index))
	      {
		++num_new_covered_tuples;
	      }
	  }

	return;
      }

    for (unsigned int j = start_idx_for_next; j < current_param_idx; ++j)
      {
	param_indices[current_count] = parameter_index_map[j];
	ipog_horizontal_update_coverage_map_recursion (
	    j + 1, current_count + 1, current_param_idx, model,
	    parameter_index_map, binomial_coeffs, test, cov_map,
	    num_current_param_values, current_param_value, param_indices,
	    cov_map_first_level_index, num_new_covered_tuples);
      }
  }

  unsigned long long
  ipog_horizontal_update_coverage_map (
      unsigned int current_param_idx, unsigned int strength,
      const citcpp::detail::model &model,
      const std::vector<unsigned int> &parameter_index_map,
      const citcpp::detail::binom_coeff_table &binomial_coeffs,
      const citcpp::detail::test &test, citcpp::detail::coverage_map &cov_map,
      citcpp::detail::strength_vector<unsigned int> &param_indices,
      tf::Executor *executor)
  {
    using namespace citcpp::detail;

    const unsigned int num_current_param_values =
	model.get_parameters ()[parameter_index_map[current_param_idx]];
    const int current_param_value =
	test.get_values ()[parameter_index_map[current_param_idx]];

    if (!executor)
      {
	coverage_map::size_type cov_map_first_level_index = 0;
	unsigned long long num_new_covered_tuples = 0;

	ipog_horizontal_update_coverage_map_recursion (
	    0, 0, current_param_idx, model, parameter_index_map,
	    binomial_coeffs, test, cov_map, num_current_param_values,
	    current_param_value, param_indices, cov_map_first_level_index,
	    num_new_covered_tuples);

	return num_new_covered_tuples;
      }
    else
      {
	std::atomic_ullong num_new_covered_tuples;

	// In the sequential case, we iterate over C(current_param_idx, strength - 1)
	// combinations, since we have to choose strength - 1 parameters among parameters
	// less than the current one (we index parameters beginning at 0).
	// In the parallel case, we loop over fixations of the first selected parameter. Each
	// fixed first parameter constitutes a job that can be executed concurrently.
	// However, for each such job, we need an offset into the first-level array
	// of the coverage map. This index is the number of parameter combinations
	// processed by the selection with a lower index of the first parameter,
	// since combinations are stored in lexicographic oder.
	// For first param index = 0, we have an offset 0. For first param index = 1,
	// we have an offset of C(current_param_idx - 1, strength - 2),
	// since strength - 2 is the number of choices left to be made (since we have fixed
	// the first parameter), and we can only choose these from (current_param_idx - 1) parameters
	// if the have fixed the first parameter index to be 0.
	// For first param index = 2, we have an offset of
	// C(current_param_idx - 1, strength - 2) + C(current_param_idx - 2, strength - 2),
	// since strength - 2 is the number of choices left to be made (since we have fixed
	// the first parameter), and we can only choose these from (current_param_idx - 2)
	// parameters if we have fixed the first parameter index to be 1.
	// In addition, we have to add the offset of the job where the first param
	// index was fixed to 1. This continues in this way. So for the job with first param
	// index = i, we need the following offset: sum over j from 1 to i of C(current_param_idx - j, strength - 2).
	//
	// Now while we could compute this in a loop, there is a formula for computing
	// the sum over n from k to n_max of C(n, k),
	// which is called Hockey-stick Identity.
	// This sum is given by C(n_max + 1 , k + 1).
	// We can leverage upon that. What we need to compute is:
	// sum over n from (strength - 2) to (current_param_idx - 1) of C(n, strength - 2) -
	// sum over n from (strength - 2) to (current_param_idx - 1 - i) of C(n , strength - 2)
	// According to the formula, this is given by:
	// C(current_param_idx, strength - 1) - C(current_param_idx - i, strength - 1)
	const unsigned long long first_level_array_offset_part1 =
	    binomial_coeffs.get_coefficient (current_param_idx, strength - 1);

	tf::Taskflow taskflow;
	taskflow.for_each_index (
	    0u,
	    current_param_idx,
	    1u,
	    [current_param_idx, strength, &model, &parameter_index_map,
	     &binomial_coeffs, &test, &cov_map, num_current_param_values,
	     current_param_value, &num_new_covered_tuples,
	     first_level_array_offset_part1]
	    (unsigned int i)
	      {
		unsigned long long local_num_new_covered_tuples = 0;
		strength_vector<unsigned int> param_indices (strength - 1);
		param_indices[0] = parameter_index_map[i];
		coverage_map::size_type cov_map_first_level_index = first_level_array_offset_part1 -
		binomial_coeffs.get_coefficient (current_param_idx - i,
		    strength - 1);

		ipog_horizontal_update_coverage_map_recursion (
		    i + 1, 1, current_param_idx, model, parameter_index_map,
		    binomial_coeffs, test, cov_map, num_current_param_values,
		    current_param_value, param_indices, cov_map_first_level_index,
		    local_num_new_covered_tuples);

		num_new_covered_tuples.fetch_add(local_num_new_covered_tuples, std::memory_order_acq_rel);
	      });

	executor->run (taskflow).wait ();

	return num_new_covered_tuples;
      }
  }

  bool
  ipog_vertical_extension_try_inject_value_combo (
      const unsigned int real_current_param_idx,
      const int current_param_value_to_cover, citcpp::detail::test &t,
      const citcpp::detail::strength_vector<unsigned int> &param_indices,
      const citcpp::detail::strength_vector<int> &values_to_cover)
  {
    bool can_inject_value_combination = true;
    int overwritten_dont_cares = 0;
    for (unsigned int i = 0; i < param_indices.size (); ++i)
      {
	const unsigned int param_idx = param_indices[i];
	const int param_value_to_cover = values_to_cover[i];
	const int param_value_in_test = t.get_values ()[param_idx];

	if (param_value_in_test >= 0
	    && param_value_to_cover != param_value_in_test)
	  {
	    // Cannot inject value combination in this test, moving on to the next
	    // one.
	    can_inject_value_combination = false;
	    break;
	  }

	if (param_value_in_test < 0)
	  {
	    ++overwritten_dont_cares;
	  }
      }

    if (can_inject_value_combination)
      {
	for (unsigned int i = 0; i < param_indices.size (); ++i)
	  {
	    const unsigned int param_idx = param_indices[i];
	    const int param_value_to_cover = values_to_cover[i];
	    t.get_values ()[param_idx] = param_value_to_cover;
	  }

	if (t.get_values ()[real_current_param_idx] < 0)
	  {
	    ++overwritten_dont_cares;
	  }
	t.get_values ()[real_current_param_idx] = current_param_value_to_cover;

	t.set_num_dont_care_values (
	    t.get_num_dont_care_values () - overwritten_dont_cares);

	return true;
      }

    return false;
  }

  void
  ipog_vertical_extension_func (
      const unsigned int current_param_idx,
      const unsigned int real_current_param_idx,
      const citcpp::detail::model &model,
      const unsigned long long num_missing_combinations_to_cover,
      citcpp::detail::test_set &test_set,
      citcpp::detail::ipog_horizontal_extension_result &partitioning_of_tests_according_to_current_values,
      const citcpp::detail::strength_vector<unsigned int> &param_indices,
      const citcpp::detail::strength_vector<int> &values_to_cover,
      citcpp::detail::coverage_map::second_level_type &value_combinations,
      citcpp::detail::coverage_map::second_level_type::size_type cov_map_second_level_index,
      unsigned long long &num_new_covered_tuples)
  {
    using namespace citcpp::detail;

    // First we check whether the value combination is covered, because if it is not,
    // then there is no point try to fit it into some test.
    if (value_combinations.test_and_set (cov_map_second_level_index))
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
    const int current_param_value_to_cover = values_to_cover.back ();

    // Iterate over the tests with the same value for the current parameter value
    // we have to cover.
    for (test &t : partitioning_of_tests_according_to_current_values.value_to_row_mapping[current_param_value_to_cover])
      {
	// We also skip tests which do not have at least one don't care
	// value.
	if (t.get_num_dont_care_values () == 0)
	  {
	    continue;
	  }

	if (ipog_vertical_extension_try_inject_value_combo (
	    real_current_param_idx, current_param_value_to_cover, t,
	    param_indices, values_to_cover))
	  {
	    // Return, since we have found a test and injected the value combination.
	    return;
	  }
      }

    // Now we iterate over the tests with a don't care value for the current parameter.
    for (auto it =
	partitioning_of_tests_according_to_current_values.rows_with_current_parameter_dont_care_value.begin ();
	it
	    != partitioning_of_tests_according_to_current_values.rows_with_current_parameter_dont_care_value.end ();
	++it)
      {
	test &t = *it;

	if (ipog_vertical_extension_try_inject_value_combo (
	    real_current_param_idx, current_param_value_to_cover, t,
	    param_indices, values_to_cover))
	  {
	    // Since we have successfully injected the combination, the test must be
	    // moved to a different partition for looking it up when trying
	    // to inject other combinations with the same value for the current
	    // parameter.
	    partitioning_of_tests_according_to_current_values.rows_with_current_parameter_dont_care_value.erase (
		it);
	    partitioning_of_tests_according_to_current_values.value_to_row_mapping[current_param_value_to_cover].push_back (
		t);

	    // Return, since we have found a test and injected the value combination.
	    return;
	  }
      }

    // If we have reached this point, then we did not find a matching test.
    // Thus, we have to add a new one with the value combination.
    // Initialize all values of the test with don't care.
    test t (model.get_parameters ().size (), -1);
    t.set_num_dont_care_values (
	(current_param_idx + 1) - (param_indices.size () + 1));

    for (unsigned int i = 0; i < param_indices.size (); ++i)
      {
	const unsigned int param_idx = param_indices[i];
	const int param_value_to_cover = values_to_cover[i];
	t.get_values ()[param_idx] = param_value_to_cover;
      }
    t.get_values ()[real_current_param_idx] = current_param_value_to_cover;

    test_set.get_list_of_tests ().push_back (std::move (t));

    // Update the mapping from values of the current parameter to the tests.
    partitioning_of_tests_according_to_current_values.value_to_row_mapping[current_param_value_to_cover].push_back (
	test_set.get_list_of_tests ().back ());
  }

  void
  ipog_vertical_extension_recursion_level2 (
      const unsigned int current_param_idx,
      const unsigned int real_current_param_idx,
      const citcpp::detail::model &model,
      const unsigned long long num_missing_combinations_to_cover,
      citcpp::detail::test_set &test_set,
      citcpp::detail::ipog_horizontal_extension_result &partitioning_of_tests_according_to_current_values,
      const citcpp::detail::strength_vector<unsigned int> &param_indices,
      const unsigned int num_current_param_values,
      unsigned int current_index,
      citcpp::detail::strength_vector<int> &values_to_cover,
      citcpp::detail::coverage_map::second_level_type &value_combinations,
      citcpp::detail::coverage_map::second_level_type::size_type &cov_map_second_level_index,
      unsigned long long &num_new_covered_tuples)
  {
    using namespace citcpp::detail;

    if (current_index == values_to_cover.size ())
      {
	ipog_vertical_extension_func (
	    current_param_idx, real_current_param_idx, model,
	    num_missing_combinations_to_cover, test_set,
	    partitioning_of_tests_according_to_current_values, param_indices,
	    values_to_cover, value_combinations, cov_map_second_level_index,
	    num_new_covered_tuples);
	++cov_map_second_level_index;

	return;
      }

    // The current range goes from 0 to max_value[current_index]
    unsigned int max_val =
	current_index < param_indices.size () ?
	    model.get_parameters ()[param_indices[current_index]] :
	    num_current_param_values;

    for (unsigned int i = 0; i < max_val; ++i)
      {
	if (num_new_covered_tuples >= num_missing_combinations_to_cover)
	  {
	    return;
	  }

	values_to_cover[current_index] = i;

	ipog_vertical_extension_recursion_level2 (
	    current_param_idx, real_current_param_idx, model,
	    num_missing_combinations_to_cover, test_set,
	    partitioning_of_tests_according_to_current_values, param_indices,
	    num_current_param_values, current_index + 1, values_to_cover,
	    value_combinations, cov_map_second_level_index,
	    num_new_covered_tuples);
      }
  }

  void
  ipog_vertical_extension_recursion (
      unsigned int start_idx_for_next,
      unsigned int current_count,
      const unsigned int current_param_idx,
      const unsigned int real_current_param_idx,
      const citcpp::detail::model &model,
      const std::vector<unsigned int> &parameter_index_map,
      const citcpp::detail::binom_coeff_table &binomial_coeffs,
      citcpp::detail::test_set &test_set,
      citcpp::detail::coverage_map &cov_map,
      citcpp::detail::ipog_horizontal_extension_result &partitioning_of_tests_according_to_current_values,
      const unsigned int num_current_param_values,
      const unsigned long long num_missing_combinations_to_cover,
      citcpp::detail::strength_vector<unsigned int> &param_indices,
      citcpp::detail::strength_vector<int> &values,
      citcpp::detail::coverage_map::size_type &cov_map_first_level_index,
      unsigned long long &num_new_covered_tuples)
  {
    using namespace citcpp::detail;

    if (current_count == param_indices.size ())
      {
	coverage_map::second_level_type &value_combinations =
	    cov_map.get_coverage_map ()[cov_map_first_level_index];

	++cov_map_first_level_index;

	if (value_combinations.all ())
	  {
	    // We do not have any uncovered value combinations left. Thus, there is no point
	    // in trying to match any test for the parameter combination.
	    return;
	  }

	citcpp::detail::coverage_map::second_level_type::size_type cov_map_second_level_index =
	    0;

	ipog_vertical_extension_recursion_level2 (
	    current_param_idx, real_current_param_idx, model,
	    num_missing_combinations_to_cover, test_set,
	    partitioning_of_tests_according_to_current_values, param_indices,
	    num_current_param_values, 0, values, value_combinations,
	    cov_map_second_level_index, num_new_covered_tuples);

	return;
      }

    for (unsigned int j = start_idx_for_next; j < current_param_idx; ++j)
      {
	if (num_new_covered_tuples >= num_missing_combinations_to_cover)
	  {
	    return;
	  }

	param_indices[current_count] = parameter_index_map[j];
	ipog_vertical_extension_recursion (
	    j + 1, current_count + 1, current_param_idx, real_current_param_idx,
	    model, parameter_index_map, binomial_coeffs, test_set, cov_map,
	    partitioning_of_tests_according_to_current_values,
	    num_current_param_values, num_missing_combinations_to_cover,
	    param_indices, values, cov_map_first_level_index,
	    num_new_covered_tuples);
      }
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

    unsigned long long
    ipog_loop_init (const unsigned int current_param_idx,
		    const unsigned int strength, const model &model,
		    const std::vector<unsigned int> &parameter_index_map,
		    const binom_coeff_table &binomial_coeffs,
		    coverage_map &cov_map, tf::Executor *executor)
    {
      const unsigned int real_current_param_idx =
	  parameter_index_map[current_param_idx];
      const int num_current_param_values =
	  model.get_parameters ()[real_current_param_idx];
      coverage_map::size_type cov_map_first_level_index = 0;

      const unsigned long long num_combinations_to_cover =
	  ipog_loop_init_recursion (0, 0, num_current_param_values,
				    current_param_idx, strength - 1, model,
				    parameter_index_map, binomial_coeffs,
				    cov_map, cov_map_first_level_index);

      return num_combinations_to_cover;
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
	{ std::vector<list_intrusive<test>> (num_current_param_values),
	    list_intrusive<test> (), 0 };

      strength_vector<unsigned int> param_indices (strength - 1);
      unsigned int last_picked_value = 0;
      std::vector<unsigned int> value_to_num_picked (num_current_param_values);

      for (test &t : test_set.get_list_of_tests ())
	{
	  int selected_value = ipog_horizontal_select_best_value (
	      current_param_idx, strength, model, parameter_index_map,
	      binomial_coeffs, t, cov_map, param_indices, last_picked_value,
	      value_to_num_picked, executor);

	  if (selected_value >= 0)
	    {
	      // We might not have selected any value. This can happen, if no matter
	      // which value we would pick, the coverage gain would be 0.
	      // If so, our best option is to keep it as don't care, in order for
	      // later vertical extension steps to exploit that don't care value.
	      // If we have selected a value however with most coverage, then we set it in the
	      // test accordingly.
	      t.get_values ()[parameter_index_map[current_param_idx]] =
		  selected_value;
	    }

	  // The last step consists in updating the coverage map.
	  unsigned long long num_new_covered_tuples =
	      selected_value >= 0 ?
		  ipog_horizontal_update_coverage_map (current_param_idx,
						       strength, model,
						       parameter_index_map,
						       binomial_coeffs, t,
						       cov_map, param_indices,
						       executor) :
		  0;

	  // Keep track of how many tuples we have covered in addition.
	  result.num_new_covered_tuples += num_new_covered_tuples;

	  // Maintain a mapping from values of the current parameter to the tests.
	  if (selected_value >= 0)
	    {
	      result.value_to_row_mapping[selected_value].push_back (t);
	    }
	  else
	    {
	      result.rows_with_current_parameter_dont_care_value.push_back (t);
	    }

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
	const unsigned int current_param_idx,
	const unsigned int strength,
	const model &model,
	const std::vector<unsigned int> &parameter_index_map,
	const unsigned long long num_missing_combinations_to_cover,
	const binom_coeff_table &binomial_coeffs,
	ipog_horizontal_extension_result &partitioning_of_tests_according_to_current_values,
	test_set &test_set, coverage_map &cov_map)
    {
      // First initialize the result object.
      ipog_vertical_extension_result result =
	{ 0 };

      const unsigned int real_current_param_idx =
	  parameter_index_map[current_param_idx];
      const int num_current_param_values =
	  model.get_parameters ()[real_current_param_idx];

      strength_vector<unsigned int> param_indices (strength - 1);
      unsigned long long num_new_covered_tuples = 0;
      strength_vector<int> values (strength);
      coverage_map::size_type cov_map_first_level_index = 0;

      ipog_vertical_extension_recursion (
	  0, 0, current_param_idx, real_current_param_idx, model,
	  parameter_index_map, binomial_coeffs, test_set, cov_map,
	  partitioning_of_tests_according_to_current_values,
	  num_current_param_values, num_missing_combinations_to_cover,
	  param_indices, values, cov_map_first_level_index,
	  num_new_covered_tuples);

      result.num_new_covered_tuples = num_new_covered_tuples;

      return result;
    }
  }
}
