#include <ranges>
#include <new>
#include "ipog_algorithm_uniform_strength.hpp"
#include "for_each_cross_product_elem.hpp"

namespace
{
  struct alignas( std::hardware_destructive_interference_size ) aligned_ull_value
  {
    unsigned long long value;
  };

  class ipog_horizontal_select_best_value_per_param_combo_functor
  {
  public:
    ipog_horizontal_select_best_value_per_param_combo_functor (
	const citcpp::detail::model &model, const citcpp::detail::test &test,
	std::vector<unsigned long long> &gain_per_value) :
	model_ (model), test_ (test), gain_per_value_ (gain_per_value)
    {
    }

    bool
    operator() (const citcpp::detail::coverage_map_iterator_state &cov_map_it)
    {
      using namespace citcpp::detail;

      const strength_vector<unsigned int> &param_indices =
	  cov_map_it.get_parameter_indices ();
      const coverage_map_ipog::second_level_type &value_combinations =
	  cov_map_it.get_bitset ();

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
	  coverage_map_ipog::second_level_type::size_type base_index = 0;
	  for (std::vector<unsigned int>::size_type i = 0;
	      i < param_indices.size () - 1; ++i)
	    {
	      const unsigned int param_idx = param_indices[i];
	      const int param_value = test_.get_values ()[param_idx];

	      if (param_value < 0)
		{
		  // We have found a don't care value for that combination in
		  // the considered test.
		  return true;
		}

	      coverage_map_ipog::second_level_type::size_type addend =
		  param_value;
	      for (std::vector<unsigned int>::size_type j = i + 1;
		  j < param_indices.size (); ++j)
		{
		  addend *= model_.get_parameters ()[param_indices[j]];
		}
	      base_index += addend;
	    }

	  // If we have found a don't care value in one of the [0, ... ,current_param_idx - 1]
	  // parameters, then we skip the combination in the coverage gain computation.
	  for (unsigned int value = 0; value < gain_per_value_.size (); ++value)
	    {
	      if (!value_combinations.test (base_index + value))
		{
		  gain_per_value_[value] += 1;
		}
	    }
	}

      return true;
    }

