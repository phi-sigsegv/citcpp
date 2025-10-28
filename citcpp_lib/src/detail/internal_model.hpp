#ifndef MODEL_HPP_
#define MODEL_HPP_

#include <algorithm>
#include <citcpp/model.hpp>
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
    internal_model(const model& input_model);

    /**
     * Returns the input which this internal model has been created from.
     */
    const model& get_input_model() const;

    /**
     * Returns the list of number of values for each of the parameters of the
     * model. The parameter is identified by its index in the input model.
     */
    const std::vector<unsigned int>& get_parameter_num_values() const {
      return parameters_;
    }

    /**
     * Constructs and returns a test set based on the given internal test set
     * representation.
     */
    test_set create_from_internal_test_set(
        const internal_test_set& test_set) const;

    /**
     * Constructs and returns a test set based on the given internal test set
     * representation.
     */
    test_set create_from_internal_test_set(
        const internal_test_set& test_set,
        std::string_view value_separator) const;

  private:
    void convert_test_set(const internal_test_set& src, test_set& tgt) const;
    void convert_test(const test& src, std::vector<parameter_value>& tgt) const;

  private:
    const model& input_model_;
    std::vector<unsigned int> parameters_;
};

/**
 * This is an internal representation of a relation, i.e.
 * a set of parameters and a corresponding interaction strength.
 */
class internal_relation {
  public:
    internal_relation(const std::vector<unsigned int>& parameter_index_map,
                      unsigned int specified_interaction_strength);

    internal_relation(std::vector<unsigned int>&& parameter_index_map,
                      unsigned int specified_interaction_strength);

    const std::vector<unsigned int>& get_parameter_index_map() const {
      return parameter_index_map_;
    }

    unsigned int get_specified_interaction_strength() const {
      return specified_interaction_strength_;
    }

    unsigned int get_current_interaction_strength() const {
      return std::min(get_specified_interaction_strength(),
                      get_current_param_idx() + 1);
    }

    unsigned int get_current_param_idx() const { return current_param_idx_; }

    void set_current_param_idx(unsigned int current_param_idx) {
      current_param_idx_ = current_param_idx;
    }

  private:
    std::vector<unsigned int> parameter_index_map_;
    unsigned int specified_interaction_strength_;
    unsigned int current_param_idx_;
};

}  // namespace detail
}  // namespace citcpp

#endif /* MODEL_HPP_ */
