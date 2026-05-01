#include "citcpp_sylvan_ldd.hpp"

#include <sylvan.h>
#include <sylvan_int.h>

#include <algorithm>
#include <fstream>
#include <utility>

#include "lace_lifecycle.hpp"

using namespace ::sylvan;

namespace {

inline int match_ldds(MDD* one, MDD* two) {
  MDD m1 = *one, m2 = *two;
  if (m1 == lddmc_false || m2 == lddmc_false) return 0;
  mddnode_t n1 = LDD_GETNODE(m1), n2 = LDD_GETNODE(m2);
  uint32_t v1 = mddnode_getvalue(n1), v2 = mddnode_getvalue(n2);
  while (v1 != v2) {
    if (v1 < v2) {
      m1 = mddnode_getright(n1);
      if (m1 == lddmc_false) return 0;
      n1 = LDD_GETNODE(m1);
      v1 = mddnode_getvalue(n1);
    } else if (v1 > v2) {
      m2 = mddnode_getright(n2);
      if (m2 == lddmc_false) return 0;
      n2 = LDD_GETNODE(m2);
      v2 = mddnode_getvalue(n2);
    }
  }
  *one = m1;
  *two = m2;
  return 1;
}

inline MDD sylvan_one_level_set_add_value(MDD mdd, uint32_t value) {
  if (mdd <= lddmc_true) return lddmc_makenode(value, lddmc_true, mdd);

  mddnode_t n = LDD_GETNODE(mdd);
  uint32_t n_value = mddnode_getvalue(n);
  if (n_value < value)
    return lddmc_makenode(
        n_value, lddmc_true,
        sylvan_one_level_set_add_value(mddnode_getright(n), value));
  if (n_value > value) return lddmc_makenode(value, lddmc_true, mdd);
  /* (n_value == value) */ return mdd;
}

inline MDD sylvan_one_level_set_union(MDD a, MDD b) {
  if (a == b) return a;
  if (a == lddmc_false) return b;
  if (b == lddmc_false) return a;

  mddnode_t na = LDD_GETNODE(a);
  mddnode_t nb = LDD_GETNODE(b);

  const uint32_t na_value = mddnode_getvalue(na);
  const uint32_t nb_value = mddnode_getvalue(nb);

  if (na_value < nb_value) {
    return lddmc_makenode(na_value, lddmc_true,
                          sylvan_one_level_set_union(mddnode_getright(na), b));
  } else if (na_value == nb_value) {
    return lddmc_makenode(
        na_value, lddmc_true,
        sylvan_one_level_set_union(mddnode_getright(na), mddnode_getright(nb)));
  } else /* na_value > nb_value */ {
    return lddmc_makenode(nb_value, lddmc_true,
                          sylvan_one_level_set_union(a, mddnode_getright(nb)));
  }
}

TASK_3(MDD, sylvan_make_node, uint32_t, value, MDD, ifeq, MDD, ifneq) {
  return lddmc_makenode(value, ifeq, ifneq);
}

TASK_3(MDD, sylvan_create_relational_proposition, citcpp::relational_operator,
       op, uint32_t, value, uint32_t, variable_domain_size) {

  switch (op) {
    case citcpp::relational_operator::NEQ: {
      MDD prop_ldd = lddmc_false;
      for (int i = variable_domain_size - 1; i >= 0; --i) {
        if (i != value) {
          prop_ldd = lddmc_makenode(i, lddmc_true, prop_ldd);
        }
      }

      return prop_ldd;
    }
    case citcpp::relational_operator::LT: {
      MDD prop_ldd = lddmc_false;
      for (int i = variable_domain_size - 1; i >= 0; --i) {
        if (i < value) {
          prop_ldd = lddmc_makenode(i, lddmc_true, prop_ldd);
        }
      }

      return prop_ldd;
    }
    case citcpp::relational_operator::LE: {
      MDD prop_ldd = lddmc_false;
      for (int i = variable_domain_size - 1; i >= 0; --i) {
        if (i <= value) {
          prop_ldd = lddmc_makenode(i, lddmc_true, prop_ldd);
        }
      }

      return prop_ldd;
    }
    case citcpp::relational_operator::GE: {
      MDD prop_ldd = lddmc_false;
      for (int i = variable_domain_size - 1; i >= 0; --i) {
        if (i >= value) {
          prop_ldd = lddmc_makenode(i, lddmc_true, prop_ldd);
        }
      }

      return prop_ldd;
    }
    case citcpp::relational_operator::GT: {
      MDD prop_ldd = lddmc_false;
      for (int i = variable_domain_size - 1; i >= 0; --i) {
        if (i > value) {
          prop_ldd = lddmc_makenode(i, lddmc_true, prop_ldd);
        }
      }

      return prop_ldd;
    }
    case citcpp::relational_operator::EQ:
    default: {
      return lddmc_makenode(value, lddmc_true, lddmc_false);
    }
  }
}

TASK_4(MDD, sylvan_create_projection_cube, const uint32_t*, target_variables,
       int, num_target_variables, const uint32_t*, local_variables, int,
       num_local_variables) {

  MDD cube = lddmc_true;
  int local_var_idx = num_local_variables - 1;
  bool found_last_local_var = false;
  bool found_non_local_target_var = false;
  for (int i = num_target_variables - 1; i >= 0; --i) {
    uint32_t var = target_variables[i];

    if (local_var_idx >= 0 && var == local_variables[local_var_idx]) {
      if (found_non_local_target_var) {
        cube = lddmc_makenode(1, cube, lddmc_false);
      } else {
        cube = lddmc_makenode(-1, cube, lddmc_false);
      }
      --local_var_idx;
      found_last_local_var = true;
    } else {
      // var is not part of local variables.
      found_non_local_target_var = true;
      if (found_last_local_var) {
        cube = lddmc_makenode(0, cube, lddmc_false);
      } else {
        cube = lddmc_makenode(-2, cube, lddmc_false);
      }
    }
  }

  return cube;
}

static const uint64_t CACHE_MDD_CONTAINS_PART_ASSIGN = (60LL << 40);

TASK_4(int, sylvan_sat_with_partial_assignment_recursive, MDD, ldd, uint32_t,
       var_idx, MDD, variables_cube, MDD, values_cube) {

  if (variables_cube == lddmc_true) {
    return 1;
  }
  if (ldd == lddmc_false) {
    return 0;
  }

  assert(ldd != lddmc_true);

  mddnode_t var_node = LDD_GETNODE(variables_cube);
  uint32_t variable = mddnode_getvalue(var_node);

  if (variable == var_idx) {
    // We reached a variable which we have an assignment for.
    mddnode_t value_node = LDD_GETNODE(values_cube);
    uint32_t value = mddnode_getvalue(value_node);
    // Now we scan the right nodes for a node with a corresponding value.
    // Only if we can find such, the assignment is valid.
    while (ldd != lddmc_false) {
      const mddnode_t nldd = LDD_GETNODE(ldd);
      const uint32_t v = mddnode_getvalue(nldd);
      if (v == value) {
        // We have found the value we are looking for. So recurse from here.
        // Use cached result if possible.
        uint64_t sat;
        if (!cache_get3(CACHE_MDD_CONTAINS_PART_ASSIGN, ldd, variables_cube,
                        values_cube, &sat)) {
          sat = CALL(sylvan_sat_with_partial_assignment_recursive,
                     mddnode_getdown(nldd), var_idx + 1,
                     mddnode_getdown(var_node), mddnode_getdown(value_node));
          cache_put3(CACHE_MDD_CONTAINS_PART_ASSIGN, ldd, variables_cube,
                     values_cube, sat);
        }

        return sat;
      }
      if (v > value) {
        // The value is greater then the one we are searching for.
        // That means that the value does not lead to a path to one.
        break;
      }

      // The value we are looking for might still be on this level, move
      // to the right (where higher values are stored).
      ldd = mddnode_getright(nldd);
    }

    return 0;
  } else {
    // We are not interested in the current variable, hence we have to follow
    // all paths.
    const mddnode_t nldd = LDD_GETNODE(ldd);
    uint64_t sat = 0;
    // Use cached result if possible.
    if (!cache_get3(CACHE_MDD_CONTAINS_PART_ASSIGN, ldd, variables_cube,
                    values_cube, &sat)) {
      /* right = */ SPAWN(sylvan_sat_with_partial_assignment_recursive,
                          mddnode_getright(nldd), var_idx, variables_cube,
                          values_cube);
      uint64_t down_sat =
          CALL(sylvan_sat_with_partial_assignment_recursive,
               mddnode_getdown(nldd), var_idx + 1, variables_cube, values_cube);
      uint64_t right_sat = SYNC(sylvan_sat_with_partial_assignment_recursive);
      sat = down_sat || right_sat;
      cache_put3(CACHE_MDD_CONTAINS_PART_ASSIGN, ldd, variables_cube,
                 values_cube, sat);
    }

    return sat;
  }
}

TASK_5(int, sylvan_sat_with_partial_assignment, MDD, ldd, const uint32_t*,
       variables, int, num_variables, const int*, values, int, num_values) {

  // First we create cubes that specifies which variable we have assignments
  // for, and what are the assigned values.
  MDD variables_cube = lddmc_true;
  MDD values_cube = lddmc_true;
  for (int v = num_variables - 1; v >= 0; --v) {
    const uint32_t var = variables[v];
    const int value = values[var];

    if (value >= 0) {
      variables_cube = lddmc_makenode(v, variables_cube, lddmc_false);
      values_cube = lddmc_makenode(value, values_cube, lddmc_false);
    }
  }

  lddmc_refs_push(variables_cube);
  lddmc_refs_push(values_cube);

  const int sat = CALL(sylvan_sat_with_partial_assignment_recursive, ldd, 0,
                       variables_cube, values_cube);

  lddmc_refs_pop(2);

  return sat;
}

static const uint64_t CACHE_MDD_GET_VALID_ASSIGNS = (61LL << 40);

TASK_5(MDD, sylvan_get_valid_variable_assignments_recursive, MDD, ldd, uint32_t,
       var_idx, MDD, variables_cube, MDD, values_cube, int, variable_index) {

  if (variables_cube == lddmc_true && variable_index < var_idx) {
    // No more valid value for the parameter on this path.
    return lddmc_true;
  }
  if (ldd == lddmc_false) {
    // No valid value for the parameter on this path.
    return lddmc_false;
  }

  assert(ldd != lddmc_true);

  mddnode_t var_node = LDD_GETNODE(variables_cube);
  uint32_t variable = mddnode_getvalue(var_node);

  if (variables_cube != lddmc_true && variable == var_idx) {
    // We reached a variable, which is part of the partial assignment.
    mddnode_t value_node = LDD_GETNODE(values_cube);
    uint32_t value = mddnode_getvalue(value_node);
    while (ldd != lddmc_false) {
      const mddnode_t nldd = LDD_GETNODE(ldd);
      const uint32_t v = mddnode_getvalue(nldd);
      if (v == value) {
        // We have found the value we are looking for. So recurse from here.
        // Use cached result if possible.
        MDD collected_values;
        if (!cache_get4(CACHE_MDD_GET_VALID_ASSIGNS, ldd, variables_cube,
                        values_cube, variable_index, &collected_values)) {
          collected_values = CALL(
              sylvan_get_valid_variable_assignments_recursive,
              mddnode_getdown(nldd), var_idx + 1, mddnode_getdown(var_node),
              mddnode_getdown(value_node), variable_index);
          cache_put4(CACHE_MDD_GET_VALID_ASSIGNS, ldd, variables_cube,
                     values_cube, variable_index, collected_values);
        }

        return collected_values;
      }
      if (v > value) {
        // The value is greater then the one we are searching for.
        // That means that the value does not lead to a path to one.
        break;
      }

      // The value we are looking for might still be on this level, move
      // to the right (where higher values are stored).
      ldd = mddnode_getright(nldd);
    }

    // No valid value for the parameter on this path.
    return lddmc_false;
  } else {
    // We have to follow all paths.
    const mddnode_t nldd = LDD_GETNODE(ldd);
    MDD collected_values;
    // Use cached result if possible.
    if (!cache_get4(CACHE_MDD_GET_VALID_ASSIGNS, ldd, variables_cube,
                    values_cube, variable_index, &collected_values)) {
      /* right = */ lddmc_refs_spawn(
          SPAWN(sylvan_get_valid_variable_assignments_recursive,
                mddnode_getright(nldd), var_idx, variables_cube, values_cube,
                variable_index));
      MDD down_set = CALL(sylvan_get_valid_variable_assignments_recursive,
                          mddnode_getdown(nldd), var_idx + 1, variables_cube,
                          values_cube, variable_index);
      lddmc_refs_push(down_set);
      MDD right_set = lddmc_refs_sync(
          SYNC(sylvan_get_valid_variable_assignments_recursive));
      lddmc_refs_pop(1);
      if (variable_index == var_idx) {
        // We have reached the variable whose valid values we want to collect.
        if (down_set == lddmc_true) {
          // The down node leads to true, which means that a path suffix exists
          // that is consistent with the partial assignment. So from this node,
          // the current value is valid, and maybe also greater values, which we
          // find by following the right node.
          const uint32_t v = mddnode_getvalue(nldd);
          collected_values = sylvan_one_level_set_add_value(right_set, v);
        } else {
          // The current value is not feasible on this path. So we just collect
          // values from the right node where we potentially have greater valid
          // values.
          collected_values = right_set;
        }
      } else {
        // The current variable in not the one we want to collect values from.
        // Either that variable is still to come in the variable order, or we
        // reached a depth deeper than the variable index. In both cases, we
        // just collect the union of values resulting from following down and
        // right nodes.
        //
        // If the variable is still to come, then we either get "false" or a set
        // of valid values.
        //
        // If we have already reached a depth deeper than the variable, then we
        // either get "true" or "false", which the union operation returns
        // "true" for, if at least one of the arguments is "true". This is
        // exactly what we want in this case: Returning "true" on a path suffix
        // means that it is consistent with the partial assignment. So the
        // values found for the variable further up the diagram are all valid.
        collected_values = sylvan_one_level_set_union(down_set, right_set);
      }
      cache_put4(CACHE_MDD_GET_VALID_ASSIGNS, ldd, variables_cube, values_cube,
                 variable_index, collected_values);
    }

    return collected_values;
  }
}

TASK_6(MDD, sylvan_get_valid_variable_assignments, MDD, ldd, const uint32_t*,
       variables, int, num_variables, const int*, values, int, num_values,
       uint32_t, variable) {

  // First we create cubes that specify which variable we have assignments
  // for, and what are the assigned values.
  MDD variables_cube = lddmc_true;
  MDD values_cube = lddmc_true;
  int variable_index = -1;
  for (int v = num_variables - 1; v >= 0; --v) {
    const uint32_t var = variables[v];
    if (var == variable) {
      variable_index = v;
    }

    const int value = values[var];
    if (value >= 0) {
      variables_cube = lddmc_makenode(v, variables_cube, lddmc_false);
      values_cube = lddmc_makenode(value, values_cube, lddmc_false);
    }
  }

  if (variable_index < 0) {
    // The variable which we want to search values for is unconstrained.
    // We report this by returning true.
    return lddmc_true;
  }

  lddmc_refs_push(variables_cube);
  lddmc_refs_push(values_cube);

  const MDD collected_values =
      CALL(sylvan_get_valid_variable_assignments_recursive, ldd, 0,
           variables_cube, values_cube, variable_index);

  lddmc_refs_pop(2);

  return collected_values;
}

TASK_2(MDD, sylvan_create_cube_from_assignments, const int*, assignments, int,
       num_assignments) {

  MDD cube = lddmc_true;
  for (int var = num_assignments - 1; var >= 0; --var) {
    const int value = assignments[var];
    if (value >= 0) {
      cube = lddmc_makenode(value, cube, lddmc_false);
    }
  }

  return cube;
}

static const uint64_t CACHE_MDD_FULL_SAT_ONE_PART_ASSIGN = (62LL << 40);

TASK_4(MDD, sylvan_full_sat_one_under_partial_assignment_recursive, MDD, ldd,
       uint32_t, var_idx, MDD, variables_cube, MDD, values_cube) {

  if (ldd == lddmc_true) {
    return lddmc_true;
  }
  if (ldd == lddmc_false) {
    return lddmc_false;
  }

  mddnode_t var_node = LDD_GETNODE(variables_cube);

  if (variables_cube != lddmc_true && mddnode_getvalue(var_node) == var_idx) {
    // We reached a variable which we have an assignment for.
    mddnode_t value_node = LDD_GETNODE(values_cube);
    uint32_t value = mddnode_getvalue(value_node);
    // Now we scan the right nodes for a node with a corresponding value.
    // Only if we can find such, the path which we are currently following
    // can lead to a full assignment consistent with the partial assignment.
    while (ldd != lddmc_false) {
      const mddnode_t nldd = LDD_GETNODE(ldd);
      const uint32_t v = mddnode_getvalue(nldd);
      if (v == value) {
        // We have found the value we are looking for. So recurse from here.
        // Use cached result if possible.
        MDD full_sat_one_cube;
        if (!cache_get3(CACHE_MDD_FULL_SAT_ONE_PART_ASSIGN, ldd, variables_cube,
                        values_cube, &full_sat_one_cube)) {
          full_sat_one_cube =
              CALL(sylvan_full_sat_one_under_partial_assignment_recursive,
                   mddnode_getdown(nldd), var_idx + 1,
                   mddnode_getdown(var_node), mddnode_getdown(value_node));
          // Prepend the current assignment to the cube.
          if (full_sat_one_cube != lddmc_false) {
            full_sat_one_cube =
                lddmc_makenode(value, full_sat_one_cube, lddmc_false);
          }
          cache_put3(CACHE_MDD_FULL_SAT_ONE_PART_ASSIGN, ldd, variables_cube,
                     values_cube, full_sat_one_cube);
        }

        return full_sat_one_cube;
      }
      if (v > value) {
        // The value is greater then the one we are searching for.
        // That means that the current path cannot lead to a full assignment
        // consistent with the partial assignment.
        break;
      }

      // The value we are looking for might still be on this level, move
      // to the right (where higher values are stored).
      ldd = mddnode_getright(nldd);
    }

    return lddmc_false;
  } else {
    // We do not have an assignment for the current variable, hence we have
    // to follow all paths.
    const mddnode_t nldd = LDD_GETNODE(ldd);
    MDD full_sat_one_cube;
    // Use cached result if possible.
    if (!cache_get3(CACHE_MDD_FULL_SAT_ONE_PART_ASSIGN, ldd, variables_cube,
                    values_cube, &full_sat_one_cube)) {
      /* right = */ SPAWN(
          sylvan_full_sat_one_under_partial_assignment_recursive,
          mddnode_getright(nldd), var_idx, variables_cube, values_cube);
      MDD down_full_sat_one_cube =
          CALL(sylvan_full_sat_one_under_partial_assignment_recursive,
               mddnode_getdown(nldd), var_idx + 1, variables_cube, values_cube);
      lddmc_refs_push(down_full_sat_one_cube);
      MDD right_full_sat_one_cube =
          SYNC(sylvan_full_sat_one_under_partial_assignment_recursive);
      lddmc_refs_pop(1);

      if (down_full_sat_one_cube != lddmc_false) {
        full_sat_one_cube = lddmc_makenode(mddnode_getvalue(nldd),
                                           down_full_sat_one_cube, lddmc_false);
      } else if (right_full_sat_one_cube != lddmc_false) {
        full_sat_one_cube = right_full_sat_one_cube;
      } else {
        full_sat_one_cube = lddmc_false;
      }

      cache_put3(CACHE_MDD_FULL_SAT_ONE_PART_ASSIGN, ldd, variables_cube,
                 values_cube, full_sat_one_cube);
    }

    return full_sat_one_cube;
  }
}

TASK_5(MDD, sylvan_full_sat_one_under_partial_assignment, MDD, ldd,
       const uint32_t*, variables, int, num_variables, const int*, values, int,
       num_values) {

  // First we create cubes that specify which variables we have assignments
  // for, and what are the assigned values.
  MDD variables_cube = lddmc_true;
  MDD values_cube = lddmc_true;
  for (int v = num_variables - 1; v >= 0; --v) {
    const uint32_t var = variables[v];
    const int value = values[var];

    if (value >= 0) {
      variables_cube = lddmc_makenode(v, variables_cube, lddmc_false);
      values_cube = lddmc_makenode(value, values_cube, lddmc_false);
    }
  }

  lddmc_refs_push(variables_cube);
  lddmc_refs_push(values_cube);

  MDD full_sat_one_cube =
      CALL(sylvan_full_sat_one_under_partial_assignment_recursive, ldd, 0,
           variables_cube, values_cube);

  lddmc_refs_pop(2);

  return full_sat_one_cube;
}

TASK_3(MDD, sylvan_create_universe, const uint32_t*, variables, int, count,
       const unsigned int*, domain_sizes) {

  MDD universe = lddmc_true;
  for (int var_idx = count - 1; var_idx >= 0; --var_idx) {
    MDD prepended_universe = lddmc_false;
    for (int v = domain_sizes[variables[var_idx]] - 1; v >= 0; --v) {
      prepended_universe = lddmc_makenode(v, universe, prepended_universe);
    }

    universe = prepended_universe;
  }

  return universe;
}

static const uint64_t CACHE_MDD_INV_PROJ = (63LL << 40);

TASK_3(MDD, sylvan_inv_project, MDD, a, MDD, b, MDD, proj) {
  if (a == b) return a;
  if (a == lddmc_false || b == lddmc_false) return lddmc_false;

  mddnode_t p_node = LDD_GETNODE(proj);
  uint32_t p_val = mddnode_getvalue(p_node);
  if (p_val == (uint32_t)-2) return a;

  assert(a != lddmc_true);

  if (p_val == (uint32_t)-1) return b;

  if (p_val == 1) assert(b != lddmc_true);

  /* Test gc */
  sylvan_gc_test();

  if (p_val == 1) {
    if (!match_ldds(&a, &b)) return lddmc_false;
  }

  /* Access cache */
  MDD result;
  if (cache_get3(CACHE_MDD_INV_PROJ, a, b, proj, &result)) {
    return result;
  }

  /* Perform recursive calculation */
  mddnode_t na = LDD_GETNODE(a);
  MDD down;

  const MDD na_right = mddnode_getright(na);
  const MDD na_down = mddnode_getdown(na);
  const MDD p_down = mddnode_getdown(p_node);

  if (p_val == 1) {
    mddnode_t nb = LDD_GETNODE(b);
    const MDD nb_right = mddnode_getright(nb);
    const MDD nb_down = mddnode_getdown(nb);
    /* right = */ lddmc_refs_spawn(
        SPAWN(sylvan_inv_project, na_right, nb_right, proj));
    down = CALL(sylvan_inv_project, na_down, nb_down, p_down);
  } else {
    /* right = */ lddmc_refs_spawn(
        SPAWN(sylvan_inv_project, na_right, b, proj));
    down = CALL(sylvan_inv_project, na_down, b, p_down);
  }
  lddmc_refs_push(down);
  MDD right = lddmc_refs_sync(SYNC(sylvan_inv_project));
  lddmc_refs_pop(1);
  result = lddmc_makenode(mddnode_getvalue(na), down, right);

  /* Write to cache */
  cache_put3(CACHE_MDD_INV_PROJ, a, b, proj, result);

  return result;
}

#define sylvan_make_node(value, ifeq, ifneq) \
  RUN(sylvan_make_node, value, ifeq, ifneq)

#define sylvan_create_relational_proposition(op, value, variable_domain_size) \
  RUN(sylvan_create_relational_proposition, op, value, variable_domain_size)

#define sylvan_create_projection_cube(target_variables, num_target_variables, \
                                      local_variables, num_local_variables)   \
  RUN(sylvan_create_projection_cube, target_variables, num_target_variables,  \
      local_variables, num_local_variables)

