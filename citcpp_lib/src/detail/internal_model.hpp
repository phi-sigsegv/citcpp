#ifndef MODEL_HPP_
#define MODEL_HPP_

#include <vector>
#include <citcpp/input_model.hpp>
#include <citcpp/test_set.hpp>
#include "internal_test_set.hpp"

namespace citcpp
{
  namespace detail
  {
    /**
     * This is an internal representation of a given input model.
     */
    class model
    {
    public:
      /**
       * Constructs an internal representation of the given input model.
       * The given list of parameter allows to specify a desired different
       * ordering of the parameters in the internal model.
       */
      model (const input_model &input_model,
	     const std::vector<parameter> &ordered_parameters);

      /**
       * Returns the input which this internal model has been created from.
       */
      const input_model&
      get_input_model () const;

      /**
       * Returns the list of parameters of this model.
       */
      const std::vector<unsigned int>&
      get_parameters () const;

      /**
       * Constructs and returns a test set based on the given internal test set
       * representation.
       */
      citcpp::test_set
      create_from_internal_test_set (
	  const citcpp::detail::test_set &test_set) const;

    private:
      void
      convert_test (const test &src, std::vector<parameter_value> &tgt) const;

    private:
      const input_model &input_model_;
      const std::vector<unsigned int> parameter_index_map_;
      std::vector<unsigned int> parameters_;
    };
  }
}

#endif /* MODEL_HPP_ */
