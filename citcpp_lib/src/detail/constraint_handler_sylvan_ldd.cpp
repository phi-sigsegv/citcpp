#include "constraint_handler_sylvan_ldd.hpp"

#include <lace.h>

#include <atomic>
#include <mutex>
#include <numeric>
#include <vector>

#include "parameter_preprocessor.hpp"

namespace {

class parameter_collector_visitor {
  public:
    parameter_collector_visitor(
        const std::unordered_map<std::string, unsigned int>& name_to_idx)
        : name_to_idx_(name_to_idx), collected_params_() {}

    void operator()(const citcpp::boolean_literal&) {}

    void operator()(const citcpp::boolean_proposition& prop) {
      add_param(prop.get_parameter());
    }

    void operator()(const citcpp::enum_proposition& prop) {
      add_param(prop.get_parameter());
    }

    void operator()(const citcpp::int_proposition& prop) {
      add_param(prop.get_parameter());
    }

    void operator()(const citcpp::implication& impl) {
      impl.get_left_operand()->accept(*this);
      impl.get_right_operand()->accept(*this);
    }

    void operator()(const citcpp::and_expression& and_expr) {
      for (const auto& operand : and_expr.get_operands()) {
        operand->accept(*this);
      }
    }

    void operator()(const citcpp::or_expression& or_expr) {
      for (const auto& operand : or_expr.get_operands()) {
        operand->accept(*this);
      }
    }

    const std::vector<unsigned int>& get_collected_params() const {
      return collected_params_;
    }

  private:
    void add_param(const citcpp::parameter_reference& ref) {
      auto it = name_to_idx_.find(ref.get_name());
      if (it != name_to_idx_.end()) {
        collected_params_.push_back(it->second);
      }
    }

