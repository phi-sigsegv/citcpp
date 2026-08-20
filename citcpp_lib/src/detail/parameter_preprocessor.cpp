#include "parameter_preprocessor.hpp"

#include <algorithm>
#include <citcpp/constraints.hpp>
#include <deque>
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

std::vector<std::set<unsigned int>> build_adjacency_list(
    const citcpp::detail::internal_model& model) {

  using namespace citcpp;
  using namespace citcpp::detail;

  const auto& input_model = model.get_input_model();
  const auto& parameters = input_model.get_parameters();
  const unsigned int num_params = static_cast<unsigned int>(parameters.size());

  // Map parameter names to indices for efficient lookups
  std::unordered_map<std::string, unsigned int> name_to_idx;
  for (unsigned int i = 0; i < num_params; ++i) {
    name_to_idx[parameters[i].get_name()] = i;
  }

  // Build adjacency list (interaction graph)
  std::vector<std::set<unsigned int>> adj(num_params);

  if (num_params == 0) {
    return adj;
  }

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

  return adj;
}

}  // namespace

namespace citcpp {
namespace detail {

std::vector<std::vector<unsigned int>> compute_parameter_partitions(
    const internal_model& model,
    const std::vector<unsigned int>& parameter_order) {

  // Step 1: Build adjacency list (interaction graph).
  std::vector<std::set<unsigned int>> adj(build_adjacency_list(model));

  // Step 2: Traverse Graph to Find Connected Components.
  const unsigned int num_params = model.get_parameter_num_values().size();
  std::set<unsigned int> visited;
  std::vector<std::vector<unsigned int>> partitions;

  for (unsigned int i = 0; i < num_params; ++i) {
    unsigned int param = parameter_order[i];
    if (visited.insert(param).second) {
      // Did not process parameter yet.
      // Start a new disjoint partition with it.
      std::vector<unsigned int> current_component;
      std::deque<unsigned int> queue;
      queue.push_back(param);

      // Since we process parameters in decreasing order, the initial
      // parameter added to the current_component is always the lowest one
      // in the order.
      // This is because in the following while loop we only add parameters
      // not visited yet, which can only be the ones with an order greater than
      // the initial parameter of a component.

      while (!queue.empty()) {
        param = queue.front();
        queue.pop_front();

        // Add the parameter to the component.
        current_component.push_back(param);

        for (unsigned connected_param : adj[param]) {
          if (visited.insert(connected_param).second) {
            // Did not process the connected parameter yet.
            queue.push_back(connected_param);
          }
        }
      }

      partitions.push_back(std::move(current_component));
    }
  }

  // Since we process parameters in decreasing order, the initial
  // parameter added to each_component is always the lowest one
  // in the order. Thus, it defines how partitions must be ordered,
  // which is again already correct by construction, due to processing
  // parameters in decreasing order in the outer loop.
  // So there's nothing to be sorted explicitly.

  return partitions;
}

std::vector<unsigned int> compute_mcmf_variable_order(
    const internal_model& model) {

  const unsigned int num_params = model.get_parameter_num_values().size();

  if (num_params == 0) {
    return {};
  }

  // Build adjacency list (interaction graph)
  std::vector<std::set<unsigned int>> adj(build_adjacency_list(model));

  std::vector<unsigned int> order;
  order.reserve(num_params);
  std::vector<bool> is_ordered(num_params, false);
  std::vector<unsigned int> ordered_neighbors_count(num_params, 0);

  // Step 1: Find the parameter with the largest connectivity (Master)
  int first_param = -1;
  unsigned int max_connectivity = 0;

  for (unsigned int i = 0; i < num_params; ++i) {
    if (adj[i].size() > max_connectivity) {
      max_connectivity = static_cast<unsigned int>(adj[i].size());
      first_param = static_cast<int>(i);
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
        best_p = static_cast<int>(i);
        max_proximity = ordered_neighbors_count[i];
        max_total_connectivity = static_cast<unsigned int>(adj[i].size());
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

std::vector<unsigned int> compute_deceasing_domain_size_variable_order(
    const internal_model& model) {

  const auto& param_num_values = model.get_parameter_num_values();

  std::vector<unsigned int> order(param_num_values.size());
  std::iota(order.begin(), order.end(), 0);

  std::sort(order.begin(), order.end(),
            [&param_num_values](const unsigned int& index1,
                                const unsigned int& index2) {
              if (param_num_values[index1] != param_num_values[index2]) {
                return param_num_values[index1] > param_num_values[index2];
              }
              return index1 < index2;
            });

  return order;
}

}  // namespace detail
}  // namespace citcpp
