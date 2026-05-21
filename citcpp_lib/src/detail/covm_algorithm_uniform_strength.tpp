#include <functional>
#include <mutex>

#include "bitset.hpp"
#include "citcpp_utils.hpp"
#include "covm_algorithm_uniform_strength.hpp"
#include "param_combo_iteration.hpp"
#include "shared_aux_types.hpp"
#include "shared_constants.hpp"

namespace citcpp {
namespace detail {

class test_validity_checker_sequential {
  public:
    test_validity_checker_sequential(const constraint_handler& constr_handler)
        : constr_handler_(constr_handler) {}

    bool operator()(const test& t) const {
      return constr_handler_.is_valid_partial_test(t);
    }

  private:
    const constraint_handler& constr_handler_;
};

class test_validity_checker_parallel {
  public:
    test_validity_checker_parallel(const constraint_handler& constr_handler)
        : constr_handler_(constr_handler), mut_() {}

    bool operator()(const test& t) const {
      if (constr_handler_.is_thread_safe()) {
        return check_concurrently(t);
      } else {
        return check_mutally_excluded(t);
      }
    }

  private:
    bool check_mutally_excluded(const test& t) const {
      std::lock_guard<std::mutex> guard(mut_);
      const bool is_valid = constr_handler_.is_valid_partial_test(t);
      return is_valid;
    }

    bool check_concurrently(const test& t) const {
      return constr_handler_.is_valid_partial_test(t);
    }

  private:
    const constraint_handler& constr_handler_;
    mutable std::mutex mut_;
};

template <typename T_PARTIAL_TEST_VALIDITY_PRED>
class covm_per_param_combo_functor {
  public:
    covm_per_param_combo_functor(
        const unsigned int strength, const internal_model& model,
        const internal_test_set& test_set,
        const T_PARTIAL_TEST_VALIDITY_PRED& validity_pred,
        const unsigned int bitset_backing_array_size,
        covm_exec_handle_impl& exec_handle)
        : model_(model),
          test_set_(test_set),
          validity_pred_(validity_pred),
          value_indices_(strength),
          scratch_test_(model.get_parameter_num_values().size(), -1),
          bitset_backing_array_(bitset_backing_array_size),
          num_invalid_tuples_(0),
          covered_tuples_(test_set.get_list_of_tests().size(), 0),
          cov_level_to_num_param_combos_{0},
          exec_handle_(exec_handle) {}

    bool operator()(const param_vector& param_indices) {
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

      if (!values_combo_bitset.all()) {
        visit_all_value_combos_of_param_combo(
            model_, param_indices, value_indices_, *this, param_indices,
            values_combo_bitset);
        reset_scratch_test(param_indices);
      }

      double param_coverage_fraction = (double)values_combo_bitset.count() /
                                       (double)values_combo_bitset.size();

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
        cov_level_to_num_param_combos_[index]++;
      }

      exec_handle_.add_number_of_processed_combinations(
          values_combo_bitset.size());

      if (exec_handle_.is_job_aborted()) {
        return false;
      }

      return true;
    }

    bool operator()(const value_vector& value_indices,
                    bitset_non_owning_uint64::size_type bitpos,
                    const param_vector& param_indices,
                    bitset_non_owning_uint64& values_combo_bitset) {

      if (!values_combo_bitset.test(bitpos)) {
        const bool valid_tuple = is_valid_tuple(param_indices, value_indices);
        if (!valid_tuple) {
          values_combo_bitset.set(bitpos);
          ++num_invalid_tuples_;
        }
      }

      return !values_combo_bitset.all();
    }

    unsigned long long get_num_invalid_tuples() const {
      return num_invalid_tuples_;
    }

    const std::vector<unsigned long long>& get_coverered_tuples() const {
      return covered_tuples_;
    }

    const citcpp::coverage_measurement::t_coverage_level_to_num_param_combos&
    get_coverage_level_to_num_param_combos() const {

      return cov_level_to_num_param_combos_;
    }

  private:
    bool is_valid_tuple(const param_vector& param_indices,
                        const value_vector& value_indices) {

      for (unsigned int i = 0; i < param_indices.size(); ++i) {
        const unsigned int param_idx = param_indices[i];
        const int param_value_to_cover = value_indices[i];
        scratch_test_.get_values()[param_idx] = param_value_to_cover;
      }

      bool res = validity_pred_(scratch_test_);

      return res;
    }