#define sylvan_sat_with_partial_assignment(ldd, variables, num_variables, \
                                           values, num_values)            \
  RUN(sylvan_sat_with_partial_assignment, ldd, variables, num_variables,  \
      values, num_values)

#define sylvan_get_valid_variable_assignments(ldd, variables, num_variables, \
                                              values, num_values, variable)  \
  RUN(sylvan_get_valid_variable_assignments, ldd, variables, num_variables,  \
      values, num_values, variable)

#define sylvan_create_cube_from_assignments(assignments, num_assignments) \
  RUN(sylvan_create_cube_from_assignments, assignments, num_assignments)

#define sylvan_full_sat_one_under_partial_assignment(               \
    ldd, variables, num_variables, values, num_values)              \
  RUN(sylvan_full_sat_one_under_partial_assignment, ldd, variables, \
      num_variables, values, num_values)

#define sylvan_create_universe(variables, count, domain_sizes) \
  RUN(sylvan_create_universe, variables, count, domain_sizes)

#define sylvan_inv_project(a, b, proj) RUN(sylvan_inv_project, a, b, proj)

std::vector<uint32_t> get_common_variables(
    const std::vector<uint32_t>& lhs_vars,
    const std::vector<uint32_t>& rhs_vars) {

  int lhs_idx = 0;
  int rhs_idx = 0;
  std::vector<uint32_t> common_variables;

  while (lhs_idx < lhs_vars.size() || rhs_idx < rhs_vars.size()) {
    if (lhs_idx < lhs_vars.size()) {
      if (rhs_idx < rhs_vars.size()) {
        // Both have variables remaining.
        if (lhs_vars[lhs_idx] == rhs_vars[rhs_idx]) {
          common_variables.push_back(lhs_vars[lhs_idx]);
          ++lhs_idx;
          ++rhs_idx;
        } else if (lhs_vars[lhs_idx] < rhs_vars[rhs_idx]) {
          common_variables.push_back(lhs_vars[lhs_idx++]);
        } else {
          common_variables.push_back(rhs_vars[rhs_idx++]);
        }
      } else {
        // Only lhs has variables remaining.
        common_variables.push_back(lhs_vars[lhs_idx++]);
      }
    } else {
      // Only rhs has variables remaining. This is guaranteed due
      // to the while condition.
      common_variables.push_back(rhs_vars[rhs_idx++]);
    }
  }

  return common_variables;
}

