#ifndef DETAIL_CITCPP_IPOG_HPP_
#define DETAIL_CITCPP_IPOG_HPP_

#include <thread>
#include "citcpp_algo_if.hpp"
#include "exec_handle_impl.hpp"
#include "internal_model.hpp"
#include "set_of_covered_pv_combinations.hpp"
#include "citcpp_algo_common.hpp"
#include "index_combinator.hpp"

namespace citcpp
{
  namespace detail
  {
    /**
     * This class provides an implementation of the IPOG algorithm.
     */
    class citcpp_ipog : public citcpp_algo_if
    {
    public:
      citcpp_ipog (const input_model &input_model) :
	  citcpp_algo_if (), input_model_ (input_model), model_ (input_model_), strength_ (
	      1), test_set_ ()
      {
      }

      citcpp_ipog (input_model &&input_model) :
	  citcpp_algo_if (), input_model_ (std::move (input_model)), model_ (
	      input_model_), strength_ (1), test_set_ ()
      {
      }

      /**
       * Too lazy to implement/ensuring that it is well-defined.
       */
      citcpp_ipog (citcpp_ipog&&) = delete;
      citcpp_ipog (const citcpp_ipog&) = delete;

      /**
       * Too lazy to implement/ensuring that it is well-defined.
       */
      citcpp_ipog&
      operator= (citcpp_ipog&&) = delete;
      citcpp_ipog&
      operator= (const citcpp_ipog&) = delete;

      void
      set_interaction_strength (unsigned int t)
      {
	strength_ = t;
      }

      /**
       * This is the entry point to be called by a thread.
       */
      void
      entry_point (exec_handle_impl &exec_handle)
      {
	// First we compute the number of combination we have to cover.
	unsigned long long number_of_combination_to_cover =
	    number_of_combinations_to_cover (model_, strength_);
	exec_handle.set_number_of_combinations_to_cover (
	    number_of_combination_to_cover);

	::citcpp::test_set ts (
	    model_.create_from_internal_test_set (test_set_));

	// Set the generated test set.
	// This will also signal to the client that we are done.
	exec_handle.set_test_set (std::move (ts));
      }

    private:
      /**
       * Function to mark all t-way combinations covered by a given test case
       */
      void
      update_covered_combinations (
	  const std::vector<int> &test_case,
	  set_of_covered_pv_combinations covered_combinations)
      {
	citcpp::detail::index_combinator param_index_combinator (
	    model_.get_parameters ().size (), strength_, 1);

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

    private:
      const citcpp::input_model input_model_;
      const model model_;
      unsigned int strength_;
      test_set test_set_;
    };
  }
}

#endif /* DETAIL_CITCPP_IPOG_HPP_ */
