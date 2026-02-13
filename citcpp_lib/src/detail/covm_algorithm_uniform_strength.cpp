#include "covm_algorithm_uniform_strength.hpp"

#include "bitset.hpp"
#include "citcpp_utils.hpp"
#include "param_combo_iteration.hpp"
#include "shared_constants.hpp"

namespace {

struct alignas(citcpp::detail::false_sharing_avoidance_alignment)
    aligned_array_wrapper {
    citcpp::detail::array_wrapper_uint64 value;
};

struct alignas(citcpp::detail::false_sharing_avoidance_alignment)
    aligned_coverage_level_to_num_param_combos {
    citcpp::coverage_measurement::t_coverage_level_to_num_param_combos value;
};

class covm_per_param_combo_functor {
  public:
    covm_per_param_combo_functor(
        const citcpp::detail::internal_model& model,
        const citcpp::detail::internal_test_set& test_set,
        const unsigned int bitset_backing_array_size,
        citcpp::detail::covm_exec_handle_impl& exec_handle,
        citcpp::coverage_measurement& covm)
        : model_(model),
          test_set_(test_set),
          bitset_backing_array_(bitset_backing_array_size),
          exec_handle_(exec_handle),
          covered_tuples_(test_set.get_list_of_tests().size(), 0),
          covm_(covm) {}

    bool operator()(const citcpp::detail::param_vector& param_indices) {
      using namespace citcpp::detail;

      bitset_non_owning_uint64::size_type bitset_size = 1;
      for (auto p : param_indices) {
        bitset_size *= model_.get_parameter_num_values()[p];
      }
      bitset_non_owning_uint64 values_combo_bitset(bitset_size);
      values_combo_bitset.set_backing_array(bitset_backing_array_.get_array());
      values_combo_bitset.reset();

      int test_index = 0;
      for (const test& test : test_set_.get_list_of_tests()) {
        // Here we compute an index into the bitset. To do so, we treat the
        // number of values of each parameter as a kind of radix. Consider
        // three parameters p_0, p_1, p_2. Now say that v_i is the number of
        // values for p_i. If we now have values x_0, x_1, x_2, then the index
        // is x_0 * v_1 * v_2 + x_1 * v_2 + x_2.
        bitset_non_owning_uint64::size_type index = 0;
        bool found_dont_care = false;
        for (std::vector<unsigned int>::size_type i = 0;
             i < param_indices.size(); ++i) {
          const unsigned int param_idx = param_indices[i];
          const int param_value = test.get_values()[param_idx];

          if (param_value < 0) {
            // We have found a don't care value for that combination in
            // the considered test in one of the parameters.
            // There is nothing to be updated concerning the
            // coverage.
            found_dont_care = true;
            break;
          }

          bitset_non_owning_uint64::size_type addend = param_value;
          for (std::vector<unsigned int>::size_type j = i + 1;
               j < param_indices.size(); ++j) {
            addend *= model_.get_parameter_num_values()[param_indices[j]];
          }
          index += addend;
        }

        if (!found_dont_care) {
          if (!values_combo_bitset.test_and_set(index)) {
            covered_tuples_[test_index]++;
          }
        }

        ++test_index;
      }

      double param_coverage_fraction = (double)values_combo_bitset.count() /
                                       (double)values_combo_bitset.size();
      covm_.add_coverage_of_param_combos(1, param_coverage_fraction);
      exec_handle_.add_number_of_checked_combinations(
          values_combo_bitset.size());

      if (exec_handle_.is_job_aborted()) {
        return false;
      }

      return true;
    }

    const std::vector<unsigned long long>& get_coverered_tuples() {
      return covered_tuples_;
    }

  private:
    const citcpp::detail::internal_model& model_;
    const citcpp::detail::internal_test_set& test_set_;
    citcpp::detail::array_wrapper_uint64 bitset_backing_array_;
    citcpp::detail::covm_exec_handle_impl& exec_handle_;
    std::vector<unsigned long long> covered_tuples_;
    citcpp::coverage_measurement& covm_;
};

class covm_per_param_combo_functor_parallel {
  public:
    covm_per_param_combo_functor_parallel(
        const citcpp::detail::internal_model& model,
        const citcpp::detail::internal_test_set& test_set,
        const unsigned int bitset_backing_array_size,
        citcpp::detail::covm_exec_handle_impl& exec_handle,
        const citcpp::detail::param_combo_parallel_iterator& param_combo_it)
        : model_(model),
          test_set_(test_set),
          exec_handle_(exec_handle),
          param_combo_it_(param_combo_it),
          bitset_backing_array_(param_combo_it.get_num_workers(),
                                {{bitset_backing_array_size}}),
          covered_tuples_(param_combo_it.get_num_workers(),
                          citcpp::detail::aligned_vector<unsigned long long>(
                              test_set.get_list_of_tests().size(), 0)),
          cov_level_to_num_param_combos_(param_combo_it.get_num_workers()) {}

