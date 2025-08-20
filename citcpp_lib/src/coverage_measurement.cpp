#include <citcpp/coverage_measurement.hpp>
#include <cmath>

namespace citcpp {

std::ostream &operator<<(std::ostream &os, const coverage_measurement &covm) {
  const unsigned long long num_combos_to_cover =
      covm.get_number_of_combinations_to_cover();

  unsigned int test_index = 0;
  unsigned int five_percent_factor = 1;
  double cov_threshold = five_percent_factor * 0.05;
  os << "Coverage by tests:\n";
  for (unsigned long long num_covered_combos : covm.get_covered_tuples()) {
    const double fraction_covered =
        (double)num_covered_combos / (double)num_combos_to_cover;

    if (fraction_covered >= cov_threshold) {
      const double percent_of_tests =
          std::round(1000.0 * (double)test_index /
                     (double)covm.get_covered_tuples().size()) /
          10.0;

      os << "Coverage >= " << cov_threshold * 100.0
         << "% = " << num_covered_combos << "/" << num_combos_to_cover << " : "
         << test_index << "/" << covm.get_covered_tuples().size() << " = "
         << percent_of_tests << "% of tests\n";

      ++five_percent_factor;
      cov_threshold = five_percent_factor * 0.05;
    }

    ++test_index;
  }

  const unsigned long long num_param_combos_to_cover =
      covm.get_number_of_param_combos_to_cover();

  os << "param combinations: " << num_param_combos_to_cover << "\n";
  os << "param combinations coverage:\n";
  for (int i = 0; i <= 20; ++i) {
    const double frac = (double)i * 0.05;
    const unsigned long long num_param_combos = covm[frac];
    const double percent_of_all_param_combos =
        (double)num_param_combos / (double)num_param_combos_to_cover * 100.0;
    os << "Coverage >= " << frac * 100.0 << "% : " << num_param_combos << "/"
       << num_param_combos_to_cover << " = " << percent_of_all_param_combos
       << "%\n";
  }

  return os;
}

}  // namespace citcpp
