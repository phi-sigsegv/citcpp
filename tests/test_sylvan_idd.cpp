#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest.h>

#include <algorithm>
#include <cstdint>
#include <mutex>

#include "../citcpp_lib/src/detail/citcpp_sylvan_ldd.hpp"

namespace {

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

class sylvan_lifecycle {
  public:
    sylvan_lifecycle() { maybe_initialize_sylvan(1); }

    ~sylvan_lifecycle() { maybe_shutdown_sylvan(); }
};

TEST_CASE("sylvan IDD, testing single value") {
  using namespace citcpp;
  using namespace citcpp::detail;

  sylvan_lifecycle sylvan_lifecycle;

  sylvan_idd idd(3, 7);
  uint32_t lb = 0;
  uint32_t ub = 0;
  idd.get_interval(lb, ub);

  CHECK(encode_interval(7, 7) == idd.get_encoded_interval());
  CHECK(lb == 7);
  CHECK(ub == 7);
  CHECK(idd.get_down_node() == sylvan_idd::iddTrue());
  CHECK(idd.get_right_node() == sylvan_idd::iddFalse());
  CHECK(idd.node_count() == 1);
  CHECK((int)idd.sat_count() == 1);
}

TEST_CASE("sylvan IDD, testing merge logic bug") {
  using namespace citcpp;
  using namespace citcpp::detail;

  sylvan_lifecycle sylvan_lifecycle;

  // We want to create a situation where:
  // result = [10, 11] with D1 -> [12, 20] with D2 -> [21, 25] with D2
  // D1 = {var1=1}, D2 = {var1=1, var1=2}

  sylvan_idd d1(1, 1);    // var1 = 1
  sylvan_idd d2_2(1, 2);  // var1 = 2

  std::vector<unsigned int> domain_sizes{0, 3};  // var0: any, var1: 3
  sylvan_idd d2 =
      sylvan_idd::project_union(d1, d2_2, domain_sizes);  // var1 = 1 or 2

  sylvan_idd a_part(0, relational_operator::LE, 20, 31);   // var0 <= 20
  sylvan_idd a_part2(0, relational_operator::GE, 10, 31);  // var0 >= 10
  sylvan_idd a_range =
      sylvan_idd::project_intersect(a_part, a_part2);  // var0 in [10, 20]

  sylvan_idd a = sylvan_idd::project_intersect(a_range, d1);

  sylvan_idd b_part(0, relational_operator::LE, 25, 31);
  sylvan_idd b_part2(0, relational_operator::GE, 12, 31);
  sylvan_idd b_range =
      sylvan_idd::project_intersect(b_part, b_part2);  // var0 in [12, 25]

  sylvan_idd b = sylvan_idd::project_intersect(b_range, d2);

  sylvan_idd union_res = sylvan_idd::project_union(a, b, domain_sizes);

  // Let's check the top level intervals.
  uint32_t lb, ub;
  union_res.get_interval(lb, ub);
  CHECK(lb == 10);
  CHECK(ub == 11);

  sylvan_idd right = union_res.get_right_node();
  CHECK(right != sylvan_idd::iddFalse());
  right.get_interval(lb, ub);

  // If merged, the next interval should be [12, 25].
  // If NOT merged (the bug), it will be [12, 20].
  CHECK(ub == 25);
}

TEST_CASE("sylvan IDD, testing boundary conditions") {
  using namespace citcpp;
  using namespace citcpp::detail;

  sylvan_lifecycle sylvan_lifecycle;
  std::vector<unsigned int> domain_sizes{65536};

  // Test union with 65535
  sylvan_idd v_0_to_65534(0, relational_operator::LE, 65534, 65536);
  sylvan_idd v_65535(0, 65535);

  sylvan_idd union_res =
      sylvan_idd::project_union(v_0_to_65534, v_65535, domain_sizes);
  CHECK(union_res.get_encoded_interval() == encode_interval(0, 65535));
  CHECK(union_res.get_right_node() == sylvan_idd::iddFalse());

  // Test union with overlapping 65535
  sylvan_idd v_65530_to_65535(0, relational_operator::GE, 65530, 65536);
  sylvan_idd v_65532_to_65535(0, relational_operator::GE, 65532, 65536);
  sylvan_idd union_overlap = sylvan_idd::project_union(
      v_65530_to_65535, v_65532_to_65535, domain_sizes);
  CHECK(union_overlap.get_encoded_interval() == encode_interval(65530, 65535));

  // Test intersection with 65535
  sylvan_idd intersect_res =
      sylvan_idd::project_intersect(v_65530_to_65535, v_65532_to_65535);
  CHECK(intersect_res.get_encoded_interval() == encode_interval(65532, 65535));

  // Test connectivity at the boundary
  sylvan_idd v_0(0, 0);
  sylvan_idd v_1(0, 1);
  sylvan_idd union_0_1 = sylvan_idd::project_union(v_0, v_1, domain_sizes);
  CHECK(union_0_1.get_encoded_interval() == encode_interval(0, 1));
}

TEST_CASE("sylvan IDD, testing atomic prop") {
  using namespace citcpp;
  using namespace citcpp::detail;

  sylvan_lifecycle sylvan_lifecycle;

  uint32_t lb = 0;
  uint32_t ub = 0;

  sylvan_idd v3_eq_7(3, relational_operator::EQ, 7, 10);
  v3_eq_7.get_interval(lb, ub);
  CHECK(encode_interval(7, 7) == v3_eq_7.get_encoded_interval());
  CHECK(lb == 7);
  CHECK(ub == 7);
  CHECK(v3_eq_7.get_down_node() == sylvan_idd::iddTrue());
  CHECK(v3_eq_7.get_right_node() == sylvan_idd::iddFalse());
  CHECK(v3_eq_7.node_count() == 1);
  CHECK((int)v3_eq_7.sat_count() == 1);

  sylvan_idd v3_le_7(3, relational_operator::LE, 7, 10);
  v3_le_7.get_interval(lb, ub);
  CHECK(encode_interval(0, 7) == v3_le_7.get_encoded_interval());
  CHECK(lb == 0);
  CHECK(ub == 7);
  CHECK(v3_le_7.get_down_node() == sylvan_idd::iddTrue());
  CHECK(v3_le_7.get_right_node() == sylvan_idd::iddFalse());
  CHECK(v3_le_7.node_count() == 1);
  CHECK((int)v3_le_7.sat_count() == 8);

  sylvan_idd v3_lt_7(3, relational_operator::LT, 7, 10);
  v3_lt_7.get_interval(lb, ub);
  CHECK(encode_interval(0, 6) == v3_lt_7.get_encoded_interval());
  CHECK(lb == 0);
  CHECK(ub == 6);
  CHECK(v3_lt_7.get_down_node() == sylvan_idd::iddTrue());
  CHECK(v3_lt_7.get_right_node() == sylvan_idd::iddFalse());
  CHECK(v3_lt_7.node_count() == 1);
  CHECK((int)v3_lt_7.sat_count() == 7);

  sylvan_idd v3_ge_7(3, relational_operator::GE, 7, 10);
  v3_ge_7.get_interval(lb, ub);
  CHECK(encode_interval(7, 9) == v3_ge_7.get_encoded_interval());
  CHECK(lb == 7);
  CHECK(ub == 9);
  CHECK(v3_ge_7.get_down_node() == sylvan_idd::iddTrue());
  CHECK(v3_ge_7.get_right_node() == sylvan_idd::iddFalse());
  CHECK(v3_ge_7.node_count() == 1);
  CHECK((int)v3_ge_7.sat_count() == 3);

  sylvan_idd v3_gt_7(3, relational_operator::GT, 7, 10);
  v3_gt_7.get_interval(lb, ub);
  CHECK(encode_interval(8, 9) == v3_gt_7.get_encoded_interval());
  CHECK(lb == 8);
  CHECK(ub == 9);
  CHECK(v3_gt_7.get_down_node() == sylvan_idd::iddTrue());
  CHECK(v3_gt_7.get_right_node() == sylvan_idd::iddFalse());
  CHECK(v3_gt_7.node_count() == 1);
  CHECK((int)v3_gt_7.sat_count() == 2);

  sylvan_idd v3_neq_7_first_half(3, relational_operator::NEQ, 7, 10);
  v3_neq_7_first_half.get_interval(lb, ub);
  CHECK(encode_interval(0, 6) == v3_neq_7_first_half.get_encoded_interval());
  CHECK(lb == 0);
  CHECK(ub == 6);
  CHECK(v3_neq_7_first_half.get_down_node() == sylvan_idd::iddTrue());
  CHECK(v3_neq_7_first_half.get_right_node() != sylvan_idd::iddFalse());
  CHECK(v3_neq_7_first_half.node_count() == 2);
  CHECK((int)v3_neq_7_first_half.sat_count() == 9);

  sylvan_idd v3_neq_7_second_half(v3_neq_7_first_half.get_right_node());
  v3_neq_7_second_half.get_interval(lb, ub);
  CHECK(encode_interval(8, 9) == v3_neq_7_second_half.get_encoded_interval());
  CHECK(lb == 8);
  CHECK(ub == 9);
  CHECK(v3_neq_7_second_half.get_down_node() == sylvan_idd::iddTrue());
  CHECK(v3_neq_7_second_half.get_right_node() == sylvan_idd::iddFalse());
  CHECK(v3_neq_7_second_half.node_count() == 1);
  CHECK((int)v3_neq_7_second_half.sat_count() == 2);
}

TEST_CASE("sylvan IDD, testing cube from assignments") {
  using namespace citcpp;
  using namespace citcpp::detail;

  sylvan_lifecycle sylvan_lifecycle;

  uint32_t lb = 0;
  uint32_t ub = 0;

  std::vector<int> assignments{7, 3, 5, -1, 10, -1};
  sylvan_idd assignment_idd(assignments);

  CHECK(4 == assignment_idd.get_variables().size());

  assignment_idd.get_interval(lb, ub);
  CHECK(encode_interval(7, 7) == assignment_idd.get_encoded_interval());
  CHECK(lb == 7);
  CHECK(ub == 7);
  CHECK(assignment_idd.get_right_node() == sylvan_idd::iddFalse());
  CHECK(assignment_idd.node_count() == 4);
  CHECK((int)assignment_idd.sat_count() == 1);

  assignment_idd = assignment_idd.get_down_node();
  CHECK(assignment_idd != sylvan_idd::iddFalse());
  CHECK(assignment_idd != sylvan_idd::iddTrue());
  assignment_idd.get_interval(lb, ub);
  CHECK(encode_interval(3, 3) == assignment_idd.get_encoded_interval());
  CHECK(lb == 3);
  CHECK(ub == 3);
  CHECK(assignment_idd.get_right_node() == sylvan_idd::iddFalse());
  CHECK(assignment_idd.node_count() == 3);
  CHECK((int)assignment_idd.sat_count() == 1);

  assignment_idd = assignment_idd.get_down_node();
  CHECK(assignment_idd != sylvan_idd::iddFalse());
  CHECK(assignment_idd != sylvan_idd::iddTrue());
  assignment_idd.get_interval(lb, ub);
  CHECK(encode_interval(5, 5) == assignment_idd.get_encoded_interval());
  CHECK(lb == 5);
  CHECK(ub == 5);
  CHECK(assignment_idd.get_right_node() == sylvan_idd::iddFalse());
  CHECK(assignment_idd.node_count() == 2);
  CHECK((int)assignment_idd.sat_count() == 1);

  assignment_idd = assignment_idd.get_down_node();
  CHECK(assignment_idd != sylvan_idd::iddFalse());
  CHECK(assignment_idd != sylvan_idd::iddTrue());
  assignment_idd.get_interval(lb, ub);
  CHECK(encode_interval(10, 10) == assignment_idd.get_encoded_interval());
  CHECK(lb == 10);
  CHECK(ub == 10);
  CHECK(assignment_idd.get_right_node() == sylvan_idd::iddFalse());
  CHECK(assignment_idd.node_count() == 1);
  CHECK((int)assignment_idd.sat_count() == 1);

  CHECK(assignment_idd.get_down_node() == sylvan_idd::iddTrue());
}

TEST_CASE("sylvan IDD, testing atomic prop & AND") {
  using namespace citcpp;
  using namespace citcpp::detail;

  sylvan_lifecycle sylvan_lifecycle;

  uint32_t lb = 0;
  uint32_t ub = 0;

  {
    sylvan_idd intersection = sylvan_idd::project_intersect(
        sylvan_idd(0, relational_operator::GT, 1, 5),
        sylvan_idd(1, relational_operator::NEQ, 1, 5));
    intersection = sylvan_idd::project_intersect(
        intersection, sylvan_idd(1, relational_operator::GE, 2, 5));

    CHECK(intersection != sylvan_idd::iddFalse());
    CHECK(intersection != sylvan_idd::iddTrue());
    intersection.get_interval(lb, ub);
    CHECK(encode_interval(2, 4) == intersection.get_encoded_interval());
    CHECK(lb == 2);
    CHECK(ub == 4);
    CHECK(intersection.get_right_node() == sylvan_idd::iddFalse());
    CHECK(intersection.node_count() == 2);
    CHECK((int)intersection.sat_count() == 9);

    CHECK(encode_interval(2, 4) ==
          intersection.get_down_node().get_encoded_interval());
    CHECK(intersection.get_down_node().node_count() == 1);
    CHECK((int)intersection.get_down_node().sat_count() == 3);
    CHECK(intersection.get_down_node().get_down_node() ==
          sylvan_idd::iddTrue());
    CHECK(intersection.get_down_node().get_right_node() ==
          sylvan_idd::iddFalse());

    CHECK(!intersection.is_sat_with_partial_assignment(std::vector<int>{2, 0}));
    CHECK(intersection.is_sat_with_partial_assignment(std::vector<int>{2, 2}));
    CHECK(intersection.is_sat_with_partial_assignment(std::vector<int>{4, -1}));
    CHECK(
        !intersection.is_sat_with_partial_assignment(std::vector<int>{0, -1}));
    CHECK(!intersection.is_sat_with_partial_assignment(std::vector<int>{3, 1}));

    std::vector<int> assignment{-1, -1};
    intersection.get_sat_one(assignment);
    CHECK(assignment[0] == 2);
    CHECK(assignment[1] == 2);

    std::fill(assignment.begin(), assignment.end(), -1);
    intersection.get_sat_one_under_partial_assignment(assignment);
    CHECK(assignment[0] == 2);
    CHECK(assignment[1] == 2);

    assignment[0] = -1;
    assignment[1] = 3;
    intersection.get_sat_one_under_partial_assignment(assignment);
    CHECK(assignment[0] == 2);
    CHECK(assignment[1] == 3);
  }

  {
    sylvan_idd intersection = sylvan_idd::project_intersect(
        sylvan_idd(0, relational_operator::GT, 1, 5),
        sylvan_idd(1, relational_operator::NEQ, 1, 5));
    intersection = sylvan_idd::project_intersect(
        intersection, sylvan_idd(1, relational_operator::LE, 3, 5));

    CHECK(intersection != sylvan_idd::iddFalse());
    CHECK(intersection != sylvan_idd::iddTrue());
    intersection.get_interval(lb, ub);
    CHECK(encode_interval(2, 4) == intersection.get_encoded_interval());
    CHECK(lb == 2);
    CHECK(ub == 4);
    CHECK(intersection.get_right_node() == sylvan_idd::iddFalse());
    CHECK(intersection.node_count() == 3);
    CHECK((int)intersection.sat_count() == 9);

    CHECK(encode_interval(0, 0) ==
          intersection.get_down_node().get_encoded_interval());
    CHECK(intersection.get_down_node().node_count() == 2);
    CHECK((int)intersection.get_down_node().sat_count() == 3);
    CHECK(intersection.get_down_node().get_down_node() ==
          sylvan_idd::iddTrue());

    CHECK(encode_interval(2, 3) ==
          intersection.get_down_node().get_right_node().get_encoded_interval());
    CHECK(intersection.get_down_node().get_right_node().node_count() == 1);
    CHECK((int)intersection.get_down_node().get_right_node().sat_count() == 2);
    CHECK(intersection.get_down_node().get_right_node().get_down_node() ==
          sylvan_idd::iddTrue());

    CHECK(intersection.is_sat_with_partial_assignment(std::vector<int>{2, 0}));
    CHECK(intersection.is_sat_with_partial_assignment(std::vector<int>{2, 3}));
    CHECK(!intersection.is_sat_with_partial_assignment(std::vector<int>{2, 4}));
    CHECK(intersection.is_sat_with_partial_assignment(std::vector<int>{4, -1}));
    CHECK(
        !intersection.is_sat_with_partial_assignment(std::vector<int>{0, -1}));
    CHECK(!intersection.is_sat_with_partial_assignment(std::vector<int>{3, 1}));

    std::vector<int> assignment{-1, -1};
    intersection.get_sat_one(assignment);
    CHECK(assignment[0] == 2);
    CHECK(assignment[1] == 0);

    std::fill(assignment.begin(), assignment.end(), -1);
    intersection.get_sat_one_under_partial_assignment(assignment);
    CHECK(assignment[0] == 2);
    CHECK(assignment[1] == 0);

    assignment[0] = -1;
    assignment[1] = 3;
    intersection.get_sat_one_under_partial_assignment(assignment);
    CHECK(assignment[0] == 2);
    CHECK(assignment[1] == 3);
  }
}

TEST_CASE("sylvan IDD, testing atomic prop & OR") {
  using namespace citcpp;
  using namespace citcpp::detail;

  sylvan_lifecycle sylvan_lifecycle;

  uint32_t lb = 0;
  uint32_t ub = 0;

  {
    sylvan_idd union_set = sylvan_idd::project_intersect(
        sylvan_idd(0, relational_operator::GT, 1, 5),
        sylvan_idd(1, relational_operator::NEQ, 1, 5));
    union_set = sylvan_idd::project_union(
        union_set,
        sylvan_idd::project_intersect(
            sylvan_idd(0, relational_operator::EQ, 2, 5),
            sylvan_idd(1, relational_operator::LE, 3, 5)),
        std::vector<unsigned int>{5, 5});

    CHECK(union_set != sylvan_idd::iddFalse());
    CHECK(union_set != sylvan_idd::iddTrue());
    union_set.get_interval(lb, ub);
    CHECK(encode_interval(2, 2) == union_set.get_encoded_interval());
    CHECK(lb == 2);
    CHECK(ub == 2);
    CHECK(union_set.node_count() == 5);
    CHECK((int)union_set.sat_count() == 13);

    CHECK(encode_interval(0, 4) ==
          union_set.get_down_node().get_encoded_interval());
    CHECK(union_set.get_down_node().node_count() == 1);
    CHECK((int)union_set.get_down_node().sat_count() == 5);
    CHECK(union_set.get_down_node().get_down_node() == sylvan_idd::iddTrue());

    CHECK(encode_interval(3, 4) ==
          union_set.get_right_node().get_encoded_interval());
    CHECK(union_set.get_right_node().node_count() == 3);
    CHECK((int)union_set.get_right_node().sat_count() == 8);
    CHECK(union_set.get_right_node().get_right_node() ==
          sylvan_idd::iddFalse());

    CHECK(encode_interval(0, 0) ==
          union_set.get_right_node().get_down_node().get_encoded_interval());
    CHECK(union_set.get_right_node().get_down_node().node_count() == 2);
    CHECK((int)union_set.get_right_node().get_down_node().sat_count() == 4);
    CHECK(union_set.get_right_node().get_down_node().get_down_node() ==
          sylvan_idd::iddTrue());

    CHECK(encode_interval(2, 4) == union_set.get_right_node()
                                       .get_down_node()
                                       .get_right_node()
                                       .get_encoded_interval());
    CHECK(union_set.get_right_node()
              .get_down_node()
              .get_right_node()
              .node_count() == 1);
    CHECK((int)union_set.get_right_node()
              .get_down_node()
              .get_right_node()
              .sat_count() == 3);
    CHECK(union_set.get_right_node()
              .get_down_node()
              .get_right_node()
              .get_down_node() == sylvan_idd::iddTrue());
    CHECK(union_set.get_right_node()
              .get_down_node()
              .get_right_node()
              .get_right_node() == sylvan_idd::iddFalse());
  }
}

TEST_CASE("sylvan IDD, mark_valid_value_combinations") {
  using namespace citcpp;
  using namespace citcpp::detail;

  sylvan_lifecycle sylvan_lifecycle;

  SUBCASE("Basic matching (all variables in cube are in IDD)") {
    // P0 != P1, domains 2, 2
    std::vector<unsigned int> domain_sizes{2, 2};

    // (P0=0 AND P1=1) OR (P0=1 AND P1=0)
    sylvan_idd c1 =
        sylvan_idd::project_intersect(sylvan_idd(0, 0), sylvan_idd(1, 1));
    sylvan_idd c2 =
        sylvan_idd::project_intersect(sylvan_idd(0, 1), sylvan_idd(1, 0));
    sylvan_idd constraint = sylvan_idd::project_union(c1, c2, domain_sizes);

    param_vector p_indices = {0, 1};
    coverage_map_second_level cov(4, p_indices);

    constraint.mark_valid_value_combinations(cov, domain_sizes);

    CHECK(cov.is_valid(1));   // (0, 1)
    CHECK(cov.is_valid(2));   // (1, 0)
    CHECK(!cov.is_valid(0));  // (0, 0)
    CHECK(!cov.is_valid(3));  // (1, 1)
  }

  SUBCASE("Don't care in IDD (cube variable missing from IDD path)") {
    // IDD only constrains P0=0. P1 is free.
    // Tuple of interest (P0, P1).
    std::vector<unsigned int> domain_sizes{2, 2};
    sylvan_idd constraint(0, 0);  // P0 = 0

    param_vector p_indices = {0, 1};
    coverage_map_second_level cov(4, p_indices);

    constraint.mark_valid_value_combinations(cov, domain_sizes);

    CHECK(cov.is_valid(0));   // (0, 0)
    CHECK(cov.is_valid(1));   // (0, 1)
    CHECK(!cov.is_valid(2));  // (1, 0)
    CHECK(!cov.is_valid(3));  // (1, 1)
  }

  SUBCASE("Don't care in cube (IDD variable missing from cube)") {
    // IDD constrains P0=0 and P1=0.
    // Tuple of interest (P0).
    std::vector<unsigned int> domain_sizes{2, 2};
    sylvan_idd constraint =
        sylvan_idd::project_intersect(sylvan_idd(0, 0), sylvan_idd(1, 0));

    param_vector p_indices = {0};
    coverage_map_second_level cov(2, p_indices);

    constraint.mark_valid_value_combinations(cov, domain_sizes);

    CHECK(cov.is_valid(0));   // P0=0 is valid
    CHECK(!cov.is_valid(1));  // P0=1 is invalid
  }

  SUBCASE("Unordered parameter indices in cube") {
    // P0 != P1, domains 2, 2
    std::vector<unsigned int> domain_sizes{2, 2};

    // (P0=0 AND P1=1) OR (P0=1 AND P1=0)
    sylvan_idd c1 =
        sylvan_idd::project_intersect(sylvan_idd(0, 0), sylvan_idd(1, 1));
    sylvan_idd c2 =
        sylvan_idd::project_intersect(sylvan_idd(0, 1), sylvan_idd(1, 0));
    sylvan_idd constraint = sylvan_idd::project_union(c1, c2, domain_sizes);

    // Tuple (P1, P0)
    param_vector p_indices = {1, 0};
    coverage_map_second_level cov(4, p_indices);

    constraint.mark_valid_value_combinations(cov, domain_sizes);

    // Valid combinations: P1=1, P0=0 -> index 1*2 + 0 = 2
    //                    P1=0, P0=1 -> index 0*2 + 1 = 1
    CHECK(cov.is_valid(2));
    CHECK(cov.is_valid(1));
    CHECK(!cov.is_valid(0));
    CHECK(!cov.is_valid(3));
  }

  SUBCASE("Empty IDD (False)") {
    std::vector<unsigned int> domain_sizes{2, 2};
    sylvan_idd constraint = sylvan_idd::iddFalse();

    param_vector p_indices = {0, 1};
    coverage_map_second_level cov(4, p_indices);

    constraint.mark_valid_value_combinations(cov, domain_sizes);

    CHECK(!cov.is_valid(0));
    CHECK(!cov.is_valid(1));
    CHECK(!cov.is_valid(2));
    CHECK(!cov.is_valid(3));
  }

  SUBCASE("IDD True") {
    std::vector<unsigned int> domain_sizes{2, 2};
    sylvan_idd constraint = sylvan_idd::iddTrue();

    param_vector p_indices = {0, 1};
    coverage_map_second_level cov(4, p_indices);

    constraint.mark_valid_value_combinations(cov, domain_sizes);

    CHECK(cov.is_valid(0));
    CHECK(cov.is_valid(1));
    CHECK(cov.is_valid(2));
    CHECK(cov.is_valid(3));
  }

  SUBCASE("Larger domains and mixed constraints") {
    // P0 in [0, 2], P1 in [1, 3], P2 free. Domains 5, 5, 5
    std::vector<unsigned int> domain_sizes{5, 5, 5};
    sylvan_idd c0(0, relational_operator::LE, 2, 5);
    sylvan_idd c1 = sylvan_idd::project_intersect(
        sylvan_idd(1, relational_operator::GE, 1, 5),
        sylvan_idd(1, relational_operator::LE, 3, 5));
    sylvan_idd constraint = sylvan_idd::project_intersect(c0, c1);

    param_vector p_indices = {0, 1, 2};
    coverage_map_second_level cov(125, p_indices);

    constraint.mark_valid_value_combinations(cov, domain_sizes);

    // Check some points
    // (0, 1, 0) -> 0*25 + 1*5 + 0 = 5. Should be valid.
    CHECK(cov.is_valid(5));
    // (2, 3, 4) -> 2*25 + 3*5 + 4 = 50 + 15 + 4 = 69. Should be valid.
    CHECK(cov.is_valid(69));
    // (3, 1, 0) -> 3*25 + 1*5 + 0 = 80. Should be invalid (P0=3).
    CHECK(!cov.is_valid(80));
    // (0, 0, 0) -> 0. Should be invalid (P1=0).
    CHECK(!cov.is_valid(0));
    // (0, 4, 0) -> 20. Should be invalid (P1=4).
    CHECK(!cov.is_valid(20));
  }

  SUBCASE("Skipped variables in IDD path") {
    // IDD: P0=0, P2=0. P1 is free.
    // Cube: (P0, P1, P2)
    std::vector<unsigned int> domain_sizes{2, 2, 2};
    sylvan_idd constraint =
        sylvan_idd::project_intersect(sylvan_idd(0, 0), sylvan_idd(2, 0));

    param_vector p_indices = {0, 1, 2};
    coverage_map_second_level cov(8, p_indices);

    constraint.mark_valid_value_combinations(cov, domain_sizes);

    // (0, 0, 0) -> 0. Valid.
    CHECK(cov.is_valid(0));
    // (0, 1, 0) -> 0*4 + 1*2 + 0 = 2. Valid.
    CHECK(cov.is_valid(2));
    // (0, 0, 1) -> 1. Invalid.
    CHECK(!cov.is_valid(1));
    // (1, 0, 0) -> 4. Invalid.
    CHECK(!cov.is_valid(4));
  }

  SUBCASE("IDD variables interleaved with cube variables") {
    // IDD: P0=0, P2=0, P4=0. P1, P3 are free.
    // Cube: (P1, P3, P4)
    std::vector<unsigned int> domain_sizes{2, 2, 2, 2, 2};
    sylvan_idd constraint = sylvan_idd::project_intersect(
        sylvan_idd::project_intersect(sylvan_idd(0, 0), sylvan_idd(2, 0)),
        sylvan_idd(4, 0));

    param_vector p_indices = {1, 3, 4};
    coverage_map_second_level cov(8, p_indices);

    constraint.mark_valid_value_combinations(cov, domain_sizes);

    // Any value of P1, P3 is valid as long as P4=0.
    // (P1=0, P3=0, P4=0) -> 0. Valid.
    CHECK(cov.is_valid(0));
    // (P1=1, P3=1, P4=0) -> 1*4 + 1*2 + 0 = 6. Valid.
    CHECK(cov.is_valid(6));
    // (P1=0, P3=0, P4=1) -> 1. Invalid.
    CHECK(!cov.is_valid(1));
  }
}

}  // namespace
