#include <algorithm>
#include <chrono>
#include <taskflow/taskflow.hpp>
#include "citcpp_ipog.hpp"
#include "exec_handle_ipog_impl.hpp"
#include "citcpp_algo_common.hpp"
#include "for_each_cross_product_elem.hpp"
#include "coverage_map.hpp"
#include "binom_coeff_table.hpp"
#include "ipog_algorithm_uniform_strength.hpp"

namespace
{
  std::vector<citcpp::parameter>
  get_parameters_sorted_by_number_of_values_desc (
      const std::vector<citcpp::parameter> &params)
  {
    using namespace citcpp;

    std::vector<parameter> sorted_params (params);

    std::sort (sorted_params.begin (), sorted_params.end (), []
    (const parameter &p1, const parameter &p2)
      { return p1.get_values ().size () > p2.get_values ().size ();});

    return sorted_params;
  }

  void
  main_ipog_loop (const citcpp::detail::model &model, unsigned int strength,
		  citcpp::detail::test_set &test_set,
		  citcpp::detail::exec_handle_ipog_impl &exec_handle)
  {
    using namespace citcpp::detail;

    tf::Executor executor;

    // First we compute the number of combination we have to cover.
    unsigned long long number_combos_to_cover =
	number_of_combinations_to_cover (executor, model, strength);
    exec_handle.set_number_of_combinations_to_cover (number_combos_to_cover);

    if (exec_handle.is_job_aborted ())
      {
	return;
      }

      {
	// Step 1: Initialize for the first t parameters.
	std::vector<unsigned int> parameter_index_map (
	    model.get_parameters ().size ());
	std::iota (parameter_index_map.begin (), parameter_index_map.end (), 0);
	auto initial_step_res = create_all_value_combinations (
	    strength, model, parameter_index_map, test_set);
	exec_handle.add_number_of_covered_combinations (
	    initial_step_res.num_created_combinations);
	exec_handle.set_number_of_processed_parameters (strength);
      }

      {
	// Here is the main IPOG loop.
	const binom_coeff_table binomial_coeffs (
	    model.get_parameters ().size ());
	std::vector<unsigned int> parameter_index_map (
	    model.get_parameters ().size ());
	std::iota (parameter_index_map.begin (), parameter_index_map.end (), 0);

	for (unsigned int current_param_idx = strength;
	    current_param_idx < model.get_parameters ().size ();
	    ++current_param_idx)
	  {
	    if (exec_handle.is_job_aborted ())
	      {
		return;
	      }

	    if (model.get_parameters ()[parameter_index_map[current_param_idx]]
		<= 1)
	      {
		// If the current parameter only has only value, then
		// we can treat this situation much simpler: We just have
		// to add that particular value to each test and update
		// the number of covered combinations.
		for (test &t : test_set.get_list_of_tests ())
		  {
		    t.get_values ()[parameter_index_map[current_param_idx]] = 0;
		  }

		unsigned long long number_combos_to_cover =
		    number_of_combinations_to_cover (executor,
						     current_param_idx, model,
						     parameter_index_map,
						     strength - 1);
		exec_handle.add_number_of_covered_combinations (
		    number_combos_to_cover);
	      }
	    else
	      {
		coverage_map cov_map (current_param_idx, strength - 1,
				      binomial_coeffs);

		unsigned long long number_combos_to_cover = ipog_loop_init (
		    current_param_idx, strength, model, parameter_index_map,
		    binomial_coeffs, cov_map, nullptr);

		auto horizontal_ext_res = ipog_horizontal_extension (
		    current_param_idx, strength, model, parameter_index_map,
		    number_combos_to_cover, binomial_coeffs, test_set, cov_map,
		    nullptr);

		number_combos_to_cover -=
		    horizontal_ext_res.num_new_covered_tuples;
		exec_handle.add_number_of_covered_combinations (
		    horizontal_ext_res.num_new_covered_tuples);

		if (exec_handle.is_job_aborted ())
		  {
		    exec_handle.set_number_of_processed_parameters (
			current_param_idx + 1);
		    return;
		  }

		if (number_combos_to_cover > 0)
		  {
		    auto vertical_ext_res = ipog_vertical_extension (
			current_param_idx, strength, model, parameter_index_map,
			number_combos_to_cover, binomial_coeffs,
			horizontal_ext_res.value_to_row_mapping, test_set,
			cov_map);

		    number_combos_to_cover -=
			vertical_ext_res.num_new_covered_tuples;
		    exec_handle.add_number_of_covered_combinations (
			vertical_ext_res.num_new_covered_tuples);
		  }
	      }

	    exec_handle.set_number_of_processed_parameters (
		current_param_idx + 1);
	  }
      }
  }

  void
  replace_dont_care_values (citcpp::detail::test_set &test_set,
			    const citcpp::detail::model &model)
  {
    using namespace citcpp::detail;

    for (test &t : test_set.get_list_of_tests ())
      {
	for (unsigned int i = 0; i < t.get_values ().size (); ++i)
	  {
	    int &value = t.get_values ()[i];
	    if (value < 0)
	      {
		// Found don't care value. We simply replace it with the
		// first value of the respective parameter.
		value = 0;
	      }
	  }
      }
  }
}

namespace citcpp
{
  namespace detail
  {
    citcpp_ipog::citcpp_ipog (const input_model &input_model,
			      const covering_array_computation_config &config) :
	config_ (config), input_model_ (input_model), model_ (
	    input_model_,
	    get_parameters_sorted_by_number_of_values_desc (
		input_model_.get_parameters ())), strength_ (1), test_set_ ()
    {
    }

    citcpp_ipog::citcpp_ipog (input_model &&input_model,
			      const covering_array_computation_config &config) :
	config_ (config), input_model_ (std::move (input_model)), model_ (
	    input_model_,
	    get_parameters_sorted_by_number_of_values_desc (
		input_model_.get_parameters ())), strength_ (1), test_set_ ()
    {
    }

    void
    citcpp_ipog::set_interaction_strength (unsigned int t)
    {
      strength_ = t;
    }

    void
    citcpp_ipog::entry_point (exec_handle_ipog_impl &exec_handle)
    {
      const auto t_start = std::chrono::high_resolution_clock::now ();

      main_ipog_loop (model_, strength_, test_set_, exec_handle);
      if (config_.replace_dont_care_values ())
	{
	  replace_dont_care_values (test_set_, model_);
	}
      ::citcpp::test_set ts (model_.create_from_internal_test_set (test_set_));

      const auto t_end = std::chrono::high_resolution_clock::now ();
      const auto duration_in_milli_seconds = duration_cast<
	  std::chrono::milliseconds> (t_end - t_start);
      exec_handle.set_duration_in_milli_seconds (
	  duration_in_milli_seconds.count ());

      // Set the generated test set.
      // This will also signal to the client that we are done.
      exec_handle.set_test_set (std::move (ts));
    }
  }
}
