#include <bit>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>

#include "bitset.hpp"
#include "citcpp_utils.hpp"
#include "covm_algorithm_uniform_strength.hpp"
#include "param_combo_iteration.hpp"
#include "shared_constants.hpp"

namespace citcpp {
namespace detail {

class test_validity_checker_sequential {
  public:
    test_validity_checker_sequential(constraint_handler& constr_handler)
        : constr_handler_(constr_handler) {}

    // assigned: the parameters assigned in t (see
    // constraint_handler::is_valid_partial_test).
    bool operator()(const test& t, const param_vector& assigned) const {
      return constr_handler_.is_valid_partial_test(t, assigned);
    }

  private:
    constraint_handler& constr_handler_;
};

class test_validity_checker_parallel {
  public:
    test_validity_checker_parallel(constraint_handler& constr_handler)
        : constr_handler_(constr_handler) {}

    // assigned: the parameters assigned in t. Called by concurrent workers
    // of the Lace executor, which constraint_handler::is_valid_partial_test
    // allows.
    bool operator()(const test& t, const param_vector& assigned) const {
      return constr_handler_.is_valid_partial_test(t, assigned);
    }

  private:
    constraint_handler& constr_handler_;
};

// The test set in transposed form: per parameter value, the bitset of the
// tests having that value (num_words words per value, starting at word
// offsets[p] * num_words for the values of parameter p), and per parameter,
// its values in the tests (contiguous over the tests, so that a scan of the
// tests reads only the columns of the parameter combination). Built once per
// measurement and shared (read only) by all functors.
struct covm_test_columns {
    covm_test_columns(const internal_model& model,
                      const internal_test_set& test_set) {
      const std::vector<unsigned int>& num_values =
          model.get_parameter_num_values();
      offsets.assign(num_values.size(), 0);
      std::size_t num_columns = 0;
      for (std::size_t p = 0; p < num_values.size(); ++p) {
        offsets[p] = num_columns;
        num_columns += num_values[p];
      }
      num_tests = test_set.get_list_of_tests().size();
      num_words = (num_tests + 63) / 64;
      bits.assign(num_columns * num_words, 0);
      values.assign(num_values.size() * num_tests, DONT_CARE);
      std::size_t test_index = 0;
      for (const test& test : test_set.get_list_of_tests()) {
        for (std::size_t p = 0; p < num_values.size(); ++p) {
          const int value = test.get_values()[p];
          if (value >= 0) {
            bits[(offsets[p] + static_cast<std::size_t>(value)) * num_words +
                 test_index / 64] |= std::uint64_t(1) << (test_index % 64);
            values[p * num_tests + test_index] =
                static_cast<std::uint16_t>(value);
          }
        }
        ++test_index;
      }
    }

    // The values of parameter p in the tests (DONT_CARE for a don't care
    // value).
    const std::uint16_t* column(std::size_t p) const {
      return values.data() + p * num_tests;
    }

    // A parameter has at most 65535 values (see check_parameters_and_domains
    // in citcpp.cpp), so its values are 0 to DONT_CARE - 1.
    static constexpr std::uint16_t DONT_CARE = 0xFFFF;

    std::vector<std::size_t> offsets;
    std::size_t num_words = 0;
    std::vector<std::uint64_t> bits;
    std::size_t num_tests = 0;
    std::vector<std::uint16_t> values;
};

// The scratch tests (one value per parameter, all don't care while unused)
// for the validity checks of the functors. One per functor would need memory
// quadratic in the number of parameters (there is one functor per task, i.e.
// per parameter for t = 2); instead, a functor takes one from here for a
// parameter combination that needs validity checks, and returns it
// afterwards.
class covm_scratch_tests {
  public:
    explicit covm_scratch_tests(std::size_t num_params)
        : num_params_(num_params) {}

    test* acquire() {
      std::lock_guard<std::mutex> guard(mut_);
      if (free_.empty()) {
        all_.push_back(std::make_unique<test>(num_params_, -1));
        return all_.back().get();
      }
      test* t = free_.back();
      free_.pop_back();
      return t;
    }

    void release(test* t) {
      std::lock_guard<std::mutex> guard(mut_);
      free_.push_back(t);
    }

