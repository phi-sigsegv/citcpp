#ifndef COVERAGE_MEASUREMENT_HPP_
#define COVERAGE_MEASUREMENT_HPP_

#include <algorithm>
#include <array>
#include <ostream>
#include <vector>

namespace citcpp {

/**
 * Represents the results of a coverage measurement of a given test set.
 */
class coverage_measurement {
  public:
    static constexpr int NUM_DIFFERENTIATED_COVERAGE_LEVELS = 21;
    typedef std::array<unsigned long long, NUM_DIFFERENTIATED_COVERAGE_LEVELS>
        t_coverage_level_to_num_param_combos;

    coverage_measurement()
        : num_param_combos_to_cover_(0),
          num_tuples_to_cover_(0),
          covered_tuples_(),
          cov_level_to_num_param_combos_() {}

    /**
     * Returns the number of parameter combinations that have to be covered.
     */
    unsigned long long get_number_of_param_combos_to_cover() const {
      return num_param_combos_to_cover_;
    }

    void set_number_of_param_combos_to_cover(
        unsigned long long num_param_combos_to_cover) {
      num_param_combos_to_cover_ = num_param_combos_to_cover;
    }

    coverage_measurement& number_of_param_combos_to_cover(
        unsigned long long num_param_combos_to_cover) {

      num_param_combos_to_cover_ = num_param_combos_to_cover;
      return *this;
    }

    /**
     * Returns the number of combinations / value tuples that have to be
     * covered.
     */
    unsigned long long get_number_of_combinations_to_cover() const {
      return num_tuples_to_cover_;
    }

    void set_number_of_combinations_to_cover(
        unsigned long long num_tuples_to_cover) {

      num_tuples_to_cover_ = num_tuples_to_cover;
    }

    coverage_measurement& number_of_combinations_to_cover(
        unsigned long long num_tuples_to_cover) {

      num_tuples_to_cover_ = num_tuples_to_cover;
      return *this;
    }

    /**
     * Returns a vector of the number of covered tuples. The numbers are in an
     * incremental fashion: The number at index i denotes the number of tuples
     * covered by the tests from index 0 up to index i in the given test set.
     */
    const std::vector<unsigned long long>& get_covered_tuples() const {
      return covered_tuples_;
    }

    /**
     * Returns a vector of the number of covered tuples. The numbers are in an
     * incremental fashion: The number at index i denotes the number of tuples
     * covered by the tests from index 0 up to index i in the given test set.
     */
    std::vector<unsigned long long>& get_covered_tuples() {
      return covered_tuples_;
    }

    void set_coverered_tuples(
        const std::vector<unsigned long long>& covered_tuples) {
      covered_tuples_ = covered_tuples;
    }

    void set_coverered_tuples(
        std::vector<unsigned long long>&& covered_tuples) {
      covered_tuples_ = std::move(covered_tuples);
    }

    coverage_measurement& coverered_tuples(
        const std::vector<unsigned long long>& covered_tuples) {
      covered_tuples_ = covered_tuples;
      return *this;
    }

    coverage_measurement& coverered_tuples(
        std::vector<unsigned long long>&& covered_tuples) {
      covered_tuples_ = std::move(covered_tuples);
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
      int index =
          std::min((int)((double)(NUM_DIFFERENTIATED_COVERAGE_LEVELS - 1) *
                         coverage_fraction),
                   NUM_DIFFERENTIATED_COVERAGE_LEVELS - 1);

      return cov_level_to_num_param_combos_[index];
    }

    /**
     * Returns for a coverage level (specified as a fraction in the range [0,
     * 1]), the number of parameter combinations, whose coverage is equal or
     * greater than that fraction.
     * A coverage level of 1.0 means that all the value combinations of the
     * parameter combinations have been covered.
     */
    unsigned long long& operator[](double coverage_fraction) {
      coverage_fraction = std::max(std::min(coverage_fraction, 1.0), 0.0);

      // Map the coverage fraction to the appropriate array index.
      int index =
          std::min((int)((double)(NUM_DIFFERENTIATED_COVERAGE_LEVELS - 1) *
                         coverage_fraction),
                   NUM_DIFFERENTIATED_COVERAGE_LEVELS - 1);

      return cov_level_to_num_param_combos_[std::min(
          index, NUM_DIFFERENTIATED_COVERAGE_LEVELS - 1)];
    }

    /**
     * Adds the information that the given number of parameter combinations
     * have been covered to the specified extent.
     */
    void add_coverage_of_param_combos(unsigned long long num_param_combos,
                                      double coverage_fraction) {

      coverage_fraction = std::max(std::min(coverage_fraction, 1.0), 0.0);

      // Map the coverage fraction to the appropriate array index.
      int index =
          std::min((int)((double)(NUM_DIFFERENTIATED_COVERAGE_LEVELS - 1) *
                         coverage_fraction),
                   NUM_DIFFERENTIATED_COVERAGE_LEVELS - 1);

      for (; index >= 0; --index) {
        cov_level_to_num_param_combos_[index] += num_param_combos;
      }
    }

    void set_coverage_level_to_num_param_combos(
        const t_coverage_level_to_num_param_combos&
            cov_level_to_num_param_combos) {

      cov_level_to_num_param_combos_ = cov_level_to_num_param_combos;
    }

    void set_coverage_level_to_num_param_combos(
        t_coverage_level_to_num_param_combos&& cov_level_to_num_param_combos) {

      cov_level_to_num_param_combos_ = std::move(cov_level_to_num_param_combos);
    }

    coverage_measurement& coverage_level_to_num_param_combos(
        const t_coverage_level_to_num_param_combos&
            cov_level_to_num_param_combos) {

      cov_level_to_num_param_combos_ = cov_level_to_num_param_combos;
      return *this;
    }

    coverage_measurement& coverage_level_to_num_param_combos(
        t_coverage_level_to_num_param_combos&& cov_level_to_num_param_combos) {

      cov_level_to_num_param_combos_ = std::move(cov_level_to_num_param_combos);
      return *this;
    }

    bool operator==(const coverage_measurement& other) const {
      return num_param_combos_to_cover_ == other.num_param_combos_to_cover_ &&
             num_tuples_to_cover_ == other.num_tuples_to_cover_ &&
             covered_tuples_ == other.covered_tuples_ &&
             cov_level_to_num_param_combos_ ==
                 other.cov_level_to_num_param_combos_;
    }

    friend std::ostream& operator<<(std::ostream& os,
                                    const coverage_measurement& covm);

  private:
    unsigned long long num_param_combos_to_cover_;
    unsigned long long num_tuples_to_cover_;
    std::vector<unsigned long long> covered_tuples_;
    t_coverage_level_to_num_param_combos cov_level_to_num_param_combos_;
};

}  // namespace citcpp

#endif /* COVERAGE_MEASUREMENT_HPP_ */