    const std::unordered_map<std::string, unsigned int>& name_to_idx_;
    std::vector<unsigned int> collected_params_;
};

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
        const std::vector<unsigned int>& parameter_to_level,
        const std::vector<unsigned int>& reordered_domain_sizes)
        : model_(model),
          param_to_level_(),
          reordered_domain_sizes_(reordered_domain_sizes),
          negate_(false) {

      unsigned int idx = 0;
      for (const auto& param : model_.get_input_model().get_parameters()) {
        parameter_name_map_[param.get_name()] = &param;
        param_to_level_.emplace(param.get_name(), parameter_to_level[idx]);
        ++idx;
      }
    }

    T_DD operator()(const citcpp::boolean_literal& lit) {
      using namespace citcpp::detail;
      using namespace citcpp;

      T_DD dd = lit ? true_false_dd_trait<T_DD>::false_dd()
                    : true_false_dd_trait<T_DD>::true_dd();

      return dd;
    }

    T_DD operator()(const citcpp::boolean_proposition& prop) {
      using namespace citcpp::detail;
      using namespace citcpp;

      const parameter& param =
          *parameter_name_map_.at(prop.get_parameter().get_name());
      const uint32_t param_level = param_to_level_.at(param.get_name());
      const uint32_t domain_size =
          static_cast<uint32_t>(param.get_values().size());

      uint32_t value_index = 0;
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
      const uint32_t param_level = param_to_level_.at(param.get_name());
      const uint32_t domain_size =
          static_cast<uint32_t>(param.get_values().size());

      uint32_t value_index = 0;
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
      const uint32_t param_level = param_to_level_.at(param.get_name());
      const uint32_t domain_size =
          static_cast<uint32_t>(param.get_values().size());

      switch (prop.get_operator()) {
        case relational_operator::EQ: {
          uint32_t v = 0;
          for (const auto& param_value : param.get_values()) {
            int value_as_int = param_value;
            if (value_as_int == prop.get_compared_value()) {
              return T_DD(
                  param_level,
                  negate_ ? relational_operator::NEQ : relational_operator::EQ,
                  v, domain_size);
            }

            ++v;
          }

          return negate_ ? true_false_dd_trait<T_DD>::true_dd()
                         : true_false_dd_trait<T_DD>::false_dd();
        }
        case relational_operator::LE: {
          int greatest_value =
              param.get_values()[param.get_values().size() - 1];
          if (greatest_value <= prop.get_compared_value()) {
            return negate_ ? true_false_dd_trait<T_DD>::false_dd()
                           : true_false_dd_trait<T_DD>::true_dd();
          }

          for (int v = static_cast<int>(param.get_values().size() - 1); v >= 0;
               --v) {
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
          int greatest_value =
              param.get_values()[param.get_values().size() - 1];
          if (greatest_value < prop.get_compared_value()) {
            return negate_ ? true_false_dd_trait<T_DD>::false_dd()
                           : true_false_dd_trait<T_DD>::true_dd();
          }

          for (int v = static_cast<int>(param.get_values().size() - 1); v >= 0;
               --v) {
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
          int smallest_value = param.get_values()[0];
          if (smallest_value >= prop.get_compared_value()) {
            return negate_ ? true_false_dd_trait<T_DD>::false_dd()
                           : true_false_dd_trait<T_DD>::true_dd();
          }

          uint32_t v = 0;
          for (const auto& param_value : param.get_values()) {
            int value_as_int = param_value;
            if (value_as_int >= prop.get_compared_value()) {
              return T_DD(
                  param_level,
                  negate_ ? relational_operator::LT : relational_operator::GE,
                  v, domain_size);
            }

            ++v;
          }

          return negate_ ? true_false_dd_trait<T_DD>::true_dd()
                         : true_false_dd_trait<T_DD>::false_dd();
        }
        case relational_operator::GT: {
          int smallest_value = param.get_values()[0];
          if (smallest_value > prop.get_compared_value()) {
            return negate_ ? true_false_dd_trait<T_DD>::false_dd()
                           : true_false_dd_trait<T_DD>::true_dd();
          }

          uint32_t v = 0;
          for (const auto& param_value : param.get_values()) {
            int value_as_int = param_value;
            if (value_as_int > prop.get_compared_value()) {
              // We create and LDD, which represents that X >= value_as_int,
              // which means that X >= value_as_int > a,
              // where a is the lower bound from the proposition.
              return T_DD(
                  param_level,
                  negate_ ? relational_operator::LT : relational_operator::GE,
                  v, domain_size);
            }

            ++v;
          }

          return negate_ ? true_false_dd_trait<T_DD>::true_dd()
                         : true_false_dd_trait<T_DD>::false_dd();
        }
        default: {
          uint32_t v = 0;
          for (const auto& param_value : param.get_values()) {
            int value_as_int = param_value;
            if (value_as_int == prop.get_compared_value()) {
              return T_DD(
                  param_level,
                  negate_ ? relational_operator::EQ : relational_operator::NEQ,
                  v, domain_size);
            }

            ++v;
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

      T_DD ldd = negate_ ? T_DD::project_intersect(premise, consequence)
                         : T_DD::project_union(premise, consequence,
                                               reordered_domain_sizes_);

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
                              reordered_domain_sizes_);
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
                              reordered_domain_sizes_);
          }
        }
      }

      return ldd;
    }

  private:
    const citcpp::detail::internal_model& model_;
    std::unordered_map<std::string, const citcpp::parameter*>
        parameter_name_map_;
    std::unordered_map<citcpp::parameter_reference, uint32_t,
                       citcpp::parameter_reference_hash>
        param_to_level_;
    const std::vector<unsigned int>& reordered_domain_sizes_;
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

void maybe_initialize_sylvan(unsigned int num_workers,
                             std::size_t memory_limit_in_bytes) {
  using namespace citcpp::detail;
  using namespace citcpp;

  std::lock_guard<std::mutex> lock(get_global_sylvan_init_mutex());

  int& instance_cnt = get_lobal_sylan_init_counter();
  if (instance_cnt == 0) {
    sylvan::init_lace(num_workers, 0);
    sylvan::init_package(memory_limit_in_bytes, 3, 3);
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
                        std::size_t test_index,
                        const citcpp::detail::constraint_handler* handler,
                        citcpp::detail::bitset_uint64* result, std::mutex* mut)
        : test_(test),
          test_index_(test_index),
          handler_(handler),
          result_(result),
          mut_(mut) {}

    void operator()() {
      const bool is_valid = handler_->is_valid_partial_test(*test_);

      if (is_valid) {
        mark_test_as_valid();
      }
    }

  private:
    void mark_test_as_valid() {
      std::lock_guard<std::mutex> guard(*mut_);
      result_->set(
          static_cast<citcpp::detail::bitset_uint64::size_type>(test_index_));
    }

  private:
    const citcpp::detail::test* test_{nullptr};
    std::size_t test_index_{0};
    const citcpp::detail::constraint_handler* handler_{nullptr};
    citcpp::detail::bitset_uint64* result_{nullptr};
    std::mutex* mut_{nullptr};
};

