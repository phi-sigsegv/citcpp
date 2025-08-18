#include "covm_algorithm_uniform_strength.hpp"

#include <new>

namespace {

class covm_per_param_combo_functor {
  public:
    covm_per_param_combo_functor(
        const citcpp::detail::model &model,
        const citcpp::detail::test_set &test_set,
        citcpp::detail::covm_exec_handle_impl &exec_handle,
        citcpp::coverage_measurement &covm)
        : model_(model),
          test_set_(test_set),
          exec_handle_(exec_handle),
          covered_tuples_(test_set.get_list_of_tests().size(), 0),
          covm_(covm) {}

    bool operator()(
        citcpp::detail::coverage_map::second_level_type &value_combinations) {
      using namespace citcpp::detail;

      const param_vector &param_indices =
          value_combinations.get_parameter_indices();

      int test_index = 0;
      for (const test &test : test_set_.get_list_of_tests()) {
        // Here we compute an index into the bitset. To do so, we treat the
        // number of values of each parameter as a kind of radix. Consider
        // three parameters p_0, p_1, p_2. Now say that v_i is the number of
        // values for p_i. If we now have values x_0, x_1, x_2, then the index
        // is x_0 * v_1 * v_2 + x_1 * v_2 + x_2.
        coverage_map::second_level_type::size_type index =
            test.get_values()[param_indices[param_indices.size() - 1]];
        for (std::vector<unsigned int>::size_type i = 0;
             i < param_indices.size() - 1; ++i) {
          const unsigned int param_idx = param_indices[i];
          const int param_value = test.get_values()[param_idx];

          if (param_value < 0) {
            // We have found a don't care value for that combination in
            // the considered test in one of the parameters.
            // There is nothing to be updated concerning the
            // coverage.
            continue;
          }

          coverage_map::second_level_type::size_type addend = param_value;
          for (std::vector<unsigned int>::size_type j = i + 1;
               j < param_indices.size(); ++j) {
            addend *= model_.get_parameters()[param_indices[j]];
          }
          index += addend;
        }

        if (!value_combinations.test_and_set(index)) {
          covered_tuples_[test_index]++;
        }

        ++test_index;
      }

      double param_coverage_fraction = (double)value_combinations.count() /
                                       (double)value_combinations.size();
      covm_.add_coverage_of_param_combos(1, param_coverage_fraction);
      exec_handle_.add_number_of_checked_combinations(
          value_combinations.size());

      if (exec_handle_.is_job_aborted()) {
        return false;
      }

      return true;
    }

    const std::vector<unsigned long long> &get_coverered_tuples() {
      return covered_tuples_;
    }

  private:
    const citcpp::detail::model &model_;
    const citcpp::detail::test_set &test_set_;
    citcpp::detail::covm_exec_handle_impl &exec_handle_;
    std::vector<unsigned long long> covered_tuples_;
    citcpp::coverage_measurement &covm_;
};

}  // namespace

namespace citcpp {
namespace detail {

void measure_coverage(const model &model, const test_set &test_set,
                      coverage_map &cov_map, covm_exec_handle_impl &exec_handle,
                      citcpp::coverage_measurement &covm) {

  covm_per_param_combo_functor per_param_combo_functor(model, test_set,
                                                       exec_handle, covm);

  coverage_map_iterator cov_map_it = cov_map.create_iterator();
  cov_map_it.visit_all_parameter_combinations(per_param_combo_functor);

  std::vector<unsigned long long> covered_tuples(
      per_param_combo_functor.get_coverered_tuples());
  for (unsigned int test_index = 0; test_index < covered_tuples.size();
       ++test_index) {

    if (test_index > 0) {
      covered_tuples[test_index] =
          covered_tuples[test_index] + covered_tuples[test_index - 1];
    }
  }

  covm.set_coverered_tuples(covered_tuples);
}

void measure_coverage(const model &model, const test_set &test_set,
                      coverage_map &cov_map, covm_exec_handle_impl &exec_handle,
                      citcpp::coverage_measurement &covm, thread_pool &tp) {}

}  // namespace detail
}  // namespace citcpp