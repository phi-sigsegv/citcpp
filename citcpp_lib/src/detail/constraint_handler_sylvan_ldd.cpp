#include "constraint_handler_sylvan_ldd.hpp"

#include <lace.h>

#include <mutex>
#include <numeric>

#include "coverage_map.hpp"
#include "datatypes_config.hpp"

namespace {

template <typename T_DD>
struct true_false_dd_trait {};

template <>
struct true_false_dd_trait<citcpp::detail::sylvan_idd> {
    static citcpp::detail::sylvan_idd true_dd() {
      return citcpp::detail::sylvan_idd::iddTrue();
    }
    static citcpp::detail::sylvan_idd false_dd() {
      return citcpp::detail::sylvan_idd::iddFalse();
    }
};

template <typename T_DD>
class constraint_to_xdd_visitor {
  public:
    constraint_to_xdd_visitor(
        const citcpp::detail::internal_model& model,
        const std::vector<unsigned int>& parameter_to_level)
        : model_(model), param_to_level_(), negate_(false) {

      int idx = 0;
      for (const auto& param : model_.get_input_model().get_parameters()) {
        parameter_name_map_[param.get_name()] = &param;
        param_to_level_.emplace(param.get_name(), parameter_to_level[idx]);
        ++idx;
      }
    }

    T_DD operator()(const citcpp::boolean_proposition& prop) {

      using namespace citcpp::detail;
      using namespace citcpp;

      const parameter& param =
          *parameter_name_map_.at(prop.get_parameter().get_name());
      const int param_level = param_to_level_.at(param.get_name());
      const int domain_size = param.get_values().size();

      int value_index = 0;
      for (const auto& value : param.get_values()) {
        bool value_as_bool = value;
        if (value_as_bool == prop.get_compared_value()) {
          break;
        }
        ++value_index;
      }

      switch (prop.get_operator()) {
        case relational_operator::EQ:
          if (value_index >= param.get_values().size()) {
            return negate_ ? true_false_dd_trait<T_DD>::true_dd()
                           : true_false_dd_trait<T_DD>::false_dd();
          } else {
            return T_DD(
                param_level,
                negate_ ? relational_operator::NEQ : relational_operator::EQ,
                value_index, domain_size);
          }
        default:
          if (value_index >= param.get_values().size()) {
            return negate_ ? true_false_dd_trait<T_DD>::false_dd()
                           : true_false_dd_trait<T_DD>::true_dd();
          } else {
            return T_DD(
                param_level,
                negate_ ? relational_operator::EQ : relational_operator::NEQ,
                value_index, domain_size);
          }
      }
    }

    T_DD operator()(const citcpp::enum_proposition& prop) {

      using namespace citcpp::detail;
      using namespace citcpp;

      const parameter& param =
          *parameter_name_map_.at(prop.get_parameter().get_name());
      const int param_level = param_to_level_.at(param.get_name());
      const int domain_size = param.get_values().size();

      int value_index = 0;
      for (const auto& value : param.get_values()) {
        const std::string& value_as_string = value;
        if (value_as_string == prop.get_compared_value()) {
          break;
        }
        ++value_index;
      }

      switch (prop.get_operator()) {
        case relational_operator::EQ:
          if (value_index >= param.get_values().size()) {
            return negate_ ? true_false_dd_trait<T_DD>::true_dd()
                           : true_false_dd_trait<T_DD>::false_dd();
          } else {
            return T_DD(
                param_level,
                negate_ ? relational_operator::NEQ : relational_operator::EQ,
                value_index, domain_size);
          }
        default:
          if (value_index >= param.get_values().size()) {
            return negate_ ? true_false_dd_trait<T_DD>::false_dd()
                           : true_false_dd_trait<T_DD>::true_dd();
          } else {
            return T_DD(
                param_level,
                negate_ ? relational_operator::EQ : relational_operator::NEQ,
                value_index, domain_size);
          }
      }
    }

