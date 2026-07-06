#include "idd_variable_ordering.hpp"

#include <algorithm>
#include <citcpp/constraints.hpp>
#include <numeric>
#include <set>
#include <unordered_map>

namespace {

/**
 * Visitor that collects all unique parameters involved in a constraint.
 */
class parameter_collector_visitor {
  public:
    parameter_collector_visitor(
        const std::unordered_map<std::string, unsigned int>& name_to_idx)
        : name_to_idx_(name_to_idx) {}

    void operator()(const citcpp::boolean_literal& lit) {}

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

    const std::set<unsigned int>& get_collected_params() const {
      return collected_params_;
    }

  private:
    void add_param(const citcpp::parameter_reference& ref) {
      auto it = name_to_idx_.find(ref.get_name());
      if (it != name_to_idx_.end()) {
        collected_params_.insert(it->second);
      }
    }

    const std::unordered_map<std::string, unsigned int>& name_to_idx_;
    std::set<unsigned int> collected_params_;
};

}  // namespace

namespace citcpp {
namespace detail {

std::vector<unsigned int> compute_mcmf_variable_order(
    const internal_model& model) {

  const auto& input_model = model.get_input_model();
  const auto& parameters = input_model.get_parameters();
  const unsigned int num_params = parameters.size();

  if (num_params == 0) {
    return {};
  }

  // Map parameter names to indices for efficient lookups
  std::unordered_map<std::string, unsigned int> name_to_idx;
  for (unsigned int i = 0; i < num_params; ++i) {
    name_to_idx[parameters[i].get_name()] = i;
  }

  // Build adjacency list (interaction graph)
  std::vector<std::set<unsigned int>> adj(num_params);
  for (const auto& constraint : input_model.get_constraints()) {
    parameter_collector_visitor visitor(name_to_idx);
    constraint->accept(visitor);
    const auto& involved = visitor.get_collected_params();

    for (unsigned int p1 : involved) {
      for (unsigned int p2 : involved) {
        if (p1 != p2) {
          adj[p1].insert(p2);
          adj[p2].insert(p1);
        }
      }
    }
  }

  std::vector<unsigned int> order;
  order.reserve(num_params);
  std::vector<bool> is_ordered(num_params, false);
  std::vector<unsigned int> ordered_neighbors_count(num_params, 0);

  // Step 1: Find the parameter with the largest connectivity (Master)
  int first_param = -1;
  unsigned int max_connectivity = 0;

  for (unsigned int i = 0; i < num_params; ++i) {
    if (adj[i].size() > max_connectivity) {
      max_connectivity = adj[i].size();
      first_param = i;
    }
  }

  // If no constraints, just use original order
  if (first_param == -1) {
    std::vector<unsigned int> identity_order(num_params);
    std::iota(identity_order.begin(), identity_order.end(), 0);
    return identity_order;
  }

  auto add_to_order = [&](unsigned int p) {
    order.push_back(p);
    is_ordered[p] = true;
    for (unsigned int neighbor : adj[p]) {
      if (!is_ordered[neighbor]) {
        ordered_neighbors_count[neighbor]++;
      }
    }
  };

  add_to_order(first_param);

  // Step 2: Greedily add remaining parameters
  while (order.size() < num_params) {
    int best_p = -1;
    unsigned int max_proximity = 0;
    unsigned int max_total_connectivity = 0;

    for (unsigned int i = 0; i < num_params; ++i) {
      if (is_ordered[i]) continue;

      bool better = false;
      if (best_p == -1) {
        better = true;
      } else if (ordered_neighbors_count[i] > max_proximity) {
        better = true;
      } else if (ordered_neighbors_count[i] == max_proximity) {
        if (adj[i].size() > max_total_connectivity) {
          better = true;
        }
      }

      if (better) {
        best_p = i;
        max_proximity = ordered_neighbors_count[i];
        max_total_connectivity = adj[i].size();
      }
    }

    if (best_p != -1) {
      add_to_order(best_p);
    } else {
      // Should not happen unless there's a logic error, but as a safety:
      for (unsigned int i = 0; i < num_params; ++i) {
        if (!is_ordered[i]) {
          add_to_order(i);
          break;
        }
      }
    }
  }

  return order;
}

}  // namespace detail
}  // namespace citcpp
