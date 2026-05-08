#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#include <algorithm>
#include <cstdint>
#include <mutex>
#include <vector>
#include "../citcpp_lib/src/detail/citcpp_sylvan_ldd.hpp"

namespace {

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
  std::lock_guard<std::mutex> lock(get_global_sylvan_init_mutex());
  int& instance_cnt = get_lobal_sylan_init_counter();
  if (instance_cnt == 0) {
    sylvan::init_lace(num_workers, 0);
    sylvan::init_package(1LL << 20, 1LL << 23, 1LL << 20, 1LL << 23);
    sylvan::init_ldd();
  }
  instance_cnt++;
}

void maybe_quit_sylvan() {
  using namespace citcpp::detail;
  std::lock_guard<std::mutex> lock(get_global_sylvan_init_mutex());
  int& instance_cnt = get_lobal_sylan_init_counter();
  instance_cnt--;
  if (instance_cnt == 0) {
    sylvan::quit_package();
    sylvan::quit_lace();
  }
}

} // namespace

TEST_CASE("IDD Projection") {
  using namespace citcpp::detail;
  maybe_initialize_sylvan(0);

  SUBCASE("Identity Projection") {
    std::vector<int> assignments = {1, 2, -1};
    sylvan_idd idd(assignments);
    
    sylvan_idd projected = idd.project({0, 1});
    CHECK(projected == idd);
    CHECK(projected.get_variables() == std::vector<uint32_t>{0, 1});
  }

  SUBCASE("Empty Projection") {
    std::vector<int> assignments = {1, 2};
    sylvan_idd idd(assignments);
    
    sylvan_idd projected = idd.project({});
    CHECK(projected == sylvan_idd::iddTrue());
    CHECK(projected.get_variables().empty());
  }

  SUBCASE("Simple Cube Projection") {
    // X=1, Y=2 (X is var 0, Y is var 1)
    std::vector<int> assignments = {1, 2};
    sylvan_idd idd(assignments);

    // Project onto X
    sylvan_idd proj_x = idd.project({0});
    CHECK(proj_x.get_variables() == std::vector<uint32_t>{0});
    std::vector<int> sat_x(1, -1);
    proj_x.get_sat_one(sat_x);
    CHECK(sat_x[0] == 1);
    CHECK(proj_x.sat_count() == 1.0);

    // Project onto Y
    sylvan_idd proj_y = idd.project({1});
    CHECK(proj_y.get_variables() == std::vector<uint32_t>{1});
    std::vector<int> sat_y(2, -1); // size 2 because var index is 1
    proj_y.get_sat_one(sat_y);
    CHECK(sat_y[1] == 2);
    CHECK(proj_y.sat_count() == 1.0);
  }

  SUBCASE("Disjunction Projection") {
    // (X=1, Y=2) OR (X=1, Y=3)
    std::vector<int> a1 = {1, 2};
    std::vector<int> a2 = {1, 3};
    sylvan_idd idd1(a1);
    sylvan_idd idd2(a2);
    sylvan_idd idd = sylvan_idd::project_union(idd1, idd2, {10, 10});

    // Project onto X
    sylvan_idd proj_x = idd.project({0});
    CHECK(proj_x.get_variables() == std::vector<uint32_t>{0});
    CHECK(proj_x.sat_count() == 1.0); // Both cubes lead to X=1
    
    std::vector<int> sat_x(1, -1);
    proj_x.get_sat_one(sat_x);
    CHECK(sat_x[0] == 1);

    // Project onto Y
    sylvan_idd proj_y = idd.project({1});
    CHECK(proj_y.get_variables() == std::vector<uint32_t>{1});
    CHECK(proj_y.sat_count() == 2.0); // Y=2 and Y=3 are valid
  }

  SUBCASE("Interval Merging during Projection") {
    // (X=1, Y=1) OR (X=1, Y=2)
    std::vector<int> a1 = {1, 1};
    std::vector<int> a2 = {1, 2};
    sylvan_idd idd1(a1);
    sylvan_idd idd2(a2);
    sylvan_idd idd = sylvan_idd::project_union(idd1, idd2, {10, 10});

    // Project onto Y
    sylvan_idd proj_y = idd.project({1});
    CHECK(proj_y.get_variables() == std::vector<uint32_t>{1});
    CHECK(proj_y.sat_count() == 2.0);
    
    // Verify it's a single interval [1, 2]
    uint32_t lb, ub;
    proj_y.get_interval(lb, ub);
    CHECK(lb == 1);
    CHECK(ub == 2);
    CHECK(proj_y.get_right_node() == sylvan_idd::iddFalse());
  }

  maybe_quit_sylvan();
}