VOID_TASK_1(lace_check_test_validity_task, check_validity_task*, functor) {
  (*functor)();
}

VOID_TASK_3(lace_check_validity_of_partial_test, citcpp::detail::bitset_uint64*,
            result, const citcpp::detail::internal_test_set*, test_set,
            const citcpp::detail::constraint_handler*, c_handler) {

  std::mutex mut;

  std::vector<check_validity_task> tasks(test_set->get_list_of_tests().size());
  std::size_t test_index = 0;
  for (const auto& t : test_set->get_list_of_tests()) {
    tasks[test_index] =
        check_validity_task(&t, test_index, c_handler, result, &mut);
    SPAWN(lace_check_test_validity_task, &tasks[test_index]);
    ++test_index;
  }

  for (std::size_t i = 0; i < tasks.size(); ++i) {
    SYNC(lace_check_test_validity_task);
  }
}

class alignas(citcpp::detail::false_sharing_avoidance_alignment)
    get_valid_parameter_assignments_task {

  public:
    get_valid_parameter_assignments_task() = default;

    get_valid_parameter_assignments_task(
        const citcpp::detail::test* test, unsigned int param_idx,
        std::size_t test_index,
        const citcpp::detail::constraint_handler* handler,
        std::vector<citcpp::detail::bitset_uint64>* results)
        : test_(test),
          param_idx_(param_idx),
          test_index_(test_index),
          handler_(handler),
          results_(results) {}

    void operator()() {
      (*results_)[test_index_] =
          handler_->get_valid_parameter_assignments(*test_, param_idx_);
    }

  private:
    const citcpp::detail::test* test_{nullptr};
    unsigned int param_idx_{0};
    std::size_t test_index_{0};
    const citcpp::detail::constraint_handler* handler_{nullptr};
    std::vector<citcpp::detail::bitset_uint64>* results_{nullptr};
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
  std::size_t test_index = 0;
  for (const auto& t : test_set->get_list_of_tests()) {
    tasks[test_index] = get_valid_parameter_assignments_task(
        &t, param_idx, test_index, c_handler, result);
    SPAWN(lace_get_valid_parameter_assignments_task, &tasks[test_index]);
    ++test_index;
  }

  for (std::size_t i = 0; i < tasks.size(); ++i) {
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

    void operator()() { handler_->replace_dont_care_values(*test_); }

  private:
    citcpp::detail::test* test_{nullptr};
    const citcpp::detail::constraint_handler* handler_{nullptr};
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
  std::size_t test_index = 0;
  for (auto& t : test_set->get_list_of_tests()) {
    tasks[test_index] = replace_dont_care_values_task(&t, c_handler);
    SPAWN(lace_replace_dont_care_values_task, &tasks[test_index]);
    ++test_index;
  }

  for (std::size_t i = 0; i < tasks.size(); ++i) {
    SYNC(lace_replace_dont_care_values_task);
  }
}

struct lace_get_first_test_valid_for_assignment_ctx {
    const citcpp::detail::param_vector* param_indices = nullptr;
    const citcpp::detail::value_vector* value_indices = nullptr;
    std::atomic_size_t min_valid_index = 0;
    citcpp::detail::test_list_intrusive_integ* test = nullptr;
    citcpp::detail::spin_lock lock;
};

VOID_TASK_5(lace_get_first_test_valid_for_assignment_task,
            citcpp::detail::list_intrusive<
                citcpp::detail::test_list_intrusive_integ>::iterator,
            test_it, size_t, start, size_t, end,
            const citcpp::detail::constraint_handler_sylvan_idd*, handler,
            lace_get_first_test_valid_for_assignment_ctx*, ctx) {

  if (end - start <= 64) {
    const citcpp::detail::param_vector& param_indices = *(ctx->param_indices);
    const citcpp::detail::value_vector& value_indices = *(ctx->value_indices);

    // Small Buffer Optimization to completely avoid heap allocations for the
    // rollback array
    constexpr size_t static_limit = 8;
    int local_old_values[static_limit];
    std::vector<int> heap_old_values;
    int* old_values = local_old_values;
    if (param_indices.size() > static_limit) {
      heap_old_values.resize(param_indices.size());
      old_values = heap_old_values.data();
    }

    for (size_t i = start; i < end; ++i, ++test_it) {
      if (i >= ctx->min_valid_index.load(std::memory_order_relaxed)) {
        break;
      }

      citcpp::detail::test_list_intrusive_integ& list_node = *test_it;
      citcpp::detail::test& t = list_node.get_test();

      bool covers_combo = true;

      for (std::size_t j = 0; j < param_indices.size(); ++j) {
        const std::size_t param_idx = param_indices[j];
        const int param_value_to_assign = value_indices[j];
        const int param_value_in_test = t.get_values()[param_idx];

        old_values[j] = param_value_in_test;
        t.get_values()[param_idx] = param_value_to_assign;

        if (param_value_in_test >= 0 &&
            param_value_to_assign != param_value_in_test) {
          covers_combo = false;
        }
      }

      covers_combo = covers_combo && handler->is_valid_partial_test(t);

      // Rollback the changes we did to the test.
      for (std::size_t j = 0; j < param_indices.size(); ++j) {
        const std::size_t param_idx = param_indices[j];
        t.get_values()[param_idx] = old_values[j];
      }

      if (covers_combo) {
        size_t current_min =
            ctx->min_valid_index.load(std::memory_order_relaxed);
        if (i < current_min) {
          std::lock_guard<citcpp::detail::spin_lock> guard(ctx->lock);
          current_min = ctx->min_valid_index.load(std::memory_order_relaxed);
          if (i < current_min) {
            ctx->min_valid_index.store(i, std::memory_order_relaxed);
            ctx->test = &list_node;
          }
        }
        break;
      }
    }
    return;
  }

  size_t mid = start + (end - start) / 2;
  citcpp::detail::list_intrusive<citcpp::detail::test_list_intrusive_integ>::
      iterator second_half_test_it(test_it);
  second_half_test_it += (mid - start);
  SPAWN(lace_get_first_test_valid_for_assignment_task, second_half_test_it, mid,
        end, handler, ctx);
  CALL(lace_get_first_test_valid_for_assignment_task, test_it, start, mid,
       handler, ctx);
  SYNC(lace_get_first_test_valid_for_assignment_task);
}

std::vector<unsigned int> initialize_variable_order(
    const citcpp::detail::internal_model& model,
    const std::vector<unsigned int>& variable_order) {

  const auto& domain_sizes = model.get_parameter_num_values();

  if (variable_order.empty()) {
    std::vector<unsigned int> default_variable_order;
    default_variable_order.resize(domain_sizes.size());
    std::iota(default_variable_order.begin(), default_variable_order.end(), 0);

    return default_variable_order;
  }

  return variable_order;
}

}  // namespace