struct interval {
    uint16_t lb;
    uint16_t ub;
};

uint32_t encode_interval(uint16_t lb, uint16_t ub) {
  uint32_t packed = ((uint32_t)ub << 16) | lb;

  return packed;
}

uint32_t encode_interval(interval ival) {
  return encode_interval(ival.lb, ival.ub);
}

interval decode_interval(uint32_t packed_interval) {
  uint16_t lb = packed_interval & 0xFFFF;
  uint16_t ub = (packed_interval & 0xFFFF0000) >> 16;
  return interval{lb, ub};
}

inline uint16_t MAX(uint16_t a, uint16_t b) { return (a > b) ? a : b; }
inline uint16_t MIN(uint16_t a, uint16_t b) { return (a < b) ? a : b; }

interval interval_intersection(interval a, interval b) {
  return interval{MAX(a.lb, b.lb), MIN(a.ub, b.ub)};
}

/**
 * Connected intervals form a contiguous range and their union
 * is well defined.
 */
int is_intervals_connected(interval a, interval b) {
  return (uint32_t)a.lb <= (uint32_t)b.ub + 1 &&
         (uint32_t)b.lb <= (uint32_t)a.ub + 1;
}

interval interval_union(interval a, interval b) {
  return interval{MIN(a.lb, b.lb), MAX(a.ub, b.ub)};
}

int is_valid(interval ival) { return ival.lb <= ival.ub; }

TASK_3(MDD, sylvan_idd_create_relational_proposition,
       citcpp::relational_operator, op, uint32_t, value, uint32_t,
       variable_domain_size) {

  switch (op) {
    case citcpp::relational_operator::NEQ: {
      MDD greater_ival =
          value >= variable_domain_size - 1
              ? lddmc_false
              : lddmc_makenode(
                    encode_interval(value + 1, variable_domain_size - 1),
                    lddmc_true, lddmc_false);

      MDD prop_ldd = value <= 0 ? greater_ival
                                : lddmc_makenode(encode_interval(0, value - 1),
                                                 lddmc_true, greater_ival);

      return prop_ldd;
    }
    case citcpp::relational_operator::LT: {
      MDD prop_ldd = value <= 0 ? lddmc_false
                                : lddmc_makenode(encode_interval(0, value - 1),
                                                 lddmc_true, lddmc_false);

      return prop_ldd;
    }
    case citcpp::relational_operator::LE: {
      MDD prop_ldd =
          lddmc_makenode(encode_interval(0, value), lddmc_true, lddmc_false);

      return prop_ldd;
    }
    case citcpp::relational_operator::GE: {
      MDD prop_ldd =
          lddmc_makenode(encode_interval(value, variable_domain_size - 1),
                         lddmc_true, lddmc_false);

      return prop_ldd;
    }
    case citcpp::relational_operator::GT: {
      MDD prop_ldd =
          value >= variable_domain_size - 1
              ? lddmc_false
              : lddmc_makenode(
                    encode_interval(value + 1, variable_domain_size - 1),
                    lddmc_true, lddmc_false);

      return prop_ldd;
    }
    case citcpp::relational_operator::EQ:
    default: {
      return lddmc_makenode(encode_interval(value, value), lddmc_true,
                            lddmc_false);
    }
  }
}

static const uint64_t CACHE_IDD_CONTAINS_PART_ASSIGN = (64LL << 40);

TASK_4(int, sylvan_idd_sat_with_partial_assignment_recursive, MDD, ldd,
       uint32_t, var_idx, MDD, variables_cube, MDD, values_cube) {

  if (variables_cube == lddmc_true) {
    return 1;
  }
  if (ldd == lddmc_false) {
    return 0;
  }

  assert(ldd != lddmc_true);

  mddnode_t var_node = LDD_GETNODE(variables_cube);
  uint32_t variable = mddnode_getvalue(var_node);

  if (variable == var_idx) {
    // We reached a variable which we have an assignment for.
    mddnode_t value_node = LDD_GETNODE(values_cube);
    uint32_t value = mddnode_getvalue(value_node);
    // Now we scan the right nodes for a node with a corresponding value.
    // Only if we can find such, the assignment is valid.
    while (ldd != lddmc_false) {
      const mddnode_t nldd = LDD_GETNODE(ldd);
      const uint32_t v = mddnode_getvalue(nldd);
      const interval ival = decode_interval(v);
      if (ival.lb <= value && value <= ival.ub) {
        // We have found the value we are looking for. So recurse from here.
        // Use cached result if possible.
        uint64_t sat;
        if (!cache_get3(CACHE_IDD_CONTAINS_PART_ASSIGN, ldd, variables_cube,
                        values_cube, &sat)) {
          sat = CALL(sylvan_idd_sat_with_partial_assignment_recursive,
                     mddnode_getdown(nldd), var_idx + 1,
                     mddnode_getdown(var_node), mddnode_getdown(value_node));
          cache_put3(CACHE_IDD_CONTAINS_PART_ASSIGN, ldd, variables_cube,
                     values_cube, sat);
        }

        return sat;
      }
      if (ival.lb > value) {
        // The value is greater then the one we are searching for.
        // That means that the value does not lead to a path to one.
        break;
      }

      // The value we are looking for might still be on this level, move
      // to the right (where higher values are stored).
      ldd = mddnode_getright(nldd);
    }

    return 0;
  } else {
    // We are not interested in the current variable, hence we have to follow
    // all paths.
    const mddnode_t nldd = LDD_GETNODE(ldd);
    uint64_t sat = 0;
    // Use cached result if possible.
    if (!cache_get3(CACHE_IDD_CONTAINS_PART_ASSIGN, ldd, variables_cube,
                    values_cube, &sat)) {
      /* right = */ SPAWN(sylvan_idd_sat_with_partial_assignment_recursive,
                          mddnode_getright(nldd), var_idx, variables_cube,
                          values_cube);
      uint64_t down_sat =
          CALL(sylvan_idd_sat_with_partial_assignment_recursive,
               mddnode_getdown(nldd), var_idx + 1, variables_cube, values_cube);
      uint64_t right_sat =
          SYNC(sylvan_idd_sat_with_partial_assignment_recursive);
      sat = down_sat || right_sat;
      cache_put3(CACHE_IDD_CONTAINS_PART_ASSIGN, ldd, variables_cube,
                 values_cube, sat);
    }

    return sat;
  }
}

TASK_5(int, sylvan_idd_sat_with_partial_assignment, MDD, ldd, const uint32_t*,
       variables, int, num_variables, const int*, values, int, num_values) {

  // First we create cubes that specifies which variable we have assignments
  // for, and what are the assigned values.
  MDD variables_cube = lddmc_true;
  MDD values_cube = lddmc_true;
  for (int v = num_variables - 1; v >= 0; --v) {
    const uint32_t var = variables[v];
    const int value = values[var];

    if (value >= 0) {
      variables_cube = lddmc_makenode(v, variables_cube, lddmc_false);
      values_cube = lddmc_makenode(value, values_cube, lddmc_false);
    }
  }

  lddmc_refs_push(variables_cube);
  lddmc_refs_push(values_cube);

  const int sat = CALL(sylvan_idd_sat_with_partial_assignment_recursive, ldd, 0,
                       variables_cube, values_cube);

  lddmc_refs_pop(2);

  return sat;
}

/**
 * This is just the method lddmc_intersect from the sylvan library modified
 * accordingly to work on encoded intervals.
 */
TASK_2(MDD, sylvan_idd_intersect, MDD, a, MDD, b) {
  /* Terminal cases */
  if (a == b) return a;
  if (a == lddmc_false || b == lddmc_false) return lddmc_false;
  assert(a != lddmc_true && b != lddmc_true);

  /* Test gc */
  sylvan_gc_test();

  sylvan_stats_count(LDD_INTERSECT);

  /* Get nodes */
  mddnode_t na = LDD_GETNODE(a);
  mddnode_t nb = LDD_GETNODE(b);
  uint32_t na_value = mddnode_getvalue(na);
  uint32_t nb_value = mddnode_getvalue(nb);
  interval a_ival = decode_interval(na_value);
  interval b_ival = decode_interval(nb_value);
  interval intersection = interval_intersection(a_ival, b_ival);

  /* Skip nodes if possible */
  while (!is_valid(intersection)) {
    // Simply comparing the encoded intervals is fine,
    // as order on the encoded intervals is equivalent to
    // order of the decoded intervals.
    if (na_value < nb_value) {
      a = mddnode_getright(na);
      if (a == lddmc_false) return lddmc_false;
      na = LDD_GETNODE(a);
      na_value = mddnode_getvalue(na);
      a_ival = decode_interval(na_value);
    } else if (nb_value < na_value) {
      b = mddnode_getright(nb);
      if (b == lddmc_false) return lddmc_false;
      nb = LDD_GETNODE(b);
      nb_value = mddnode_getvalue(nb);
      b_ival = decode_interval(nb_value);
    }
    intersection = interval_intersection(a_ival, b_ival);
  }

  // Once we get to this point, the intervals represented
  // by a and b have a non-empty intersection.

  /* Access cache */
  MDD result;
  if (cache_get3(CACHE_MDD_INTERSECT, a, b, 0, &result)) {
    sylvan_stats_count(LDD_INTERSECT_CACHED);
    return result;
  }

  /* Perform recursive calculation */
  const MDD na_right = mddnode_getright(na);
  const MDD nb_right = mddnode_getright(nb);
  const MDD na_down = mddnode_getdown(na);
  const MDD nb_down = mddnode_getdown(nb);

  if (a_ival.ub == b_ival.ub) {
    // The upper bounds of both intervals coincide. So when moving to greater
    // intervals (the right), there won't be any interval that can have an
    // intersection with a_ival or b_ival.
    lddmc_refs_spawn(SPAWN(sylvan_idd_intersect, na_right, nb_right));
  } else if (na_value < nb_value) {
    // The upper bound of a is lower than the upper bound of b.
    // Thus, we fetch the next greater interval with respect to values
    // from the variable associated to a.
    // We need to keep the interval b however, since it can happen that
    // the next greater interval from a also has an intersection with
    // the current interval from b.
    lddmc_refs_spawn(SPAWN(sylvan_idd_intersect, na_right, b));
  } else {
    // The upper bound of b is lower than the upper bound of a.
    // Thus, we fetch the next greater interval with respect to values
    // from the variable associated to b.
    // We need to keep the interval a however, since it can happen that
    // the next greater interval from b also has an intersection with
    // the current interval from a.
    lddmc_refs_spawn(SPAWN(sylvan_idd_intersect, a, nb_right));
  }
  MDD down = CALL(sylvan_idd_intersect, na_down, nb_down);
  lddmc_refs_push(down);
  MDD right = lddmc_refs_sync(SYNC(sylvan_idd_intersect));
  lddmc_refs_pop(1);
  result = lddmc_makenode(encode_interval(intersection), down, right);

  /* Write to cache */
  if (cache_put3(CACHE_MDD_INTERSECT, a, b, 0, result))
    sylvan_stats_count(LDD_INTERSECT_CACHEDPUT);

  return result;
}

/**
 * This is just the method lddmc_join from the sylvan library modified
 * accordingly to work on encoded intervals.
 *
 * proj: -2 (end; quantify rest), -1 (end; keep rest), 0 (quantify), 1 (keep)
 */
