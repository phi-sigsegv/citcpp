#include "citcpp_utils.hpp"
#include "ipog_measure_testset.hpp"

namespace citcpp {
namespace detail {

inline void measure_coverage(
    ipog_coverage_map::second_level_type& value_combinations,
    const internal_model& model, const internal_test_set& test_set,
    unsigned long long& num_covered_tuples) {

  const param_vector& param_indices =
      value_combinations.get_parameter_indices();

  for (const test& test : test_set.get_list_of_tests()) {
    // Here we compute an index into the bitset. To do so, we treat the
    // number of values of each parameter as a kind of radix. Consider
    // three parameters p_0, p_1, p_2. Now say that v_i is the number of
    // values for p_i. If we now have values x_0, x_1, x_2, then the index
    // is x_0 * v_1 * v_2 + x_1 * v_2 + x_2.
    ipog_coverage_map::second_level_type::size_type index = 0;
    bool found_dont_care = false;
    for (std::vector<unsigned int>::size_type i = 0; i < param_indices.size();
         ++i) {
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

      ipog_coverage_map::second_level_type::size_type addend = param_value;
      for (std::vector<unsigned int>::size_type j = i + 1;
           j < param_indices.size(); ++j) {
        addend *= model.get_parameter_num_values()[param_indices[j]];
      }
      index += addend;
    }

    if (!found_dont_care) {
      if (!value_combinations.test_and_set(index)) {
        ++num_covered_tuples;
      }
    }
  }
}

class ipog_measure_per_param_combo_functor {
  public:
    ipog_measure_per_param_combo_functor(const internal_model& model,
                                         const internal_test_set& test_set)
        : model_(model), test_set_(test_set), num_covered_tuples_(0) {}

    void operator()(ipog_coverage_map::second_level_type& value_combinations) {
      measure_coverage(value_combinations, model_, test_set_,
                       num_covered_tuples_);
    }

    unsigned long long get_num_covered_tuples() const {
      return num_covered_tuples_;
    }

    void reset() { num_covered_tuples_ = 0; }

  private:
    const internal_model& model_;
    const internal_test_set& test_set_;
    unsigned long long num_covered_tuples_;
};

template <conc_is_void_functor_executor T_EXEC>
class alignas(false_sharing_avoidance_alignment)
    ipog_measure_per_param_combo_functor_parallel
    : public functor_task_base<
          ipog_measure_per_param_combo_functor_parallel<T_EXEC>> {

  private:
    typedef functor_task_base<
        ipog_measure_per_param_combo_functor_parallel<T_EXEC>>
        base_type;

  public:
    ipog_measure_per_param_combo_functor_parallel(
        const internal_model& model, const internal_test_set& test_set)
        : per_param_combo_functor_(model, test_set),
          cov_map_(nullptr),
          start_index_(0),
          end_index_(0) {}

    void operator()() {
      auto& cov_map = cov_map_->get_coverage_map();

      for (unsigned long long i = start_index_; i < end_index_; ++i) {
        per_param_combo_functor_(cov_map[i]);
      }
    }

    void set_task_parameters(ipog_coverage_map* cov_map,
                             unsigned long long start_index,
                             unsigned long long end_index) {
      base_type::reset();
      cov_map_ = cov_map;
      start_index_ = start_index;
      end_index_ = end_index;
    }

    unsigned long long get_num_covered_tuples() const {
      return per_param_combo_functor_.get_num_covered_tuples();
    }

    void reset() { per_param_combo_functor_.reset(); }

  private:
    ipog_measure_per_param_combo_functor per_param_combo_functor_;
    ipog_coverage_map* cov_map_;
    unsigned long long start_index_;
    unsigned long long end_index_;
};

inline ipog_measure_testset_result ipog_measure_testset(
    const internal_model& model, const internal_test_set& test_set,
    std::vector<std::pair<const internal_relation*, ipog_coverage_map>>&
        relations) {

  // First initialize the result object.
  ipog_measure_testset_result result;

  ipog_measure_per_param_combo_functor per_param_combo_functor(model, test_set);

  for (auto& rel_and_cov_map : relations) {
    per_param_combo_functor.reset();
    for (auto& value_combinations : rel_and_cov_map.second.get_coverage_map()) {
      per_param_combo_functor(value_combinations);
    }

    result.num_covered_tuples[rel_and_cov_map.first] =
        per_param_combo_functor.get_num_covered_tuples();
  }

  return result;
}

template <conc_is_void_functor_executor T_EXEC>
ipog_measure_testset_result ipog_measure_testset(
    const internal_model& model, const internal_test_set& test_set,
    std::vector<std::pair<const internal_relation*, ipog_coverage_map>>&
        relations,
    T_EXEC& exec) {

  // First initialize the result object.
  ipog_measure_testset_result result;

  thread_local_vector<ipog_measure_per_param_combo_functor_parallel<T_EXEC>>
      thread_local_functors(exec.get_num_workers() * 8, {model, test_set});

  for (auto& rel_and_cov_map : relations) {
    ipog_coverage_map* cov_map = &rel_and_cov_map.second;
    const unsigned long long total_param_combos =
        cov_map->get_coverage_map().size();
    const unsigned long long num_tasks = std::min(
        (unsigned long long)thread_local_functors.size(), total_param_combos);
    const unsigned long long per_task_combos = total_param_combos / num_tasks;

    {
      auto exec_scope(exec.create_execution_scope());
      for (unsigned long long i = 0; i < num_tasks - 1; ++i) {
        auto& thread_local_func = thread_local_functors[i];
        thread_local_func.set_task_parameters(cov_map, per_task_combos * i,
                                              per_task_combos * (i + 1));
        exec_scope.spawn_execution(thread_local_func);
      }
      {
        auto& thread_local_func = thread_local_functors[num_tasks - 1];
        thread_local_func.set_task_parameters(
            cov_map, per_task_combos * (num_tasks - 1), total_param_combos);
        exec_scope.spawn_execution(thread_local_func);
      }
    }

    for (unsigned long long i = 0; i < num_tasks; ++i) {
      auto& thread_local_functor = thread_local_functors[i];
      result.num_covered_tuples[rel_and_cov_map.first] +=
          thread_local_functor.get_num_covered_tuples();

      thread_local_functor.reset();
    }
  }

  return result;
}

}  // namespace detail
}  // namespace citcpp