#include "internal_model.hpp"

namespace citcpp
{
  namespace detail
  {
    Model::Model (const InputModel &input_model) :
	input_model_ (input_model), parameters_ ()
    {
      for (const Parameter &p : input_model.getParameters ())
	{
	  parameters_.push_back (p.getValues ().size ());
	}
    }

    const InputModel&
    Model::getInputModel () const
    {
      return input_model_;
    }

    const std::vector<unsigned int>&
    Model::getParameters () const
    {
      return parameters_;
    }

    ::citcpp::TestSet
    Model::createFromInternalTestSet (
	const ::citcpp::detail::TestSet &testset) const
    {
      ::citcpp::TestSet ret;

      for (const std::vector<int> &test : testset.getListOfTests ())
	{
	  ret.getListOfTests ().emplace_back ();
	  convertTest (test, ret.getListOfTests ().back ());
	}

      return ret;
    }

    void
    Model::convertTest (const std::vector<int> &src,
			std::vector<ParameterValue> &tgt) const
    {
      for (std::vector<int>::size_type p = 0; p < src.size (); ++p)
	{
	  int pv = src.at (p);

	  const Parameter &param = input_model_.getParameters ().at (p);
	  if (pv >= 0
	      && (std::vector<ParameterValue>::size_type) pv
		  < param.getValues ().size ())
	    {
	      tgt.push_back (param.getValues ().at (pv));
	    }
	  else
	    {
	      // TODO: Add invalid value here....
	      tgt.push_back (param.getValues ().at (pv));
	    }
	}
    }
  }
}