TASK_4(MDD, sylvan_idd_join, MDD, a, MDD, b, MDD, a_proj, MDD, b_proj) {
  if (a == lddmc_false || b == lddmc_false) return lddmc_false;

  /* Test gc */
  sylvan_gc_test();

  mddnode_t n_a_proj = LDD_GETNODE(a_proj);
  mddnode_t n_b_proj = LDD_GETNODE(b_proj);
  uint32_t a_proj_val = mddnode_getvalue(n_a_proj);
  uint32_t b_proj_val = mddnode_getvalue(n_b_proj);

  while (a_proj_val == 0 && b_proj_val == 0) {
    a_proj = mddnode_getdown(n_a_proj);
    b_proj = mddnode_getdown(n_b_proj);
    n_a_proj = LDD_GETNODE(a_proj);
    n_b_proj = LDD_GETNODE(b_proj);
    a_proj_val = mddnode_getvalue(n_a_proj);
    b_proj_val = mddnode_getvalue(n_b_proj);
  }

  if (a_proj_val == (uint32_t)-2) return b;  // no a left
  if (b_proj_val == (uint32_t)-2) return a;  // no b left
  if (a_proj_val == (uint32_t)-1 && b_proj_val == (uint32_t)-1)
    return CALL(sylvan_idd_intersect, a, b);

  // At this point, only proj_val {-1, 0, 1}; max one with -1; max one with 0.
  const int keep_a = a_proj_val != 0;
  const int keep_b = b_proj_val != 0;

  /* Get nodes */
  mddnode_t na = LDD_GETNODE(a);
  mddnode_t nb = LDD_GETNODE(b);
  uint32_t na_value = mddnode_getvalue(na);
  uint32_t nb_value = mddnode_getvalue(nb);
  interval a_ival = decode_interval(na_value);
  interval b_ival = decode_interval(nb_value);
  interval intersection = interval_intersection(a_ival, b_ival);

  if (keep_a && keep_b) {
    // If both 'keep', then match values
    while (!is_valid(intersection)) {
      // Simply comparing the encoded intervals is fine,
      // as order on the encoded intervals is equivalent to
      // order of the decoded intervals.
      if (na_value < nb_value) {
        a = mddnode_getright(na);
        if (a == lddmc_false) return lddmc_false;
        na = LDD_GETNODE(a);
        na_value = mddnode_getvalue(na);
        a_ival = decode_interval(na_value);
      } else if (nb_value < na_value) {
        b = mddnode_getright(nb);
        if (b == lddmc_false) return lddmc_false;
        nb = LDD_GETNODE(b);
        nb_value = mddnode_getvalue(nb);
        b_ival = decode_interval(nb_value);
      }
      intersection = interval_intersection(a_ival, b_ival);
    }
  }

  sylvan_stats_count(LDD_JOIN);

  /* Access cache */
  MDD result;
  if (cache_get4(CACHE_MDD_JOIN, a, b, a_proj, b_proj, &result)) {
    sylvan_stats_count(LDD_JOIN_CACHED);
    return result;
  }

  /* Perform recursive calculation */
  uint32_t val;
  MDD down;

  const MDD na_right = mddnode_getright(na);
  const MDD na_down = mddnode_getdown(na);
  const MDD nb_right = mddnode_getright(nb);
  const MDD nb_down = mddnode_getdown(nb);
  const MDD a_proj_down = mddnode_getdown(n_a_proj);
  const MDD b_proj_down = mddnode_getdown(n_b_proj);

  // Make copies (for cache)
  MDD _a_proj = a_proj, _b_proj = b_proj;
  if (keep_a) {
    if (keep_b) {
      val = encode_interval(intersection);
      if (a_ival.ub == b_ival.ub) {
        // The upper bounds of both intervals coincide. So when moving to
        // greater intervals (the right), there won't be any interval that can
        // have an intersection with a_ival or b_ival.
        lddmc_refs_spawn(
            SPAWN(sylvan_idd_join, na_right, nb_right, a_proj, b_proj));
      } else if (na_value < nb_value) {
        // The upper bound of a is lower than the upper bound of b.
        // Thus, we fetch the next greater interval with respect to values
        // from the variable associated to a.
        // We need to keep the interval b however, since it can happen that
        // the next greater interval from a also has an intersection with
        // the current interval from b.
        lddmc_refs_spawn(SPAWN(sylvan_idd_join, na_right, b, a_proj, b_proj));
      } else {
        // The upper bound of b is lower than the upper bound of a.
        // Thus, we fetch the next greater interval with respect to values
        // from the variable associated to b.
        // We need to keep the interval a however, since it can happen that
        // the next greater interval from b also has an intersection with
        // the current interval from a.
        lddmc_refs_spawn(SPAWN(sylvan_idd_join, a, nb_right, a_proj, b_proj));
      }
      if (a_proj_val != (uint32_t)-1) a_proj = a_proj_down;
      if (b_proj_val != (uint32_t)-1) b_proj = b_proj_down;
      down = CALL(sylvan_idd_join, na_down, nb_down, a_proj, b_proj);
    } else {
      val = na_value;
      lddmc_refs_spawn(SPAWN(sylvan_idd_join, na_right, b, a_proj, b_proj));
      if (a_proj_val != (uint32_t)-1) a_proj = a_proj_down;
      if (b_proj_val != (uint32_t)-1) b_proj = b_proj_down;
      down = CALL(sylvan_idd_join, na_down, b, a_proj, b_proj);
    }
  } else {
    val = nb_value;
    lddmc_refs_spawn(SPAWN(sylvan_idd_join, a, nb_right, a_proj, b_proj));
    if (a_proj_val != (uint32_t)-1) a_proj = a_proj_down;
    if (b_proj_val != (uint32_t)-1) b_proj = b_proj_down;
    down = CALL(sylvan_idd_join, a, nb_down, a_proj, b_proj);
  }

  lddmc_refs_push(down);
  MDD right = lddmc_refs_sync(SYNC(sylvan_idd_join));
  lddmc_refs_pop(1);
  result = lddmc_makenode(val, down, right);

  /* Write to cache */
  if (cache_put4(CACHE_MDD_JOIN, a, b, _a_proj, _b_proj, result))
    sylvan_stats_count(LDD_JOIN_CACHEDPUT);

  return result;
}

inline MDD sylvan_idd_one_level_set_add_interval(MDD mdd, interval ival) {
  if (mdd <= lddmc_true)
    return lddmc_makenode(encode_interval(ival), lddmc_true, mdd);

  mddnode_t n = LDD_GETNODE(mdd);
  uint32_t n_value = mddnode_getvalue(n);
  interval _ival = decode_interval(n_value);

  if (is_intervals_connected(_ival, ival)) {
    interval span = interval_union(_ival, ival);
    MDD right = mddnode_getright(n);
    mddnode_t n_right;
    interval right_ival;
    if (right != lddmc_false) {
      n_right = LDD_GETNODE(right);
      right_ival = decode_interval(mddnode_getvalue(n_right));
    }
    while (right != lddmc_false && is_intervals_connected(span, right_ival)) {
      span = interval_union(span, right_ival);
      right = mddnode_getright(n_right);
      if (right != lddmc_false) {
        n_right = LDD_GETNODE(right);
        right_ival = decode_interval(mddnode_getvalue(n_right));
      }
    }

    // right now points to the next greater interval that is not connected,
    // or false (no greater interval exists).
    // So we can now return a node with the created interval.
    return lddmc_makenode(encode_interval(span), lddmc_true, right);
  }

  // When reaching this point, the given interval and the interval stored
  // in the node are not connected and also do not have an intersection.

  if (ival.ub < _ival.lb) {
    // The given interval is ordered before the one stored in the node.
    return lddmc_makenode(encode_interval(ival), lddmc_true, mdd);
  } else {
    // The given interval is ordered after the one stored in the node.
    return lddmc_makenode(
        n_value, lddmc_true,
        sylvan_idd_one_level_set_add_interval(mddnode_getright(n), ival));
  }
}

inline MDD sylvan_idd_one_level_set_add_encoded_interval(MDD mdd,
                                                         uint32_t value) {
  if (mdd <= lddmc_true) return lddmc_makenode(value, lddmc_true, mdd);

  interval ival = decode_interval(value);

  return sylvan_idd_one_level_set_add_interval(mdd, ival);
}

inline MDD sylvan_idd_one_level_set_union(MDD a, MDD b) {
  if (a == b) return a;
  if (a == lddmc_false) return b;
  if (b == lddmc_false) return a;

  mddnode_t na = LDD_GETNODE(a);
  mddnode_t nb = LDD_GETNODE(b);
  const uint32_t na_value = mddnode_getvalue(na);
  const uint32_t nb_value = mddnode_getvalue(nb);
  interval a_ival = decode_interval(na_value);
  interval b_ival = decode_interval(nb_value);

  if (is_intervals_connected(a_ival, b_ival)) {
    interval span = interval_union(a_ival, b_ival);
    MDD right = sylvan_idd_one_level_set_union(mddnode_getright(na),
                                               mddnode_getright(nb));
    mddnode_t n_right;
    interval right_ival;
    if (right != lddmc_false) {
      n_right = LDD_GETNODE(right);
      right_ival = decode_interval(mddnode_getvalue(n_right));
    }
    while (right != lddmc_false && is_intervals_connected(span, right_ival)) {
      span = interval_union(span, right_ival);
      right = mddnode_getright(n_right);
      if (right != lddmc_false) {
        n_right = LDD_GETNODE(right);
        right_ival = decode_interval(mddnode_getvalue(n_right));
      }
    }

    // right now points to the next greater interval that is not connected,
    // or false (no greater interval exists).
    // So we can now return a node with the created interval.
    return lddmc_makenode(encode_interval(span), lddmc_true, right);
  }

  // When reaching this point, the intervals stored
  // in the nodes are not connected.

  if (a_ival.ub < b_ival.lb) {
    // The interval stored in node a is ordered before the one stored in node b.
    return lddmc_makenode(
        na_value, lddmc_true,
        sylvan_idd_one_level_set_union(mddnode_getright(na), b));
  } else {
    // The interval stored in node b is ordered before the one stored in node a.
    return lddmc_makenode(
        nb_value, lddmc_true,
        sylvan_idd_one_level_set_union(a, mddnode_getright(nb)));
  }
}

static const uint64_t CACHE_IDD_GET_VALID_ASSIGNS = (65LL << 40);

TASK_5(MDD, sylvan_idd_get_valid_variable_assignments_recursive, MDD, ldd,
       uint32_t, var_idx, MDD, variables_cube, MDD, values_cube, int,
       variable_index) {
  if (variables_cube == lddmc_true && variable_index < var_idx) {
    // No more valid value for the parameter on this path.
    return lddmc_true;
  }
  if (ldd == lddmc_false) {
    // No valid value for the parameter on this path.
    return lddmc_false;
  }

  assert(ldd != lddmc_true);

  mddnode_t var_node = LDD_GETNODE(variables_cube);
  uint32_t variable = mddnode_getvalue(var_node);

  if (variables_cube != lddmc_true && variable == var_idx) {
    // We reached a variable, which is part of the partial assignment.
    mddnode_t value_node = LDD_GETNODE(values_cube);
    uint32_t value = mddnode_getvalue(value_node);
    while (ldd != lddmc_false) {
      const mddnode_t nldd = LDD_GETNODE(ldd);
      const uint32_t v = mddnode_getvalue(nldd);
      const MDD nldd_down = mddnode_getdown(nldd);
      const MDD nldd_right = mddnode_getright(nldd);
      const interval ival = decode_interval(v);
      if (ival.lb <= value && value <= ival.ub) {
        // We have found the value we are looking for. So recurse from here.
        // Use cached result if possible.
        MDD collected_values;
        if (!cache_get4(CACHE_IDD_GET_VALID_ASSIGNS, ldd, variables_cube,
                        values_cube, variable_index, &collected_values)) {
          collected_values =
              CALL(sylvan_idd_get_valid_variable_assignments_recursive,
                   nldd_down, var_idx + 1, mddnode_getdown(var_node),
                   mddnode_getdown(value_node), variable_index);
          cache_put4(CACHE_IDD_GET_VALID_ASSIGNS, ldd, variables_cube,
                     values_cube, variable_index, collected_values);
        }

        return collected_values;
      }
      if (ival.lb > value) {
        // The value is greater then the one we are searching for.
        // That means that the value does not lead to a path to one.
        break;
      }

      // The value we are looking for might still be on this level, move
      // to the right (where higher values are stored).
      ldd = nldd_right;
    }

    // No valid value for the parameter on this path.
    return lddmc_false;
  } else {
    // We have to follow all paths.
    const mddnode_t nldd = LDD_GETNODE(ldd);
    const MDD nldd_right = mddnode_getright(nldd);
    const MDD nldd_down = mddnode_getdown(nldd);

    MDD collected_values;
    // Use cached result if possible.
    if (!cache_get4(CACHE_IDD_GET_VALID_ASSIGNS, ldd, variables_cube,
                    values_cube, variable_index, &collected_values)) {
      /* right = */ lddmc_refs_spawn(
          SPAWN(sylvan_idd_get_valid_variable_assignments_recursive, nldd_right,
                var_idx, variables_cube, values_cube, variable_index));
      MDD down_set =
          CALL(sylvan_idd_get_valid_variable_assignments_recursive, nldd_down,
               var_idx + 1, variables_cube, values_cube, variable_index);
      lddmc_refs_push(down_set);
      MDD right_set = lddmc_refs_sync(
          SYNC(sylvan_idd_get_valid_variable_assignments_recursive));
      lddmc_refs_pop(1);
      if (variable_index == var_idx) {
        // We have reached the variable whose valid values we want to collect.
        if (down_set == lddmc_true) {
          // The down node leads to true, which means that a path suffix exists
          // that is consistent with the partial assignment. So from this node,
          // the current value is valid, and maybe also greater values, which we
          // find by following the right node.
          const uint32_t v = mddnode_getvalue(nldd);
          collected_values =
              sylvan_idd_one_level_set_add_encoded_interval(right_set, v);
        } else {
          // The current value is not feasible on this path. So we just collect
          // values from the right node where we potentially have greater valid
          // values.
          collected_values = right_set;
        }
      } else {
        // The current variable in not the one we want to collect values from.
        // Either that variable is still to come in the variable order, or we
        // reached a depth deeper than the variable index. In both cases, we
        // just collect the union of values resulting from following down and
        // right nodes.
        //
        // If the variable is still to come, then we either get "false" or a set
        // of valid values.
        //
        // If we have already reached a depth deeper than the variable, then we
        // either get "true" or "false", which the union operation returns
        // "true" for, if at least one of the arguments is "true". This is
        // exactly what we want in this case: Returning "true" on a path suffix
        // means that it is consistent with the partial assignment. So the
        // values found for the variable further up the diagram are all valid.
        collected_values = sylvan_idd_one_level_set_union(down_set, right_set);
      }
      cache_put4(CACHE_IDD_GET_VALID_ASSIGNS, ldd, variables_cube, values_cube,
                 variable_index, collected_values);
    }

    return collected_values;
  }
}