  private:
    std::size_t num_params_;
    std::mutex mut_;
    std::vector<std::unique_ptr<test>> all_;
    std::vector<test*> free_;
};

template <typename T_PARTIAL_TEST_VALIDITY_PRED>
class covm_per_param_combo_functor {
  public:
    covm_per_param_combo_functor(
        unsigned int strength, const internal_model& model,
        const internal_test_set& test_set,
        const covm_test_columns& test_columns,
        covm_scratch_tests& scratch_tests,
        const T_PARTIAL_TEST_VALIDITY_PRED& validity_pred,
        bitset_uint64::size_type bitset_backing_array_size,
        covm_exec_handle_impl& exec_handle)
        : model_(model),
          test_set_(test_set),
          test_columns_(test_columns),
          scratch_tests_(&scratch_tests),
          validity_pred_(validity_pred),
          value_indices_(strength),
          weights_(strength),
          values_combo_bitset_(bitset_backing_array_size),
          num_invalid_tuples_(0),
          covered_tuples_(test_set.get_list_of_tests().size(), 0),
          cov_level_to_num_param_combos_{0},
          exec_handle_(exec_handle),
          and_buffers_((strength - 1) * test_columns.num_words, 0) {}

    bool operator()(const param_vector& param_indices) {
      bitset_uint64::size_type bitset_size = 1;
      for (auto p : param_indices) {
        bitset_size *= static_cast<bitset_uint64::size_type>(
            model_.get_parameter_num_values()[p]);
      }
      values_combo_bitset_.reset_with_new_size(bitset_size);

      // Pre-calculate weights for index computation
      bitset_uint64::size_type weight = 1;
      for (int i = static_cast<int>(param_indices.size() - 1); i >= 0; --i) {
        weights_[i] = weight;
        weight *= static_cast<bitset_uint64::size_type>(
            model_.get_parameter_num_values()[param_indices[i]]);
      }

      if (use_test_columns(param_indices)) {
        mark_covered_tuples_by_test_columns(param_indices, 0, nullptr, 0);
      } else {
        mark_covered_tuples_by_test_scan(param_indices);
      }

      if (!values_combo_bitset_.all()) {
        visit_all_value_combos_of_param_combo(
            model_, param_indices, value_indices_, *this, param_indices);
        reset_scratch_test(param_indices);
      }

      double param_coverage_fraction =
          static_cast<double>(values_combo_bitset_.count()) /
          static_cast<double>(values_combo_bitset_.size());

      param_coverage_fraction =
          std::max(std::min(param_coverage_fraction, 1.0), 0.0);

      // Map the coverage fraction to the appropriate array index.
      int index = std::min(
          static_cast<unsigned int>(
              static_cast<double>(citcpp::coverage_measurement::
                                      NUM_DIFFERENTIATED_COVERAGE_LEVELS -
                                  1) *
              param_coverage_fraction),
          citcpp::coverage_measurement::NUM_DIFFERENTIATED_COVERAGE_LEVELS - 1);

      for (; index >= 0; --index) {
        cov_level_to_num_param_combos_[index]++;
      }

      exec_handle_.add_number_of_processed_combinations(
          values_combo_bitset_.size());

      if (exec_handle_.is_job_aborted()) {
        return false;
      }

      return true;
    }

