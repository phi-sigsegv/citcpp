#include <algorithm>
#include <citcpp/coverage_measurement.hpp>
#include <cmath>

namespace citcpp {

unsigned long long coverage_measurement::get_number_of_param_combos_to_cover()
    const {

  return num_param_combos_to_cover_;
}

void coverage_measurement::set_number_of_param_combos_to_cover(
    unsigned long long num_param_combos_to_cover) {
  num_param_combos_to_cover_ = num_param_combos_to_cover;
}

coverage_measurement& coverage_measurement::number_of_param_combos_to_cover(
    unsigned long long num_param_combos_to_cover) {

  set_number_of_param_combos_to_cover(num_param_combos_to_cover);

  return *this;
}

unsigned long long coverage_measurement::get_number_of_combinations_to_cover()
    const {
  return num_tuples_to_cover_;
}

void coverage_measurement::set_number_of_combinations_to_cover(
    unsigned long long num_tuples_to_cover) {

  num_tuples_to_cover_ = num_tuples_to_cover;
}

coverage_measurement& coverage_measurement::number_of_combinations_to_cover(
    unsigned long long num_tuples_to_cover) {

  set_number_of_combinations_to_cover(num_tuples_to_cover);

  return *this;
}

const std::vector<unsigned long long>&
coverage_measurement::get_covered_tuples() const {

  return covered_tuples_;
}

std::vector<unsigned long long>& coverage_measurement::get_covered_tuples() {
  return covered_tuples_;
}

void coverage_measurement::set_coverered_tuples(
    std::vector<unsigned long long> covered_tuples) {

  covered_tuples_ = std::move(covered_tuples);
}

coverage_measurement& coverage_measurement::coverered_tuples(
    std::vector<unsigned long long> covered_tuples) {

  covered_tuples_ = std::move(covered_tuples);

  return *this;
}

unsigned long long coverage_measurement::operator[](
    double coverage_fraction) const {

  coverage_fraction = std::max(std::min(coverage_fraction, 1.0), 0.0);

  // Map the coverage fraction to the appropriate array index.
  unsigned int index =
      std::min(static_cast<unsigned int>(
                   static_cast<double>(NUM_DIFFERENTIATED_COVERAGE_LEVELS - 1) *
                   coverage_fraction),
               NUM_DIFFERENTIATED_COVERAGE_LEVELS - 1);

  return cov_level_to_num_param_combos_[index];
}

void coverage_measurement::add_coverage_of_param_combos(
    unsigned long long num_param_combos, double coverage_fraction) {

  coverage_fraction = std::max(std::min(coverage_fraction, 1.0), 0.0);

  // Map the coverage fraction to the appropriate array index.
  int index =
      std::min(static_cast<unsigned int>(
                   static_cast<double>(NUM_DIFFERENTIATED_COVERAGE_LEVELS - 1) *
                   coverage_fraction),
               NUM_DIFFERENTIATED_COVERAGE_LEVELS - 1);

  for (; index >= 0; --index) {
    cov_level_to_num_param_combos_[index] += num_param_combos;
  }
}

void coverage_measurement::set_coverage_level_to_num_param_combos(
    t_coverage_level_to_num_param_combos cov_level_to_num_param_combos) {

  cov_level_to_num_param_combos_ = std::move(cov_level_to_num_param_combos);
}

coverage_measurement& coverage_measurement::coverage_level_to_num_param_combos(
    t_coverage_level_to_num_param_combos cov_level_to_num_param_combos) {

  cov_level_to_num_param_combos_ = std::move(cov_level_to_num_param_combos);

  return *this;
}

bool coverage_measurement::operator==(const coverage_measurement& other) const {
  return num_param_combos_to_cover_ == other.num_param_combos_to_cover_ &&
         num_tuples_to_cover_ == other.num_tuples_to_cover_ &&
         covered_tuples_ == other.covered_tuples_ &&
         cov_level_to_num_param_combos_ == other.cov_level_to_num_param_combos_;
}

std::ostream& operator<<(std::ostream& os, const coverage_measurement& covm) {
  const unsigned long long num_combos_to_cover =
      covm.get_number_of_combinations_to_cover();

  unsigned int test_index = 0;
  unsigned int percent_covered_threshold = 5;
  os << "Coverage by tests:\n";
  for (unsigned long long num_covered_combos : covm.get_covered_tuples()) {
    const double fraction_covered = static_cast<double>(num_covered_combos) /
                                    static_cast<double>(num_combos_to_cover);
    const unsigned int percent_covered =
        (static_cast<unsigned int>(std::floor(fraction_covered * 100.0)) / 5) *
        5;

    if (percent_covered >= percent_covered_threshold) {
      const double percent_of_tests =
          std::ceil(1000.0 * static_cast<double>(test_index + 1) /
                    static_cast<double>(covm.get_covered_tuples().size())) /
          10.0;

      os << "Coverage >= " << percent_covered << "% = " << num_covered_combos
         << "/" << num_combos_to_cover << " : " << (test_index + 1) << "/"
         << covm.get_covered_tuples().size() << " = " << percent_of_tests
         << "% of tests\n";

      percent_covered_threshold = percent_covered + 5;
    }

    ++test_index;
  }

  const unsigned long long num_param_combos_to_cover =
      covm.get_number_of_param_combos_to_cover();

  os << "param combinations: " << num_param_combos_to_cover << "\n";
  os << "param combinations coverage:\n";
  for (unsigned int i = 0; i <= 20; ++i) {
    const double frac = static_cast<double>(i) * 0.05;
    const unsigned long long num_param_combos = covm[frac];
    const double percent_of_all_param_combos =
        static_cast<double>(num_param_combos) /
        static_cast<double>(num_param_combos_to_cover) * 100.0;
    os << "Coverage >= " << frac * 100.0 << "% : " << num_param_combos << "/"
       << num_param_combos_to_cover << " = " << percent_of_all_param_combos
       << "%\n";
  }

  return os;
}

}  // namespace citcpp
