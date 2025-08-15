#ifndef COVERAGE_MEASUREMENT_HPP_
#define COVERAGE_MEASUREMENT_HPP_

#include <algorithm>
#include <array>
#include <vector>

namespace citcpp {

/**
 * Represents the results of a coverage measurement of a given test set.
 */
class coverage_measurement {
  public:
    static constexpr unsigned int NUM_DIFFERENTIATED_COVERAGE_LEVELS = 21;
    typedef std::array<unsigned long long, NUM_DIFFERENTIATED_COVERAGE_LEVELS>
        t_coverage_level_to_num_param_combos;

    /**
     * Returns the number of parameter combinations that have to be covered.
     */
    const unsigned long long &get_number_of_param_combos_to_cover() const {
      return num_param_combos_to_cover_;
    }

    /**
     * Returns the number of parameter combinations that have to be covered.
     */
    unsigned long long &get_number_of_param_combos_to_cover() {
      return num_param_combos_to_cover_;
    }

    void set_number_of_param_combos_to_cover(
        unsigned long long num_param_combos_to_cover) {
      num_param_combos_to_cover_ = num_param_combos_to_cover;
    }

    coverage_measurement &number_of_param_combos_to_cover(
        unsigned long long num_param_combos_to_cover) {

      num_param_combos_to_cover_ = num_param_combos_to_cover;
      return *this;
    }

    /**
     * Returns the number of combinations / value tuples that have to be
     * covered.
     */
    const unsigned long long &get_number_of_combinations_to_cover() const {
      return num_tuples_to_cover_;
    }

    /**
     * Returns the number of combinations / value tuples that have to be
     * covered.
     */
    unsigned long long &get_number_of_combinations_to_cover() {
      return num_tuples_to_cover_;
    }

    void set_number_of_combinations_to_cover(
        unsigned long long num_tuples_to_cover) {

      num_tuples_to_cover_ = num_tuples_to_cover;
    }

    coverage_measurement &number_of_combinations_to_cover(
        unsigned long long num_tuples_to_cover) {

      num_tuples_to_cover_ = num_tuples_to_cover;
      return *this;
    }

    /**
     * Returns the number of tests whose coverage has been measured.
     * Due to the fact that the coverage measurement might have been
     * aborted, the number of tests measured so far could be less than
     * the number of all tests in the given test set.
     */
    unsigned int get_number_of_measured_tests() const {
      return covered_tuples_.size();
    }

    /**
     * Returns a vector of the number of covered tuples. The numbers are in an
     * incremental fashion: The number at index i denotes the number of tuples
     * covered by the tests from index 0 up to index i in the given test set.
     */
    const std::vector<unsigned long long> &get_covered_tuples() const {
      return covered_tuples_;
    }

    /**
     * Returns a vector of the number of covered tuples. The numbers are in an
     * incremental fashion: The number at index i denotes the number of tuples
     * covered by the tests from index 0 up to index i in the given test set.
     */
    std::vector<unsigned long long> &get_coverered_tuples() {
      return covered_tuples_;
    }

    void set_coverered_tuples(
        const std::vector<unsigned long long> &covered_tuples) {
      covered_tuples_ = covered_tuples;
    }

    coverage_measurement &coverered_tuples(
        const std::vector<unsigned long long> &covered_tuples) {
      covered_tuples_ = covered_tuples;
      return *this;
    }

    /**
     * Returns for a coverage level (specified as a fraction in the range [0,
     * 1]), the number of parameter combinations, whose coverage is equal or
     * greater than that fraction.
     * A coverage level of 1.0 means that all the value combinations of the
     * parameter combinations have been covered.
     */
    unsigned long long operator[](double coverage_fraction) const {
      coverage_fraction = std::max(std::min(coverage_fraction, 1.0), 0.0);

      // Map the coverage fraction to the appropriate array index.
      unsigned int index =
          (unsigned int)((double)(NUM_DIFFERENTIATED_COVERAGE_LEVELS - 1) *
                         coverage_fraction);

      return cov_level_to_num_param_combos_[std::min(
          index, NUM_DIFFERENTIATED_COVERAGE_LEVELS - 1)];
    }

    /**
     * Returns for a coverage level (specified as a fraction in the range [0,
     * 1]), the number of parameter combinations, whose coverage is equal or
     * greater than that fraction.
     * A coverage level of 1.0 means that all the value combinations of the
     * parameter combinations have been covered.
     */
    unsigned long long &operator[](double coverage_fraction) {
      coverage_fraction = std::max(std::min(coverage_fraction, 1.0), 0.0);

      // Map the coverage fraction to the appropriate array index.
      unsigned int index =
          (unsigned int)((double)(NUM_DIFFERENTIATED_COVERAGE_LEVELS - 1) *
                         coverage_fraction);

      return cov_level_to_num_param_combos_[std::min(
          index, NUM_DIFFERENTIATED_COVERAGE_LEVELS - 1)];
    }

    void set_coverage_level_to_num_param_combos(
        const t_coverage_level_to_num_param_combos
            &cov_level_to_num_param_combos) {

      cov_level_to_num_param_combos_ = cov_level_to_num_param_combos;
    }

    coverage_measurement &coverage_level_to_num_param_combos(
        const t_coverage_level_to_num_param_combos
            &cov_level_to_num_param_combos) {

      cov_level_to_num_param_combos_ = cov_level_to_num_param_combos;
      return *this;
    }

    bool operator==(const coverage_measurement &other) const {
      return num_param_combos_to_cover_ == other.num_param_combos_to_cover_ &&
             num_tuples_to_cover_ == other.num_tuples_to_cover_ &&
             covered_tuples_ == other.covered_tuples_ &&
             cov_level_to_num_param_combos_ ==
                 other.cov_level_to_num_param_combos_;
    }

  private:
    unsigned long long num_param_combos_to_cover_;
    unsigned long long num_tuples_to_cover_;
    std::vector<unsigned long long> covered_tuples_;
    t_coverage_level_to_num_param_combos cov_level_to_num_param_combos_;
};

}  // namespace citcpp

#endif /* COVERAGE_MEASUREMENT_HPP_ */
