#ifndef DETAIL_MODEL_SIMPLIFIER_HPP_
#define DETAIL_MODEL_SIMPLIFIER_HPP_

#include <citcpp/constraints.hpp>
#include <citcpp/model.hpp>
#include <unordered_map>

namespace citcpp {
namespace detail {

/**
 * This method simplifies the given model returning it.
 */
model& simplify_model(model& m);

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_MODEL_SIMPLIFIER_HPP_ */