    T_DD operator()(const citcpp::int_proposition& prop) const {

      using namespace citcpp::detail;
      using namespace citcpp;

      const parameter& param =
          *parameter_name_map_.at(prop.get_parameter().get_name());
      const int param_level = param_to_level_.at(param.get_name());
      const int domain_size = param.get_values().size();

      switch (prop.get_operator()) {
        case relational_operator::EQ: {
          for (int v = 0; v < param.get_values().size(); ++v) {
            int value_as_int = param.get_values()[v];
            if (value_as_int == prop.get_compared_value()) {
              return T_DD(
                  param_level,
                  negate_ ? relational_operator::NEQ : relational_operator::EQ,
                  v, domain_size);
            }
          }

          return negate_ ? true_false_dd_trait<T_DD>::true_dd()
                         : true_false_dd_trait<T_DD>::false_dd();
        }
        case relational_operator::LE: {
          if ((int)param.get_values()[param.get_values().size() - 1] <=
              prop.get_compared_value()) {
            return negate_ ? true_false_dd_trait<T_DD>::false_dd()
                           : true_false_dd_trait<T_DD>::true_dd();
          }

          for (int v = param.get_values().size() - 1; v >= 0; --v) {
            int value_as_int = param.get_values()[v];
            if (value_as_int <= prop.get_compared_value()) {
              return T_DD(
                  param_level,
                  negate_ ? relational_operator::GT : relational_operator::LE,
                  v, domain_size);
            }
          }

          return negate_ ? true_false_dd_trait<T_DD>::true_dd()
                         : true_false_dd_trait<T_DD>::false_dd();
        }
        case relational_operator::LT: {
          if ((int)param.get_values()[param.get_values().size() - 1] <
              prop.get_compared_value()) {
            return negate_ ? true_false_dd_trait<T_DD>::false_dd()
                           : true_false_dd_trait<T_DD>::true_dd();
          }

          for (int v = param.get_values().size() - 1; v >= 0; --v) {
            int value_as_int = param.get_values()[v];
            if (value_as_int < prop.get_compared_value()) {
              // We create and LDD, which represents that X <= value_as_int,
              // which means that X <= value_as_int < a,
              // where a is the upper bound from the proposition.
              return T_DD(
                  param_level,
                  negate_ ? relational_operator::GT : relational_operator::LE,
                  v, domain_size);
            }
          }

          return negate_ ? true_false_dd_trait<T_DD>::true_dd()
                         : true_false_dd_trait<T_DD>::false_dd();
        }
        case relational_operator::GE: {
          if ((int)param.get_values()[0] >= prop.get_compared_value()) {
            return negate_ ? true_false_dd_trait<T_DD>::false_dd()
                           : true_false_dd_trait<T_DD>::true_dd();
          }

          for (int v = 0; v < param.get_values().size(); ++v) {
            int value_as_int = param.get_values()[v];
            if (value_as_int >= prop.get_compared_value()) {
              return T_DD(
                  param_level,
                  negate_ ? relational_operator::LT : relational_operator::GE,
                  v, domain_size);
            }
          }

          return negate_ ? true_false_dd_trait<T_DD>::true_dd()
                         : true_false_dd_trait<T_DD>::false_dd();
        }
        case relational_operator::GT: {
          if ((int)param.get_values()[0] > prop.get_compared_value()) {
            return negate_ ? true_false_dd_trait<T_DD>::false_dd()
                           : true_false_dd_trait<T_DD>::true_dd();
          }

          for (int v = 0; v < param.get_values().size(); ++v) {
            int value_as_int = param.get_values()[v];
            if (value_as_int > prop.get_compared_value()) {
              // We create and LDD, which represents that X >= value_as_int,
              // which means that X >= value_as_int > a,
              // where a is the lower bound from the proposition.
              return T_DD(
                  param_level,
                  negate_ ? relational_operator::LT : relational_operator::GE,
                  v, domain_size);
            }
          }

          return negate_ ? true_false_dd_trait<T_DD>::true_dd()
                         : true_false_dd_trait<T_DD>::false_dd();
        }
        default: {
          for (int v = 0; v < param.get_values().size(); ++v) {
            int value_as_int = param.get_values()[v];
            if (value_as_int == prop.get_compared_value()) {
              return T_DD(
                  param_level,
                  negate_ ? relational_operator::EQ : relational_operator::NEQ,
                  v, domain_size);
            }
          }

          return negate_ ? true_false_dd_trait<T_DD>::false_dd()
                         : true_false_dd_trait<T_DD>::true_dd();
        }
      }
    }

