#include "citcpp_sylvan_ldd.hpp"

#include <sylvan.h>
#include <sylvan_int.h>

#include <algorithm>
#include <utility>

using namespace ::sylvan;

namespace {

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
  for (int i = num_target_variables - 1; i >= 0; --i) {
    uint32_t var = target_variables[i];

    if (local_var_idx >= 0 && var == local_variables[local_var_idx]) {
      cube = lddmc_makenode(1, cube, lddmc_false);
      --local_var_idx;
      found_last_local_var = true;
    } else {
      // var is not part of local variables.
      if (found_last_local_var) {
        cube = lddmc_makenode(0, cube, lddmc_false);
      } else {
        cube = lddmc_makenode(-2, cube, lddmc_false);
      }
    }
  }

  return cube;
}

static const uint64_t CACHE_MDD_UNION_JOIN = (35LL << 40);

TASK_4(MDD, sylvan_join_union, MDD, a, MDD, b, MDD, a_proj, MDD, b_proj) {
  if (a == lddmc_false) return b;
  if (b == lddmc_false) return a;

  /* Test gc */
  sylvan_gc_test();

  /* Improve cache behavior */
  if (a < b) {
    MDD tmp = b;
    b = a;
    a = tmp;

    tmp = b_proj;
    b_proj = a_proj;
    a_proj = tmp;
  }

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
    return CALL(lddmc_union, a, b);

  /* Access cache */
  MDD result;
  if (cache_get4(CACHE_MDD_UNION_JOIN, a, b, a_proj, b_proj, &result)) {
    return result;
  }

  // At this point, only proj_val {-1, 0, 1}; max one with -1; max one with 0.
  const int keep_a = a_proj_val != 0;
  const int keep_b = b_proj_val != 0;

  /* Perform recursive calculation */
  const mddnode_t na = LDD_GETNODE(a);
  const mddnode_t nb = LDD_GETNODE(b);

  // Make copies (for cache)
  MDD _a_proj = a_proj, _b_proj = b_proj;
  if (keep_a && keep_b) {
    const uint32_t na_value = mddnode_getvalue(na);
    const uint32_t nb_value = mddnode_getvalue(nb);

    if (na_value < nb_value) {
      MDD right =
          CALL(sylvan_join_union, mddnode_getright(na), b, a_proj, b_proj);
      result = lddmc_makenode(na_value, mddnode_getdown(na), right);
    } else if (na_value > nb_value) {
      MDD right =
          CALL(sylvan_join_union, a, mddnode_getright(nb), a_proj, b_proj);
      result = lddmc_makenode(nb_value, mddnode_getdown(nb), right);
    } else /* na_value == nb_value */ {
      lddmc_refs_spawn(SPAWN(sylvan_join_union, mddnode_getright(na),
                             mddnode_getright(nb), a_proj, b_proj));
      if (a_proj_val != (uint32_t)-1) a_proj = mddnode_getdown(n_a_proj);
      if (b_proj_val != (uint32_t)-1) b_proj = mddnode_getdown(n_b_proj);
      MDD down = CALL(sylvan_join_union, mddnode_getdown(na),
                      mddnode_getdown(nb), a_proj, b_proj);

      lddmc_refs_push(down);
      MDD right = lddmc_refs_sync(SYNC(sylvan_join_union));
      lddmc_refs_pop(1);
      result = lddmc_makenode(na_value, down, right);
    }
  } else {
    uint32_t val;
    MDD down;

    if (keep_a) {
      // project b
      val = mddnode_getvalue(na);
      lddmc_refs_spawn(
          SPAWN(sylvan_join_union, mddnode_getright(na), b, a_proj, b_proj));
      if (a_proj_val != (uint32_t)-1) a_proj = mddnode_getdown(n_a_proj);
      if (b_proj_val != (uint32_t)-1) b_proj = mddnode_getdown(n_b_proj);
      down = CALL(sylvan_join_union, mddnode_getdown(na), b, a_proj, b_proj);
    } else {
      // project a
      val = mddnode_getvalue(nb);
      lddmc_refs_spawn(
          SPAWN(sylvan_join_union, a, mddnode_getright(nb), a_proj, b_proj));
      if (a_proj_val != (uint32_t)-1) a_proj = mddnode_getdown(n_a_proj);
      if (b_proj_val != (uint32_t)-1) b_proj = mddnode_getdown(n_b_proj);
      down = CALL(sylvan_join_union, a, mddnode_getdown(nb), a_proj, b_proj);
    }

    lddmc_refs_push(down);
    MDD right = lddmc_refs_sync(SYNC(sylvan_join_union));
    lddmc_refs_pop(1);
    result = lddmc_makenode(val, down, right);
  }

  /* Write to cache */
  cache_put4(CACHE_MDD_UNION_JOIN, a, b, _a_proj, _b_proj, result);

  return result;
}

