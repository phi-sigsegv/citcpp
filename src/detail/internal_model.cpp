#include "internal_model.hpp"

namespace
{
  std::vector<unsigned int>
  compute_parameter_index_map (
      const citcpp::input_model &input_model,
      const std::vector<citcpp::parameter> &parameter_order)
  {
    std::vector<unsigned int> parameter_index_map;

    for (unsigned int internal_param_idx = 0;
	internal_param_idx < parameter_order.size (); ++internal_param_idx)
      {
	const citcpp::parameter &param = parameter_order[internal_param_idx];
	// Find the parameter in the user input model, in particular its index.
	for (unsigned int user_param_index = 0;
	    user_param_index < input_model.get_parameters ().size ();
	    ++user_param_index)
	  {
	    if (input_model.get_parameters ()[user_param_index] == param)
	      {
		parameter_index_map.push_back (user_param_index);
		break;
	      }
	  }
      }

    return parameter_index_map;
  }
}

namespace citcpp
{
  namespace detail
  {
    const parameter_value DONT_CARE_PARAMETER_VALUE
      { "*" };

    model::model (const input_model &input_model,
		  const std::vector<parameter> &ordered_parameters) :
	input_model_ (input_model), parameter_index_map_ (
	    compute_parameter_index_map (input_model, ordered_parameters)), parameters_ ()
    {
      for (const parameter &p : ordered_parameters)
	{
	  parameters_.push_back (p.get_values ().size ());
	}
    }

    const input_model&
    model::get_input_model () const
    {
      return input_model_;
    }

    const std::vector<unsigned int>&
    model::get_parameters () const
    {
      return parameters_;
    }

    ::citcpp::test_set
    model::create_from_internal_test_set (
	const ::citcpp::detail::test_set &test_set) const
    {
      ::citcpp::test_set ret;

      for (const test &test : test_set.get_list_of_tests ())
	{
	  ret.get_list_of_tests ().emplace_back ();
	  convert_test (test, ret.get_list_of_tests ().back ());
	}

      return ret;
    }

    void
    model::convert_test (const test &src,
			 std::vector<parameter_value> &tgt) const
    {
      for (test::size_type p = 0; p < src.get_values ().size (); ++p)
	{
	  int pv = src.get_values ().at (p);
	  unsigned param_index_in_model = parameter_index_map_[p];

	  const parameter &param = input_model_.get_parameters ().at (
	      param_index_in_model);
	  if (pv >= 0
	      && (std::vector<parameter_value>::size_type) pv
		  < param.get_values ().size ())
	    {
	      tgt.push_back (param.get_values ().at (pv));
	    }
	  else
	    {
	      tgt.push_back (DONT_CARE_PARAMETER_VALUE);
	    }
	}
    }
  }
}
