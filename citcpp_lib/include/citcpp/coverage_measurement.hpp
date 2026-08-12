#ifndef COVERAGE_MEASUREMENT_HPP_
#define COVERAGE_MEASUREMENT_HPP_

#include <array>
#include <ostream>
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
    unsigned long long get_number_of_param_combos_to_cover() const;

    void set_number_of_param_combos_to_cover(
        unsigned long long num_param_combos_to_cover);

    coverage_measurement& number_of_param_combos_to_cover(
        unsigned long long num_param_combos_to_cover);

    /**
     * Returns the number of combinations / value tuples that have to be
     * covered.
     */
    unsigned long long get_number_of_combinations_to_cover() const;

    void set_number_of_combinations_to_cover(
        unsigned long long num_tuples_to_cover);

    coverage_measurement& number_of_combinations_to_cover(
        unsigned long long num_tuples_to_cover);

    /**
     * Returns a vector of the number of covered tuples. The numbers are in an
     * incremental/cumulative fashion: The number at index i denotes the number
     * of tuples covered by the tests from index 0 up to index i in the given
     * test set.
     */
    const std::vector<unsigned long long>& get_covered_tuples() const;

    /**
     * Returns a vector of the number of covered tuples. The numbers are in an
     * incremental/cumulative fashion: The number at index i denotes the number
     * of tuples covered by the tests from index 0 up to index i in the given
     * test set.
     */
    std::vector<unsigned long long>& get_covered_tuples();

    void set_coverered_tuples(std::vector<unsigned long long> covered_tuples);

    coverage_measurement& coverered_tuples(
        std::vector<unsigned long long> covered_tuples);

    /**
     * Returns for a coverage level (specified as a fraction in the range [0,
     * 1]), the number of parameter combinations, whose coverage is equal or
     * greater than that fraction.
     * A coverage level of 1.0 means that all the value combinations of the
     * parameter combinations have been covered.
     * The resolution is in steps of 5%, meaning passing 0.03 and 0.04 to
     * this function yield the very same result.
     */
    unsigned long long operator[](double coverage_fraction) const;

    /**
     * Adds the information that the given number of parameter combinations
     * have been covered to the specified extent.
     */
    void add_coverage_of_param_combos(unsigned long long num_param_combos,
                                      double coverage_fraction);

    void set_coverage_level_to_num_param_combos(
        t_coverage_level_to_num_param_combos cov_level_to_num_param_combos);

    coverage_measurement& coverage_level_to_num_param_combos(
        t_coverage_level_to_num_param_combos cov_level_to_num_param_combos);

    bool operator==(const coverage_measurement& other) const;

    friend std::ostream& operator<<(std::ostream& os,
                                    const coverage_measurement& covm);

  private:
    unsigned long long num_param_combos_to_cover_{0};
    unsigned long long num_tuples_to_cover_{0};
    std::vector<unsigned long long> covered_tuples_{};
    t_coverage_level_to_num_param_combos cov_level_to_num_param_combos_{0};
};

}  // namespace citcpp

#endif /* COVERAGE_MEASUREMENT_HPP_ */
