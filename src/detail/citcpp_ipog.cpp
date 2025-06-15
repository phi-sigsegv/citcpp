#include <algorithm>
#include "citcpp_ipog.hpp"
#include "for_each_cross_product_elem.hpp"

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
  initial_step_creating_all_value_combinations (
      const citcpp::detail::model &model, unsigned int t,
      citcpp::detail::test_set &test_set)
  {
    using namespace citcpp::detail;

    std::vector<unsigned int> first_strength_params (
	model.get_parameters ().begin (), model.get_parameters ().begin () + t);

    for_each_cross_product_elem (
	first_strength_params,
	[&model, &test_set]
	(const std::vector<unsigned int> &next_cross_product_elem)
	  {
	    // Initialize all values of the test with don't care.
	    test t(model.get_parameters().size(), -1);
	    t.set_num_dont_care_values(t.get_values().size());

	    // Replace the first t elements with the cross product element.
	    std::copy(next_cross_product_elem.begin(),next_cross_product_elem.end(), t.get_values().begin());
	    t.set_num_dont_care_values(t.get_num_dont_care_values() - next_cross_product_elem.size());

	    test_set.get_list_of_tests().push_back(std::move(t));
	  });
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

      // First we compute the number of combination we have to cover.
      unsigned long long number_of_combination_to_cover =
	  number_of_combinations_to_cover (executor, model_, strength_);
      exec_handle.set_number_of_combinations_to_cover (
	  number_of_combination_to_cover);

      initial_step_creating_all_value_combinations (model_, strength_,
						    test_set_);

      exec_handle.add_number_of_covered_combinations (
	  test_set_.get_list_of_tests ().size ());

      ::citcpp::test_set ts (model_.create_from_internal_test_set (test_set_));

      // Set the generated test set.
      // This will also signal to the client that we are done.
      exec_handle.set_test_set (std::move (ts));
    }
  }
}
