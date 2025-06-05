#include "internal_model.hpp"

namespace citcpp
{
  namespace detail
  {
    const parameter_value DONT_CARE_PARAMETER_VALUE
      { "*" };

    model::model (const input_model &input_model) :
	input_model_ (input_model), parameters_ ()
    {
      for (const parameter &p : input_model.get_parameters ())
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

      for (const std::vector<int> &test : test_set.get_list_of_tests ())
	{
	  ret.get_list_of_tests ().emplace_back ();
	  convert_test (test, ret.get_list_of_tests ().back ());
	}

      return ret;
    }

    void
    model::convert_test (const std::vector<int> &src,
			 std::vector<parameter_value> &tgt) const
    {
      for (std::vector<int>::size_type p = 0; p < src.size (); ++p)
	{
	  int pv = src.at (p);

	  const parameter &param = input_model_.get_parameters ().at (p);
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