namespace citcpp {
namespace detail {

constraint_handler_sylvan_base::constraint_handler_sylvan_base(
    unsigned int num_workers, std::size_t memory_limit_in_bytes)
    : base_type() {

  maybe_initialize_sylvan(num_workers, memory_limit_in_bytes);
}

constraint_handler_sylvan_base::~constraint_handler_sylvan_base() {
  maybe_shutdown_sylvan();
}

constraint_handler_sylvan_idd::constraint_handler_sylvan_idd(
    const internal_model& model, unsigned int num_workers,
    std::size_t memory_limit_in_bytes)
    : constraint_handler_sylvan_idd(model, {}, num_workers,
                                    memory_limit_in_bytes) {}

constraint_handler_sylvan_idd::constraint_handler_sylvan_idd(
    const internal_model& model, unsigned int num_workers,
    std::size_t memory_limit_in_bytes,
    constraint_handler_init_progress& exec_handle)
    : constraint_handler_sylvan_idd(model, {}, num_workers,
                                    memory_limit_in_bytes, exec_handle) {}

constraint_handler_sylvan_idd::constraint_handler_sylvan_idd(
    const internal_model& model,
    const std::vector<unsigned int>& variable_order, unsigned int num_workers,
    std::size_t memory_limit_in_bytes)
    : base_type(num_workers, memory_limit_in_bytes),
      model_(model),
      test_to_idd_(),
      is_per_test_idd_enabled_(false),
      components_(),
      parameter_to_component_idx_() {

  setup_partitioned_idds(initialize_variable_order(model, variable_order),
                         nullptr);
}

constraint_handler_sylvan_idd::constraint_handler_sylvan_idd(
    const internal_model& model,
    const std::vector<unsigned int>& variable_order, unsigned int num_workers,
    std::size_t memory_limit_in_bytes,
    constraint_handler_init_progress& exec_handle)
    : base_type(num_workers, memory_limit_in_bytes),
      model_(model),
      test_to_idd_(),
      is_per_test_idd_enabled_(false),
      components_(),
      parameter_to_component_idx_() {

  setup_partitioned_idds(initialize_variable_order(model, variable_order),
                         &exec_handle);
}

void constraint_handler_sylvan_idd::setup_partitioned_idds(
    const std::vector<unsigned int>& variable_order,
    constraint_handler_init_progress* exec_handle) {

  // Step 1: Find all constrained parameters
  std::unordered_map<std::string, unsigned int> name_to_idx;
  unsigned int p_idx = 0;
  for (const auto& param : model_.get_input_model().get_parameters()) {
    name_to_idx[param.get_name()] = p_idx++;
  }

  std::vector<bool> is_constrained(model_.get_parameter_num_values().size(),
                                   false);
  for (const auto& constr : model_.get_input_model().get_constraints()) {
    parameter_collector_visitor visitor(name_to_idx);
    constr->accept(visitor);
    for (unsigned int p : visitor.get_collected_params()) {
      is_constrained[p] = true;
    }
  }

  // Step 2: Compute parameter partitions using variable_order_
  std::vector<std::vector<unsigned int>> partitions =
      compute_parameter_partitions(model_, variable_order);

  // Step 3: Setup components
  parameter_to_component_idx_.assign(model_.get_parameter_num_values().size(),
                                     -1);

  const auto& domain_sizes = model_.get_parameter_num_values();

  for (const auto& part : partitions) {
    bool has_constrained = false;
    for (unsigned int p : part) {
      if (is_constrained[p]) {
        has_constrained = true;
        break;
      }
    }
    if (!has_constrained) {
      continue;
    }

    // Create a new component
    component_idd_info comp;
    comp.partition = part;

    // Filter global variable_order_ to keep only parameters in this partition
    // AND that are constrained
    for (unsigned int p : variable_order) {
      if (is_constrained[p] &&
          std::find(part.begin(), part.end(), p) != part.end()) {
        comp.variable_order.push_back(p);
      }
    }

    // Initialize comp.parameter_to_level and comp.reordered_domain_sizes
    comp.parameter_to_level.resize(domain_sizes.size());
    comp.reordered_domain_sizes.resize(comp.variable_order.size());
    for (unsigned int level = 0; level < comp.variable_order.size(); ++level) {
      const std::size_t param_idx = comp.variable_order[level];
      comp.parameter_to_level[param_idx] = level;
      comp.reordered_domain_sizes[level] = domain_sizes[param_idx];
    }

    comp.idd = sylvan_idd::iddTrue();

    // Add to components
    int comp_idx = static_cast<int>(components_.size());
    components_.push_back(std::move(comp));

    // Map each parameter in the partition to the component index
    for (unsigned int p : part) {
      parameter_to_component_idx_[p] = comp_idx;
    }
  }

  // Step 4: Process constraints and associate them with their respective
  // components
  for (const auto& constr : model_.get_input_model().get_constraints()) {
    parameter_collector_visitor visitor(name_to_idx);
    constr->accept(visitor);
    const auto& involved = visitor.get_collected_params();

    if (involved.empty()) {
      // Apply to all components
      for (auto& comp : components_) {
        constraint_to_xdd_visitor<sylvan_idd> comp_visitor(
            model_, comp.parameter_to_level, comp.reordered_domain_sizes);
        sylvan_idd constr_idd = constr->accept<sylvan_idd>(comp_visitor);
        if (comp.idd == sylvan_idd::iddTrue()) {
          comp.idd = constr_idd;
        } else {
          comp.idd.project_intersect(constr_idd);
        }
      }
    } else {
      unsigned int p = involved[0];
      int comp_idx = parameter_to_component_idx_[p];
      if (comp_idx >= 0) {
        auto& comp = components_[comp_idx];
        constraint_to_xdd_visitor<sylvan_idd> comp_visitor(
            model_, comp.parameter_to_level, comp.reordered_domain_sizes);
        sylvan_idd constr_idd = constr->accept<sylvan_idd>(comp_visitor);
        if (comp.idd == sylvan_idd::iddTrue()) {
          comp.idd = constr_idd;
        } else {
          comp.idd.project_intersect(constr_idd);
        }
      }
    }

    if (exec_handle) {
      exec_handle->add_constraint_handler_init_progress_current(1);
    }
  }
}

const component_idd_info*
constraint_handler_sylvan_idd::get_component_for_parameter(
    unsigned int param_idx) const {

  if (param_idx < parameter_to_component_idx_.size()) {
    int idx = parameter_to_component_idx_[param_idx];
    if (idx >= 0) {
      return &components_[idx];
    }
  }
  return nullptr;
}

component_idd_info* constraint_handler_sylvan_idd::get_component_for_parameter(
    unsigned int param_idx) {

  return const_cast<component_idd_info*>(
      static_cast<const constraint_handler_sylvan_idd&>(*this)
          .get_component_for_parameter(param_idx));
}

bool constraint_handler_sylvan_idd::is_thread_safe() const { return false; }

bool constraint_handler_sylvan_idd::is_valid_partial_test(const test& t) const {
  for (const auto& comp : components_) {
    auto it = comp.test_to_idd.find(&t);
    if (it != comp.test_to_idd.end()) {
      if (!it->second.is_sat_with_partial_assignment(t.get_values(),
                                                     &comp.variable_order)) {
        return false;
      }
    } else {
      if (!comp.idd.is_sat_with_partial_assignment(t.get_values(),
                                                   &comp.variable_order)) {
        return false;
      }
    }
  }
  return true;
}

void constraint_handler_sylvan_idd::mark_valid_tuples(
    coverage_bitset& value_combinations,
    const param_vector& param_indices) const {

  if (value_combinations.all_valid()) return;

  const unsigned int t = param_indices.size();

  // Group parameter indices by the component they belong to
  struct active_component_info {
      const component_idd_info* comp = nullptr;
      std::vector<unsigned int> positions;  // indices in param_indices
      param_vector P_comp;                  // parameter indices in comp
      std::shared_ptr<coverage_bitset> temp_cov;
      std::vector<std::size_t> temp_weights;
  };

  std::vector<active_component_info> active_comps;

  for (unsigned int i = 0; i < t; ++i) {
    unsigned int param_idx = param_indices[i];
    const auto* comp = get_component_for_parameter(param_idx);
    if (comp) {
      // Find or insert component info
      auto it = std::find_if(active_comps.begin(), active_comps.end(),
                             [comp](const active_component_info& info) {
                               return info.comp == comp;
                             });
      if (it != active_comps.end()) {
        it->positions.push_back(i);
        it->P_comp.push_back(param_idx);
      } else {
        active_component_info new_info;
        new_info.comp = comp;
        new_info.positions.push_back(i);
        new_info.P_comp.push_back(param_idx);
        active_comps.push_back(std::move(new_info));
      }
    }
  }

  // If there are no active components, all tuples are valid
  if (active_comps.empty()) {
    value_combinations.set_all_valid();
    return;
  }

  // Optimization: If there is exactly one active component and it contains all
  // parameter indices
  if (active_comps.size() == 1 && active_comps[0].positions.size() == t) {
    const auto& active_comp = active_comps[0];
    active_comp.comp->idd.mark_valid_value_combinations(
        value_combinations, param_indices, model_.get_parameter_num_values(),
        &active_comp.comp->parameter_to_level);
    return;
  }

  // General case: evaluate on each active component and combine
  const auto& domain_sizes = model_.get_parameter_num_values();

  for (auto& active_comp : active_comps) {
    // Calculate total combinations in active_comp
    std::size_t num_combinations = 1;
    for (unsigned int param_idx : active_comp.P_comp) {
      num_combinations *= domain_sizes[param_idx];
    }

    active_comp.temp_cov = std::make_shared<coverage_bitset>(num_combinations);
    active_comp.comp->idd.mark_valid_value_combinations(
        *active_comp.temp_cov, active_comp.P_comp, domain_sizes,
        &active_comp.comp->parameter_to_level);

    // Calculate temp weights
    active_comp.temp_weights.resize(active_comp.P_comp.size());
    std::size_t temp_weight = 1;
    for (int j = static_cast<int>(active_comp.P_comp.size()) - 1; j >= 0; --j) {
      active_comp.temp_weights[j] = temp_weight;
      temp_weight *= domain_sizes[active_comp.P_comp[j]];
    }
  }

  // Precompute weights for the overall combinations
  std::vector<std::size_t> weights(t);
  std::size_t current_weight = 1;
  for (int i = static_cast<int>(t) - 1; i >= 0; --i) {
    weights[i] = current_weight;
    current_weight *= domain_sizes[param_indices[i]];
  }

  // Iterate over all possible combinations
  std::vector<unsigned int> v(t, 0);
  while (true) {
    bool combination_valid = true;
    for (const auto& active_comp : active_comps) {
      std::size_t sub_index = 0;
      for (std::size_t j = 0; j < active_comp.positions.size(); ++j) {
        sub_index += v[active_comp.positions[j]] * active_comp.temp_weights[j];
      }
      if (!active_comp.temp_cov->is_valid(sub_index)) {
        combination_valid = false;
        break;
      }
    }

    if (combination_valid) {
      std::size_t index = 0;
      for (std::size_t i = 0; i < t; ++i) {
        index += v[i] * weights[i];
      }
      value_combinations.set_valid(index);
    }

    // Advance to the next combination
    int i = static_cast<int>(t) - 1;
    while (i >= 0) {
      v[i]++;
      if (v[i] < domain_sizes[param_indices[i]]) {
        break;
      }
      v[i] = 0;
      i--;
    }
    if (i < 0) {
      break;
    }
  }
}

bitset_uint64 constraint_handler_sylvan_idd::check_validity_of_partial_tests(
    const internal_test_set& test_set) const {

  bitset_uint64 result(static_cast<bitset_uint64::size_type>(
      test_set.get_list_of_tests().size()));

  RUN(lace_check_validity_of_partial_test, &result, &test_set, this);

  return result;
}

bitset_uint64 constraint_handler_sylvan_idd::get_valid_parameter_assignments(
    const test& t, unsigned int param_idx) const {

  const auto* comp_ptr = get_component_for_parameter(param_idx);
  if (!comp_ptr) {
    const unsigned int num_param_values =
        model_.get_parameter_num_values()[param_idx];
    bitset_uint64 values(
        static_cast<bitset_uint64::size_type>(num_param_values));
    values.set();

    return values;
  }

  auto it = comp_ptr->test_to_idd.find(&t);
  if (it != comp_ptr->test_to_idd.end()) {
    return it->second.get_valid_variable_assignments(
        static_cast<uint32_t>(comp_ptr->parameter_to_level[param_idx]),
        static_cast<uint32_t>(model_.get_parameter_num_values()[param_idx]),
        t.get_values(), &comp_ptr->variable_order);
  }

  return comp_ptr->idd.get_valid_variable_assignments(
      static_cast<uint32_t>(comp_ptr->parameter_to_level[param_idx]),
      static_cast<uint32_t>(model_.get_parameter_num_values()[param_idx]),
      t.get_values(), &comp_ptr->variable_order);
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
  for (const auto& comp : components_) {
    auto it = comp.test_to_idd.find(&t);
    if (it != comp.test_to_idd.end()) {
      it->second.get_sat_one_under_partial_assignment(t.get_values(),
                                                      &comp.variable_order);
    } else {
      comp.idd.get_sat_one_under_partial_assignment(t.get_values(),
                                                    &comp.variable_order);
    }
  }

  // The call above only replaces don't care values for constrained variables.
  // So the test may still contain  don't care values for unconstrained
  // variables, which we also need to replace. This is easy however, since we
  // can simply replace all of them by the first value of the respective domain.
  for (std::size_t i = 0; i < t.get_values().size(); ++i) {
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

test_list_intrusive_integ*
constraint_handler_sylvan_idd::get_first_test_valid_for_assignment(
    list_intrusive<test_list_intrusive_integ>& test_list,
    const param_vector& param_indices,
    const value_vector& value_indices) const {

  if (test_list.empty()) {
    return nullptr;
  }

  // If there is only one worker thread or the list is small, run sequentially
  // to avoid parallel setup/overhead.
  if (lace_workers() <= 1 || test_list.size() < 64) {
    constexpr size_t static_limit = 8;
    int local_old_values[static_limit];
    std::vector<int> heap_old_values;
    int* old_values = local_old_values;
    if (param_indices.size() > static_limit) {
      heap_old_values.resize(param_indices.size());
      old_values = heap_old_values.data();
    }

    for (test_list_intrusive_integ& list_node : test_list) {
      test& t = list_node.get_test();

      bool covers_combo = true;
      for (std::size_t i = 0; i < param_indices.size(); ++i) {
        const std::size_t param_idx = param_indices[i];
        const int param_value_to_assign = value_indices[i];
        const int param_value_in_test = t.get_values()[param_idx];

        old_values[i] = param_value_in_test;
        t.get_values()[param_idx] = param_value_to_assign;

        if (param_value_in_test >= 0 &&
            param_value_to_assign != param_value_in_test) {
          covers_combo = false;
        }
      }

      covers_combo = covers_combo && is_valid_partial_test(t);

      // Rollback the changes we did to the test.
      for (std::size_t i = 0; i < param_indices.size(); ++i) {
        const std::size_t param_idx = param_indices[i];
        t.get_values()[param_idx] = old_values[i];
      }

      if (covers_combo) {
        return &list_node;
      }
    }
    return nullptr;
  }

  lace_get_first_test_valid_for_assignment_ctx ctx;
  ctx.param_indices = &param_indices;
  ctx.value_indices = &value_indices;
  ctx.min_valid_index.store(test_list.size(), std::memory_order_relaxed);
  ctx.test = nullptr;

  RUN(lace_get_first_test_valid_for_assignment_task, test_list.begin(), 0,
      test_list.size(), this, &ctx);

  return ctx.test;
}

void constraint_handler_sylvan_idd::cache_partial_test(const test* t) {
  if (is_per_test_idd_enabled_) {
    for (auto& comp : components_) {
      comp.test_to_idd.emplace(
          t, sylvan_idd::project_intersect(
                 comp.idd, sylvan_idd(t->get_values(), &comp.variable_order)));
    }
  }
}

void constraint_handler_sylvan_idd::update_cached_partial_test(const test* t) {
  if (is_per_test_idd_enabled_) {
    for (auto& comp : components_) {
      auto it = comp.test_to_idd.find(t);
      if (it != comp.test_to_idd.end()) {
        it->second.project_intersect(
            sylvan_idd(t->get_values(), &comp.variable_order));
      } else {
        comp.test_to_idd.emplace(
            t,
            sylvan_idd::project_intersect(
                comp.idd, sylvan_idd(t->get_values(), &comp.variable_order)));
      }
    }
  }
}

void constraint_handler_sylvan_idd::update_cached_partial_test(
    const test* t, unsigned int param_idx, int value) {

  if (is_per_test_idd_enabled_) {
    auto* comp_ptr = get_component_for_parameter(param_idx);
    if (comp_ptr) {
      auto it = comp_ptr->test_to_idd.find(t);
      if (it != comp_ptr->test_to_idd.end()) {
        it->second.project_intersect(sylvan_idd(
            static_cast<uint32_t>(comp_ptr->parameter_to_level[param_idx]),
            static_cast<uint32_t>(value)));
      } else {
        auto emplace_result = comp_ptr->test_to_idd.emplace(
            t, sylvan_idd::project_intersect(
                   comp_ptr->idd,
                   sylvan_idd(t->get_values(), &comp_ptr->variable_order)));
        emplace_result.first->second.project_intersect(sylvan_idd(
            static_cast<uint32_t>(comp_ptr->parameter_to_level[param_idx]),
            static_cast<uint32_t>(value)));
      }
    }
  }
}

bool constraint_handler_sylvan_idd::is_per_test_idd_enabled() const {
  return is_per_test_idd_enabled_;
}

void constraint_handler_sylvan_idd::use_per_test_idd(bool enabled) {
  is_per_test_idd_enabled_ = enabled;
}

size_t constraint_handler_sylvan_idd::node_count() const {
  size_t node_cnt = 0;
  for (const auto& comp : components_) {
    node_cnt += comp.idd.node_count();
  }

  return node_cnt;
}

long double constraint_handler_sylvan_idd::sat_count() const {
  long double sat_cnt = 0.0;
  for (const auto& comp : components_) {
    sat_cnt += comp.idd.sat_count();
  }

  return sat_cnt;
}

}  // namespace detail
}  // namespace citcpp