    T_DD operator()(const citcpp::implication& impl) {
      using namespace citcpp::detail;
      using namespace citcpp;

      negate_ = !negate_;
      T_DD premise = impl.get_left_operand()->accept<T_DD>(*this);
      negate_ = !negate_;

      T_DD consequence = impl.get_right_operand()->accept<T_DD>(*this);

      T_DD ldd = negate_
                     ? T_DD::project_intersect(premise, consequence)
                     : T_DD::project_union(premise, consequence,
                                           model_.get_parameter_num_values());

      return ldd;
    }

    T_DD operator()(const citcpp::and_expression& and_expr) {

      using namespace citcpp::detail;
      using namespace citcpp;

      T_DD additive_identify = negate_ ? true_false_dd_trait<T_DD>::false_dd()
                                       : true_false_dd_trait<T_DD>::true_dd();
      T_DD ldd = additive_identify;
      for (const auto& operand : and_expr.get_operands()) {
        if (ldd == additive_identify) {
          ldd = operand->accept<T_DD>(*this);
        } else {
          if (negate_) {
            ldd.project_union(operand->accept<T_DD>(*this),
                              model_.get_parameter_num_values());
          } else {
            ldd.project_intersect(operand->accept<T_DD>(*this));
          }
        }
      }

      return ldd;
    }

    T_DD operator()(const citcpp::or_expression& or_expr) {

      using namespace citcpp::detail;
      using namespace citcpp;

      T_DD additive_identify = negate_ ? true_false_dd_trait<T_DD>::true_dd()
                                       : true_false_dd_trait<T_DD>::false_dd();
      T_DD ldd = additive_identify;
      for (const auto& operand : or_expr.get_operands()) {
        if (ldd == additive_identify) {
          ldd = operand->accept<T_DD>(*this);
        } else {
          if (negate_) {
            ldd.project_intersect(operand->accept<T_DD>(*this));
          } else {
            ldd.project_union(operand->accept<T_DD>(*this),
                              model_.get_parameter_num_values());
          }
        }
      }

      return ldd;
    }

