#include <citcpp/coverage_measurement.hpp>

namespace citcpp {

std::ostream &operator<<(std::ostream &os, const coverage_measurement &covm) {
  const unsigned long long num_covered_combos =
      covm.get_covered_tuples().empty() ? 0 : covm.get_covered_tuples().back();
  const unsigned long long num_combos_to_cover =
      covm.get_number_of_combinations_to_cover();
  const double precent_done =
      (double)num_covered_combos / (double)num_combos_to_cover * 100.0;
  const unsigned long long num_param_combos_to_cover =
      covm.get_number_of_param_combos_to_cover();

  os << "tuples: (" << num_covered_combos << " / " << num_combos_to_cover
     << ") " << precent_done << "%\n";
  os << "param combinations: " << num_param_combos_to_cover << "\n";

  os << "param combinations coverage:\n";
  for (int i = 0; i <= 20; ++i) {
    const double frac = (double)i * 0.05;
    const unsigned long long num_param_combos = covm[frac];
    const double percent_of_all_param_combos =
        (double)num_param_combos / (double)num_param_combos_to_cover * 100.0;
    os << "Coverage >= " << frac * 100.0 << ": " << num_param_combos << "/"
       << num_param_combos_to_cover << " = " << percent_of_all_param_combos
       << "%\n";
  }

  return os;
}

}  // namespace citcpp