    bool operator()(const citcpp::detail::param_vector& param_indices) {
      using namespace citcpp::detail;

      bitset_non_owning_uint64::size_type bitset_size = 1;
      for (auto p : param_indices) {
        bitset_size *= model_.get_parameter_num_values()[p];
      }
      bitset_non_owning_uint64 values_combo_bitset(bitset_size);
      values_combo_bitset.set_backing_array(
          bitset_backing_array_[param_combo_it_.get_worker_id()]
              .value.get_array());
      values_combo_bitset.reset();

      int test_index = 0;
      for (const test& test : test_set_.get_list_of_tests()) {
        // Here we compute an index into the bitset. To do so, we treat the
        // number of values of each parameter as a kind of radix. Consider
        // three parameters p_0, p_1, p_2. Now say that v_i is the number of
        // values for p_i. If we now have values x_0, x_1, x_2, then the index
        // is x_0 * v_1 * v_2 + x_1 * v_2 + x_2.
        bitset_non_owning_uint64::size_type index = 0;
        bool found_dont_care = false;
        for (std::vector<unsigned int>::size_type i = 0;
             i < param_indices.size(); ++i) {
          const unsigned int param_idx = param_indices[i];
          const int param_value = test.get_values()[param_idx];

          if (param_value < 0) {
            // We have found a don't care value for that combination in
            // the considered test in one of the parameters.
            // There is nothing to be updated concerning the
            // coverage.
            found_dont_care = true;
            break;
          }

          bitset_non_owning_uint64::size_type addend = param_value;
          for (std::vector<unsigned int>::size_type j = i + 1;
               j < param_indices.size(); ++j) {
            addend *= model_.get_parameter_num_values()[param_indices[j]];
          }
          index += addend;
        }

        if (!found_dont_care) {
          if (!values_combo_bitset.test_and_set(index)) {
            auto& thread_local_covered_tuples =
                covered_tuples_[param_combo_it_.get_worker_id()];
            thread_local_covered_tuples.value[test_index]++;
          }
        }

        ++test_index;
      }

      double param_coverage_fraction = (double)values_combo_bitset.count() /
                                       (double)values_combo_bitset.size();

      auto& thread_local_cov_level_to_num_param_combos =
          cov_level_to_num_param_combos_[param_combo_it_.get_worker_id()];

      param_coverage_fraction =
          std::max(std::min(param_coverage_fraction, 1.0), 0.0);

      // Map the coverage fraction to the appropriate array index.
      int index = std::min(
          (int)((double)(citcpp::coverage_measurement::
                             NUM_DIFFERENTIATED_COVERAGE_LEVELS -
                         1) *
                param_coverage_fraction),
          citcpp::coverage_measurement::NUM_DIFFERENTIATED_COVERAGE_LEVELS - 1);

      for (; index >= 0; --index) {
        thread_local_cov_level_to_num_param_combos.value[index]++;
      }

      exec_handle_.add_number_of_checked_combinations(
          values_combo_bitset.size());

      if (exec_handle_.is_job_aborted()) {
        return false;
      }

      return true;
    }

    const citcpp::detail::thread_local_vector<
        citcpp::detail::aligned_vector<unsigned long long>>&
    get_coverered_tuples() const {

      return covered_tuples_;
    }

    const citcpp::detail::thread_local_vector<
        aligned_coverage_level_to_num_param_combos>&
    get_cov_level_to_num_param_combos() const {

      return cov_level_to_num_param_combos_;
    }