  private:
    const citcpp::detail::model &model_;
    const citcpp::detail::test &test_;
    std::vector<unsigned long long> &gain_per_value_;
  };

  class ipog_horizontal_select_best_value_per_param_combo_functor_parallel
  {
  public:
    ipog_horizontal_select_best_value_per_param_combo_functor_parallel (
	const citcpp::detail::model &model,
	const citcpp::detail::test &test,
	citcpp::detail::thread_local_vector<std::vector<aligned_ull_value>> &gain_per_value,
	const citcpp::detail::coverage_map_parallel_iterator<true> &cov_map_it) :
	model_ (model), test_ (test), gain_per_value_ (gain_per_value), cov_map_it_ (
	    cov_map_it)
    {
    }

    bool
    operator() (const citcpp::detail::coverage_map_iterator_state &cov_map_it)
    {
      using namespace citcpp::detail;

      const strength_vector<unsigned int> &param_indices =
	  cov_map_it.get_parameter_indices ();
      const coverage_map_ipog::second_level_type &value_combinations =
	  cov_map_it.get_bitset ();

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
	  coverage_map_ipog::second_level_type::size_type base_index = 0;
	  for (std::vector<unsigned int>::size_type i = 0;
	      i < param_indices.size () - 1; ++i)
	    {
	      const unsigned int param_idx = param_indices[i];
	      const int param_value = test_.get_values ()[param_idx];

	      if (param_value < 0)
		{
		  // We have found a don't care value for that combination in
		  // the considered test.
		  return true;
		}

	      coverage_map_ipog::second_level_type::size_type addend =
		  param_value;
	      for (std::vector<unsigned int>::size_type j = i + 1;
		  j < param_indices.size (); ++j)
		{
		  addend *= model_.get_parameters ()[param_indices[j]];
		}
	      base_index += addend;
	    }

	  // If we have found a don't care value in one of the [0, ... ,current_param_idx - 1]
	  // parameters, then we skip the combination in the coverage gain computation.
	  std::vector<aligned_ull_value> &thread_local_gain_per_value =
	      gain_per_value_[cov_map_it_.get_worker_id ()];
	  for (unsigned int value = 0;
	      value < thread_local_gain_per_value.size (); ++value)
	    {
	      if (!value_combinations.test (base_index + value))
		{
		  thread_local_gain_per_value[value].value += 1;
		}
	    }
	}

      return true;
    }

  private:
    const citcpp::detail::model &model_;
    const citcpp::detail::test &test_;
    citcpp::detail::thread_local_vector<std::vector<aligned_ull_value>> &gain_per_value_;
    const citcpp::detail::coverage_map_parallel_iterator<true> &cov_map_it_;
  };

  int
  ipog_horizontal_select_best_value (
      const unsigned int num_current_param_values,
      const citcpp::detail::model &model, const citcpp::detail::test &test,
      citcpp::detail::coverage_map_iterator<true> &cov_map_it,
      unsigned int &last_picked_value,
      std::vector<unsigned int> &value_to_num_picked)
  {
    using namespace citcpp::detail;

    // This is an array containing the coverage gain per value of the current parameter.
    std::vector<unsigned long long> gain_per_value (num_current_param_values);

    ipog_horizontal_select_best_value_per_param_combo_functor per_param_combo_functor (
	model, test, gain_per_value);
    cov_map_it.visit_all_parameter_combinations (per_param_combo_functor);

    int value_with_max_gain = -1;
    unsigned long long max_gain = 0;
    for (unsigned int v_index = 0; v_index < gain_per_value.size (); ++v_index)
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

  int
  ipog_horizontal_select_best_value (
      const unsigned int num_current_param_values,
      const citcpp::detail::model &model, const citcpp::detail::test &test,
      citcpp::detail::coverage_map_parallel_iterator<true> &cov_map_it,
      unsigned int &last_picked_value,
      std::vector<unsigned int> &value_to_num_picked)
  {
    using namespace citcpp::detail;

    // This is an array containing the coverage gain per value of the current parameter.
    thread_local_vector<std::vector<aligned_ull_value>> gain_per_value (
	cov_map_it.get_num_workers (),
	std::vector<aligned_ull_value> (num_current_param_values));

    ipog_horizontal_select_best_value_per_param_combo_functor_parallel per_param_combo_functor (
	model, test, gain_per_value, cov_map_it);
    cov_map_it.visit_all_parameter_combinations (per_param_combo_functor);

    int value_with_max_gain = -1;
    unsigned long long max_gain = 0;
    for (unsigned int v_index = 0; v_index < num_current_param_values;
	++v_index)
      {
	unsigned int value = (v_index + last_picked_value + 1)
	    % num_current_param_values;

	unsigned long long value_gain = 0;
	for (std::vector<aligned_ull_value> &thread_local_gain_per_value : gain_per_value)
	  {
	    value_gain += thread_local_gain_per_value[value].value;
	  }

	if (value_gain > max_gain)
	  {
	    value_with_max_gain = value;
	    max_gain = value_gain;
	  }
	else if (value_gain == max_gain)
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

  class ipog_horizontal_update_coverage_map_per_param_combo_functor
  {
  public:
    ipog_horizontal_update_coverage_map_per_param_combo_functor (
	const citcpp::detail::model &model, const citcpp::detail::test &test,
	const int current_param_selected_value) :
	model_ (model), test_ (test), current_param_selected_value_ (
	    current_param_selected_value), num_new_covered_tuples_ (0)
    {
    }

    bool
    operator() (citcpp::detail::coverage_map_iterator_state &cov_map_it)
    {
      using namespace citcpp::detail;

      const strength_vector<unsigned int> &param_indices =
	  cov_map_it.get_parameter_indices ();
      coverage_map_ipog::second_level_type &value_combinations =
	  cov_map_it.get_bitset ();

      if (!value_combinations.all ())
	{
	  // Here we compute an index into the bitset. To do so, we treat the number of values
	  // of each parameter as a kind of radix. Consider three parameters p_0, p_1, p_2.
	  // The last parameter is always the current one processed by IPOG.
	  // Now say that v_i is the number of values for p_i. If we now have values
	  // x_0, x_1, x_2, then the index is x_0 * v_1 * v_2 + x_1 * v_2 + x_2.
	  coverage_map_ipog::second_level_type::size_type index =
	      current_param_selected_value_;
	  for (std::vector<unsigned int>::size_type i = 0;
	      i < param_indices.size () - 1; ++i)
	    {
	      const unsigned int param_idx = param_indices[i];
	      const int param_value = test_.get_values ()[param_idx];

	      if (param_value < 0)
		{
		  // We have found a don't care value for that combination in
		  // the considered test in one of the [0, ... ,current_param_idx - 1]
		  // parameters. There is nothing to be updated concerning the coverage.
		  // This combination will be taken care of during the vertical extension step.
		  return true;
		}

	      coverage_map_ipog::second_level_type::size_type addend =
		  param_value;
	      for (std::vector<unsigned int>::size_type j = i + 1;
		  j < param_indices.size (); ++j)
		{
		  addend *= model_.get_parameters ()[param_indices[j]];
		}
	      index += addend;
	    }

	  if (!value_combinations.test_and_set (index))
	    {
	      ++num_new_covered_tuples_;
	    }
	}

      return true;
    }

    unsigned long long
    get_num_new_covered_tuples () const
    {
      return num_new_covered_tuples_;
    }

  private:
    const citcpp::detail::model &model_;
    const citcpp::detail::test &test_;
    const int current_param_selected_value_;
    unsigned long long num_new_covered_tuples_;
  };

  class ipog_horizontal_update_coverage_map_per_param_combo_functor_parallel
  {
  public:
    ipog_horizontal_update_coverage_map_per_param_combo_functor_parallel (
	const citcpp::detail::model &model, const citcpp::detail::test &test,
	const int current_param_selected_value,
	const citcpp::detail::coverage_map_parallel_iterator<true> &cov_map_it) :
	model_ (model), test_ (test), current_param_selected_value_ (
	    current_param_selected_value), num_new_covered_tuples_ (
	    cov_map_it.get_num_workers ()), cov_map_it_ (cov_map_it)
    {
    }

    bool
    operator() (citcpp::detail::coverage_map_iterator_state &cov_map_it)
    {
      using namespace citcpp::detail;

      const strength_vector<unsigned int> &param_indices =
	  cov_map_it.get_parameter_indices ();
      coverage_map_ipog::second_level_type &value_combinations =
	  cov_map_it.get_bitset ();

      if (!value_combinations.all ())
	{
	  // Here we compute an index into the bitset. To do so, we treat the number of values
	  // of each parameter as a kind of radix. Consider three parameters p_0, p_1, p_2.
	  // The last parameter is always the current one processed by IPOG.
	  // Now say that v_i is the number of values for p_i. If we now have values
	  // x_0, x_1, x_2, then the index is x_0 * v_1 * v_2 + x_1 * v_2 + x_2.
	  coverage_map_ipog::second_level_type::size_type index =
	      current_param_selected_value_;
	  for (std::vector<unsigned int>::size_type i = 0;
	      i < param_indices.size () - 1; ++i)
	    {
	      const unsigned int param_idx = param_indices[i];
	      const int param_value = test_.get_values ()[param_idx];

	      if (param_value < 0)
		{
		  // We have found a don't care value for that combination in
		  // the considered test in one of the [0, ... ,current_param_idx - 1]
		  // parameters. There is nothing to be updated concerning the coverage.
		  // This combination will be taken care of during the vertical extension step.
		  return true;
		}

	      coverage_map_ipog::second_level_type::size_type addend =
		  param_value;
	      for (std::vector<unsigned int>::size_type j = i + 1;
		  j < param_indices.size (); ++j)
		{
		  addend *= model_.get_parameters ()[param_indices[j]];
		}
	      index += addend;
	    }

	  if (!value_combinations.test_and_set (index))
	    {
	      ++num_new_covered_tuples_[cov_map_it_.get_worker_id ()].value;
	      // ++num_new_covered_tuples_;
	    }
	}

      return true;
    }

    unsigned long long
    get_num_new_covered_tuples () const
    {
      unsigned long long ret = 0;
      for (const auto &i : num_new_covered_tuples_)
	{
	  ret += i.value;
	}

      return ret;
    }

  private:
    const citcpp::detail::model &model_;
    const citcpp::detail::test &test_;
    const int current_param_selected_value_;
    citcpp::detail::thread_local_vector<aligned_ull_value> num_new_covered_tuples_;
    const citcpp::detail::coverage_map_parallel_iterator<true> &cov_map_it_;
  };

  unsigned long long
  ipog_horizontal_update_coverage_map (
      const citcpp::detail::model &model, const citcpp::detail::test &test,
      const int current_param_selected_value,
      citcpp::detail::coverage_map_iterator<true> &cov_map_it)
  {
    using namespace citcpp::detail;

    ipog_horizontal_update_coverage_map_per_param_combo_functor per_param_combo_functor (
	model, test, current_param_selected_value);
    cov_map_it.visit_all_parameter_combinations (per_param_combo_functor);

    return per_param_combo_functor.get_num_new_covered_tuples ();
  }

  unsigned long long
  ipog_horizontal_update_coverage_map (
      const citcpp::detail::model &model, const citcpp::detail::test &test,
      const int current_param_selected_value,
      citcpp::detail::coverage_map_parallel_iterator<true> &cov_map_it)
  {
    using namespace citcpp::detail;

    ipog_horizontal_update_coverage_map_per_param_combo_functor_parallel per_param_combo_functor (
	model, test, current_param_selected_value, cov_map_it);
    cov_map_it.visit_all_parameter_combinations (per_param_combo_functor);

    return per_param_combo_functor.get_num_new_covered_tuples ();
  }

  class ipog_vertical_extension_functor
  {
  public:
    ipog_vertical_extension_functor (
	const unsigned int current_param_idx,
	const citcpp::detail::model &model,
	citcpp::detail::test_set &test_set,
	citcpp::detail::ipog_horizontal_extension_result &partitioning_of_tests_according_to_current_values,
	const unsigned long long num_missing_combinations_to_cover) :
	current_param_idx_ (current_param_idx), model_ (model), test_set_ (
	    test_set), partitioning_of_tests_according_to_current_values_ (
	    partitioning_of_tests_according_to_current_values), num_missing_combinations_to_cover_ (
	    num_missing_combinations_to_cover), num_new_covered_tuples_ (0)
    {
    }

    bool
    operator() (citcpp::detail::coverage_map_iterator_state &cov_map_it)
    {
      using namespace citcpp::detail;

      ipog_vertical_extension_func (cov_map_it);

      return num_new_covered_tuples_ < num_missing_combinations_to_cover_;
    }

    unsigned long long
    get_num_new_covered_tuples () const
    {
      return num_new_covered_tuples_;
    }

  private:
    void
    ipog_vertical_extension_func (
	citcpp::detail::coverage_map_iterator_state &cov_map_it)
    {
      using namespace citcpp::detail;

      // First we check whether the value combination is covered, because if it is not,
      // then there is no point try to fit it into some test.
      coverage_map_ipog::second_level_type &value_combinations =
	  cov_map_it.get_bitset ();
      const strength_vector<unsigned int> &param_indices =
	  cov_map_it.get_parameter_indices ();
      const strength_vector<int> &value_indices =
	  cov_map_it.get_value_indices ();

      if (value_combinations.test_and_set (cov_map_it.get_bitpos ()))
	{
	  return;
	}

      ++num_new_covered_tuples_;

      // Now we iterate over all tests trying to fit the value combination.
      // However, we do not iterate over the entire test test, but instead
      // leverage upon its partition according to the value of the current
      // parameter as set by the horizontal extension for a test.
      // First we determine the value of the current parameter in the value
      // combination we want to cover.
      const int current_param_value_to_cover = value_indices.back ();

      // Iterate over the tests with the same value for the current parameter value
      // we have to cover.
      for (test &t : partitioning_of_tests_according_to_current_values_.value_to_row_mapping[current_param_value_to_cover])
	{
	  // We also skip tests which do not have at least one don't care
	  // value.
	  if (t.get_num_dont_care_values () == 0)
	    {
	      continue;
	    }

	  if (ipog_vertical_extension_try_inject_value_combo (param_indices,
							      value_indices, t))
	    {
	      // Return, since we have found a test and injected the value combination.
	      return;
	    }
	}

      // Now we iterate over the tests with a don't care value for the current parameter.
      for (auto it =
	  partitioning_of_tests_according_to_current_values_.rows_with_current_parameter_dont_care_value.begin ();
	  it
	      != partitioning_of_tests_according_to_current_values_.rows_with_current_parameter_dont_care_value.end ();
	  ++it)
	{
	  test &t = *it;

	  if (ipog_vertical_extension_try_inject_value_combo (param_indices,
							      value_indices, t))
	    {
	      // Since we have successfully injected the combination, the test must be
	      // moved to a different partition for looking it up when trying
	      // to inject other combinations with the same value for the current
	      // parameter.
	      partitioning_of_tests_according_to_current_values_.rows_with_current_parameter_dont_care_value.erase (
		  it);
	      partitioning_of_tests_according_to_current_values_.value_to_row_mapping[current_param_value_to_cover].push_back (
		  t);

	      // Return, since we have found a test and injected the value combination.
	      return;
	    }
	}

      // If we have reached this point, then we did not find a matching test.
      // Thus, we have to add a new one with the value combination.
      // Initialize all values of the test with don't care.
      test t (model_.get_parameters ().size (), -1);
      t.set_num_dont_care_values (
	  (current_param_idx_ + 1) - param_indices.size ());

      for (unsigned int i = 0; i < param_indices.size (); ++i)
	{
	  const unsigned int param_idx = param_indices[i];
	  const int param_value_to_cover = value_indices[i];
	  t.get_values ()[param_idx] = param_value_to_cover;
	}

      test_set_.get_list_of_tests ().push_back (std::move (t));

      // Update the mapping from values of the current parameter to the tests.
      partitioning_of_tests_according_to_current_values_.value_to_row_mapping[current_param_value_to_cover].push_back (
	  test_set_.get_list_of_tests ().back ());
    }

    bool
    ipog_vertical_extension_try_inject_value_combo (
	const citcpp::detail::strength_vector<unsigned int> &param_indices,
	const citcpp::detail::strength_vector<int> &value_indices,
	citcpp::detail::test &t)
    {
      int overwritten_dont_cares = 0;
      for (unsigned int i = 0; i < param_indices.size (); ++i)
	{
	  const unsigned int param_idx = param_indices[i];
	  const int param_value_to_cover = value_indices[i];
	  const int param_value_in_test = t.get_values ()[param_idx];

	  if (param_value_in_test >= 0
	      && param_value_to_cover != param_value_in_test)
	    {
	      // Cannot inject value combination in this test, moving on to the next
	      // one.
	      return false;
	    }

	  if (param_value_in_test < 0)
	    {
	      ++overwritten_dont_cares;
	    }
	}

      for (unsigned int i = 0; i < param_indices.size (); ++i)
	{
	  const unsigned int param_idx = param_indices[i];
	  const int param_value_to_cover = value_indices[i];
	  t.get_values ()[param_idx] = param_value_to_cover;
	}

      t.set_num_dont_care_values (
	  t.get_num_dont_care_values () - overwritten_dont_cares);

      return true;
    }

  private:
    const unsigned int current_param_idx_;
    const citcpp::detail::model &model_;
    citcpp::detail::test_set &test_set_;
    citcpp::detail::ipog_horizontal_extension_result &partitioning_of_tests_according_to_current_values_;
    const unsigned long long num_missing_combinations_to_cover_;
    unsigned long long num_new_covered_tuples_;
  };
}

namespace citcpp
{
  namespace detail
  {
    create_all_value_combinations_result
    create_all_value_combinations (
	unsigned int strength, const model &model,
	const std::vector<unsigned int> &parameter_index_map,
	citcpp::detail::test_set &test_set)
    {
      auto l_map_to_param_idx = [&parameter_index_map]
      (int idx)
	{ return parameter_index_map[idx];};

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
	  [&model, &parameter_index_map, &test_set, &result]
	  (const std::vector<unsigned int> &next_cross_product_elem)
	    {
	      // Initialize all values of the test with don't care.
	      test t(model.get_parameters().size(), -1);
	      t.set_num_dont_care_values(0);

	      // Replace the first t elements with the cross product element.
	      for (unsigned int index = 0; index < next_cross_product_elem.size(); ++index)
		{
		  t.get_values()[parameter_index_map[index]] = next_cross_product_elem[index];
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
	test_set &test_set, coverage_map_ipog &cov_map)
    {
      const unsigned int real_current_param_idx =
	  parameter_index_map[current_param_idx];
      const int num_current_param_values =
	  model.get_parameters ()[real_current_param_idx];

      // First initialize the result object.
      ipog_horizontal_extension_result result
	{ std::vector<list_intrusive<test>> (num_current_param_values),
	    list_intrusive<test> (), 0 };

      unsigned int last_picked_value = 0;
      std::vector<unsigned int> value_to_num_picked (num_current_param_values);
      coverage_map_iterator<true> cov_map_it = cov_map.create_iterator ();

      for (test &t : test_set.get_list_of_tests ())
	{
	  if (strength > 2)
	    {
	      last_picked_value = num_current_param_values - 1;
	    }

	  int selected_value = ipog_horizontal_select_best_value (
	      num_current_param_values, model, t, cov_map_it, last_picked_value,
	      value_to_num_picked);

	  if (selected_value >= 0)
	    {
	      // We might not have selected any value. This can happen, if no matter
	      // which value we would pick, the coverage gain would be 0.
	      // If so, our best option is to keep it as don't care, in order for
	      // later vertical extension steps to exploit that don't care value.
	      // If we have selected a value however with most coverage, then we set it in the
	      // test accordingly.
	      t.get_values ()[real_current_param_idx] = selected_value;
	    }

	  // The last step consists in updating the coverage map.
	  unsigned long long num_new_covered_tuples =
	      selected_value >= 0 ?
		  ipog_horizontal_update_coverage_map (model, t, selected_value,
						       cov_map_it) :
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

    ipog_horizontal_extension_result
    ipog_horizontal_extension (
	const unsigned int current_param_idx, const unsigned int strength,
	const model &model,
	const std::vector<unsigned int> &parameter_index_map,
	const unsigned long long num_missing_combinations_to_cover,
	test_set &test_set, coverage_map_ipog &cov_map, thread_pool &tp)
    {
      const unsigned int real_current_param_idx =
	  parameter_index_map[current_param_idx];
      const int num_current_param_values =
	  model.get_parameters ()[real_current_param_idx];

      // First initialize the result object.
      ipog_horizontal_extension_result result
	{ std::vector<list_intrusive<test>> (num_current_param_values),
	    list_intrusive<test> (), 0 };

      unsigned int last_picked_value = 0;
      std::vector<unsigned int> value_to_num_picked (num_current_param_values);
      coverage_map_parallel_iterator<true> cov_map_it =
	  cov_map.create_parallel_iterator (tp);

      for (test &t : test_set.get_list_of_tests ())
	{
	  if (strength > 2)
	    {
	      last_picked_value = num_current_param_values - 1;
	    }

	  int selected_value = ipog_horizontal_select_best_value (
	      num_current_param_values, model, t, cov_map_it, last_picked_value,
	      value_to_num_picked);

	  if (selected_value >= 0)
	    {
	      // We might not have selected any value. This can happen, if no matter
	      // which value we would pick, the coverage gain would be 0.
	      // If so, our best option is to keep it as don't care, in order for
	      // later vertical extension steps to exploit that don't care value.
	      // If we have selected a value however with most coverage, then we set it in the
	      // test accordingly.
	      t.get_values ()[real_current_param_idx] = selected_value;
	    }

	  // The last step consists in updating the coverage map.
	  unsigned long long num_new_covered_tuples =
	      selected_value >= 0 ?
		  ipog_horizontal_update_coverage_map (model, t, selected_value,
						       cov_map_it) :
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
	const model &model,
	const unsigned long long num_missing_combinations_to_cover,
	ipog_horizontal_extension_result &partitioning_of_tests_according_to_current_values,
	test_set &test_set, coverage_map_ipog &cov_map)
    {
      // First initialize the result object.
      ipog_vertical_extension_result result =
	{ 0 };

      coverage_map_iterator<true> cov_map_it = cov_map.create_iterator ();
      ipog_vertical_extension_functor functor (
	  current_param_idx, model, test_set,
	  partitioning_of_tests_according_to_current_values,
	  num_missing_combinations_to_cover);

      cov_map_it.visit_all_tuples (true, functor);

      result.num_new_covered_tuples = functor.get_num_new_covered_tuples ();

      return result;
    }
  }
}
