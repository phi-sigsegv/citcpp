#include <algorithm>
#include <chrono>
#include "citcpp_ipog.hpp"
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

  /**
   * Function to mark all t-way combinations covered by a given test case
   */
  void
  update_covered_combinations (
      const citcpp::detail::model &model, unsigned int strength,
      const std::vector<int> &test_case,
      citcpp::detail::set_of_covered_pv_combinations covered_combinations)
  {
    citcpp::detail::index_combinator param_index_combinator (
	model.get_parameters ().size (), strength, 1);

    while (param_index_combinator.has_next (0))
      {
	const std::vector<unsigned int> &param_index_combination =
	    param_index_combinator.next (0);

	std::vector<unsigned int> combo_values;
	bool valid_combo = true;
	for (unsigned int param_index : param_index_combination)
	  {
	    if (test_case[param_index] == -1)
	      { // If any value is unassigned, this combination isn't fully formed yet
		valid_combo = false;
		break;
	      }
	    combo_values.push_back (test_case[param_index]);
	  }
	if (valid_combo)
	  {
	    covered_combinations.insert (
	      { param_index_combination, combo_values });
	  }
      }
  }

  void
  main_ipog_loop (const citcpp::detail::model &model, unsigned int strength,
		  citcpp::detail::test_set &test_set,
		  citcpp::detail::exec_handle_impl &exec_handle)
  {
    using namespace citcpp::detail;

    binom_coeff_table binomial_coeffs (model.get_parameters ().size ());
    std::vector<unsigned int> parameter_index_map (
	model.get_parameters ().size ());
    std::iota (parameter_index_map.begin (), parameter_index_map.end (), 0);

    for (unsigned int current_param_idx = strength;
	current_param_idx < model.get_parameters ().size ();
	++current_param_idx)
      {
      }
  }
}

namespace citcpp
{
  namespace detail
  {
    citcpp_ipog::citcpp_ipog (const input_model &input_model) :
	citcpp_algo_if (), input_model_ (input_model), model_ (
	    input_model_,
	    get_parameters_sorted_by_number_of_values_desc (
		input_model_.get_parameters ())), strength_ (1), test_set_ ()
    {
    }

    citcpp_ipog::citcpp_ipog (input_model &&input_model) :
	citcpp_algo_if (), input_model_ (std::move (input_model)), model_ (
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
    citcpp_ipog::entry_point (exec_handle_impl &exec_handle)
    {
      tf::Executor executor;

      const auto t_start = std::chrono::high_resolution_clock::now ();

      // First we compute the number of combination we have to cover.
      unsigned long long number_of_combination_to_cover =
	  number_of_combinations_to_cover (executor, model_, strength_);
      exec_handle.set_number_of_combinations_to_cover (
	  number_of_combination_to_cover);

      std::vector<unsigned int> parameter_index_map (
	  model_.get_parameters ().size ());
      std::iota (parameter_index_map.begin (), parameter_index_map.end (), 0);
      auto initial_step_res = create_all_value_combinations (
	  strength_, model_, parameter_index_map, test_set_);
      exec_handle.add_number_of_covered_combinations (
	  initial_step_res.num_created_combinations);

      main_ipog_loop (model_, strength_, test_set_, exec_handle);

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
