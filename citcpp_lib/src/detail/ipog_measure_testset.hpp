#ifndef IPOG_MEASURE_TESTSET_HPP_
#define IPOG_MEASURE_TESTSET_HPP_

#include "cagen_exec_handle_base.hpp"
#include "coverage_map.hpp"
#include "datatypes_config.hpp"
#include "internal_model.hpp"
#include "internal_test_set.hpp"

namespace citcpp {
namespace detail {

struct ipog_measure_testset_result {
    unsigned long long num_covered_tuples;
};

ipog_measure_testset_result ipog_measure_testset(
    const model &model, const internal_test_set &test_set,
    coverage_map &cov_map, cagen_exec_handle_base &exec_handle);

ipog_measure_testset_result ipog_measure_testset(
    const model &model, const internal_test_set &test_set,
    coverage_map &cov_map, cagen_exec_handle_base &exec_handle,
    thread_pool &tp);

}  // namespace detail
}  // namespace citcpp

#endif /* IPOG_MEASURE_TESTSET_HPP_ */