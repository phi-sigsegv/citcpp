#include "citcpp_sylvan_ldd.hpp"

#include <sylvan.h>
#include <sylvan_int.h>

#include <algorithm>
#include <atomic>
#include <fstream>
#include <iterator>
#include <utility>
#include <vector>

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

  const int value_as_int = static_cast<int>(value);

  switch (op) {
    case citcpp::relational_operator::NEQ: {
      MDD prop_ldd = lddmc_false;
      for (int i = variable_domain_size - 1; i >= 0; --i) {
        if (i != value_as_int) {
          prop_ldd = lddmc_makenode(i, lddmc_true, prop_ldd);
        }
      }

      return prop_ldd;
    }
    case citcpp::relational_operator::LT: {
      MDD prop_ldd = lddmc_false;
      for (int i = variable_domain_size - 1; i >= 0; --i) {
        if (i < value_as_int) {
          prop_ldd = lddmc_makenode(i, lddmc_true, prop_ldd);
        }
      }

      return prop_ldd;
    }
    case citcpp::relational_operator::LE: {
      MDD prop_ldd = lddmc_false;
      for (int i = variable_domain_size - 1; i >= 0; --i) {
        if (i <= value_as_int) {
          prop_ldd = lddmc_makenode(i, lddmc_true, prop_ldd);
        }
      }

      return prop_ldd;
    }
    case citcpp::relational_operator::GE: {
      MDD prop_ldd = lddmc_false;
      for (int i = variable_domain_size - 1; i >= 0; --i) {
        if (i >= value_as_int) {
          prop_ldd = lddmc_makenode(i, lddmc_true, prop_ldd);
        }
      }

      return prop_ldd;
    }
    case citcpp::relational_operator::GT: {
      MDD prop_ldd = lddmc_false;
      for (int i = variable_domain_size - 1; i >= 0; --i) {
        if (i > value_as_int) {
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

std::vector<uint32_t> get_common_variables(
    const std::vector<uint32_t>& lhs_vars,
    const std::vector<uint32_t>& rhs_vars) {

  std::size_t lhs_idx = 0;
  std::size_t rhs_idx = 0;
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

template <typename T>
uint32_t encode_interval(T lb, T ub) {
  return encode_interval((uint16_t)lb, (uint16_t)ub);
}

uint32_t encode_interval(interval ival) {
  return encode_interval(ival.lb, ival.ub);
}

interval decode_interval(uint32_t packed_interval) {
  uint16_t lb = packed_interval & 0xFFFF;
  uint16_t ub = (uint16_t)((packed_interval & 0xFFFF0000) >> 16);
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

static MDD sylvan_idd_makenode(uint32_t val, MDD down, MDD right) {
  if (right != lddmc_false) {
    mddnode_t n_right = LDD_GETNODE(right);
    if (mddnode_getdown(n_right) == down) {
      interval ival = decode_interval(val);
      interval right_ival = decode_interval(mddnode_getvalue(n_right));
      if (is_intervals_connected(ival, right_ival)) {
        return lddmc_makenode(encode_interval(interval_union(ival, right_ival)),
                              down, mddnode_getright(n_right));
      }
    }
  }
  return lddmc_makenode(val, down, right);
}

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

TASK_4(MDD, sylvan_idd_create_projection_cube, const uint32_t*,
       target_variables, int, num_target_variables, const uint32_t*,
       local_variables, int, num_local_variables) {

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

static const uint64_t CACHE_IDD_CONTAINS_PART_ASSIGN = (64LL << 40);
static const uint64_t CACHE_IDD_MARK_VALID = (65LL << 40);

TASK_5(uint64_t, sylvan_idd_sat_with_partial_assignment_recursive, MDD, ldd,
       uint32_t, var_idx, MDD, variables_cube, MDD, values_cube,
       std::atomic<bool>*, terminate) {

  if (variables_cube == lddmc_true) {
    *terminate = true;
    return 1;
  }
  if (ldd == lddmc_false || *terminate) {
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
      if (*terminate) return 0;
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
                     mddnode_getdown(var_node), mddnode_getdown(value_node),
                     terminate);
          if (!*terminate || sat == 1) {
            cache_put3(CACHE_IDD_CONTAINS_PART_ASSIGN, ldd, variables_cube,
                       values_cube, sat);
          }
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
                          values_cube, terminate);
      uint64_t down_sat = CALL(sylvan_idd_sat_with_partial_assignment_recursive,
                               mddnode_getdown(nldd), var_idx + 1,
                               variables_cube, values_cube, terminate);
      uint64_t right_sat =
          SYNC(sylvan_idd_sat_with_partial_assignment_recursive);
      sat = down_sat || right_sat;
      if (!*terminate || sat == 1) {
        cache_put3(CACHE_IDD_CONTAINS_PART_ASSIGN, ldd, variables_cube,
                   values_cube, sat);
      }
    }

    return sat;
  }
}

struct partial_assignment_ctx {
    const uint32_t* variables;
    int num_variables;
    const int* values;
    int num_values;
    const unsigned int* variable_order;
};

TASK_2(uint64_t, sylvan_idd_sat_with_partial_assignment, MDD, ldd,
       const partial_assignment_ctx*, ctx) {

  // First we create cubes that specifies which variable we have assignments
  // for, and what are the assigned values.
  MDD variables_cube = lddmc_true;
  MDD values_cube = lddmc_true;
  for (int v = ctx->num_variables - 1; v >= 0; --v) {
    const uint32_t var = ctx->variables[v];
    const int value = ctx->variable_order
                          ? ctx->values[ctx->variable_order[var]]
                          : ctx->values[var];

    if (value >= 0) {
      variables_cube = lddmc_makenode(v, variables_cube, lddmc_false);
      values_cube = lddmc_makenode(value, values_cube, lddmc_false);
    }
  }

  lddmc_refs_push(variables_cube);
  lddmc_refs_push(values_cube);

  std::atomic<bool> terminate(false);
  const uint64_t sat = CALL(sylvan_idd_sat_with_partial_assignment_recursive,
                            ldd, 0, variables_cube, values_cube, &terminate);

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
  result = sylvan_idd_makenode(encode_interval(intersection), down, right);

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
  if (a == lddmc_true || b == lddmc_true) return lddmc_true;

  mddnode_t na = LDD_GETNODE(a);
  mddnode_t nb = LDD_GETNODE(b);
  const uint32_t na_value = mddnode_getvalue(na);
  const uint32_t nb_value = mddnode_getvalue(nb);
  const MDD na_right = mddnode_getright(na);
  const MDD nb_right = mddnode_getright(nb);
  interval a_ival = decode_interval(na_value);
  interval b_ival = decode_interval(nb_value);

  if (is_intervals_connected(a_ival, b_ival)) {
    interval span = interval_union(a_ival, b_ival);
    MDD right = sylvan_idd_one_level_set_union(na_right, nb_right);
    lddmc_refs_push(right);

    while (right != lddmc_false) {
      mddnode_t n_right = LDD_GETNODE(right);
      interval right_ival = decode_interval(mddnode_getvalue(n_right));
      if (is_intervals_connected(span, right_ival)) {
        span = interval_union(span, right_ival);
        MDD next_right = mddnode_getright(n_right);
        lddmc_refs_pop(1);
        right = next_right;
        lddmc_refs_push(right);
      } else {
        break;
      }
    }

    MDD result = lddmc_makenode(encode_interval(span), lddmc_true, right);
    lddmc_refs_pop(1);
    return result;
  }

  // When reaching this point, the intervals stored
  // in the nodes are not connected.

  if (a_ival.ub < b_ival.lb) {
    // The interval stored in node a is ordered before the one stored in node b.
    MDD right = sylvan_idd_one_level_set_union(na_right, b);
    return lddmc_makenode(na_value, lddmc_true, right);
  } else {
    // The interval stored in node b is ordered before the one stored in node a.
    MDD right = sylvan_idd_one_level_set_union(a, nb_right);
    return lddmc_makenode(nb_value, lddmc_true, right);
  }
}

static const uint64_t CACHE_IDD_GET_VALID_ASSIGNS = (65LL << 40);

TASK_7(MDD, sylvan_idd_get_valid_variable_assignments_recursive, MDD, ldd, MDD,
       variables_cube, MDD, values_cube, uint32_t, cur_var_idx, uint32_t,
       variable_index, std::atomic<bool>*, terminate, MDD, full_domain) {
  if (variables_cube == lddmc_true && variable_index < cur_var_idx) {
    // No more valid value for the parameter on this path.
    return lddmc_true;
  }
  if (ldd == lddmc_false || *terminate) {
    // No valid value for the parameter on this path.
    return lddmc_false;
  }

  assert(ldd != lddmc_true);

  mddnode_t var_node = LDD_GETNODE(variables_cube);
  uint32_t variable = mddnode_getvalue(var_node);

  if (variables_cube != lddmc_true && variable == cur_var_idx) {
    // We reached a variable, which is part of the partial assignment.
    mddnode_t value_node = LDD_GETNODE(values_cube);
    uint32_t value = mddnode_getvalue(value_node);
    while (ldd != lddmc_false) {
      if (*terminate) return lddmc_false;
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
          collected_values = CALL(
              sylvan_idd_get_valid_variable_assignments_recursive, nldd_down,
              mddnode_getdown(var_node), mddnode_getdown(value_node),
              cur_var_idx + 1, variable_index, terminate, full_domain);
          if (!*terminate || collected_values == full_domain) {
            cache_put4(CACHE_IDD_GET_VALID_ASSIGNS, ldd, variables_cube,
                       values_cube, variable_index, collected_values);
          }
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
                variables_cube, values_cube, cur_var_idx, variable_index,
                terminate, full_domain));
      MDD down_set =
          CALL(sylvan_idd_get_valid_variable_assignments_recursive, nldd_down,
               variables_cube, values_cube, cur_var_idx + 1, variable_index,
               terminate, full_domain);
      lddmc_refs_push(down_set);
      MDD right_set = lddmc_refs_sync(
          SYNC(sylvan_idd_get_valid_variable_assignments_recursive));
      lddmc_refs_pop(1);

      if (variable_index == cur_var_idx) {
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

      if (collected_values == full_domain) {
        *terminate = true;
      } else if (*terminate) {
        // Some other thread found the full domain, and we don't have it (yet).
        // To avoid returning partial results, we return lddmc_false.
        return lddmc_false;
      }

      if (!*terminate || collected_values == full_domain) {
        cache_put4(CACHE_IDD_GET_VALID_ASSIGNS, ldd, variables_cube,
                   values_cube, variable_index, collected_values);
      }
    }

    return collected_values;
  }
}

TASK_4(MDD, sylvan_idd_get_valid_variable_assignments, MDD, ldd, uint32_t,
       variable, uint32_t, domain_size, const partial_assignment_ctx*, ctx) {

  // First we create cubes that specify which variable we have assignments
  // for, and what are the assigned values.
  MDD variables_cube = lddmc_true;
  MDD values_cube = lddmc_true;
  int variable_index = -1;
  for (int v = ctx->num_variables - 1; v >= 0; --v) {
    const uint32_t var = ctx->variables[v];
    if (var == variable) {
      variable_index = v;
    }

    const int value = ctx->variable_order
                          ? ctx->values[ctx->variable_order[var]]
                          : ctx->values[var];
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

  MDD full_domain = lddmc_makenode(encode_interval(0, domain_size - 1),
                                   lddmc_true, lddmc_false);
  lddmc_refs_push(full_domain);
  std::atomic<bool> terminate(false);

  const MDD collected_values = CALL(
      sylvan_idd_get_valid_variable_assignments_recursive, ldd, variables_cube,
      values_cube, 0, (uint32_t)variable_index, &terminate, full_domain);

  lddmc_refs_pop(3);

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

TASK_3(MDD, sylvan_idd_create_cube_from_assignments_with_order, const int*,
       assignments, const unsigned int*, variable_order, int, num_levels) {

  MDD cube = lddmc_true;
  for (int level = num_levels - 1; level >= 0; --level) {
    const int value = assignments[variable_order[level]];
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

TASK_2(MDD, sylvan_idd_full_sat_one_under_partial_assignment, MDD, ldd,
       const partial_assignment_ctx*, ctx) {

  // First we create cubes that specify which variables we have assignments
  // for, and what are the assigned values.
  MDD variables_cube = lddmc_true;
  MDD values_cube = lddmc_true;
  for (int v = ctx->num_variables - 1; v >= 0; --v) {
    const uint32_t var = ctx->variables[v];
    const int value = ctx->variable_order
                          ? ctx->values[ctx->variable_order[var]]
                          : ctx->values[var];

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
  result = sylvan_idd_makenode(val, down, right);

  /* Write to cache */
  cache_put3(CACHE_IDD_INV_PROJ, a, b, proj, result);

  return result;
}

static const uint64_t CACHE_IDD_UNION = (68LL << 40);

TASK_2(MDD, sylvan_idd_union, MDD, a, MDD, b) {
  /* Terminal cases */
  if (a == b) return a;
  if (a == lddmc_false) return b;
  if (b == lddmc_false) return a;
  assert(a != lddmc_true && b != lddmc_true);

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
  const uint32_t na_val = mddnode_getvalue(na);
  const uint32_t nb_val = mddnode_getvalue(nb);
  const MDD na_down = mddnode_getdown(na);
  const MDD na_right = mddnode_getright(na);
  const MDD nb_down = mddnode_getdown(nb);
  const MDD nb_right = mddnode_getright(nb);

  interval a_ival = decode_interval(na_val);
  interval b_ival = decode_interval(nb_val);

  if (a_ival.lb < b_ival.lb) {
    /* Case 1: a starts before b */
    if (a_ival.ub < b_ival.lb) {
      /* Case 1a: a is entirely before b */
      MDD right = CALL(sylvan_idd_union, na_right, b);
      result = sylvan_idd_makenode(na_val, na_down, right);
    } else {
      /* Case 1b: a and b overlap or are adjacent at the start */
      /* Split a into [a.lb, b.lb-1] and [b.lb, a.ub] */
      interval first = {a_ival.lb, (uint16_t)(b_ival.lb - 1)};
      interval second = {b_ival.lb, a_ival.ub};
      MDD a_second = lddmc_makenode(encode_interval(second), na_down, na_right);
      lddmc_refs_push(a_second);
      MDD right = CALL(sylvan_idd_union, a_second, b);
      lddmc_refs_pop(1);
      result = sylvan_idd_makenode(encode_interval(first), na_down, right);
    }
  } else if (b_ival.lb < a_ival.lb) {
    /* Case 2: b starts before a */
    if (b_ival.ub < a_ival.lb) {
      /* Case 2a: b is entirely before a */
      MDD right = CALL(sylvan_idd_union, a, nb_right);
      result = sylvan_idd_makenode(nb_val, nb_down, right);
    } else {
      /* Case 2b: b and a overlap or are adjacent at the start */
      /* Split b into [b.lb, a.lb-1] and [a.lb, b.ub] */
      interval first = {b_ival.lb, (uint16_t)(a_ival.lb - 1)};
      interval second = {a_ival.lb, b_ival.ub};
      MDD b_second = lddmc_makenode(encode_interval(second), nb_down, nb_right);
      lddmc_refs_push(b_second);
      MDD right = CALL(sylvan_idd_union, a, b_second);
      lddmc_refs_pop(1);
      result = sylvan_idd_makenode(encode_interval(first), nb_down, right);
    }
  } else {
    /* Case 3: a and b start at the same value */
    interval intersection = interval_intersection(a_ival, b_ival);
    lddmc_refs_spawn(SPAWN(sylvan_idd_union, na_down, nb_down));

    MDD right;
    if (a_ival.ub < b_ival.ub) {
      /* Split b into [b.lb, a.ub] and [a.ub+1, b.ub] */
      interval b_rem = {(uint16_t)(a_ival.ub + 1), b_ival.ub};
      MDD mod_b = lddmc_makenode(encode_interval(b_rem), nb_down, nb_right);
      lddmc_refs_push(mod_b);
      right = CALL(sylvan_idd_union, na_right, mod_b);
      lddmc_refs_pop(1);
    } else if (b_ival.ub < a_ival.ub) {
      /* Split a into [a.lb, b.ub] and [b.ub+1, a.ub] */
      interval a_rem = {(uint16_t)(b_ival.ub + 1), a_ival.ub};
      MDD mod_a = lddmc_makenode(encode_interval(a_rem), na_down, na_right);
      lddmc_refs_push(mod_a);
      right = CALL(sylvan_idd_union, mod_a, nb_right);
      lddmc_refs_pop(1);
    } else {
      /* a and b have identical intervals */
      right = CALL(sylvan_idd_union, na_right, nb_right);
    }

    lddmc_refs_push(right);
    MDD down = lddmc_refs_sync(SYNC(sylvan_idd_union));
    lddmc_refs_pop(1);
    result = sylvan_idd_makenode(encode_interval(intersection), down, right);
  }

  /* Write to cache */
  cache_put3(CACHE_IDD_UNION, a, b, 0, result);

  return result;
}

static const uint64_t CACHE_IDD_PROJECT = (69LL << 40);

TASK_2(MDD, sylvan_idd_project, MDD, mdd, MDD, proj) {
  if (mdd == lddmc_false) return lddmc_false;
  if (mdd == lddmc_true) return lddmc_true;

  mddnode_t p_node = LDD_GETNODE(proj);
  uint32_t p_val = mddnode_getvalue(p_node);
  if (p_val == (uint32_t)-1) return mdd;
  if (p_val == (uint32_t)-2) return lddmc_true;

  sylvan_gc_test();

  MDD result;
  if (cache_get3(CACHE_IDD_PROJECT, mdd, proj, 0, &result)) {
    return result;
  }

  mddnode_t n = LDD_GETNODE(mdd);

  if (p_val == 1) {  // Keep level
    lddmc_refs_spawn(SPAWN(sylvan_idd_project, mddnode_getright(n), proj));
    MDD down =
        CALL(sylvan_idd_project, mddnode_getdown(n), mddnode_getdown(p_node));
    lddmc_refs_push(down);
    MDD right = lddmc_refs_sync(SYNC(sylvan_idd_project));
    lddmc_refs_pop(1);
    result = sylvan_idd_makenode(mddnode_getvalue(n), down, right);
  } else {  // Quantify level (p_val == 0)
    int count = 0;
    MDD p_down = mddnode_getdown(p_node);
    MDD _mdd = mdd;
    while (1) {
      lddmc_refs_spawn(SPAWN(sylvan_idd_project, mddnode_getdown(n), p_down));
      count++;
      _mdd = mddnode_getright(n);
      if (_mdd == lddmc_false) break;
      n = LDD_GETNODE(_mdd);
    }
    result = lddmc_false;
    while (count--) {
      lddmc_refs_push(result);
      MDD down = lddmc_refs_sync(SYNC(sylvan_idd_project));
      lddmc_refs_push(down);
      result = CALL(sylvan_idd_union, result, down);
      lddmc_refs_pop(2);
    }
  }

  cache_put3(CACHE_IDD_PROJECT, mdd, proj, 0, result);

  return result;
}

struct marking_context {
    const size_t* weights;
    const unsigned int* domain_sizes;
    int t;
    const uint32_t* idd_vars;
    citcpp::detail::spin_lock* lock;
    citcpp::detail::coverage_bitset* value_combinations;
    const citcpp::detail::param_vector* param_indices;
    uint64_t invocation_id;
};

static int sylvan_idd_fill_all_recursive(MDD cube, size_t current_index,
                                         const marking_context* ctx) {

  if (ctx->value_combinations->all_valid()) return 1;

  if (cube == lddmc_true) {
    ctx->lock->lock();
    if (!ctx->value_combinations->all_valid()) {
      ctx->value_combinations->set_valid(current_index);
    }
    ctx->lock->unlock();
    return 0;
  }

  mddnode_t n_cube = LDD_GETNODE(cube);
  uint32_t cube_val = mddnode_getvalue(n_cube);
  uint32_t p_idx = cube_val & 0xFFFF;
  uint32_t domain_size = ctx->domain_sizes[(*ctx->param_indices)[p_idx]];
  size_t weight = ctx->weights[p_idx];
  MDD down_cube = mddnode_getdown(n_cube);

  for (uint32_t v = 0; v < domain_size; ++v) {
    if (sylvan_idd_fill_all_recursive(down_cube, current_index + v * weight,
                                      ctx)) {
      return 1;
    }
  }

  return 0;
}

TASK_5(int, sylvan_idd_mark_valid_recursive, MDD, ldd, MDD, cube, int,
       idd_var_idx, size_t, current_index, const marking_context*, ctx) {

  if (ctx->value_combinations->all_valid()) return 1;

  if (ldd == lddmc_false) return 0;

  if (cube == lddmc_true) {
    ctx->lock->lock();
    if (!ctx->value_combinations->all_valid()) {
      ctx->value_combinations->set_valid(current_index);
    }
    ctx->lock->unlock();
    return 0;
  }

  if (ldd == lddmc_true) {
    sylvan_idd_fill_all_recursive(cube, current_index, ctx);
    return 0;
  }

  uint64_t res;
  if (cache_get6(ldd | CACHE_IDD_MARK_VALID, cube, current_index,
                 ctx->invocation_id, 0, 0, &res, nullptr)) {
    return (int)res;
  }

  mddnode_t n_ldd = LDD_GETNODE(ldd);
  mddnode_t n_cube = LDD_GETNODE(cube);
  uint32_t cube_val = mddnode_getvalue(n_cube);
  uint32_t var_cube = cube_val >> 16;
  uint32_t p_idx = cube_val & 0xFFFF;

  uint32_t var_ldd = ctx->idd_vars[idd_var_idx];

  int aborted = 0;

  if (var_ldd < var_cube) {
    // Current IDD variable is not of interest.
    SPAWN(sylvan_idd_mark_valid_recursive, mddnode_getright(n_ldd), cube,
          idd_var_idx, current_index, ctx);
    if (CALL(sylvan_idd_mark_valid_recursive, mddnode_getdown(n_ldd), cube,
             idd_var_idx + 1, current_index, ctx))
      aborted = 1;
    if (SYNC(sylvan_idd_mark_valid_recursive)) aborted = 1;
  } else if (var_ldd == var_cube) {
    // Current variable is of interest and constrained.
    SPAWN(sylvan_idd_mark_valid_recursive, mddnode_getright(n_ldd), cube,
          idd_var_idx, current_index, ctx);

    uint32_t val = mddnode_getvalue(n_ldd);
    interval ival = decode_interval(val);
    MDD down_ldd = mddnode_getdown(n_ldd);
    MDD down_cube = mddnode_getdown(n_cube);
    size_t weight = ctx->weights[p_idx];

    for (uint32_t v = ival.lb; v <= ival.ub; ++v) {
      if (CALL(sylvan_idd_mark_valid_recursive, down_ldd, down_cube,
               idd_var_idx + 1, current_index + v * weight, ctx)) {
        aborted = 1;
        break;
      }
    }
    if (SYNC(sylvan_idd_mark_valid_recursive)) aborted = 1;
  } else {
    // var_ldd > var_cube: Variable of interest is not constrained in IDD (it's
    // free).
    uint32_t domain_size = ctx->domain_sizes[(*ctx->param_indices)[p_idx]];
    size_t weight = ctx->weights[p_idx];
    MDD down_cube = mddnode_getdown(n_cube);

    for (uint32_t v = 0; v < domain_size; ++v) {
      if (CALL(sylvan_idd_mark_valid_recursive, ldd, down_cube, idd_var_idx,
               current_index + v * weight, ctx)) {
        aborted = 1;
        break;
      }
    }
  }

  if (!aborted) {
    cache_put6(ldd | CACHE_IDD_MARK_VALID, cube, current_index,
               ctx->invocation_id, 0, 0, 0, 0);
  }

  return aborted;
}

struct p_info {
    uint32_t id;
    uint32_t pos;
};

VOID_TASK_4(sylvan_idd_mark_valid, MDD, ldd, const p_info*, vars, int, num_vars,
            const marking_context*, ctx) {

  MDD cube = lddmc_true;
  for (int i = num_vars - 1; i >= 0; --i) {
    uint32_t val = (vars[i].id << 16) | vars[i].pos;
    cube = lddmc_makenode(val, cube, lddmc_false);
  }

  lddmc_protect(&cube);

  CALL(sylvan_idd_mark_valid_recursive, ldd, cube, 0, 0, ctx);

  lddmc_unprotect(&cube);
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

#define sylvan_make_node(value, ifeq, ifneq) \
  RUN(sylvan_make_node, value, ifeq, ifneq)

#define sylvan_idd_create_relational_proposition(op, value,            \
                                                 variable_domain_size) \
  RUN(sylvan_idd_create_relational_proposition, op, value, variable_domain_size)

#define sylvan_idd_create_projection_cube(                   \
    target_variables, num_target_variables, local_variables, \
    num_local_variables)                                     \
  RUN(sylvan_idd_create_projection_cube, target_variables,   \
      num_target_variables, local_variables, num_local_variables)

#define sylvan_idd_create_cube_from_assignments_with_order(            \
    assignments, variable_order, num_levels)                           \
  RUN(sylvan_idd_create_cube_from_assignments_with_order, assignments, \
      variable_order, num_levels)

#define sylvan_idd_sat_with_partial_assignment(                        \
    ldd, variables, num_variables, values, num_values, variable_order) \
  ({                                                                   \
    partial_assignment_ctx _ctx = {variables, num_variables, values,   \
                                   num_values, variable_order};        \
    RUN(sylvan_idd_sat_with_partial_assignment, ldd, &_ctx);           \
  })

#define sylvan_idd_get_valid_variable_assignments(                             \
    ldd, variables, num_variables, values, num_values, variable, domain_size,  \
    variable_order)                                                            \
  ({                                                                           \
    partial_assignment_ctx _ctx = {variables, num_variables, values,           \
                                   num_values, variable_order};                \
    RUN(sylvan_idd_get_valid_variable_assignments, ldd, variable, domain_size, \
        &_ctx);                                                                \
  })

#define sylvan_idd_create_cube_from_assignments(assignments, num_assignments) \
  RUN(sylvan_idd_create_cube_from_assignments, assignments, num_assignments)

#define sylvan_idd_full_sat_one_under_partial_assignment(              \
    ldd, variables, num_variables, values, num_values, variable_order) \
  ({                                                                   \
    partial_assignment_ctx _ctx = {variables, num_variables, values,   \
                                   num_values, variable_order};        \
    RUN(sylvan_idd_full_sat_one_under_partial_assignment, ldd, &_ctx); \
  })

#define sylvan_idd_join(a, b, a_proj, b_proj) \
  RUN(sylvan_idd_join, a, b, a_proj, b_proj)

#define sylvan_idd_mark_valid(ldd, vars, num_vars, ctx) \
  RUN(sylvan_idd_mark_valid, ldd, vars, num_vars, ctx)

#define sylvan_idd_satcount(mdd) RUN(sylvan_idd_satcount, mdd)

#define sylvan_idd_create_universe(variables, count, domain_sizes) \
  RUN(sylvan_idd_create_universe, variables, count, domain_sizes)

#define sylvan_idd_inv_project(a, b, proj) \
  RUN(sylvan_idd_inv_project, a, b, proj)

#define sylvan_idd_project(mdd, proj) RUN(sylvan_idd_project, mdd, proj)

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

sylvan_idd::sylvan_idd() : idd_(lddmc_false), variables_() {
  lddmc_protect(&idd_);
}

sylvan_idd::sylvan_idd(uint32_t variable, uint32_t value)
    : idd_(sylvan_make_node(encode_interval(value, value), lddmc_true,
                            lddmc_false)),
      variables_(1, variable) {

  lddmc_protect(&idd_);
}

sylvan_idd::sylvan_idd(const std::vector<int>& assignments,
                       const std::vector<unsigned int>* variable_order)
    : idd_(variable_order ? sylvan_idd_create_cube_from_assignments_with_order(
                                assignments.data(), variable_order->data(),
                                variable_order->size())
                          : sylvan_idd_create_cube_from_assignments(
                                assignments.data(), assignments.size())),
      variables_() {

  if (variable_order) {
    for (int level = 0; level < variable_order->size(); ++level) {
      const int value = assignments[(*variable_order)[level]];
      if (value >= 0) {
        variables_.push_back(level);
      }
    }
  } else {
    for (int var = 0; var < assignments.size(); ++var) {
      const int value = assignments[var];
      if (value >= 0) {
        variables_.push_back(var);
      }
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

sylvan_idd sylvan_idd::project(
    const std::vector<uint32_t>& target_variables) const {
  if (idd_ == lddmc_false || idd_ == lddmc_true) return *this;

  // The resulting variables are the intersection of target_variables and this
  // IDD's variables.
  std::vector<uint32_t> actual_target_vars;
  std::set_intersection(variables_.begin(), variables_.end(),
                        target_variables.begin(), target_variables.end(),
                        std::back_inserter(actual_target_vars));

  // Efficient check: if sizes match, target covers all source variables
  // (identity)
  if (actual_target_vars.size() == variables_.size()) return *this;
  if (actual_target_vars.empty()) return sylvan_idd::iddTrue();

  // Create projection cube: iterate over SOURCE variables (variables_)
  // and mark which ones are in the TARGET subset (actual_target_vars).
  MDD proj_cube = sylvan_idd_create_projection_cube(
      variables_.data(), variables_.size(), actual_target_vars.data(),
      actual_target_vars.size());
  lddmc_protect(&proj_cube);

  MDD projected_idd = sylvan_idd_project(idd_, proj_cube);
  lddmc_unprotect(&proj_cube);

  return sylvan_idd(projected_idd, std::move(actual_target_vars));
}

void sylvan_idd::mark_valid_value_combinations(
    coverage_bitset& value_combinations, const param_vector& param_indices,
    const std::vector<unsigned int>& domain_sizes,
    const std::vector<unsigned int>* parameter_to_level) const {

  if (idd_ == lddmc_false) return;
  if (value_combinations.all_valid()) return;

  static std::atomic<uint64_t> next_invocation_id{1};
  uint64_t invocation_id =
      next_invocation_id.fetch_add(1, std::memory_order_relaxed);

  const int t = param_indices.size();

  std::vector<size_t> weights(t);
  size_t current_weight = 1;
  for (int i = t - 1; i >= 0; --i) {
    weights[i] = current_weight;
    current_weight *= domain_sizes[param_indices[i]];
  }

  // Prepare parameters of interest sorted by their ID (which is their level in
  // the IDD)
  std::vector<p_info> sorted_p;
  for (uint32_t i = 0; i < t; ++i) {
    uint32_t p_id = parameter_to_level ? (*parameter_to_level)[param_indices[i]]
                                       : param_indices[i];
    sorted_p.push_back({p_id, i});
  }
  std::sort(sorted_p.begin(), sorted_p.end(),
            [](const p_info& a, const p_info& b) { return a.id < b.id; });

  citcpp::detail::spin_lock lock;
  marking_context ctx;
  ctx.weights = weights.data();
  ctx.domain_sizes = domain_sizes.data();
  ctx.t = t;
  ctx.idd_vars = variables_.data();
  ctx.lock = &lock;
  ctx.value_combinations = &value_combinations;
  ctx.param_indices = &param_indices;
  ctx.invocation_id = invocation_id;

  sylvan_idd_mark_valid(idd_, sorted_p.data(), t, &ctx);
}

size_t sylvan_idd::node_count() const { return lddmc_nodecount(idd_); }

long double sylvan_idd::sat_count() const { return sylvan_idd_satcount(idd_); }

void sylvan_idd::get_sat_one(
    std::vector<int>& assignment,
    const std::vector<unsigned int>* variable_order) const {
  MDD cube = idd_;

  int var_idx = 0;
  while (cube != lddmc_true && cube != lddmc_false) {
    mddnode_t node = LDD_GETNODE(cube);
    uint32_t n_value = mddnode_getvalue(node);
    interval ival = decode_interval(n_value);
    uint32_t level = variables_[var_idx];
    uint32_t target_idx = variable_order ? (*variable_order)[level] : level;
    assignment[target_idx] = ival.lb;
    cube = mddnode_getdown(node);
    ++var_idx;
  }
}

void sylvan_idd::print_dot(const std::string& file_path) const {
  std::ofstream f{file_path};
  sylvan_idd_fprintdot(f, idd_);
}

bool sylvan_idd::is_sat_with_partial_assignment(
    const std::vector<int>& partial_assignment,
    const std::vector<unsigned int>* variable_order) const {

  const int res = sylvan_idd_sat_with_partial_assignment(
      idd_, variables_.data(), (int)variables_.size(),
      partial_assignment.data(), (int)partial_assignment.size(),
      variable_order ? variable_order->data() : nullptr);

  return res != 0;
}

bitset_uint64 sylvan_idd::get_valid_variable_assignments(
    uint32_t variable, uint32_t domain_size,
    const std::vector<int>& partial_assignment,
    const std::vector<unsigned int>* variable_order) const {

  bitset_uint64 valid_values_as_bitset(domain_size);

  const int value = variable_order
                        ? partial_assignment[(*variable_order)[variable]]
                        : partial_assignment[variable];
  if (value >= 0) {
    valid_values_as_bitset.set(value);

    return valid_values_as_bitset;
  }

  MDD valid_values_as_ldd = sylvan_idd_get_valid_variable_assignments(
      idd_, variables_.data(), (int)variables_.size(),
      partial_assignment.data(), (int)partial_assignment.size(), variable,
      domain_size, variable_order ? variable_order->data() : nullptr);

  if (valid_values_as_ldd == lddmc_true) {
    valid_values_as_bitset.set();
  } else {
    while (valid_values_as_ldd != lddmc_false) {
      mddnode_t node = LDD_GETNODE(valid_values_as_ldd);
      uint32_t n_value = mddnode_getvalue(node);
      interval ival = decode_interval(n_value);
      for (uint16_t ival_value = ival.lb; ival_value <= ival.ub; ++ival_value) {
        valid_values_as_bitset.set(ival_value);
      }
      valid_values_as_ldd = mddnode_getright(node);
    }
  }

  return valid_values_as_bitset;
}

void sylvan_idd::get_sat_one_under_partial_assignment(
    std::vector<int>& assignment,
    const std::vector<unsigned int>* variable_order) const {

  MDD cube = sylvan_idd_full_sat_one_under_partial_assignment(
      idd_, variables_.data(), (int)variables_.size(), assignment.data(),
      (int)assignment.size(),
      variable_order ? variable_order->data() : nullptr);

  int var_idx = 0;
  while (cube != lddmc_true && cube != lddmc_false) {
    mddnode_t node = LDD_GETNODE(cube);
    uint32_t level = variables_[var_idx];
    uint32_t target_idx =
        variable_order ? (uint32_t)(*variable_order)[level] : level;
    if (assignment[target_idx] < 0) {
      uint32_t n_value = mddnode_getvalue(node);
      interval ival = decode_interval(n_value);
      assignment[target_idx] = ival.lb;
    }
    cube = mddnode_getdown(node);
    ++var_idx;
  }
}

void sylvan::init_lace(unsigned int n_workers, std::size_t dqsize) {
  citcpp::detail::lace_init(n_workers, dqsize);
}

void sylvan::init_package(std::size_t initialTableSize,
                          std::size_t maxTableSize,
                          std::size_t initialCacheSize,
                          std::size_t maxCacheSize) {

  sylvan_set_sizes(initialTableSize, maxTableSize, initialCacheSize,
                   maxCacheSize);
  sylvan_init_package();
}

void sylvan::init_package(std::size_t memory_cap, int table_ratio,
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
