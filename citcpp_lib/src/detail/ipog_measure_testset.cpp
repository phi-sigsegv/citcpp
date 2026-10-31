#include "ipog_measure_testset.hpp"

namespace {

inline void measure_coverage(
    citcpp::detail::ipog_coverage_map::second_level_type& value_combinations,
    const citcpp::detail::internal_test_set& test_set,
    std::size_t num_seeded_tests, unsigned long long& num_covered_tuples) {

  using namespace citcpp::detail;

  std::size_t test_index = 0;
  for (const test& test : test_set.get_list_of_tests()) {
    if (test_index >= num_seeded_tests) {
      break;
    }

    // A test with a don't care value for one of the parameters covers no
    // tuple of the combination.
    if (value_combinations.cover_tuple_of(test)) {
      ++num_covered_tuples;
    }

    ++test_index;
  }
}

class ipog_measure_per_param_combo_functor {
  public:
    ipog_measure_per_param_combo_functor(
        const citcpp::detail::internal_test_set& test_set,
        std::size_t num_seeded_tests)
        : test_set_(test_set),
          num_seeded_tests_(num_seeded_tests),
          num_covered_tuples_(0) {}

    void operator()(citcpp::detail::ipog_coverage_map::second_level_type&
                        value_combinations) {
      measure_coverage(value_combinations, test_set_, num_seeded_tests_,
                       num_covered_tuples_);
    }

    unsigned long long get_num_covered_tuples() const {
      return num_covered_tuples_;
    }

    void reset() { num_covered_tuples_ = 0; }

  private:
    const citcpp::detail::internal_test_set& test_set_;
    const std::size_t num_seeded_tests_;
    unsigned long long num_covered_tuples_;
};

}  // namespace

namespace citcpp {
namespace detail {

ipog_measure_testset_result ipog_measure_testset(
    const internal_test_set& test_set, std::size_t num_seeded_tests,
    std::vector<std::pair<const internal_relation*, ipog_coverage_map>>&
        relations) {

  // First initialize the result object.
  ipog_measure_testset_result result;

  ipog_measure_per_param_combo_functor per_param_combo_functor(
      test_set, num_seeded_tests);

  for (auto& rel_and_cov_map : relations) {
    per_param_combo_functor.reset();
    for (auto& value_combinations : rel_and_cov_map.second.get_coverage_map()) {
      per_param_combo_functor(value_combinations);
    }

    result.num_covered_tuples[rel_and_cov_map.first] =
        per_param_combo_functor.get_num_covered_tuples();
  }

  return result;
}

}  // namespace detail
}  // namespace citcpp
