#ifndef MODEL_HPP_
#define MODEL_HPP_

#include <vector>
#include "../input_model.hpp"
#include "../testset.hpp"
#include "internal_testset.hpp"

namespace citcpp
{
  namespace detail
  {
    /**
     * This is an internal representation of a given InputModel.
     */
    class Model
    {
    public:
      /**
       * Constructs an internal representation of the given InputModel.
       */
      Model (const InputModel &input_model);

      /**
       * Returns the input which this internal model has been created from.
       */
      const InputModel&
      getInputModel () const;

      /**
       * Returns the list of parameters of this model.
       */
      const std::vector<unsigned int>&
      getParameters () const;

      /**
       * Constructs and returns a TestSet based on the given internal test set
       * representation.
       */
      ::citcpp::TestSet
      createFromInternalTestSet (
	  const ::citcpp::detail::TestSet &testset) const;

    private:
      void
      convertTest (const std::vector<int> &src,
		   std::vector<ParameterValue> &tgt) const;

    private:
      const InputModel &input_model_;
      std::vector<unsigned int> parameters_;
    };
  }
}

#endif /* MODEL_HPP_ */