  private:
    const citcpp::detail::internal_model& model_;
    std::unordered_map<std::string, const citcpp::parameter*>
        parameter_name_map_;
    std::unordered_map<citcpp::parameter_reference, int,
                       citcpp::parameter_reference_hash>
        param_to_level_;
    bool negate_;
};

std::mutex& get_global_sylvan_init_mutex() {
  static std::mutex mut;
  return mut;
}

int& get_lobal_sylan_init_counter() {
  static int count = 0;
  return count;
}

void maybe_initialize_sylvan(int num_workers) {
  using namespace citcpp::detail;
  using namespace citcpp;

  std::lock_guard<std::mutex> lock(get_global_sylvan_init_mutex());

  int& instance_cnt = get_lobal_sylan_init_counter();
  if (instance_cnt == 0) {
    sylvan::init_lace(num_workers, 0);
    sylvan::init_package((size_t)8 * 1024 * 1024 * 1024, 3, 3);
    sylvan::init_ldd();
  }

  instance_cnt++;
}

void maybe_shutdown_sylvan() {
  using namespace citcpp::detail;
  using namespace citcpp;

  std::lock_guard<std::mutex> lock(get_global_sylvan_init_mutex());

  int& instance_cnt = get_lobal_sylan_init_counter();
  instance_cnt--;
  if (instance_cnt == 0) {
    sylvan::quit_package();
    sylvan::quit_lace();
  }
}

class alignas(citcpp::detail::false_sharing_avoidance_alignment)
    check_validity_task {

  public:
    check_validity_task() = default;

    check_validity_task(const citcpp::detail::test* test,
                        unsigned int test_index,
                        const citcpp::detail::constraint_handler* handler,
                        citcpp::detail::bitset_uint64* result, std::mutex* mut)
        : test_(test),
          test_index_(test_index),
          handler_(handler),
          result_(result),
          mut_(mut) {}

    virtual ~check_validity_task() {}

    void operator()() {
      const bool is_valid = handler_->is_valid_partial_test(*test_);

      if (is_valid) {
        mark_test_as_valid();
      }
    }

  private:
    void mark_test_as_valid() {
      std::lock_guard<std::mutex> guard(*mut_);
      result_->set(test_index_);
    }

  private:
    const citcpp::detail::test* test_;
    unsigned int test_index_;
    const citcpp::detail::constraint_handler* handler_;
    citcpp::detail::bitset_uint64* result_;
    std::mutex* mut_;
};

VOID_TASK_1(lace_check_test_validity_task, check_validity_task*, functor) {
  (*functor)();
}

VOID_TASK_3(lace_check_validity_of_partial_test, citcpp::detail::bitset_uint64*,
            result, const citcpp::detail::internal_test_set*, test_set,
            const citcpp::detail::constraint_handler*, c_handler) {

  std::mutex mut;

  std::vector<check_validity_task> tasks(test_set->get_list_of_tests().size());
  unsigned int test_index = 0;
  for (const auto& t : test_set->get_list_of_tests()) {
    tasks[test_index] =
        check_validity_task(&t, test_index, c_handler, result, &mut);
    SPAWN(lace_check_test_validity_task, &tasks[test_index]);
    ++test_index;
  }

  for (int i = 0; i < tasks.size(); ++i) {
    SYNC(lace_check_test_validity_task);
  }
}

class alignas(citcpp::detail::false_sharing_avoidance_alignment)
    get_valid_parameter_assignments_task {

  public:
    get_valid_parameter_assignments_task() = default;

    get_valid_parameter_assignments_task(
        const citcpp::detail::test* test, unsigned int param_idx,
        unsigned int test_index,
        const citcpp::detail::constraint_handler* handler,
        std::vector<citcpp::detail::bitset_uint64>* results)
        : test_(test),
          param_idx_(param_idx),
          test_index_(test_index),
          handler_(handler),
          results_(results) {}

    virtual ~get_valid_parameter_assignments_task() {}

    void operator()() {
      (*results_)[test_index_] =
          handler_->get_valid_parameter_assignments(*test_, param_idx_);
    }

  private:
    const citcpp::detail::test* test_;
    unsigned int param_idx_;
    unsigned int test_index_;
    const citcpp::detail::constraint_handler* handler_;
    std::vector<citcpp::detail::bitset_uint64>* results_;
};

VOID_TASK_1(lace_get_valid_parameter_assignments_task,
            get_valid_parameter_assignments_task*, functor) {

  (*functor)();
}

VOID_TASK_4(lace_get_valid_parameter_assignments_for_testset_task,
            std::vector<citcpp::detail::bitset_uint64>*, result,
            const citcpp::detail::internal_test_set*, test_set, unsigned int,
            param_idx, const citcpp::detail::constraint_handler*, c_handler) {

  std::vector<get_valid_parameter_assignments_task> tasks(
      test_set->get_list_of_tests().size());
  unsigned int test_index = 0;
  for (const auto& t : test_set->get_list_of_tests()) {
    tasks[test_index] = get_valid_parameter_assignments_task(
        &t, param_idx, test_index, c_handler, result);
    SPAWN(lace_get_valid_parameter_assignments_task, &tasks[test_index]);
    ++test_index;
  }

  for (int i = 0; i < tasks.size(); ++i) {
    SYNC(lace_get_valid_parameter_assignments_task);
  }
}

class alignas(citcpp::detail::false_sharing_avoidance_alignment)
    replace_dont_care_values_task {

  public:
    replace_dont_care_values_task() = default;

    replace_dont_care_values_task(
        citcpp::detail::test* test,
        const citcpp::detail::constraint_handler* handler)
        : test_(test), handler_(handler) {}

    virtual ~replace_dont_care_values_task() {}

    void operator()() { handler_->replace_dont_care_values(*test_); }

  private:
    citcpp::detail::test* test_;
    const citcpp::detail::constraint_handler* handler_;
};

VOID_TASK_1(lace_replace_dont_care_values_task, replace_dont_care_values_task*,
            functor) {

  (*functor)();
}

VOID_TASK_2(lace_replace_dont_care_values_in_testset_task,
            citcpp::detail::internal_test_set*, test_set,
            const citcpp::detail::constraint_handler*, c_handler) {

  std::vector<replace_dont_care_values_task> tasks(
      test_set->get_list_of_tests().size());
  unsigned int test_index = 0;
  for (auto& t : test_set->get_list_of_tests()) {
    tasks[test_index] = replace_dont_care_values_task(&t, c_handler);
    SPAWN(lace_replace_dont_care_values_task, &tasks[test_index]);
    ++test_index;
  }

  for (int i = 0; i < tasks.size(); ++i) {
    SYNC(lace_replace_dont_care_values_task);
  }
}

}  // namespace

