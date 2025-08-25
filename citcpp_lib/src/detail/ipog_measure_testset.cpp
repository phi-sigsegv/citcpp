#include "ipog_measure_testset.hpp"

#include "citcpp_utils.hpp"

namespace {

class ipog_measure_per_param_combo_functor {
  public:
    ipog_measure_per_param_combo_functor(
        const citcpp::detail::model &model,
        const citcpp::detail::internal_test_set &test_set,
        citcpp::detail::coverage_map &cov_map,
        citcpp::detail::cagen_exec_handle_base &exec_handle)
        : model_(model),
          test_set_(test_set),
          cov_map_(cov_map),
          exec_handle_(exec_handle),
          num_covered_tuples_(0) {}

    bool operator()(
        citcpp::detail::coverage_map::second_level_type &value_combinations) {
      using namespace citcpp::detail;

      const param_vector &param_indices =
          value_combinations.get_parameter_indices();

      for (const test &test : test_set_.get_list_of_tests()) {
        // Here we compute an index into the bitset. To do so, we treat the
        // number of values of each parameter as a kind of radix. Consider
        // three parameters p_0, p_1, p_2. Now say that v_i is the number of
        // values for p_i. If we now have values x_0, x_1, x_2, then the index
        // is x_0 * v_1 * v_2 + x_1 * v_2 + x_2.
        coverage_map::second_level_type::size_type index = 0;
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

          coverage_map::second_level_type::size_type addend = param_value;
          for (std::vector<unsigned int>::size_type j = i + 1;
               j < param_indices.size(); ++j) {
            addend *= model_.get_parameters()[param_indices[j]];
          }
          index += addend;
        }

        if (!found_dont_care) {
          if (!value_combinations.test_and_set(index)) {
            ++num_covered_tuples_;
          }
        }
      }

      if (exec_handle_.is_job_aborted()) {
        return false;
      }

      return true;
    }

    unsigned long long get_num_covered_tuples() const {
      return num_covered_tuples_;
    }

  private:
    const citcpp::detail::model &model_;
    const citcpp::detail::internal_test_set &test_set_;
    citcpp::detail::coverage_map &cov_map_;
    citcpp::detail::cagen_exec_handle_base &exec_handle_;
    unsigned long long num_covered_tuples_;
};

class ipog_measure_per_param_combo_functor_parallel {
  public:
    ipog_measure_per_param_combo_functor_parallel(
        const citcpp::detail::model &model,
        const citcpp::detail::internal_test_set &test_set,
        citcpp::detail::coverage_map &cov_map,
        citcpp::detail::cagen_exec_handle_base &exec_handle,
        const citcpp::detail::coverage_map_parallel_iterator &cov_map_it)
        : model_(model),
          test_set_(test_set),
          cov_map_(cov_map),
          exec_handle_(exec_handle),
          cov_map_it_(cov_map_it),
          num_covered_tuples_() {}

    bool operator()(
        citcpp::detail::coverage_map::second_level_type &value_combinations) {
      using namespace citcpp::detail;

      const param_vector &param_indices =
          value_combinations.get_parameter_indices();

      auto &thread_local_num_covered_tuples_ =
          num_covered_tuples_[cov_map_it_.get_worker_id()].value;

      for (const test &test : test_set_.get_list_of_tests()) {
        // Here we compute an index into the bitset. To do so, we treat the
        // number of values of each parameter as a kind of radix. Consider
        // three parameters p_0, p_1, p_2. Now say that v_i is the number of
        // values for p_i. If we now have values x_0, x_1, x_2, then the index
        // is x_0 * v_1 * v_2 + x_1 * v_2 + x_2.
        coverage_map::second_level_type::size_type index = 0;
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

          coverage_map::second_level_type::size_type addend = param_value;
          for (std::vector<unsigned int>::size_type j = i + 1;
               j < param_indices.size(); ++j) {
            addend *= model_.get_parameters()[param_indices[j]];
          }
          index += addend;
        }

        if (!found_dont_care) {
          if (!value_combinations.test_and_set(index)) {
            ++thread_local_num_covered_tuples_;
          }
        }
      }

      if (exec_handle_.is_job_aborted()) {
        return false;
      }

      return true;
    }

    unsigned long long get_num_covered_tuples() const {
      unsigned long long res = 0;

      for (const auto &thread_local_value : num_covered_tuples_) {
        res += thread_local_value.value;
      }

      return res;
    }

  private:
    const citcpp::detail::model &model_;
    const citcpp::detail::internal_test_set &test_set_;
    citcpp::detail::coverage_map &cov_map_;
    citcpp::detail::cagen_exec_handle_base &exec_handle_;
    const citcpp::detail::coverage_map_parallel_iterator &cov_map_it_;
    citcpp::detail::thread_local_vector<citcpp::detail::aligned_ull_value>
        num_covered_tuples_;
};

}  // namespace

namespace citcpp {
namespace detail {

ipog_measure_testset_result ipog_measure_testset(
    const model &model, const internal_test_set &test_set,
    coverage_map &cov_map, cagen_exec_handle_base &exec_handle) {

  // First initialize the result object.
  ipog_measure_testset_result result = {0};

  coverage_map_iterator cov_map_it = cov_map.create_iterator();

  ipog_measure_per_param_combo_functor per_param_combo_functor(
      model, test_set, cov_map, exec_handle);

  cov_map_it.visit_all_parameter_combinations(per_param_combo_functor);

  result.num_covered_tuples = per_param_combo_functor.get_num_covered_tuples();

  return result;
}

ipog_measure_testset_result ipog_measure_testset(
    const model &model, const internal_test_set &test_set,
    coverage_map &cov_map, cagen_exec_handle_base &exec_handle,
    thread_pool &tp) {

  // First initialize the result object.
  ipog_measure_testset_result result = {0};

  coverage_map_parallel_iterator cov_map_it =
      cov_map.create_parallel_iterator(tp);

  ipog_measure_per_param_combo_functor_parallel per_param_combo_functor(
      model, test_set, cov_map, exec_handle, cov_map_it);

  cov_map_it.visit_all_parameter_combinations(per_param_combo_functor);

  result.num_covered_tuples = per_param_combo_functor.get_num_covered_tuples();

  return result;
}

}  // namespace detail
}  // namespace citcpp