TASK_6(MDD, sylvan_idd_get_valid_variable_assignments, MDD, ldd,
       const uint32_t*, variables, int, num_variables, const int*, values, int,
       num_values, uint32_t, variable) {

  // First we create cubes that specify which variable we have assignments
  // for, and what are the assigned values.
  MDD variables_cube = lddmc_true;
  MDD values_cube = lddmc_true;
  int variable_index = -1;
  for (int v = num_variables - 1; v >= 0; --v) {
    const uint32_t var = variables[v];
    if (var == variable) {
      variable_index = v;
    }

    const int value = values[var];
    if (value >= 0) {
      variables_cube = lddmc_makenode(v, variables_cube, lddmc_false);
      values_cube = lddmc_makenode(value, values_cube, lddmc_false);
    }
  }

  if (variable_index < 0) {
    // The variable which we want to search values for is unconstrained.
    // We report this by returning true.
    return lddmc_true;
  }

  lddmc_refs_push(variables_cube);
  lddmc_refs_push(values_cube);

  const MDD collected_values =
      CALL(sylvan_idd_get_valid_variable_assignments_recursive, ldd, 0,
           variables_cube, values_cube, variable_index);

  lddmc_refs_pop(2);

  return collected_values;
}

TASK_2(MDD, sylvan_idd_create_cube_from_assignments, const int*, assignments,
       int, num_assignments) {

  MDD cube = lddmc_true;
  for (int var = num_assignments - 1; var >= 0; --var) {
    const int value = assignments[var];
    if (value >= 0) {
      cube = lddmc_makenode(encode_interval(value, value), cube, lddmc_false);
    }
  }

  return cube;
}

static const uint64_t CACHE_IDD_FULL_SAT_ONE_PART_ASSIGN = (66LL << 40);

TASK_4(MDD, sylvan_idd_full_sat_one_under_partial_assignment_recursive, MDD,
       ldd, uint32_t, var_idx, MDD, variables_cube, MDD, values_cube) {

  if (ldd == lddmc_true) {
    return lddmc_true;
  }
  if (ldd == lddmc_false) {
    return lddmc_false;
  }

  mddnode_t var_node = LDD_GETNODE(variables_cube);

  if (variables_cube != lddmc_true && mddnode_getvalue(var_node) == var_idx) {
    // We reached a variable which we have an assignment for.
    mddnode_t value_node = LDD_GETNODE(values_cube);
    uint32_t value = mddnode_getvalue(value_node);
    // Now we scan the right nodes for a node with a corresponding value.
    // Only if we can find such, the path which we are currently following
    // can lead to a full assignment consistent with the partial assignment.
    while (ldd != lddmc_false) {
      const mddnode_t nldd = LDD_GETNODE(ldd);
      const uint32_t v = mddnode_getvalue(nldd);
      const interval ival = decode_interval(v);
      if (ival.lb <= value && value <= ival.ub) {
        // We have found the value we are looking for. So recurse from here.
        // Use cached result if possible.
        MDD full_sat_one_cube;
        if (!cache_get3(CACHE_IDD_FULL_SAT_ONE_PART_ASSIGN, ldd, variables_cube,
                        values_cube, &full_sat_one_cube)) {
          full_sat_one_cube =
              CALL(sylvan_idd_full_sat_one_under_partial_assignment_recursive,
                   mddnode_getdown(nldd), var_idx + 1,
                   mddnode_getdown(var_node), mddnode_getdown(value_node));
          // Prepend the current interval to the cube.
          if (full_sat_one_cube != lddmc_false) {
            full_sat_one_cube =
                lddmc_makenode(v, full_sat_one_cube, lddmc_false);
          }
          cache_put3(CACHE_IDD_FULL_SAT_ONE_PART_ASSIGN, ldd, variables_cube,
                     values_cube, full_sat_one_cube);
        }

        return full_sat_one_cube;
      }
      if (ival.lb > value) {
        // The value is greater then the one we are searching for.
        // That means that the current path cannot lead to a full assignment
        // consistent with the partial assignment.
        break;
      }

      // The value we are looking for might still be on this level, move
      // to the right (where higher values are stored).
      ldd = mddnode_getright(nldd);
    }

    return lddmc_false;
  } else {
    // We do not have an assignment for the current variable, hence we have
    // to follow all paths.
    const mddnode_t nldd = LDD_GETNODE(ldd);
    MDD full_sat_one_cube;
    // Use cached result if possible.
    if (!cache_get3(CACHE_IDD_FULL_SAT_ONE_PART_ASSIGN, ldd, variables_cube,
                    values_cube, &full_sat_one_cube)) {
      /* right = */ SPAWN(
          sylvan_idd_full_sat_one_under_partial_assignment_recursive,
          mddnode_getright(nldd), var_idx, variables_cube, values_cube);
      MDD down_full_sat_one_cube =
          CALL(sylvan_idd_full_sat_one_under_partial_assignment_recursive,
               mddnode_getdown(nldd), var_idx + 1, variables_cube, values_cube);
      lddmc_refs_push(down_full_sat_one_cube);
      MDD right_full_sat_one_cube =
          SYNC(sylvan_idd_full_sat_one_under_partial_assignment_recursive);
      lddmc_refs_pop(1);

      if (down_full_sat_one_cube != lddmc_false) {
        full_sat_one_cube = lddmc_makenode(mddnode_getvalue(nldd),
                                           down_full_sat_one_cube, lddmc_false);
      } else if (right_full_sat_one_cube != lddmc_false) {
        full_sat_one_cube = right_full_sat_one_cube;
      } else {
        full_sat_one_cube = lddmc_false;
      }

      cache_put3(CACHE_IDD_FULL_SAT_ONE_PART_ASSIGN, ldd, variables_cube,
                 values_cube, full_sat_one_cube);
    }

    return full_sat_one_cube;
  }
}

TASK_5(MDD, sylvan_idd_full_sat_one_under_partial_assignment, MDD, ldd,
       const uint32_t*, variables, int, num_variables, const int*, values, int,
       num_values) {

  // First we create cubes that specify which variables we have assignments
  // for, and what are the assigned values.
  MDD variables_cube = lddmc_true;
  MDD values_cube = lddmc_true;
  for (int v = num_variables - 1; v >= 0; --v) {
    const uint32_t var = variables[v];
    const int value = values[var];

    if (value >= 0) {
      variables_cube = lddmc_makenode(v, variables_cube, lddmc_false);
      values_cube = lddmc_makenode(value, values_cube, lddmc_false);
    }
  }

  lddmc_refs_push(variables_cube);
  lddmc_refs_push(values_cube);

  MDD full_sat_one_cube =
      CALL(sylvan_idd_full_sat_one_under_partial_assignment_recursive, ldd, 0,
           variables_cube, values_cube);

  lddmc_refs_pop(2);

  return full_sat_one_cube;
}

TASK_3(MDD, sylvan_idd_create_universe, const uint32_t*, variables, int, count,
       const unsigned int*, domain_sizes) {

  MDD universe = lddmc_true;
  for (int var_idx = count - 1; var_idx >= 0; --var_idx) {
    universe =
        lddmc_makenode(encode_interval(0, domain_sizes[variables[var_idx]] - 1),
                       universe, lddmc_false);
  }

  return universe;
}

static const uint64_t CACHE_IDD_INV_PROJ = (67LL << 40);

TASK_3(MDD, sylvan_idd_inv_project, MDD, a, MDD, b, MDD, proj) {
  if (a == b) return a;
  if (a == lddmc_false || b == lddmc_false) return lddmc_false;

  mddnode_t p_node = LDD_GETNODE(proj);
  uint32_t p_val = mddnode_getvalue(p_node);
  if (p_val == (uint32_t)-2) return a;

  assert(a != lddmc_true);

  if (p_val == (uint32_t)-1) return b;

  if (p_val == 1) assert(b != lddmc_true);

  /* Test gc */
  sylvan_gc_test();

  /* Get nodes */
  mddnode_t na = LDD_GETNODE(a);
  mddnode_t nb = LDD_GETNODE(b);
  uint32_t na_value = mddnode_getvalue(na);
  uint32_t nb_value = mddnode_getvalue(nb);
  interval a_ival = decode_interval(na_value);
  interval b_ival = decode_interval(nb_value);
  interval intersection = interval_intersection(a_ival, b_ival);

  if (p_val == 1) {
    while (!is_valid(intersection)) {
      // Simply comparing the encoded intervals is fine,
      // as order on the encoded intervals is equivalent to
      // order of the decoded intervals.
      if (na_value < nb_value) {
        a = mddnode_getright(na);
        if (a == lddmc_false) return lddmc_false;
        na = LDD_GETNODE(a);
        na_value = mddnode_getvalue(na);
        a_ival = decode_interval(na_value);
      } else if (nb_value < na_value) {
        b = mddnode_getright(nb);
        if (b == lddmc_false) return lddmc_false;
        nb = LDD_GETNODE(b);
        nb_value = mddnode_getvalue(nb);
        b_ival = decode_interval(nb_value);
      }
      intersection = interval_intersection(a_ival, b_ival);
    }
  }

  /* Access cache */
  MDD result;
  if (cache_get3(CACHE_IDD_INV_PROJ, a, b, proj, &result)) {
    return result;
  }

  /* Perform recursive calculation */
  uint32_t val;
  MDD down;

  const MDD na_right = mddnode_getright(na);
  const MDD nb_right = mddnode_getright(nb);
  const MDD na_down = mddnode_getdown(na);
  const MDD nb_down = mddnode_getdown(nb);
  const MDD p_down = mddnode_getdown(p_node);

  if (p_val == 1) {
    val = encode_interval(intersection);
    if (a_ival.ub == b_ival.ub) {
      // The upper bounds of both intervals coincide. So when moving to
      // greater intervals (the right), there won't be any interval that can
      // have an intersection with a_ival or b_ival.
      lddmc_refs_spawn(SPAWN(sylvan_idd_inv_project, na_right, nb_right, proj));
    } else if (na_value < nb_value) {
      // The upper bound of a is lower than the upper bound of b.
      // Thus, we fetch the next greater interval with respect to values
      // from the variable associated to a.
      // We need to keep the interval b however, since it can happen that
      // the next greater interval from a also has an intersection with
      // the current interval from b.
      lddmc_refs_spawn(SPAWN(sylvan_idd_inv_project, na_right, b, proj));
    } else {
      // The upper bound of b is lower than the upper bound of a.
      // Thus, we fetch the next greater interval with respect to values
      // from the variable associated to b.
      // We need to keep the interval a however, since it can happen that
      // the next greater interval from b also has an intersection with
      // the current interval from a.
      lddmc_refs_spawn(SPAWN(sylvan_idd_inv_project, a, nb_right, proj));
    }
    down = CALL(sylvan_idd_inv_project, na_down, nb_down, p_down);
  } else {
    val = na_value;
    /* right = */ lddmc_refs_spawn(
        SPAWN(sylvan_idd_inv_project, na_right, b, proj));
    down = CALL(sylvan_idd_inv_project, na_down, b, p_down);
  }
  lddmc_refs_push(down);
  MDD right = lddmc_refs_sync(SYNC(sylvan_idd_inv_project));
  lddmc_refs_pop(1);
  result = lddmc_makenode(val, down, right);

  /* Write to cache */
  cache_put3(CACHE_IDD_INV_PROJ, a, b, proj, result);

  return result;
}

static MDD merge_adjacent_idds(MDD idd) {
  bool merged_adjacent_nodes = true;

  while (merged_adjacent_nodes) {
    merged_adjacent_nodes = false;
    mddnode_t nidd = LDD_GETNODE(idd);
    uint32_t encoded_ival = mddnode_getvalue(nidd);
    interval ival = decode_interval(encoded_ival);
    MDD result_down = mddnode_getdown(nidd);
    MDD idd_right = mddnode_getright(nidd);

    if (idd_right != lddmc_false) {
      mddnode_t nidd_right = LDD_GETNODE(idd_right);
      uint32_t right_encoded_ival = mddnode_getvalue(nidd_right);
      interval right_ival = decode_interval(right_encoded_ival);
      MDD result_right_down = mddnode_getdown(nidd_right);
      MDD result_right_right = mddnode_getright(nidd_right);

      if (is_intervals_connected(ival, right_ival)) {
        // This nodes and the node next right to it have connected intervals.
        // Now check whether they point to the very same down node.
        if (result_down == result_right_down) {
          // Ok both down nodes are the same, so let's merge the nodes.
          interval span = interval_union(ival, right_ival);
          idd = lddmc_makenode(encode_interval(span), result_down,
                               result_right_right);
          merged_adjacent_nodes = true;
        }
      }
    }
  }

  return idd;
}

static const uint64_t CACHE_IDD_UNION = (68LL << 40);

