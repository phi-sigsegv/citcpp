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
class internal_model {
  public:
    /**
     * Constructs an internal representation of the given input model.
     */
    internal_model(const model &input_model);

    /**
     * Returns the input which this internal model has been created from.
     */
    const model &get_input_model() const;

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
    test_set create_from_internal_test_set(
        const internal_test_set &test_set) const;

    /**
     * Constructs and returns a test set based on the given internal test set
     * representation.
     */
    test_set create_from_internal_test_set(
        const internal_test_set &test_set,
        std::string_view value_separator) const;

  private:
    void convert_test_set(const internal_test_set &src, test_set &tgt) const;
    void convert_test(const test &src, std::vector<parameter_value> &tgt) const;

  private:
    const model &input_model_;
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
