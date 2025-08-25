#ifndef DETAIL_COVM_ALGORITHM_UNIFORM_STRENGTH_HPP_
#define DETAIL_COVM_ALGORITHM_UNIFORM_STRENGTH_HPP_

#include <citcpp/coverage_measurement.hpp>

#include "coverage_map.hpp"
#include "covm_exec_handle_impl.hpp"
#include "datatypes_config.hpp"
#include "internal_model.hpp"
#include "internal_test_set.hpp"

namespace citcpp {
namespace detail {

void measure_coverage(const model &model, const internal_test_set &test_set,
                      coverage_map &cov_map, covm_exec_handle_impl &exec_handle,
                      citcpp::coverage_measurement &covm);

void measure_coverage(const model &model, const internal_test_set &test_set,
                      coverage_map &cov_map, covm_exec_handle_impl &exec_handle,
                      citcpp::coverage_measurement &covm, thread_pool &tp);

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_COVM_ALGORITHM_UNIFORM_STRENGTH_HPP_ */