  private:
    const citcpp::detail::internal_model& model_;
    const citcpp::detail::internal_test_set& test_set_;
    citcpp::detail::covm_exec_handle_impl& exec_handle_;
    const citcpp::detail::param_combo_parallel_iterator& param_combo_it_;
    alignas(citcpp::detail::false_sharing_avoidance_alignment) citcpp::detail::
        thread_local_vector<aligned_array_wrapper> bitset_backing_array_;
    alignas(citcpp::detail::false_sharing_avoidance_alignment)
        citcpp::detail::thread_local_vector<
            citcpp::detail::aligned_vector<unsigned long long>> covered_tuples_;
    alignas(citcpp::detail::false_sharing_avoidance_alignment)
        citcpp::detail::thread_local_vector<
            aligned_coverage_level_to_num_param_combos> cov_level_to_num_param_combos_;
};

}  // namespace

namespace citcpp {
namespace detail {

void measure_coverage(const unsigned int strength, const internal_model& model,
                      const std::vector<unsigned int>& parameter_index_map,
                      const internal_test_set& test_set,
                      covm_exec_handle_impl& exec_handle,
                      citcpp::coverage_measurement& covm) {

  const unsigned int product_of_max_parameter_sizes =
      get_product_of_max_n_parameter_sizes(parameter_index_map.size(), strength,
                                           model, parameter_index_map);

  covm_per_param_combo_functor per_param_combo_functor(
      model, test_set, product_of_max_parameter_sizes, exec_handle, covm);

  param_combo_iterator param_combo_it(parameter_index_map.size(), strength,
                                      parameter_index_map, false);
  param_combo_it.visit_all_parameter_combinations(per_param_combo_functor);

  std::vector<unsigned long long> covered_tuples(
      per_param_combo_functor.get_coverered_tuples());
  for (unsigned int test_index = 0; test_index < covered_tuples.size();
       ++test_index) {

    if (test_index > 0) {
      covered_tuples[test_index] =
          covered_tuples[test_index] + covered_tuples[test_index - 1];
    }
  }

  covm.set_coverered_tuples(std::move(covered_tuples));
}

void measure_coverage(const unsigned int strength, const internal_model& model,
                      const std::vector<unsigned int>& parameter_index_map,
                      const internal_test_set& test_set,
                      covm_exec_handle_impl& exec_handle,
                      citcpp::coverage_measurement& covm, thread_pool& tp) {

  const unsigned int product_of_max_parameter_sizes =
      get_product_of_max_n_parameter_sizes(parameter_index_map.size(), strength,
                                           model, parameter_index_map);

  param_combo_parallel_iterator param_combo_it(
      parameter_index_map.size(), strength, parameter_index_map, false, tp);

  covm_per_param_combo_functor_parallel per_param_combo_functor(
      model, test_set, product_of_max_parameter_sizes, exec_handle,
      param_combo_it);

  param_combo_it.visit_all_parameter_combinations(per_param_combo_functor);

  const auto& covered_tuples(per_param_combo_functor.get_coverered_tuples());

  std::vector<unsigned long long> cumulative_covered_tuples(
      test_set.get_list_of_tests().size(), 0);

  for (unsigned int test_index = 0;
       test_index < cumulative_covered_tuples.size(); ++test_index) {

    for (const auto& thread_local_covered_tuples : covered_tuples) {
      cumulative_covered_tuples[test_index] +=
          thread_local_covered_tuples.value[test_index];
    }

    if (test_index > 0) {
      cumulative_covered_tuples[test_index] =
          cumulative_covered_tuples[test_index] +
          cumulative_covered_tuples[test_index - 1];
    }
  }

  covm.set_coverered_tuples(std::move(cumulative_covered_tuples));

  const auto& cov_level_to_num_param_combos(
      per_param_combo_functor.get_cov_level_to_num_param_combos());

  citcpp::coverage_measurement::t_coverage_level_to_num_param_combos
      cumulative_cov_level_to_num_param_combos;

  for (const auto& thread_local_cov_level_to_num_param_combos :
       cov_level_to_num_param_combos) {

    for (unsigned int i = 0;
         i < thread_local_cov_level_to_num_param_combos.value.size(); ++i) {

      cumulative_cov_level_to_num_param_combos[i] +=
          thread_local_cov_level_to_num_param_combos.value[i];
    }
  }

  covm.set_coverage_level_to_num_param_combos(
      std::move(cumulative_cov_level_to_num_param_combos));
}

}  // namespace detail
}  // namespace citcpp