static const uint64_t CACHE_MDD_CONTAINS_PART_ASSIGN = (36LL << 40);

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

static const uint64_t CACHE_MDD_GET_VALID_ASSIGNS = (37LL << 40);

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
          lddmc_refs_push(right_set);
          MDD valid_value = lddmc_makenode(v, lddmc_true, lddmc_false);
          lddmc_refs_push(valid_value);
          collected_values = lddmc_union(right_set, valid_value);
          lddmc_refs_pop(2);
        } else {
          // The current value is not feasible on this path. So we just collect
          // values from the right node where we potentially have greater valid
          // values.
          collected_values = right_set;
        }
      } else {
        // The current variable in not the one we want to collect values from.
        // Either that variable is still to come in the variable order, or we
        // reached a depth deeper than the varialbe index. In both cases, we
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
        lddmc_refs_push(down_set);
        lddmc_refs_push(right_set);
        collected_values = lddmc_union(down_set, right_set);
        lddmc_refs_pop(2);
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

static const uint64_t CACHE_MDD_FULL_SAT_ONE_PART_ASSIGN = (38LL << 40);

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
      MDD right_full_sat_one_cube =
          SYNC(sylvan_full_sat_one_under_partial_assignment_recursive);

      if (right_full_sat_one_cube != lddmc_false) {
        full_sat_one_cube = right_full_sat_one_cube;
      } else if (down_full_sat_one_cube != lddmc_false) {
        full_sat_one_cube = lddmc_makenode(mddnode_getvalue(nldd),
                                           down_full_sat_one_cube, lddmc_false);
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

  MDD full_sat_one_cube =
      CALL(sylvan_full_sat_one_under_partial_assignment_recursive, ldd, 0,
           variables_cube, values_cube);

  lddmc_refs_pop(2);

  return full_sat_one_cube;
}

#define sylvan_make_node(value, ifeq, ifneq) \
  RUN(sylvan_make_node, value, ifeq, ifneq)

#define sylvan_create_relational_proposition(op, value, variable_domain_size) \
  RUN(sylvan_create_relational_proposition, op, value, variable_domain_size)

#define sylvan_create_projection_cube(target_variables, num_target_variables, \
                                      local_variables, num_local_variables)   \
  RUN(sylvan_create_projection_cube, target_variables, num_target_variables,  \
      local_variables, num_local_variables)

#define sylvan_join_union(a, b, a_proj, b_proj) \
  RUN(sylvan_join_union, a, b, a_proj, b_proj)

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

uint32_t sylvan_ldd::get_value() const {
  const mddnode_t node = LDD_GETNODE(ldd_);
  const uint32_t value = mddnode_getvalue(node);

  return value;
}

sylvan_ldd operator*(const sylvan_ldd& lhs, const sylvan_ldd& rhs) {
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

sylvan_ldd& sylvan_ldd::operator*=(const sylvan_ldd& other) {
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

  variables_ = common_variables;

  return *this;
}

sylvan_ldd operator+(const sylvan_ldd& lhs, const sylvan_ldd& rhs) {
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

  MDD or_ldd = sylvan_join_union(lhs.ldd_, rhs.ldd_, lhs_cube, rhs_cube);

  lddmc_unprotect(&lhs_cube);
  lddmc_unprotect(&rhs_cube);

  return sylvan_ldd(or_ldd, std::move(common_variables));
}

sylvan_ldd& sylvan_ldd::operator+=(const sylvan_ldd& other) {
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

  ldd_ = sylvan_join_union(ldd_, other.ldd_, this_cube, other_cube);

  lddmc_unprotect(&this_cube);
  lddmc_unprotect(&other_cube);

  variables_ = common_variables;

  return *this;
}

size_t sylvan_ldd::node_count() const { return lddmc_nodecount(ldd_); }

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

void sylvan_ldd::get_sat_one_under_partial_assignment(
    std::vector<int>& assignment) const {

  MDD cube = sylvan_full_sat_one_under_partial_assignment(
      ldd_, variables_.data(), variables_.size(), assignment.data(),
      assignment.size());

  int var_idx = 0;
  while (cube != lddmc_true && cube != lddmc_false) {
    mddnode_t node = LDD_GETNODE(cube);
    uint32_t value = mddnode_getvalue(node);
    assignment[variables_[var_idx]] = value;
    cube = mddnode_getdown(node);
    ++var_idx;
  }
}

void sylvan::init_lace(unsigned int n_workers, size_t dqsize) {
  lace_start(n_workers, dqsize);
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

void sylvan::quit_package() { sylvan_quit(); }

}  // namespace detail
}  // namespace citcpp
