#include "constraint_handler_sylvan_ldd.hpp"

#include <mutex>

namespace {

template <typename T_DD>
struct true_false_dd_trait {};

template <>
struct true_false_dd_trait<citcpp::detail::sylvan_ldd> {
    static citcpp::detail::sylvan_ldd true_dd() {
      return citcpp::detail::sylvan_ldd::lddTrue();
    }
    static citcpp::detail::sylvan_ldd false_dd() {
      return citcpp::detail::sylvan_ldd::lddFalse();
    }
};

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
    constraint_to_xdd_visitor(const citcpp::detail::internal_model& model)
        : model_(model), param_to_index_(), negate_(false) {

      int idx = 0;
      for (const auto& param : model_.get_input_model().get_parameters()) {
        parameter_name_map_[param.get_name()] = &param;
        param_to_index_.emplace(param.get_name(), idx);
        ++idx;
      }
    }

    T_DD operator()(const citcpp::boolean_proposition& prop) {

      using namespace citcpp::detail;
      using namespace citcpp;

      const parameter& param =
          *parameter_name_map_.at(prop.get_parameter().get_name());
      const int param_idx = param_to_index_.at(param.get_name());
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
                param_idx,
                negate_ ? relational_operator::NEQ : relational_operator::EQ,
                value_index, domain_size);
          }
        default:
          if (value_index >= param.get_values().size()) {
            return negate_ ? true_false_dd_trait<T_DD>::false_dd()
                           : true_false_dd_trait<T_DD>::true_dd();
          } else {
            return T_DD(
                param_idx,
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
      const int param_idx = param_to_index_.at(param.get_name());
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
                param_idx,
                negate_ ? relational_operator::NEQ : relational_operator::EQ,
                value_index, domain_size);
          }
        default:
          if (value_index >= param.get_values().size()) {
            return negate_ ? true_false_dd_trait<T_DD>::false_dd()
                           : true_false_dd_trait<T_DD>::true_dd();
          } else {
            return T_DD(
                param_idx,
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
      const int param_idx = param_to_index_.at(param.get_name());
      const int domain_size = param.get_values().size();

      switch (prop.get_operator()) {
        case relational_operator::EQ: {
          for (int v = 0; v < param.get_values().size(); ++v) {
            int value_as_int = param.get_values()[v];
            if (value_as_int == prop.get_compared_value()) {
              return T_DD(
                  param_idx,
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
                  param_idx,
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
                  param_idx,
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
                  param_idx,
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
                  param_idx,
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
                  param_idx,
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
        param_to_index_;
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

constraint_handler_sylvan_ldd::constraint_handler_sylvan_ldd(
    const internal_model& model, int num_workers)
    : base_type(num_workers), model_(model), ldd_() {

  constraint_to_xdd_visitor<sylvan_ldd> visitor(model);
  sylvan_ldd ldd_true = sylvan_ldd::lddTrue();
  sylvan_ldd ldd = ldd_true;
  for (const auto& constr : model.get_input_model().get_constraints()) {
    if (ldd == ldd_true) {
      ldd = constr->accept<sylvan_ldd>(visitor);
    } else {
      ldd.project_intersect(constr->accept<sylvan_ldd>(visitor));
    }
  }

  ldd_ = ldd;
}

constraint_handler_sylvan_ldd::constraint_handler_sylvan_ldd(
    const internal_model& model, int num_workers,
    constraint_handler_init_progress& exec_handle)
    : constraint_handler_sylvan_base(num_workers), model_(model), ldd_() {

  constraint_to_xdd_visitor<sylvan_ldd> visitor(model);
  sylvan_ldd ldd_true = sylvan_ldd::lddTrue();
  sylvan_ldd ldd = ldd_true;
  for (const auto& constr : model.get_input_model().get_constraints()) {
    if (ldd == ldd_true) {
      ldd = constr->accept<sylvan_ldd>(visitor);
    } else {
      ldd.project_intersect(constr->accept<sylvan_ldd>(visitor));
    }
    exec_handle.add_constraint_handler_init_progress_current(1);
  }

  ldd_ = ldd;
}

bool constraint_handler_sylvan_ldd::is_thread_safe() const { return false; }

bool constraint_handler_sylvan_ldd::is_valid_partial_test(const test& t) const {
  return ldd_.is_sat_with_partial_assignment(t.get_values());
}

bitset_uint64 constraint_handler_sylvan_ldd::get_valid_parameter_assignments(
    const test& t, unsigned int param_idx) const {

  return ldd_.get_valid_variable_assignments(
      param_idx, model_.get_parameter_num_values()[param_idx], t.get_values());
}

void constraint_handler_sylvan_ldd::replace_dont_care_values(test& t) const {
  ldd_.get_sat_one_under_partial_assignment(t.get_values());
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

const sylvan_ldd& constraint_handler_sylvan_ldd::getLdd() const { return ldd_; }

constraint_handler_sylvan_idd::constraint_handler_sylvan_idd(
    const internal_model& model, int num_workers)
    : base_type(num_workers), model_(model), idd_() {

  constraint_to_xdd_visitor<sylvan_idd> visitor(model);
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
    const internal_model& model, int num_workers,
    constraint_handler_init_progress& exec_handle)
    : constraint_handler_sylvan_base(num_workers), model_(model), idd_() {

  constraint_to_xdd_visitor<sylvan_idd> visitor(model);
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

bool constraint_handler_sylvan_idd::is_thread_safe() const { return false; }

bool constraint_handler_sylvan_idd::is_valid_partial_test(const test& t) const {
  return idd_.is_sat_with_partial_assignment(t.get_values());
}

bitset_uint64 constraint_handler_sylvan_idd::get_valid_parameter_assignments(
    const test& t, unsigned int param_idx) const {

  return idd_.get_valid_variable_assignments(
      param_idx, model_.get_parameter_num_values()[param_idx], t.get_values());
}

void constraint_handler_sylvan_idd::replace_dont_care_values(test& t) const {
  idd_.get_sat_one_under_partial_assignment(t.get_values());
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

const sylvan_idd& constraint_handler_sylvan_idd::getIdd() const { return idd_; }

}  // namespace detail
}  // namespace citcpp