    void reset_scratch_test(const param_vector& param_indices) {
      for (unsigned int i = 0; i < param_indices.size(); ++i) {
        const unsigned int param_idx = param_indices[i];
        scratch_test_.get_values()[param_idx] = -1;
      }
    }

  private:
    const internal_model& model_;
    const internal_test_set& test_set_;
    const T_PARTIAL_TEST_VALIDITY_PRED& validity_pred_;
    value_vector value_indices_;
    test scratch_test_;
    array_wrapper_uint64 bitset_backing_array_;
    unsigned long long num_invalid_tuples_;
    std::vector<unsigned long long> covered_tuples_;
    citcpp::coverage_measurement::t_coverage_level_to_num_param_combos
        cov_level_to_num_param_combos_;
    covm_exec_handle_impl& exec_handle_;
};

inline void measure_coverage(
    const unsigned int strength, const internal_model& model,
    const std::vector<unsigned int>& parameter_index_map,
    const internal_test_set& test_set, const constraint_handler& constr_handler,
    covm_exec_handle_impl& exec_handle, citcpp::coverage_measurement& covm) {

  const unsigned int product_of_max_parameter_sizes =
      get_product_of_max_n_parameter_sizes(parameter_index_map.size(), strength,
                                           model, parameter_index_map);

  test_validity_checker_sequential validity_pred(constr_handler);
  covm_per_param_combo_functor<test_validity_checker_sequential>
      per_param_combo_functor(strength, model, test_set, validity_pred,
                              product_of_max_parameter_sizes, exec_handle);

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

  covm.set_number_of_combinations_to_cover(
      covm.get_number_of_combinations_to_cover() -
      per_param_combo_functor.get_num_invalid_tuples());
  covm.set_coverered_tuples(std::move(covered_tuples));
  covm.set_coverage_level_to_num_param_combos(
      per_param_combo_functor.get_coverage_level_to_num_param_combos());
}

template <conc_is_void_functor_executor T_EXEC>
void measure_coverage(const unsigned int strength, const internal_model& model,
                      const std::vector<unsigned int>& parameter_index_map,
                      const internal_test_set& test_set,
                      const constraint_handler& constr_handler,
                      covm_exec_handle_impl& exec_handle,
                      citcpp::coverage_measurement& covm, T_EXEC& exec) {

  const unsigned int product_of_max_parameter_sizes =
      get_product_of_max_n_parameter_sizes(parameter_index_map.size(), strength,
                                           model, parameter_index_map);

  test_validity_checker_parallel validity_pred(constr_handler);
  param_combo_functor_parallel_iterator<
      covm_per_param_combo_functor<test_validity_checker_parallel>, T_EXEC>
      per_param_combo_functor_parallel(
          parameter_index_map.size(), strength, parameter_index_map, false,
          exec, strength, std::cref(model), std::cref(test_set),
          std::cref(validity_pred), product_of_max_parameter_sizes,
          std::ref(exec_handle));

  per_param_combo_functor_parallel.visit_all_parameter_combinations();

  std::vector<unsigned long long> covered_tuples(
      test_set.get_list_of_tests().size(), 0);
  unsigned long long num_invalid_tuples = 0;
  citcpp::coverage_measurement::t_coverage_level_to_num_param_combos
      cov_level_to_num_param_combos{0};

  per_param_combo_functor_parallel.visit_all_functors(
      [&covered_tuples, &num_invalid_tuples, &cov_level_to_num_param_combos](
          const covm_per_param_combo_functor<test_validity_checker_parallel>&
              f) {
        for (unsigned int test_index = 0; test_index < covered_tuples.size();
             ++test_index) {
          covered_tuples[test_index] += f.get_coverered_tuples()[test_index];
        }

        num_invalid_tuples += f.get_num_invalid_tuples();

        for (unsigned int i = 0; i < cov_level_to_num_param_combos.size();
             ++i) {
          cov_level_to_num_param_combos[i] +=
              f.get_coverage_level_to_num_param_combos()[i];
        }
      });

  for (unsigned int test_index = 0; test_index < covered_tuples.size();
       ++test_index) {

    if (test_index > 0) {
      covered_tuples[test_index] =
          covered_tuples[test_index] + covered_tuples[test_index - 1];
    }
  }

  covm.set_number_of_combinations_to_cover(
      covm.get_number_of_combinations_to_cover() - num_invalid_tuples);
  covm.set_coverered_tuples(std::move(covered_tuples));
  covm.set_coverage_level_to_num_param_combos(cov_level_to_num_param_combos);
}

}  // namespace detail
}  // namespace citcpp