#ifndef DETAIL_CONSTRAINT_EVALUATOR_HPP_
#define DETAIL_CONSTRAINT_EVALUATOR_HPP_

#include <citcpp/constraints.hpp>
#include <citcpp/model.hpp>
#include <citcpp/test_set.hpp>
#include <unordered_map>

namespace citcpp {
namespace detail {

struct parameter_reference_hash {
    std::size_t operator()(const parameter_reference& param) const noexcept {
      return std::hash<std::string>{}(param.get_name());
    }
};

/**
 * This class provides means to evaluate satisfaction of constraints
 * by a given test.
 */
class constraint_evaluator {
  public:
    constraint_evaluator(const std::vector<parameter_def>& parameters) {
      int idx = 0;
      for (const auto& param : parameters) {
        param_to_index_.emplace(param.get_name(), idx);
        ++idx;
      }
    }

    constraint_evaluator(const std::vector<parameter>& parameters) {
      int idx = 0;
      for (const auto& param : parameters) {
        param_to_index_.emplace(param, idx);
        ++idx;
      }
    }

    /**
     * Evaluates whether the given test fulfills the given constraint.
     */
    bool operator()(const std::vector<parameter_value>& test,
                    const constraint& constr) const;

    /**
     * Evaluates whether the given test fulfills all the given constraints.
     */
    bool operator()(
        const std::vector<parameter_value>& test,
        const std::vector<std::unique_ptr<constraint>>& constraints) const;

    /**
     * Evaluates whether all tests in a given testset fulfill all the given
     * constraints.
     */
    bool operator()(
        const test_set& tests,
        const std::vector<std::unique_ptr<constraint>>& constraints) const;

  private:
    std::unordered_map<parameter_reference, int, parameter_reference_hash>
        param_to_index_;
};

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_CONSTRAINT_EVALUATOR_HPP_ */