TASK_2(MDD, sylvan_idd_union, MDD, a, MDD, b) {
  /* Terminal cases */
  if (a == b) return a;
  if (a == lddmc_false) return b;
  if (b == lddmc_false) return a;
  assert(a != lddmc_true && b != lddmc_true);  // expecting same length

  /* Test gc */
  sylvan_gc_test();

  sylvan_stats_count(LDD_UNION);

  /* Improve cache behavior */
  if (a < b) {
    MDD tmp = b;
    b = a;
    a = tmp;
  }

  /* Access cache */
  MDD result;
  if (cache_get3(CACHE_IDD_UNION, a, b, 0, &result)) {
    return result;
  }

  /* Get nodes */
  mddnode_t na = LDD_GETNODE(a);
  mddnode_t nb = LDD_GETNODE(b);
  const uint32_t na_value = mddnode_getvalue(na);
  const uint32_t nb_value = mddnode_getvalue(nb);
  const MDD na_down = mddnode_getdown(na);
  const MDD na_right = mddnode_getright(na);
  const MDD nb_down = mddnode_getdown(nb);
  const MDD nb_right = mddnode_getright(nb);

  interval a_ival = decode_interval(na_value);
  interval b_ival = decode_interval(nb_value);
  interval intersection = interval_intersection(a_ival, b_ival);

  /* Perform recursive calculation */
  if (is_valid(intersection)) {
    if (na_value == nb_value) {
      // The intervals do not just have an intersection, they are precisely
      // the same.
      lddmc_refs_spawn(SPAWN(sylvan_idd_union, na_down, nb_down));
      MDD right = CALL(sylvan_idd_union, na_right, nb_right);
      lddmc_refs_push(right);
      MDD down = lddmc_refs_sync(SYNC(sylvan_idd_union));
      lddmc_refs_pop(1);
      result = lddmc_makenode(na_value, down, right);
    } else {
      // We need to walk over the intersection, as well as the non-intersecting
      // parts.
      uint16_t before_intersection_lb = MIN(a_ival.lb, b_ival.lb);
      uint16_t before_intersection_ub = MAX(a_ival.lb, b_ival.lb);
      if (before_intersection_ub > 0) {
        --before_intersection_ub;
      } else {
        // We have to be careful regarding underflow. If the upper bound is
        // zero, then there is no valid interval before the intersection.
        before_intersection_lb = 1;
        before_intersection_ub = 0;
      }
      interval before_intersection =
          interval{before_intersection_lb, before_intersection_ub};

      lddmc_refs_spawn(SPAWN(sylvan_idd_union, na_down, nb_down));

      MDD right_right;
      if (na_value < nb_value) {
        // The upper bound of a is lower than the upper bound of b,
        // or if equal, the lower bound is less.
        if (intersection.ub < b_ival.ub) {
          const uint16_t reduced_b_lb = (uint16_t)(intersection.ub + 1);
          const uint16_t reduced_b_ub = b_ival.ub;
          interval reduced_b_ival = interval{reduced_b_lb, reduced_b_ub};
          MDD mod_b = lddmc_makenode(encode_interval(reduced_b_ival), nb_down,
                                     nb_right);
          lddmc_refs_push(mod_b);
          right_right = CALL(sylvan_idd_union, na_right, mod_b);
          lddmc_refs_pop(1);
        } else {
          right_right = CALL(sylvan_idd_union, na_right, nb_right);
        }
      } else {
        // The upper bound of b is lower than the upper bound of a,
        // or if equal, the lower bound is less.
        if (intersection.ub < a_ival.ub) {
          const uint16_t reduced_a_lb = (uint16_t)(intersection.ub + 1);
          const uint16_t reduced_a_ub = a_ival.ub;
          interval reduced_a_ival = interval{reduced_a_lb, reduced_a_ub};
          MDD mod_a = lddmc_makenode(encode_interval(reduced_a_ival), na_down,
                                     na_right);
          lddmc_refs_push(mod_a);
          right_right = CALL(sylvan_idd_union, mod_a, nb_right);
          lddmc_refs_pop(1);
        } else {
          right_right = CALL(sylvan_idd_union, na_right, nb_right);
        }
      }

      lddmc_refs_push(right_right);
      MDD down = lddmc_refs_sync(SYNC(sylvan_idd_union));
      lddmc_refs_pop(1);
      MDD right =
          lddmc_makenode(encode_interval(intersection), down, right_right);
      result = right;

      if (is_valid(before_intersection)) {
        // There is a valid interval before the intersection. This interval
        // of values either belong to a or b.
        if (a_ival.lb < b_ival.lb) {
          result = lddmc_makenode(encode_interval(before_intersection), na_down,
                                  right);
        } else {
          result = lddmc_makenode(encode_interval(before_intersection), nb_down,
                                  right);
        }
      }
    }
  } else {
    if (na_value < nb_value) {
      MDD right = CALL(sylvan_idd_union, na_right, b);
      result = lddmc_makenode(na_value, na_down, right);
    } else /* na_value > nb_value */ {
      MDD right = CALL(sylvan_idd_union, a, nb_right);
      result = lddmc_makenode(nb_value, nb_down, right);
    }
  }

  // Although the code above guarantees that intervals on the same
  // level of the created IDD never intersect, it does not guarantee
  // minimality in the sense of needed intervals and therefore nodes.
  // We now check whether the created node and the node next right to
  // it point to the same down node and both have adjacent intervals.
  // If that is the case, we can merge both nodes.
  // Due to the recursive nature of this method, this causes all
  // intervals to be merged to the greatest possible extent.
  result = merge_adjacent_idds(result);

  /* Write to cache */
  cache_put3(CACHE_IDD_UNION, a, b, 0, result);

  return result;
}

/**
 * This is just the method lddmc_satcount from the sylvan library modified
 * accordingly to work on encoded intervals.
 */
TASK_1(long double, sylvan_idd_satcount, MDD, mdd) {
  if (mdd == lddmc_false) return 0.0;
  if (mdd == lddmc_true) return 1.0;

  /* Perhaps execute garbage collection */
  sylvan_gc_test();

  sylvan_stats_count(LDD_SATCOUNTL);

  union {
      long double d;
      struct {
          uint64_t s1;
          uint64_t s2;
      } s;
  } hack;

  if (cache_get3(CACHE_MDD_SATCOUNTL1, mdd, 0, 0, &hack.s.s1) &&
      cache_get3(CACHE_MDD_SATCOUNTL2, mdd, 0, 0, &hack.s.s2)) {
    sylvan_stats_count(LDD_SATCOUNTL_CACHED);
    return hack.d;
  }

  mddnode_t n = LDD_GETNODE(mdd);
  uint32_t v = mddnode_getvalue(n);
  MDD n_down = mddnode_getdown(n);
  MDD n_right = mddnode_getright(n);

  interval ival = decode_interval(v);
  // Trivial interval has a span of 1.
  long double span = ival.ub - ival.lb + 1;

  SPAWN(sylvan_idd_satcount, n_down);
  long double right = CALL(sylvan_idd_satcount, n_right);
  // The down path is identical for all values from the interval
  // stored in the current node. Therefore we needs to multiply
  // this by the number of values in that interval in order to
  // get the decoded number of paths to true.
  long double down = SYNC(sylvan_idd_satcount) * span;
  hack.d = right + down;

  int c1 = cache_put3(CACHE_MDD_SATCOUNTL1, mdd, 0, 0, hack.s.s1);
  int c2 = cache_put3(CACHE_MDD_SATCOUNTL2, mdd, 0, 0, hack.s.s2);
  if (c1 && c2) sylvan_stats_count(LDD_SATCOUNTL_CACHEDPUT);

  return hack.d;
}

#define sylvan_idd_create_relational_proposition(op, value,            \
                                                 variable_domain_size) \
  RUN(sylvan_idd_create_relational_proposition, op, value, variable_domain_size)

#define sylvan_idd_create_projection_cube(                                   \
    target_variables, num_target_variables, local_variables,                 \
    num_local_variables)                                                     \
  RUN(sylvan_create_projection_cube, target_variables, num_target_variables, \
      local_variables, num_local_variables)

#define sylvan_idd_sat_with_partial_assignment(ldd, variables, num_variables, \
                                               values, num_values)            \
  RUN(sylvan_idd_sat_with_partial_assignment, ldd, variables, num_variables,  \
      values, num_values)

#define sylvan_idd_get_valid_variable_assignments(               \
    ldd, variables, num_variables, values, num_values, variable) \
  RUN(sylvan_idd_get_valid_variable_assignments, ldd, variables, \
      num_variables, values, num_values, variable)

#define sylvan_idd_create_cube_from_assignments(assignments, num_assignments) \
  RUN(sylvan_idd_create_cube_from_assignments, assignments, num_assignments)

#define sylvan_idd_full_sat_one_under_partial_assignment(               \
    ldd, variables, num_variables, values, num_values)                  \
  RUN(sylvan_idd_full_sat_one_under_partial_assignment, ldd, variables, \
      num_variables, values, num_values)

#define sylvan_idd_join(a, b, a_proj, b_proj) \
  RUN(sylvan_idd_join, a, b, a_proj, b_proj)

#define sylvan_idd_satcount(mdd) RUN(sylvan_idd_satcount, mdd)

#define sylvan_idd_create_universe(variables, count, domain_sizes) \
  RUN(sylvan_idd_create_universe, variables, count, domain_sizes)

#define sylvan_idd_inv_project(a, b, proj) \
  RUN(sylvan_idd_inv_project, a, b, proj)

#define sylvan_idd_union(a, b) RUN(sylvan_idd_union, a, b)

static void sylvan_idd_fprintdot_rec(std::ofstream& out, MDD idd) {
  // assert(mdd > lddmc_true);

  // check mark
  mddnode_t n = LDD_GETNODE(idd);
  if (mddnode_getmark(n)) return;
  mddnode_setmark(n, 1);

  // print the node
  uint32_t val = mddnode_getvalue(n);
  out << idd << " [shape=record, label=\"";
  if (mddnode_getcopy(n))
    out << "<c> *";
  else {
    interval ival = decode_interval(val);
    out << "<" << val << "> [" << ival.lb << "," << ival.ub << "]";
  }
  MDD right = mddnode_getright(n);
  while (right != lddmc_false) {
    mddnode_t n2 = LDD_GETNODE(right);
    uint32_t val2 = mddnode_getvalue(n2);
    interval ival = decode_interval(val2);
    out << "|<" << val2 << "> [" << ival.lb << "," << ival.ub << "]";
    right = mddnode_getright(n2);
    // assert(right != lddmc_true);
  }
  out << "\"];\n";

  // recurse and print the edges
  for (;;) {
    MDD down = mddnode_getdown(n);
    // assert(down != lddmc_false);
    if (down > lddmc_true) {
      sylvan_idd_fprintdot_rec(out, down);
      if (mddnode_getcopy(n)) {
        out << idd << ":c -> ";
      } else {
        out << idd << ":" << mddnode_getvalue(n) << " -> ";
      }
      if (mddnode_getcopy(LDD_GETNODE(down))) {
        out << down << ":c [style=solid];\n";
      } else {
        out << down << ":" << mddnode_getvalue(LDD_GETNODE(down))
            << " [style=solid];\n";
      }
    }
    MDD right = mddnode_getright(n);
    if (right == lddmc_false) break;
    n = LDD_GETNODE(right);
  }
}

static void sylvan_idd_fprintdot_unmark(MDD idd) {
  if (idd <= lddmc_true) return;
  mddnode_t n = LDD_GETNODE(idd);
  if (mddnode_getmark(n)) {
    mddnode_setmark(n, 0);
    for (;;) {
      sylvan_idd_fprintdot_unmark(mddnode_getdown(n));
      idd = mddnode_getright(n);
      if (idd == lddmc_false) return;
      n = LDD_GETNODE(idd);
    }
  }
}

static void sylvan_idd_fprintdot(std::ofstream& out, MDD idd) {
  out << "digraph \"DD\" {\n";
  out << "graph [dpi = 300];\n";
  out << "center = true;\n";
  out << "edge [dir = forward];\n";

  // Special case: false
  if (idd == lddmc_false) {
    out << "0 [shape=record, label=\"False\"];\n";
    out << "}\n";
    return;
  }

  // Special case: true
  if (idd == lddmc_true) {
    out << "1 [shape=record, label=\"True\"];\n";
    out << "}\n";
    return;
  }

  sylvan_idd_fprintdot_rec(out, idd);
  sylvan_idd_fprintdot_unmark(idd);

  out << "}\n";
}

}  // namespace

