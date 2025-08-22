#include <citcpp/coverage_measurement.hpp>
#include <cmath>

namespace citcpp {

std::ostream &operator<<(std::ostream &os, const coverage_measurement &covm) {
  const unsigned long long num_combos_to_cover =
      covm.get_number_of_combinations_to_cover();

  unsigned int test_index = 0;
  unsigned int percent_covered_threshold = 5;
  os << "Coverage by tests:\n";
  for (unsigned long long num_covered_combos : covm.get_covered_tuples()) {
    const double fraction_covered =
        (double)num_covered_combos / (double)num_combos_to_cover;
    const unsigned int percent_covered =
        (((unsigned int)std::floor(fraction_covered * 100.0)) / 5) * 5;

    if (percent_covered >= percent_covered_threshold) {
      const double percent_of_tests =
          std::ceil(1000.0 * (double)(test_index + 1) /
                    (double)covm.get_covered_tuples().size()) /
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
