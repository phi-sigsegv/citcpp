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
	  int v = src.at (p);

	  const Parameter &param = m_input_model.getParameters ().at (p);
	  tgt.push_back (param.getValues ().at (v));
	}
    }
  }
}
