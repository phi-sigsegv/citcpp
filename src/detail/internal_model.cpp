#include "internal_model.hpp"

namespace citcpp
{
  namespace detail
  {
    Model::Model (const InputModel &input_model) :
	m_input_model (input_model), m_parameters ()
    {
      for (const Parameter &p : input_model.getParameters ())
	{
	  m_parameters.push_back (p.getValues ().size ());
	}
    }

    const InputModel&
    Model::getInputModel () const
    {
      return m_input_model;
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
	  std::vector<ParameterValue>::size_type pv = src.at (p);

	  const Parameter &param = m_input_model.getParameters ().at (p);
	  if (pv >= 0 && pv < param.getValues ().size ())
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
