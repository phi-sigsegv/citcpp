#ifndef IPOG_MEASURE_TESTSET_HPP_
#define IPOG_MEASURE_TESTSET_HPP_

#include <unordered_map>

#include "cagen_exec_handle_base.hpp"
#include "coverage_map.hpp"
#include "functor_executor.hpp"
#include "internal_model.hpp"
#include "internal_test_set.hpp"

namespace citcpp {
namespace detail {

struct ipog_measure_testset_result {
    std::unordered_map<const internal_relation*, unsigned long long>
        num_covered_tuples;
};

ipog_measure_testset_result ipog_measure_testset(
    const internal_model& model, const internal_test_set& test_set,
    std::vector<std::pair<const internal_relation*, coverage_map>>& relations);

ipog_measure_testset_result ipog_measure_testset(
    const internal_model& model, const internal_test_set& test_set,
    std::vector<std::pair<const internal_relation*, coverage_map>>& relations,
    functor_executor& exec);

}  // namespace detail
}  // namespace citcpp

#include "ipog_measure_testset.tpp"

#endif /* IPOG_MEASURE_TESTSET_HPP_ */