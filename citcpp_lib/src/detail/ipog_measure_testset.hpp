#ifndef IPOG_MEASURE_TESTSET_HPP_
#define IPOG_MEASURE_TESTSET_HPP_

#include <unordered_map>

#include "coverage_map.hpp"
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
    std::size_t num_seeded_tests,
    std::vector<std::pair<const internal_relation*, ipog_coverage_map>>&
        relations);

}  // namespace detail
}  // namespace citcpp

#endif /* IPOG_MEASURE_TESTSET_HPP_ */