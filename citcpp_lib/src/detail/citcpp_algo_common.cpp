#include "citcpp_algo_common.hpp"

#include <numeric>
#include <unordered_map>
#include <unordered_set>

#include "parameter_preprocessor.hpp"

namespace {

class num_combos_per_param_combo_functor {
  public:
    num_combos_per_param_combo_functor(
        const citcpp::detail::internal_model& model,
        const citcpp::detail::internal_test_set& test_set,
        const unsigned int param_combo_sizes,
        const citcpp::detail::bitset_uint64::size_type
            bitset_backing_array_size)
        : model_(model),
          test_set_(test_set),
          weights_(param_combo_sizes),
          values_combo_bitset_(bitset_backing_array_size),
          num_combos_{0, 0} {}

    bool operator()(const citcpp::detail::param_vector& param_indices) {
      using namespace citcpp::detail;

      bitset_uint64::size_type bitset_size = 1;
      for (const uint16_t p : param_indices) {
        bitset_size *= model_.get_parameter_num_values()[p];
      }
      values_combo_bitset_.reset_with_new_size(bitset_size);

      // Pre-calculate weights for index computation.
      // Those are used to compute an index into the bitset. To do so, we treat
      // the number of values of each parameter as a kind of radix. Consider
      // three parameters p_0, p_1, p_2. Now say that v_i is the number of
      // values for p_i. If we now have values x_0, x_1, x_2, then the index
      // is x_0 * v_1 * v_2 + x_1 * v_2 + x_2.
      bitset_uint64::size_type weight = 1;
      for (int i = static_cast<int>(param_indices.size() - 1); i >= 0; --i) {
        weights_[i] = weight;
        weight *= model_.get_parameter_num_values()[param_indices[i]];
      }

      num_combos_.num_combos_to_cover += bitset_size;

      for (const auto& test : test_set_.get_list_of_tests()) {
        bitset_uint64::size_type index = 0;
        bool found_dont_care = false;
        for (std::size_t i = 0; i < param_indices.size(); ++i) {
          const int param_value = test.get_values()[param_indices[i]];

          if (param_value < 0) {
            // We have found a don't care value for that combination in
            // the considered test in one of the parameters.
            // There is nothing to be updated concerning the
            // coverage.
            found_dont_care = true;
            break;
          }

          index += param_value * weights_[i];
        }

        if (!found_dont_care) {
          if (!values_combo_bitset_.test_and_set(index)) {
            num_combos_.num_covered_combos++;
          }
        }
      }

      return true;
    }

    const citcpp::detail::number_of_combinations& get_number_of_combos() const {
      return num_combos_;
    }

  private:
    const citcpp::detail::internal_model& model_;
    const citcpp::detail::internal_test_set& test_set_;
    std::vector<citcpp::detail::bitset_uint64::size_type> weights_;
    citcpp::detail::bitset_uint64 values_combo_bitset_;
    citcpp::detail::number_of_combinations num_combos_;
};

bool is_covered_by(const citcpp::detail::internal_relation& rel,
                   const std::vector<unsigned int>& parameter_index_map,
                   unsigned int interaction_strength) {

  if (rel.get_specified_interaction_strength() > interaction_strength) {
    return false;
  }

  std::unordered_set<unsigned int> param_indices(parameter_index_map.begin(),
                                                 parameter_index_map.end());

  for (const unsigned int param_idx : rel.get_parameter_index_map()) {
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

  for (const unsigned int param_idx : parameter_index_map) {
    if (param_indices.find(param_idx) == param_indices.end()) {
      return false;
    }
  }

  return true;
}

}  // namespace

namespace citcpp {
namespace detail {

number_of_combinations get_number_of_combinations(
    unsigned int n, const internal_model& model,
    const std::vector<unsigned int>& parameter_index_map, unsigned int t,
    bool fixed_last_parameter, const internal_test_set& test_set) {

  const unsigned int product_of_max_parameter_sizes =
      get_product_of_max_n_parameter_sizes(
          static_cast<unsigned int>(parameter_index_map.size()), t, model,
          parameter_index_map);

  num_combos_per_param_combo_functor per_param_combo_functor(
      model, test_set, t, product_of_max_parameter_sizes);

  param_combo_iterator param_combo_it(n, t, parameter_index_map,
                                      fixed_last_parameter);
  param_combo_it.visit_all_parameter_combinations(per_param_combo_functor);

  return per_param_combo_functor.get_number_of_combos();
}

std::vector<unsigned int> create_parameter_index_map(
    const std::vector<internal_relation>& relations,
    const internal_model& internal_model) {

  std::vector<unsigned int> parameter_index_map(
      compute_decreasing_domain_size_mcmf_as_tie_variable_order(
          internal_model));

  // We remove all parameter indices from the index mapping, which
  // do not appear in any of the parameter index mappings of the relations.
  // This ensures that all index mappings are consistent regarding their
  // parameter orders.
  std::unordered_set<unsigned int> param_indices;
  for (const auto& relation : relations) {
    for (const unsigned int param_idx : relation.get_parameter_index_map()) {
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

std::vector<internal_relation> create_relations_for_cagen(
    const model& model, const internal_model& internal_model, int strength) {
  std::vector<internal_relation> relations;

  if (strength >= 1) {
    std::vector<unsigned int> parameter_index_map(
        internal_model.get_parameter_num_values().size());
    std::iota(parameter_index_map.begin(), parameter_index_map.end(), 0);

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

}  // namespace detail
}  // namespace citcpp