namespace citcpp {
namespace detail {

constraint_handler_sylvan_base::constraint_handler_sylvan_base(int num_workers)
    : base_type() {

  maybe_initialize_sylvan(num_workers);
}

constraint_handler_sylvan_base::~constraint_handler_sylvan_base() {
  maybe_shutdown_sylvan();
}

constraint_handler_sylvan_idd::constraint_handler_sylvan_idd(
    const internal_model& model, int num_workers)
    : constraint_handler_sylvan_idd(model, {}, num_workers) {}

constraint_handler_sylvan_idd::constraint_handler_sylvan_idd(
    const internal_model& model, int num_workers,
    constraint_handler_init_progress& exec_handle)
    : constraint_handler_sylvan_idd(model, {}, num_workers, exec_handle) {}

constraint_handler_sylvan_idd::constraint_handler_sylvan_idd(
    const internal_model& model,
    const std::vector<unsigned int>& variable_order, int num_workers)
    : base_type(num_workers),
      model_(model),
      idd_(),
      test_to_idd_(),
      is_per_test_idd_enabled_(false),
      variable_order_(),
      parameter_to_level_(),
      reordered_domain_sizes_() {

  initialize_variable_order(variable_order);

  constraint_to_xdd_visitor<sylvan_idd> visitor(model, parameter_to_level_);
  sylvan_idd idd_true = sylvan_idd::iddTrue();
  sylvan_idd idd = idd_true;
  for (const auto& constr : model.get_input_model().get_constraints()) {
    if (idd == idd_true) {
      idd = constr->accept<sylvan_idd>(visitor);
    } else {
      idd.project_intersect(constr->accept<sylvan_idd>(visitor));
    }
  }

  idd_ = idd;
}

constraint_handler_sylvan_idd::constraint_handler_sylvan_idd(
    const internal_model& model,
    const std::vector<unsigned int>& variable_order, int num_workers,
    constraint_handler_init_progress& exec_handle)
    : base_type(num_workers),
      model_(model),
      idd_(),
      test_to_idd_(),
      is_per_test_idd_enabled_(false),
      variable_order_(),
      parameter_to_level_(),
      reordered_domain_sizes_() {

  initialize_variable_order(variable_order);

  constraint_to_xdd_visitor<sylvan_idd> visitor(model, parameter_to_level_);
  sylvan_idd idd_true = sylvan_idd::iddTrue();
  sylvan_idd idd = idd_true;
  for (const auto& constr : model.get_input_model().get_constraints()) {
    if (idd == idd_true) {
      idd = constr->accept<sylvan_idd>(visitor);
    } else {
      idd.project_intersect(constr->accept<sylvan_idd>(visitor));
    }
    exec_handle.add_constraint_handler_init_progress_current(1);
  }

  idd_ = idd;
}

void constraint_handler_sylvan_idd::initialize_variable_order(
    const std::vector<unsigned int>& variable_order) {

  const std::vector<unsigned int>& domain_sizes =
      model_.get_parameter_num_values();

  if (variable_order.empty()) {
    variable_order_.resize(domain_sizes.size());
    std::iota(variable_order_.begin(), variable_order_.end(), 0);
  } else {
    variable_order_ = variable_order;
  }

  parameter_to_level_.resize(variable_order_.size());
  reordered_domain_sizes_.resize(variable_order_.size());

  for (unsigned int level = 0; level < variable_order_.size(); ++level) {
    const unsigned int param_idx = variable_order_[level];
    parameter_to_level_[param_idx] = level;
    reordered_domain_sizes_[level] = domain_sizes[param_idx];
  }
}

std::vector<int> constraint_handler_sylvan_idd::get_reordered_values(
    const std::vector<int>& values) const {

  std::vector<int> reordered(values.size());
  for (unsigned int level = 0; level < variable_order_.size(); ++level) {
    reordered[level] = values[variable_order_[level]];
  }

  return reordered;
}

bool constraint_handler_sylvan_idd::is_thread_safe() const { return false; }

bool constraint_handler_sylvan_idd::is_valid_partial_test(const test& t) const {
  std::vector<int> reordered_values = get_reordered_values(t.get_values());

  auto it = test_to_idd_.find(&t);
  if (it != test_to_idd_.end()) {
    return it->second.is_sat_with_partial_assignment(reordered_values);
  }

  return idd_.is_sat_with_partial_assignment(reordered_values);
}

void constraint_handler_sylvan_idd::mark_valid_tuples(
    coverage_map_second_level& value_combinations) const {

  idd_.mark_valid_value_combinations(value_combinations,
                                     model_.get_parameter_num_values(),
                                     &parameter_to_level_);
}

bitset_uint64 constraint_handler_sylvan_idd::check_validity_of_partial_tests(
    const internal_test_set& test_set) const {

  bitset_uint64 result(test_set.get_list_of_tests().size());

  RUN(lace_check_validity_of_partial_test, &result, &test_set, this);

  return result;
}

bitset_uint64 constraint_handler_sylvan_idd::get_valid_parameter_assignments(
    const test& t, unsigned int param_idx) const {

  std::vector<int> reordered_values = get_reordered_values(t.get_values());
  auto it = test_to_idd_.find(&t);
  if (it != test_to_idd_.end()) {
    return it->second.get_valid_variable_assignments(
        parameter_to_level_[param_idx],
        model_.get_parameter_num_values()[param_idx], reordered_values);
  }

  return idd_.get_valid_variable_assignments(
      parameter_to_level_[param_idx],
      model_.get_parameter_num_values()[param_idx], reordered_values);
}

std::vector<bitset_uint64>
constraint_handler_sylvan_idd::get_valid_parameter_assignments(
    const internal_test_set& test_set, unsigned int param_idx) const {

  std::vector<bitset_uint64> result(test_set.get_list_of_tests().size());

  RUN(lace_get_valid_parameter_assignments_for_testset_task, &result, &test_set,
      param_idx, this);

  return result;
}

void constraint_handler_sylvan_idd::replace_dont_care_values(test& t) const {
  std::vector<int> reordered_values = get_reordered_values(t.get_values());

  auto it = test_to_idd_.find(&t);
  if (it != test_to_idd_.end()) {
    it->second.get_sat_one_under_partial_assignment(reordered_values);
  } else {
    idd_.get_sat_one_under_partial_assignment(reordered_values);
  }

  for (unsigned int level = 0; level < variable_order_.size(); ++level) {
    t.get_values()[variable_order_[level]] = reordered_values[level];
  }

  // The call above only replaces don't care values for constrained variables.
  // So the test may still contain  don't care values for unconstrained
  // variables, which we also need to replace. This is easy however, since we
  // can simply replace all of them by the first value of the respective domain.
  for (unsigned int i = 0; i < t.get_values().size(); ++i) {
    int& value = t.get_values()[i];
    if (value < 0) {
      value = 0;
    }
  }
}

void constraint_handler_sylvan_idd::replace_dont_care_values(
    internal_test_set& test_set) const {

  RUN(lace_replace_dont_care_values_in_testset_task, &test_set, this);
}

void constraint_handler_sylvan_idd::cache_partial_test(const test* t) {
  if (is_per_test_idd_enabled_) {
    test_to_idd_.emplace(t, sylvan_idd::project_intersect(
                                idd_, get_reordered_values(t->get_values())));
  }
}

void constraint_handler_sylvan_idd::update_cached_partial_test(const test* t) {
  if (is_per_test_idd_enabled_) {
    auto it = test_to_idd_.find(t);
    if (it != test_to_idd_.end()) {
      it->second.project_intersect(
          sylvan_idd(get_reordered_values(t->get_values())));
    } else {
      cache_partial_test(t);
    }
  }
}

void constraint_handler_sylvan_idd::update_cached_partial_test(
    const test* t, unsigned int param_idx, int value) {

  if (is_per_test_idd_enabled_) {
    auto it = test_to_idd_.find(t);
    if (it != test_to_idd_.end()) {
      it->second.project_intersect(
          sylvan_idd(parameter_to_level_[param_idx], value));
    } else {
      auto emplace_result = test_to_idd_.emplace(
          t, sylvan_idd::project_intersect(
                 idd_, get_reordered_values(t->get_values())));
      emplace_result.first->second.project_intersect(
          sylvan_idd(parameter_to_level_[param_idx], value));
    }
  }
}

bool constraint_handler_sylvan_idd::is_per_test_idd_enabled() const {
  return is_per_test_idd_enabled_;
}

void constraint_handler_sylvan_idd::use_per_test_idd(bool enabled) {
  is_per_test_idd_enabled_ = enabled;
}

const sylvan_idd& constraint_handler_sylvan_idd::getIdd() const { return idd_; }

}  // namespace detail
}  // namespace citcpp
