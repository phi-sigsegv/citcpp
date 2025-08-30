#include "covm_json.hpp"

namespace citcpp {
namespace detail {

coverage_measurement_json::coverage_measurement_json(
    const coverage_measurement &covm)
    : covm_(covm) {}

std::ostream &operator<<(std::ostream &os,
                         const coverage_measurement_json &covm_json) {

  const coverage_measurement &covm = covm_json.covm_;

  os << "{\n";
  os << "  \"num_tuples_to_cover\": "
     << covm.get_number_of_combinations_to_cover() << ",\n";
  os << "  \"tuples_covered_by_tests\": [\n";
  for (unsigned int test_idx = 0; test_idx < covm.get_covered_tuples().size();
       ++test_idx) {

    const unsigned long long covered_tuples =
        covm.get_covered_tuples()[test_idx];
    os << "    " << covered_tuples;
    if (test_idx < covm.get_covered_tuples().size() - 1) {
      os << ",";
    }
    os << "\n";
  }
  os << "  ],\n";
  os << "  \"num_param_combinations_to_cover\": "
     << covm.get_number_of_param_combos_to_cover() << ",\n";
  os << "  \"param_combinations_coverage\": [\n";
  for (int i = 0; i <= 20; ++i) {
    const double frac = (double)i * 0.05;
    const unsigned long long num_param_combos = covm[frac];

    os << "    {\n";
    os << "      \"coverage\": " << frac * 100.0 << ",\n";
    os << "      \"num_param_combos\": " << num_param_combos << "\n";
    os << "    }";
    if (i < 20) {
      os << ",";
    }
    os << "\n";
  }
  os << "  ]\n";
  os << "}\n";

  return os;
}

}  // namespace detail
}  // namespace citcpp