    bool operator()(const value_vector& value_indices,
                    bitset_uint64::size_type bitpos,
                    const param_vector& param_indices) {

      if (!values_combo_bitset_.test(bitpos)) {
        const bool valid_tuple = is_valid_tuple(param_indices, value_indices);
        if (!valid_tuple) {
          values_combo_bitset_.set(bitpos);
          ++num_invalid_tuples_;
        }
      }

      return !values_combo_bitset_.all();
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
    // Here we compute an index into the bitset. To do so, we treat the
    // number of values of each parameter as a kind of radix. Consider three
    // parameters p_0, p_1, p_2. Now say that v_i is the number of values for
    // p_i. If we now have values x_0, x_1, x_2, then the index is
    // x_0 * v_1 * v_2 + x_1 * v_2 + x_2.
    void mark_covered_tuples_by_test_scan(const param_vector& param_indices) {
      // The values are read from the columns of the parameters (contiguous
      // over the tests).
      const std::size_t strength = param_indices.size();
      small_vector<const std::uint16_t*> columns(strength);
      for (std::size_t i = 0; i < strength; ++i) {
        columns[i] = test_columns_.column(param_indices[i]);
      }
      for (std::size_t test_index = 0; test_index < test_columns_.num_tests;
           ++test_index) {
        bitset_uint64::size_type index = 0;
        bool found_dont_care = false;
        for (std::size_t i = 0; i < strength; ++i) {
          const std::uint16_t param_value = columns[i][test_index];
          if (param_value == covm_test_columns::DONT_CARE) {
            // A don't care value: the test covers no tuple of the
            // combination.
            found_dont_care = true;
            break;
          }
          index +=
              static_cast<bitset_uint64::size_type>(param_value) * weights_[i];
        }
        if (!found_dont_care) {
          if (!values_combo_bitset_.test_and_set(index)) {
            covered_tuples_[test_index]++;
          }
        }
      }
    }

    // Decides whether the covered tuples of the parameter combination are
    // determined from the test columns (cost bounded by the number of value
    // prefixes times the column length) or by scanning the tests.
    bool use_test_columns(const param_vector& param_indices) const {
      const std::size_t num_tests = test_set_.get_list_of_tests().size();
      unsigned long long prefixes = 1;
      unsigned long long column_cost = 0;
      for (auto p : param_indices) {
        prefixes *= model_.get_parameter_num_values()[p];
        column_cost += prefixes * test_columns_.num_words;
      }
      return column_cost <= param_indices.size() * num_tests;
    }

    // Marks the tuples covered by the tests, level by level: and_buffers_
    // holds, per level but the last (which only looks for the lowest test),
    // the tests having the values chosen so far. The first covering test of a
    // tuple is the lowest test in the conjunction.
    void mark_covered_tuples_by_test_columns(const param_vector& param_indices,
                                             std::size_t level,
                                             const std::uint64_t* prefix,
                                             bitset_uint64::size_type index) {
      const std::size_t words = test_columns_.num_words;
      const unsigned int num_values =
          model_.get_parameter_num_values()[param_indices[level]];
      const std::uint64_t* columns =
          test_columns_.bits.data() +
          test_columns_.offsets[param_indices[level]] * words;
      const bool last_level = level + 1 == param_indices.size();
      std::uint64_t* conjunction = and_buffers_.data() + level * words;
      for (unsigned int x = 0; x < num_values; ++x) {
        const std::uint64_t* column = columns + x * words;
        const bitset_uint64::size_type value_index =
            index + static_cast<bitset_uint64::size_type>(x) * weights_[level];
        if (last_level) {
          for (std::size_t w = 0; w < words; ++w) {
            const std::uint64_t bits = prefix ? (prefix[w] & column[w])
                                              : column[w];
            if (bits) {
              values_combo_bitset_.set(value_index);
              covered_tuples_[w * 64 + static_cast<std::size_t>(
                                           std::countr_zero(bits))]++;
              break;
            }
          }
        } else {
          std::uint64_t any = 0;
          for (std::size_t w = 0; w < words; ++w) {
            conjunction[w] = prefix ? (prefix[w] & column[w]) : column[w];
            any |= conjunction[w];
          }
          if (any) {
            mark_covered_tuples_by_test_columns(param_indices, level + 1,
                                                conjunction, value_index);
          }
        }
      }
    }

    bool is_valid_tuple(const param_vector& param_indices,
                        const value_vector& value_indices) {
      if (!scratch_test_) {
        scratch_test_ = scratch_tests_->acquire();
      }
      for (std::size_t i = 0; i < param_indices.size(); ++i) {
        const int param_value_to_cover = value_indices[i];
        scratch_test_->get_values()[param_indices[i]] = param_value_to_cover;
      }

      // The scratch test has exactly the values of the tuple.
      bool res = validity_pred_(*scratch_test_, param_indices);

      return res;
    }

    // Returns the scratch test (if one was taken), with its values reset.
    void reset_scratch_test(const param_vector& param_indices) {
      if (!scratch_test_) {
        return;
      }
      for (std::size_t i = 0; i < param_indices.size(); ++i) {
        scratch_test_->get_values()[param_indices[i]] = -1;
      }
      scratch_tests_->release(scratch_test_);
      scratch_test_ = nullptr;
    }

  private:
    const internal_model& model_;
    const internal_test_set& test_set_;
    const covm_test_columns& test_columns_;
    covm_scratch_tests* scratch_tests_;
    // Taken from scratch_tests_ for the current parameter combination.
    test* scratch_test_ = nullptr;
    const T_PARTIAL_TEST_VALIDITY_PRED& validity_pred_;
    value_vector value_indices_;
    std::vector<bitset_uint64::size_type> weights_;
    bitset_uint64 values_combo_bitset_;
    unsigned long long num_invalid_tuples_;
    std::vector<unsigned long long> covered_tuples_;
    citcpp::coverage_measurement::t_coverage_level_to_num_param_combos
        cov_level_to_num_param_combos_;
    covm_exec_handle_impl& exec_handle_;
    std::vector<std::uint64_t> and_buffers_;
};

inline void measure_coverage(
    unsigned int strength, const internal_model& model,
    const std::vector<unsigned int>& parameter_index_map,
    const internal_test_set& test_set, constraint_handler& constr_handler,
    covm_exec_handle_impl& exec_handle, citcpp::coverage_measurement& covm) {

  const unsigned int product_of_max_parameter_sizes =
      get_product_of_max_n_parameter_sizes(
          static_cast<unsigned int>(parameter_index_map.size()), strength,
          model, parameter_index_map);

  test_validity_checker_sequential validity_pred(constr_handler);
  const covm_test_columns test_columns(model, test_set);
  covm_scratch_tests scratch_tests(model.get_parameter_num_values().size());
  covm_per_param_combo_functor<test_validity_checker_sequential>
      per_param_combo_functor(strength, model, test_set, test_columns,
                              scratch_tests, validity_pred,
                              product_of_max_parameter_sizes, exec_handle);

  param_combo_iterator param_combo_it(
      static_cast<unsigned int>(parameter_index_map.size()), strength,
      parameter_index_map, false);
  param_combo_it.visit_all_parameter_combinations(per_param_combo_functor);

  std::vector<unsigned long long> covered_tuples(
      per_param_combo_functor.get_coverered_tuples());
  for (std::size_t test_index = 0; test_index < covered_tuples.size();
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
void measure_coverage(unsigned int strength, const internal_model& model,
                      const std::vector<unsigned int>& parameter_index_map,
                      const internal_test_set& test_set,
                      constraint_handler& constr_handler,
                      covm_exec_handle_impl& exec_handle,
                      citcpp::coverage_measurement& covm, T_EXEC& exec) {

  const unsigned int product_of_max_parameter_sizes =
      get_product_of_max_n_parameter_sizes(
          static_cast<unsigned int>(parameter_index_map.size()), strength,
          model, parameter_index_map);

  test_validity_checker_parallel validity_pred(constr_handler);
  // One functor per task (many), all sharing the transposed test set and
  // the scratch tests.
  const covm_test_columns test_columns(model, test_set);
  covm_scratch_tests scratch_tests(model.get_parameter_num_values().size());
  param_combo_functor_parallel_iterator<
      covm_per_param_combo_functor<test_validity_checker_parallel>, T_EXEC>
      per_param_combo_functor_parallel(
          static_cast<unsigned int>(parameter_index_map.size()), strength,
          parameter_index_map, false, exec, strength, std::cref(model),
          std::cref(test_set), std::cref(test_columns),
          std::ref(scratch_tests), std::cref(validity_pred),
          product_of_max_parameter_sizes, std::ref(exec_handle));

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
        for (std::size_t test_index = 0; test_index < covered_tuples.size();
             ++test_index) {
          covered_tuples[test_index] += f.get_coverered_tuples()[test_index];
        }

        num_invalid_tuples += f.get_num_invalid_tuples();

        for (std::size_t i = 0; i < cov_level_to_num_param_combos.size(); ++i) {
          cov_level_to_num_param_combos[i] +=
              f.get_coverage_level_to_num_param_combos()[i];
        }
      });

  for (std::size_t test_index = 0; test_index < covered_tuples.size();
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