namespace citcpp {
namespace detail {

sylvan_ldd::sylvan_ldd() : ldd_(lddmc_false), variables_() {
  lddmc_protect(&ldd_);
}

sylvan_ldd::sylvan_ldd(uint32_t variable, uint32_t value)
    : ldd_(sylvan_make_node(value, lddmc_true, lddmc_false)),
      variables_(1, variable) {

  lddmc_protect(&ldd_);
}

sylvan_ldd::sylvan_ldd(const std::vector<int>& assignments)
    : ldd_(sylvan_create_cube_from_assignments(assignments.data(),
                                               assignments.size())),
      variables_() {

  for (int var = 0; var < assignments.size(); ++var) {
    const int value = assignments[var];
    if (value >= 0) {
      variables_.push_back(var);
    }
  }

  lddmc_protect(&ldd_);
}

sylvan_ldd::sylvan_ldd(uint32_t variable, relational_operator op,
                       uint32_t value, uint32_t variable_domain_size)
    : ldd_(sylvan_create_relational_proposition(op, value,
                                                variable_domain_size)),
      variables_(1, variable) {

  lddmc_protect(&ldd_);
}

sylvan_ldd::sylvan_ldd(const sylvan_ldd& other)
    : ldd_(other.ldd_), variables_(other.variables_) {

  lddmc_protect(&ldd_);
}

sylvan_ldd::sylvan_ldd(sylvan_ldd&& other)
    : ldd_(other.ldd_), variables_(std::move(other.variables_)) {

  lddmc_protect(&ldd_);
}

sylvan_ldd::sylvan_ldd(uint64_t ldd, const std::vector<uint32_t>& variables)
    : ldd_(ldd), variables_(variables) {

  lddmc_protect(&ldd_);
}

sylvan_ldd::sylvan_ldd(uint64_t ldd, std::vector<uint32_t>&& variables)
    : ldd_(ldd), variables_(std::move(variables)) {

  lddmc_protect(&ldd_);
}

sylvan_ldd::~sylvan_ldd() { lddmc_unprotect(&ldd_); }

sylvan_ldd& sylvan_ldd::operator=(const sylvan_ldd& other) {
  if (this != &other) {
    ldd_ = other.ldd_;
    variables_ = other.variables_;
  }
  return *this;
}

sylvan_ldd& sylvan_ldd::operator=(sylvan_ldd&& other) {
  if (this != &other) {
    ldd_ = other.ldd_;
    variables_ = std::move(other.variables_);
  }
  return *this;
}

sylvan_ldd sylvan_ldd::lddTrue() {
  return sylvan_ldd(lddmc_true, std::vector<uint32_t>());
}

sylvan_ldd sylvan_ldd::lddFalse() { return sylvan_ldd(); }

bool sylvan_ldd::operator==(const sylvan_ldd& other) const {
  return ldd_ == other.ldd_ && variables_ == other.variables_;
}

bool sylvan_ldd::operator!=(const sylvan_ldd& other) const {
  return ldd_ != other.ldd_ || variables_ != other.variables_;
}

sylvan_ldd sylvan_ldd::get_down_node() const {
  const mddnode_t node = LDD_GETNODE(ldd_);
  MDD down = mddnode_getdown(node);
  if (down == lddmc_true) {
    return lddTrue();
  } else if (down == lddmc_false) {
    return lddFalse();
  } else {
    std::vector<uint32_t> down_vars(variables_);
    down_vars.erase(down_vars.begin());
    return sylvan_ldd(down, std::move(down_vars));
  }
}

sylvan_ldd sylvan_ldd::get_right_node() const {
  const mddnode_t node = LDD_GETNODE(ldd_);
  MDD right = mddnode_getright(node);
  if (right == lddmc_true) {
    return lddTrue();
  } else if (right == lddmc_false) {
    return lddFalse();
  } else {
    return sylvan_ldd(right, std::vector<uint32_t>(variables_));
  }
}

uint32_t sylvan_ldd::get_value() const {
  const mddnode_t node = LDD_GETNODE(ldd_);
  const uint32_t value = mddnode_getvalue(node);

  return value;
}

const std::vector<uint32_t>& sylvan_ldd::get_variables() const {
  return variables_;
}

sylvan_ldd sylvan_ldd::project_intersect(const sylvan_ldd& lhs,
                                         const sylvan_ldd& rhs) {

  if (lhs.ldd_ == lddmc_false || rhs.ldd_ == lddmc_false)
    return sylvan_ldd::lddFalse();
  if (lhs.ldd_ == lddmc_true) return rhs;
  if (rhs.ldd_ == lddmc_true) return lhs;

  const std::vector<uint32_t>& lhs_vars = lhs.variables_;
  const std::vector<uint32_t>& rhs_vars = rhs.variables_;
  std::vector<uint32_t> common_variables(
      get_common_variables(lhs_vars, rhs_vars));

  MDD lhs_cube = sylvan_create_projection_cube(
      common_variables.data(), common_variables.size(), lhs_vars.data(),
      lhs_vars.size());
  lddmc_protect(&lhs_cube);
  MDD rhs_cube = sylvan_create_projection_cube(
      common_variables.data(), common_variables.size(), rhs_vars.data(),
      rhs_vars.size());
  lddmc_protect(&rhs_cube);

  MDD and_ldd = lddmc_join(lhs.ldd_, rhs.ldd_, lhs_cube, rhs_cube);

  lddmc_unprotect(&lhs_cube);
  lddmc_unprotect(&rhs_cube);

  return sylvan_ldd(and_ldd, std::move(common_variables));
}

sylvan_ldd& sylvan_ldd::project_intersect(const sylvan_ldd& other) {
  if (ldd_ == lddmc_false || other.ldd_ == lddmc_false) {
    ldd_ = lddmc_false;
    variables_.clear();
    return *this;
  }
  if (other.ldd_ == lddmc_true) return *this;
  if (ldd_ == lddmc_true) {
    *this = other;
    return *this;
  }

  const std::vector<uint32_t>& this_vars = variables_;
  const std::vector<uint32_t>& other_vars = other.variables_;
  std::vector<uint32_t> common_variables(
      get_common_variables(this_vars, other_vars));

  MDD this_cube = sylvan_create_projection_cube(
      common_variables.data(), common_variables.size(), this_vars.data(),
      this_vars.size());
  lddmc_protect(&this_cube);
  MDD other_cube = sylvan_create_projection_cube(
      common_variables.data(), common_variables.size(), other_vars.data(),
      other_vars.size());
  lddmc_protect(&other_cube);

  ldd_ = lddmc_join(ldd_, other.ldd_, this_cube, other_cube);

  lddmc_unprotect(&this_cube);
  lddmc_unprotect(&other_cube);

  variables_ = std::move(common_variables);

  return *this;
}

sylvan_ldd sylvan_ldd::project_union(
    const sylvan_ldd& lhs, const sylvan_ldd& rhs,
    const std::vector<unsigned int>& domain_sizes) {

  if (lhs.ldd_ == lddmc_false) return rhs;
  if (rhs.ldd_ == lddmc_false) return lhs;

  const std::vector<uint32_t>& lhs_vars = lhs.variables_;
  const std::vector<uint32_t>& rhs_vars = rhs.variables_;
  std::vector<uint32_t> common_variables(
      get_common_variables(lhs_vars, rhs_vars));

  MDD lhs_cube = sylvan_create_projection_cube(
      common_variables.data(), common_variables.size(), lhs_vars.data(),
      lhs_vars.size());
  lddmc_protect(&lhs_cube);
  MDD rhs_cube = sylvan_create_projection_cube(
      common_variables.data(), common_variables.size(), rhs_vars.data(),
      rhs_vars.size());
  lddmc_protect(&rhs_cube);
  MDD universe = sylvan_create_universe(
      common_variables.data(), common_variables.size(), domain_sizes.data());
  lddmc_protect(&universe);

  MDD lhs_projected = sylvan_inv_project(universe, lhs.ldd_, lhs_cube);
  lddmc_protect(&lhs_projected);
  lddmc_unprotect(&lhs_cube);
  MDD rhs_projected = sylvan_inv_project(universe, rhs.ldd_, rhs_cube);
  lddmc_protect(&rhs_projected);
  lddmc_unprotect(&rhs_cube);
  lddmc_unprotect(&universe);

  MDD or_ldd = lddmc_union(lhs_projected, rhs_projected);

  lddmc_unprotect(&lhs_projected);
  lddmc_unprotect(&rhs_projected);

  return sylvan_ldd(or_ldd, std::move(common_variables));
}

sylvan_ldd& sylvan_ldd::project_union(
    const sylvan_ldd& other, const std::vector<unsigned int>& domain_sizes) {

  if (other.ldd_ == lddmc_false) return *this;
  if (ldd_ == lddmc_false) {
    *this = other;
    return *this;
  }

  const std::vector<uint32_t>& this_vars = variables_;
  const std::vector<uint32_t>& other_vars = other.variables_;
  std::vector<uint32_t> common_variables(
      get_common_variables(this_vars, other_vars));

  MDD this_cube = sylvan_create_projection_cube(
      common_variables.data(), common_variables.size(), this_vars.data(),
      this_vars.size());
  lddmc_protect(&this_cube);
  MDD other_cube = sylvan_create_projection_cube(
      common_variables.data(), common_variables.size(), other_vars.data(),
      other_vars.size());
  lddmc_protect(&other_cube);
  MDD universe = sylvan_create_universe(
      common_variables.data(), common_variables.size(), domain_sizes.data());
  lddmc_protect(&universe);

  MDD this_projected = sylvan_inv_project(universe, ldd_, this_cube);
  lddmc_protect(&this_projected);
  lddmc_unprotect(&this_cube);
  MDD other_projected = sylvan_inv_project(universe, other.ldd_, other_cube);
  lddmc_protect(&other_projected);
  lddmc_unprotect(&other_cube);
  lddmc_unprotect(&universe);

  ldd_ = lddmc_union(this_projected, other_projected);

  lddmc_unprotect(&this_projected);
  lddmc_unprotect(&other_projected);

  variables_ = std::move(common_variables);

  return *this;
}

size_t sylvan_ldd::node_count() const { return lddmc_nodecount(ldd_); }

long double sylvan_ldd::sat_count() const { return lddmc_satcount(ldd_); }

void sylvan_ldd::get_sat_one(std::vector<int>& assignment) const {
  MDD cube = ldd_;

  int var_idx = 0;
  while (cube != lddmc_true && cube != lddmc_false) {
    mddnode_t node = LDD_GETNODE(cube);
    uint32_t value = mddnode_getvalue(node);
    assignment[variables_[var_idx]] = value;
    cube = mddnode_getdown(node);
    ++var_idx;
  }
}

void sylvan_ldd::print_dot(const std::string& file_path) const {
  FILE* f = fopen(file_path.data(), "w");
  lddmc_fprintdot(f, ldd_);
  fclose(f);
}

bool sylvan_ldd::is_sat_with_partial_assignment(
    const std::vector<int>& partial_assignment) const {

  const int res = sylvan_sat_with_partial_assignment(
      ldd_, variables_.data(), variables_.size(), partial_assignment.data(),
      partial_assignment.size());

  return res != 0;
}

bitset_uint64 sylvan_ldd::get_valid_variable_assignments(
    uint32_t variable, uint32_t domain_size,
    const std::vector<int>& partial_assignment) const {

  bitset_uint64 valid_values_as_bitset(domain_size);

  MDD valid_values_as_ldd = sylvan_get_valid_variable_assignments(
      ldd_, variables_.data(), variables_.size(), partial_assignment.data(),
      partial_assignment.size(), variable);

  if (valid_values_as_ldd == lddmc_true) {
    for (uint32_t v = 0; v < domain_size; ++v) {
      valid_values_as_bitset.set(v);
    }
  } else {
    while (valid_values_as_ldd != lddmc_false) {
      mddnode_t node = LDD_GETNODE(valid_values_as_ldd);
      uint32_t value = mddnode_getvalue(node);
      valid_values_as_bitset.set(value);
      valid_values_as_ldd = mddnode_getright(node);
    }
  }

  return valid_values_as_bitset;
}

void sylvan_ldd::get_sat_one_under_partial_assignment(
    std::vector<int>& assignment) const {

  MDD cube = sylvan_full_sat_one_under_partial_assignment(
      ldd_, variables_.data(), variables_.size(), assignment.data(),
      assignment.size());

  int var_idx = 0;
  while (cube != lddmc_true && cube != lddmc_false) {
    mddnode_t node = LDD_GETNODE(cube);
    if (assignment[variables_[var_idx]] < 0) {
      uint32_t value = mddnode_getvalue(node);
      assignment[variables_[var_idx]] = value;
    }
    cube = mddnode_getdown(node);
    ++var_idx;
  }
}

sylvan_idd::sylvan_idd() : idd_(lddmc_false), variables_() {
  lddmc_protect(&idd_);
}

sylvan_idd::sylvan_idd(uint32_t variable, uint32_t value)
    : idd_(sylvan_make_node(encode_interval(value, value), lddmc_true,
                            lddmc_false)),
      variables_(1, variable) {

  lddmc_protect(&idd_);
}

sylvan_idd::sylvan_idd(const std::vector<int>& assignments)
    : idd_(sylvan_idd_create_cube_from_assignments(assignments.data(),
                                                   assignments.size())),
      variables_() {

  for (int var = 0; var < assignments.size(); ++var) {
    const int value = assignments[var];
    if (value >= 0) {
      variables_.push_back(var);
    }
  }

  lddmc_protect(&idd_);
}

sylvan_idd::sylvan_idd(uint32_t variable, relational_operator op,
                       uint32_t value, uint32_t variable_domain_size)
    : idd_(sylvan_idd_create_relational_proposition(op, value,
                                                    variable_domain_size)),
      variables_(1, variable) {

  lddmc_protect(&idd_);
}

sylvan_idd::sylvan_idd(const sylvan_idd& other)
    : idd_(other.idd_), variables_(other.variables_) {

  lddmc_protect(&idd_);
}

sylvan_idd::sylvan_idd(sylvan_idd&& other)
    : idd_(other.idd_), variables_(std::move(other.variables_)) {

  lddmc_protect(&idd_);
}

sylvan_idd::sylvan_idd(uint64_t ldd, const std::vector<uint32_t>& variables)
    : idd_(ldd), variables_(variables) {

  lddmc_protect(&idd_);
}

sylvan_idd::sylvan_idd(uint64_t ldd, std::vector<uint32_t>&& variables)
    : idd_(ldd), variables_(std::move(variables)) {

  lddmc_protect(&idd_);
}

sylvan_idd::~sylvan_idd() { lddmc_unprotect(&idd_); }

sylvan_idd& sylvan_idd::operator=(const sylvan_idd& other) {
  if (this != &other) {
    idd_ = other.idd_;
    variables_ = other.variables_;
  }
  return *this;
}

sylvan_idd& sylvan_idd::operator=(sylvan_idd&& other) {
  if (this != &other) {
    idd_ = other.idd_;
    variables_ = std::move(other.variables_);
  }
  return *this;
}

sylvan_idd sylvan_idd::iddTrue() {
  return sylvan_idd(lddmc_true, std::vector<uint32_t>());
}

sylvan_idd sylvan_idd::iddFalse() { return sylvan_idd(); }

bool sylvan_idd::operator==(const sylvan_idd& other) const {
  return idd_ == other.idd_ && variables_ == other.variables_;
}

bool sylvan_idd::operator!=(const sylvan_idd& other) const {
  return idd_ != other.idd_ || variables_ != other.variables_;
}

sylvan_idd sylvan_idd::get_down_node() const {
  const mddnode_t node = LDD_GETNODE(idd_);
  MDD down = mddnode_getdown(node);
  if (down == lddmc_true) {
    return iddTrue();
  } else if (down == lddmc_false) {
    return iddFalse();
  } else {
    std::vector<uint32_t> down_vars(variables_);
    down_vars.erase(down_vars.begin());
    return sylvan_idd(down, std::move(down_vars));
  }
}

sylvan_idd sylvan_idd::get_right_node() const {
  const mddnode_t node = LDD_GETNODE(idd_);
  MDD right = mddnode_getright(node);
  if (right == lddmc_true) {
    return iddTrue();
  } else if (right == lddmc_false) {
    return iddFalse();
  } else {
    return sylvan_idd(right, std::vector<uint32_t>(variables_));
  }
}

uint32_t sylvan_idd::get_encoded_interval() const {
  const mddnode_t node = LDD_GETNODE(idd_);
  const uint32_t value = mddnode_getvalue(node);

  return value;
}

void sylvan_idd::get_interval(uint32_t& lb, uint32_t& ub) const {
  const mddnode_t node = LDD_GETNODE(idd_);
  const uint32_t value = mddnode_getvalue(node);

  interval ival(decode_interval(value));
  lb = ival.lb;
  ub = ival.ub;
}

const std::vector<uint32_t>& sylvan_idd::get_variables() const {
  return variables_;
}

sylvan_idd sylvan_idd::project_intersect(const sylvan_idd& lhs,
                                         const sylvan_idd& rhs) {

  if (lhs.idd_ == lddmc_false || rhs.idd_ == lddmc_false)
    return sylvan_idd::iddFalse();
  if (lhs.idd_ == lddmc_true) return rhs;
  if (rhs.idd_ == lddmc_true) return lhs;

  const std::vector<uint32_t>& lhs_vars = lhs.variables_;
  const std::vector<uint32_t>& rhs_vars = rhs.variables_;
  std::vector<uint32_t> common_variables(
      get_common_variables(lhs_vars, rhs_vars));

  MDD lhs_cube = sylvan_idd_create_projection_cube(
      common_variables.data(), common_variables.size(), lhs_vars.data(),
      lhs_vars.size());
  lddmc_protect(&lhs_cube);
  MDD rhs_cube = sylvan_idd_create_projection_cube(
      common_variables.data(), common_variables.size(), rhs_vars.data(),
      rhs_vars.size());
  lddmc_protect(&rhs_cube);

  MDD and_ldd = sylvan_idd_join(lhs.idd_, rhs.idd_, lhs_cube, rhs_cube);

  lddmc_unprotect(&lhs_cube);
  lddmc_unprotect(&rhs_cube);

  return sylvan_idd(and_ldd, std::move(common_variables));
}

sylvan_idd& sylvan_idd::project_intersect(const sylvan_idd& other) {
  if (idd_ == lddmc_false || other.idd_ == lddmc_false) {
    idd_ = lddmc_false;
    variables_.clear();
    return *this;
  }
  if (other.idd_ == lddmc_true) return *this;
  if (idd_ == lddmc_true) {
    *this = other;
    return *this;
  }

  const std::vector<uint32_t>& this_vars = variables_;
  const std::vector<uint32_t>& other_vars = other.variables_;
  std::vector<uint32_t> common_variables(
      get_common_variables(this_vars, other_vars));

  MDD this_cube = sylvan_idd_create_projection_cube(
      common_variables.data(), common_variables.size(), this_vars.data(),
      this_vars.size());
  lddmc_protect(&this_cube);
  MDD other_cube = sylvan_idd_create_projection_cube(
      common_variables.data(), common_variables.size(), other_vars.data(),
      other_vars.size());
  lddmc_protect(&other_cube);

  idd_ = sylvan_idd_join(idd_, other.idd_, this_cube, other_cube);

  lddmc_unprotect(&this_cube);
  lddmc_unprotect(&other_cube);

  variables_ = std::move(common_variables);

  return *this;
}

sylvan_idd sylvan_idd::project_union(
    const sylvan_idd& lhs, const sylvan_idd& rhs,
    const std::vector<unsigned int>& domain_sizes) {

  if (lhs.idd_ == lddmc_false) return rhs;
  if (rhs.idd_ == lddmc_false) return lhs;

  const std::vector<uint32_t>& lhs_vars = lhs.variables_;
  const std::vector<uint32_t>& rhs_vars = rhs.variables_;
  std::vector<uint32_t> common_variables(
      get_common_variables(lhs_vars, rhs_vars));

  MDD lhs_cube = sylvan_idd_create_projection_cube(
      common_variables.data(), common_variables.size(), lhs_vars.data(),
      lhs_vars.size());
  lddmc_protect(&lhs_cube);
  MDD rhs_cube = sylvan_idd_create_projection_cube(
      common_variables.data(), common_variables.size(), rhs_vars.data(),
      rhs_vars.size());
  lddmc_protect(&rhs_cube);
  MDD universe = sylvan_idd_create_universe(
      common_variables.data(), common_variables.size(), domain_sizes.data());
  lddmc_protect(&universe);

  MDD lhs_projected = sylvan_idd_inv_project(universe, lhs.idd_, lhs_cube);
  lddmc_protect(&lhs_projected);
  lddmc_unprotect(&lhs_cube);
  MDD rhs_projected = sylvan_idd_inv_project(universe, rhs.idd_, rhs_cube);
  lddmc_protect(&rhs_projected);
  lddmc_unprotect(&rhs_cube);
  lddmc_unprotect(&universe);

  MDD or_ldd = sylvan_idd_union(lhs_projected, rhs_projected);

  lddmc_unprotect(&lhs_projected);
  lddmc_unprotect(&rhs_projected);

  return sylvan_idd(or_ldd, std::move(common_variables));
}

sylvan_idd& sylvan_idd::project_union(
    const sylvan_idd& other, const std::vector<unsigned int>& domain_sizes) {

  if (other.idd_ == lddmc_false) return *this;
  if (idd_ == lddmc_false) {
    *this = other;
    return *this;
  }

  const std::vector<uint32_t>& this_vars = variables_;
  const std::vector<uint32_t>& other_vars = other.variables_;
  std::vector<uint32_t> common_variables(
      get_common_variables(this_vars, other_vars));

  MDD this_cube = sylvan_idd_create_projection_cube(
      common_variables.data(), common_variables.size(), this_vars.data(),
      this_vars.size());
  lddmc_protect(&this_cube);
  MDD other_cube = sylvan_idd_create_projection_cube(
      common_variables.data(), common_variables.size(), other_vars.data(),
      other_vars.size());
  lddmc_protect(&other_cube);
  MDD universe = sylvan_idd_create_universe(
      common_variables.data(), common_variables.size(), domain_sizes.data());
  lddmc_protect(&universe);

  MDD this_projected = sylvan_idd_inv_project(universe, idd_, this_cube);
  lddmc_protect(&this_projected);
  lddmc_unprotect(&this_cube);
  MDD other_projected =
      sylvan_idd_inv_project(universe, other.idd_, other_cube);
  lddmc_protect(&other_projected);
  lddmc_unprotect(&other_cube);
  lddmc_unprotect(&universe);

  idd_ = sylvan_idd_union(this_projected, other_projected);

  lddmc_unprotect(&this_projected);
  lddmc_unprotect(&other_projected);

  variables_ = std::move(common_variables);

  return *this;
}

size_t sylvan_idd::node_count() const { return lddmc_nodecount(idd_); }

long double sylvan_idd::sat_count() const { return sylvan_idd_satcount(idd_); }

void sylvan_idd::get_sat_one(std::vector<int>& assignment) const {
  MDD cube = idd_;

  int var_idx = 0;
  while (cube != lddmc_true && cube != lddmc_false) {
    mddnode_t node = LDD_GETNODE(cube);
    uint32_t n_value = mddnode_getvalue(node);
    interval ival = decode_interval(n_value);
    assignment[variables_[var_idx]] = ival.lb;
    cube = mddnode_getdown(node);
    ++var_idx;
  }
}

void sylvan_idd::print_dot(const std::string& file_path) const {
  std::ofstream f{file_path};
  sylvan_idd_fprintdot(f, idd_);
}

bool sylvan_idd::is_sat_with_partial_assignment(
    const std::vector<int>& partial_assignment) const {

  const int res = sylvan_idd_sat_with_partial_assignment(
      idd_, variables_.data(), variables_.size(), partial_assignment.data(),
      partial_assignment.size());

  return res != 0;
}

bitset_uint64 sylvan_idd::get_valid_variable_assignments(
    uint32_t variable, uint32_t domain_size,
    const std::vector<int>& partial_assignment) const {

  bitset_uint64 valid_values_as_bitset(domain_size);

  MDD valid_values_as_ldd = sylvan_idd_get_valid_variable_assignments(
      idd_, variables_.data(), variables_.size(), partial_assignment.data(),
      partial_assignment.size(), variable);

  if (valid_values_as_ldd == lddmc_true) {
    for (uint32_t v = 0; v < domain_size; ++v) {
      valid_values_as_bitset.set(v);
    }
  } else {
    while (valid_values_as_ldd != lddmc_false) {
      mddnode_t node = LDD_GETNODE(valid_values_as_ldd);
      uint32_t n_value = mddnode_getvalue(node);
      interval ival = decode_interval(n_value);
      for (uint16_t value = ival.lb; value <= ival.ub; ++value) {
        valid_values_as_bitset.set(value);
      }
      valid_values_as_ldd = mddnode_getright(node);
    }
  }

  return valid_values_as_bitset;
}

void sylvan_idd::get_sat_one_under_partial_assignment(
    std::vector<int>& assignment) const {

  MDD cube = sylvan_idd_full_sat_one_under_partial_assignment(
      idd_, variables_.data(), variables_.size(), assignment.data(),
      assignment.size());

  int var_idx = 0;
  while (cube != lddmc_true && cube != lddmc_false) {
    mddnode_t node = LDD_GETNODE(cube);
    if (assignment[variables_[var_idx]] < 0) {
      uint32_t n_value = mddnode_getvalue(node);
      interval ival = decode_interval(n_value);
      assignment[variables_[var_idx]] = ival.lb;
    }
    cube = mddnode_getdown(node);
    ++var_idx;
  }
}

void sylvan::init_lace(unsigned int n_workers, size_t dqsize) {
  citcpp::detail::lace_init(n_workers, dqsize);
}

void sylvan::init_package(size_t initialTableSize, size_t maxTableSize,
                          size_t initialCacheSize, size_t maxCacheSize) {

  sylvan_set_sizes(initialTableSize, maxTableSize, initialCacheSize,
                   maxCacheSize);
  sylvan_init_package();
}

void sylvan::init_package(size_t memory_cap, int table_ratio,
                          int initial_ratio) {

  sylvan_set_limits(memory_cap, table_ratio, initial_ratio);
  sylvan_init_package();
}

void sylvan::init_mtbdd() { sylvan_init_mtbdd(); }

void sylvan::init_ldd() { sylvan_init_ldd(); }

void sylvan::quit_lace() { citcpp::detail::lace_quit(); }

void sylvan::quit_package() { sylvan_quit(); }

}  // namespace detail
}  // namespace citcpp
