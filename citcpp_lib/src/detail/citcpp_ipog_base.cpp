#include "citcpp_ipog_base.hpp"

#include <algorithm>
#include <numeric>
#include <unordered_map>
#include <unordered_set>

#include "constraint_handler_sylvan_ldd.hpp"
#include "constraint_handler_void.hpp"

namespace {

bool is_covered_by(const citcpp::detail::internal_relation& rel,
                   const std::vector<unsigned int>& parameter_index_map,
                   unsigned int interaction_strength) {

  if (rel.get_specified_interaction_strength() > interaction_strength) {
    return false;
  }

  std::unordered_set<unsigned int> param_indices(parameter_index_map.begin(),
                                                 parameter_index_map.end());

  for (auto param_idx : rel.get_parameter_index_map()) {
    if (param_indices.find(param_idx) == param_indices.end()) {
      return false;
    }
  }

  return true;
}

bool is_covered_by(const std::vector<unsigned int>& parameter_index_map,
                   unsigned int interaction_strength,
                   const citcpp::detail::internal_relation& rel) {

  if (interaction_strength > rel.get_specified_interaction_strength()) {
    return false;
  }

  std::unordered_set<unsigned int> param_indices(
      rel.get_parameter_index_map().begin(),
      rel.get_parameter_index_map().end());

  for (auto param_idx : parameter_index_map) {
    if (param_indices.find(param_idx) == param_indices.end()) {
      return false;
    }
  }

  return true;
}

}  // namespace

namespace citcpp {
namespace detail {

std::vector<unsigned int> citcpp_ipog_base::create_parameter_index_map(
    const internal_model& internal_model) {

  const std::vector<unsigned int>& param_num_values =
      internal_model.get_parameter_num_values();

  std::vector<unsigned int> parameter_index_map(param_num_values.size());
  std::iota(parameter_index_map.begin(), parameter_index_map.end(), 0);

  std::sort(parameter_index_map.begin(), parameter_index_map.end(),
            [&param_num_values](const unsigned int& index1,
                                const unsigned int& index2) {
              return param_num_values[index1] > param_num_values[index2];
            });

  return parameter_index_map;
}

std::vector<unsigned int> citcpp_ipog_base::create_parameter_index_map(
    const std::vector<internal_relation>& relations,
    const internal_model& internal_model) {

  std::vector<unsigned int> parameter_index_map(
      create_parameter_index_map(internal_model));

  // We remove all parameter indices from the index mapping, which
  // do not appear in any of the parameter index mappings of the relations.
  // This ensures that all index mappings are consistent regarding their
  // parameter orders.
  std::unordered_set<unsigned int> param_indices;
  for (const auto& relation : relations) {
    for (unsigned int param_idx : relation.get_parameter_index_map()) {
      param_indices.insert(param_idx);
    }
  }

  auto param_idx_it = parameter_index_map.begin();
  while (param_idx_it != parameter_index_map.end()) {
    if (param_indices.find(*param_idx_it) != param_indices.end()) {
      ++param_idx_it;
    } else {
      // The parameter is irrelevant concerning coverage, since it does not
      // appear in any relation.
      param_idx_it = parameter_index_map.erase(param_idx_it);
    }
  }

  return parameter_index_map;
}

std::vector<internal_relation> citcpp_ipog_base::create_relations(
    const model& model, const internal_model& internal_model, int strength) {

  const std::vector<unsigned int>& param_num_values =
      internal_model.get_parameter_num_values();

  std::vector<internal_relation> relations;

  if (strength >= 1) {
    std::vector<unsigned int> parameter_index_map =
        create_parameter_index_map(internal_model);

    relations.emplace_back(std::move(parameter_index_map), strength);
  } else {
    std::unordered_map<std::string, unsigned int> param_name_to_index_map;
    {
      unsigned int param_index = 0;
      for (const auto& param : model.get_parameters()) {
        param_name_to_index_map[param.get_name()] = param_index;
        ++param_index;
      }
    }

    for (const auto& relation : model.get_relations()) {
      std::vector<unsigned int> parameter_index_map;

      // Find the indices of referenced parameters and add them to the relation.
      for (const auto& param_ref : relation.get_parameters()) {
        unsigned int param_idx = param_name_to_index_map[param_ref.get_name()];
        parameter_index_map.push_back(param_idx);
      }

      std::sort(parameter_index_map.begin(), parameter_index_map.end(),
                [&param_num_values](const unsigned int& index1,
                                    const unsigned int& index2) {
                  return param_num_values[index1] > param_num_values[index2];
                });

      // We walk over the relation created so far, and remove any relation,
      // which is covered by the one we are currently creating.
      auto rel_it = relations.begin();
      bool is_covered_by_other_relation = false;
      while (rel_it != relations.end()) {
        const internal_relation& other_rel = *rel_it;

        if (is_covered_by(parameter_index_map,
                          relation.get_interaction_strength(), other_rel)) {

          // The relation we are currently creating is covered by an already
          // existing one. Thus there is no point in adding it.
          // We can also abort this loop, since by transitivity, a relation
          // which would be covered by the one we are currently creating, would
          // also be covered by 'other_rel'.
          is_covered_by_other_relation = true;
          break;
        }

        if (is_covered_by(other_rel, parameter_index_map,
                          relation.get_interaction_strength())) {
          // The relation we are currently creating covers an already
          // existing one. Thus, we remove that other relation, since
          // it is superfluous.
          rel_it = relations.erase(rel_it);
        } else {
          ++rel_it;
        }
      }

      if (!is_covered_by_other_relation) {
        relations.emplace_back(std::move(parameter_index_map),
                               relation.get_interaction_strength());
      }
    }
  }

  return relations;
}

unsigned int citcpp_ipog_base::length_of_common_param_prefix(
    const citcpp::detail::internal_relation& rel,
    const std::vector<unsigned int>& parameter_index_map) {

  unsigned int param_idx = 0;
  for (; param_idx < parameter_index_map.size() &&
         param_idx < rel.get_parameter_index_map().size();
       ++param_idx) {

    if (parameter_index_map[param_idx] !=
        rel.get_parameter_index_map()[param_idx]) {
      return param_idx;
    }
  }

  return param_idx;
}

std::unique_ptr<constraint_handler> citcpp_ipog_base::create_constraint_handler(
    const internal_model& model, int num_worker_threads) {

  if (model.get_input_model().get_constraints().empty()) {
    return std::make_unique<constraint_handler_void>(model);
  } else {
    return std::make_unique<constraint_handler_sylvan_ldd>(model,
                                                           num_worker_threads);
  }
}

}  // namespace detail
}  // namespace citcpp
