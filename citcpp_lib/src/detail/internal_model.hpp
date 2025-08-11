#ifndef MODEL_HPP_
#define MODEL_HPP_

#include <citcpp/input_model.hpp>
#include <citcpp/test_set.hpp>
#include <vector>

#include "internal_test_set.hpp"

namespace citcpp {
namespace detail {

/**
 * This is an internal representation of a given input model.
 */
class model {
  public:
    /**
     * Constructs an internal representation of the given input model.
     * The given list of parameter allows to specify a desired different
     * ordering of the parameters in the internal model.
     */
    model(const input_model &input_model,
          const std::vector<parameter> &ordered_parameters);

    /**
     * Returns the input which this internal model has been created from.
     */
    const input_model &get_input_model() const;

    /**
     * Returns the list of parameters of this model.
     */
    const std::vector<unsigned int> &get_parameters() const {
      return parameters_;
    }

    /**
     * Constructs and returns a test set based on the given internal test set
     * representation.
     */
    citcpp::test_set create_from_internal_test_set(
        const citcpp::detail::test_set &test_set) const;

    /**
     * Constructs and returns a test set based on the given internal test set
     * representation.
     */
    citcpp::test_set create_from_internal_test_set(
        const citcpp::detail::test_set &test_set,
        std::string_view value_separator) const;

  private:
    void convert_test_set(const citcpp::detail::test_set &src,
                          citcpp::test_set &tgt) const;
    void convert_test(const test &src, std::vector<parameter_value> &tgt) const;

  private:
    const input_model &input_model_;
    const std::vector<unsigned int> parameter_index_map_;
    std::vector<unsigned int> parameters_;
};

/**
 * This is an internal representation of a relation, i.e.
 * a set of parameters and a corresponding interaction strength.
 */
class relation {
  public:
    relation(const std::vector<unsigned int> &parameters,
             unsigned int specified_interaction_strength);

    const std::vector<unsigned int> &get_parameters() const {
      return parameters_;
    }

    unsigned int get_specified__interaction_strength() const {
      return specified_interaction_strength_;
    }

    unsigned int get_current_interaction_strength() const {
      return current_interaction_strength_;
    }

    void set_current_interaction_strength(unsigned int interaction_strength) {
      current_interaction_strength_ = interaction_strength;
    }

  private:
    const std::vector<unsigned int> parameters_;
    const unsigned int specified_interaction_strength_;
    unsigned int current_interaction_strength_;
};

}  // namespace detail
}  // namespace citcpp

#endif /* MODEL_HPP_ */
