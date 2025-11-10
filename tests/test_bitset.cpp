#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest.h>

#include "../citcpp_lib/src/detail/bitset.hpp"

TEST_CASE("bitset, testing API, 70 bits, std::uint64_t storage type") {
  using namespace citcpp::detail;

  bitset<std::uint64_t> sut(70);

  SUBCASE("Test size()") { CHECK(sut.size() == 70); }

  SUBCASE("Test API at 0") {
    CHECK(!sut.all());
    CHECK(!sut.any());
    CHECK(sut.none());
    CHECK(sut.count() == 0);

    CHECK(!sut.test(0));
    CHECK(!sut.test_and_set(0));
    CHECK(sut.test_and_set(0));
    CHECK(sut.test(0));

    CHECK(!sut.all());
    CHECK(sut.any());
    CHECK(!sut.none());
    CHECK(sut.count() == 1);

    CHECK(sut.test_and_reset(0));
    CHECK(!sut.test_and_reset(0));
    CHECK(!sut.test(0));

    CHECK(!sut.all());
    CHECK(!sut.any());
    CHECK(sut.none());
    CHECK(sut.count() == 0);

    sut.set(0);
    CHECK(sut.test(0));

    CHECK(!sut.all());
    CHECK(sut.any());
    CHECK(!sut.none());
    CHECK(sut.count() == 1);

    sut.reset(0);
    CHECK(!sut.test(0));

    CHECK(!sut.all());
    CHECK(!sut.any());
    CHECK(sut.none());
    CHECK(sut.count() == 0);
  }

  SUBCASE("Test API at 30") {
    CHECK(!sut.all());
    CHECK(!sut.any());
    CHECK(sut.none());
    CHECK(sut.count() == 0);

    CHECK(!sut.test(30));
    CHECK(!sut.test_and_set(30));
    CHECK(sut.test_and_set(30));
    CHECK(sut.test(30));

    CHECK(!sut.all());
    CHECK(sut.any());
    CHECK(!sut.none());
    CHECK(sut.count() == 1);

    CHECK(sut.test_and_reset(30));
    CHECK(!sut.test_and_reset(30));
    CHECK(!sut.test(30));

    CHECK(!sut.all());
    CHECK(!sut.any());
    CHECK(sut.none());
    CHECK(sut.count() == 0);

    sut.set(30);
    CHECK(sut.test(30));

    CHECK(!sut.all());
    CHECK(sut.any());
    CHECK(!sut.none());
    CHECK(sut.count() == 1);

    sut.reset(30);
    CHECK(!sut.test(30));

    CHECK(!sut.all());
    CHECK(!sut.any());
    CHECK(sut.none());
    CHECK(sut.count() == 0);
  }

  SUBCASE("Test API at 63") {
    CHECK(!sut.all());
    CHECK(!sut.any());
    CHECK(sut.none());
    CHECK(sut.count() == 0);

    CHECK(!sut.test(63));
    CHECK(!sut.test_and_set(63));
    CHECK(sut.test_and_set(63));
    CHECK(sut.test(63));

    CHECK(!sut.all());
    CHECK(sut.any());
    CHECK(!sut.none());
    CHECK(sut.count() == 1);

    CHECK(sut.test_and_reset(63));
    CHECK(!sut.test_and_reset(63));
    CHECK(!sut.test(63));

    CHECK(!sut.all());
    CHECK(!sut.any());
    CHECK(sut.none());
    CHECK(sut.count() == 0);

    sut.set(63);
    CHECK(sut.test(63));

    CHECK(!sut.all());
    CHECK(sut.any());
    CHECK(!sut.none());
    CHECK(sut.count() == 1);

    sut.reset(63);
    CHECK(!sut.test(63));

    CHECK(!sut.all());
    CHECK(!sut.any());
    CHECK(sut.none());
    CHECK(sut.count() == 0);
  }

  SUBCASE("Test API at 64") {
    CHECK(!sut.all());
    CHECK(!sut.any());
    CHECK(sut.none());
    CHECK(sut.count() == 0);

    CHECK(!sut.test(64));
    CHECK(!sut.test_and_set(64));
    CHECK(sut.test_and_set(64));
    CHECK(sut.test(64));

    CHECK(!sut.all());
    CHECK(sut.any());
    CHECK(!sut.none());
    CHECK(sut.count() == 1);

    CHECK(sut.test_and_reset(64));
    CHECK(!sut.test_and_reset(64));
    CHECK(!sut.test(64));

    CHECK(!sut.all());
    CHECK(!sut.any());
    CHECK(sut.none());
    CHECK(sut.count() == 0);

    sut.set(64);
    CHECK(sut.test(64));

    CHECK(!sut.all());
    CHECK(sut.any());
    CHECK(!sut.none());
    CHECK(sut.count() == 1);

    sut.reset(64);
    CHECK(!sut.test(64));

    CHECK(!sut.all());
    CHECK(!sut.any());
    CHECK(sut.none());
    CHECK(sut.count() == 0);
  }

  SUBCASE("Test API at 65") {
    CHECK(!sut.all());
    CHECK(!sut.any());
    CHECK(sut.none());
    CHECK(sut.count() == 0);

    CHECK(!sut.test(65));
    CHECK(!sut.test_and_set(65));
    CHECK(sut.test_and_set(65));
    CHECK(sut.test(65));

    CHECK(!sut.all());
    CHECK(sut.any());
    CHECK(!sut.none());
    CHECK(sut.count() == 1);

    CHECK(sut.test_and_reset(65));
    CHECK(!sut.test_and_reset(65));
    CHECK(!sut.test(65));

    CHECK(!sut.all());
    CHECK(!sut.any());
    CHECK(sut.none());
    CHECK(sut.count() == 0);

    sut.set(65);
    CHECK(sut.test(65));

    CHECK(!sut.all());
    CHECK(sut.any());
    CHECK(!sut.none());
    CHECK(sut.count() == 1);

    sut.reset(65);
    CHECK(!sut.test(65));

    CHECK(!sut.all());
    CHECK(!sut.any());
    CHECK(sut.none());
    CHECK(sut.count() == 0);
  }

  SUBCASE("Test API at 69") {
    CHECK(!sut.all());
    CHECK(!sut.any());
    CHECK(sut.none());
    CHECK(sut.count() == 0);

    CHECK(!sut.test(69));
    CHECK(!sut.test_and_set(69));
    CHECK(sut.test_and_set(69));
    CHECK(sut.test(69));

    CHECK(!sut.all());
    CHECK(sut.any());
    CHECK(!sut.none());
    CHECK(sut.count() == 1);

    CHECK(sut.test_and_reset(69));
    CHECK(!sut.test_and_reset(69));
    CHECK(!sut.test(69));

    CHECK(!sut.all());
    CHECK(!sut.any());
    CHECK(sut.none());
    CHECK(sut.count() == 0);

    sut.set(69);
    CHECK(sut.test(69));

    CHECK(!sut.all());
    CHECK(sut.any());
    CHECK(!sut.none());
    CHECK(sut.count() == 1);

    sut.reset(69);
    CHECK(!sut.test(69));

    CHECK(!sut.all());
    CHECK(!sut.any());
    CHECK(sut.none());
    CHECK(sut.count() == 0);
  }

  SUBCASE("Test API at multiples indices") {
    CHECK(!sut.all());
    CHECK(!sut.any());
    CHECK(sut.none());
    CHECK(sut.count() == 0);

    CHECK(!sut.test(0));
    CHECK(!sut.test_and_set(0));
    CHECK(sut.test_and_set(0));
    CHECK(sut.test(0));

    CHECK(!sut.test(10));
    CHECK(!sut.test_and_set(10));
    CHECK(sut.test_and_set(10));
    CHECK(sut.test(10));

    CHECK(!sut.test(20));
    CHECK(!sut.test_and_set(20));
    CHECK(sut.test_and_set(20));
    CHECK(sut.test(20));

    CHECK(!sut.test(69));
    CHECK(!sut.test_and_set(69));
    CHECK(sut.test_and_set(69));
    CHECK(sut.test(69));

    CHECK(!sut.all());
    CHECK(sut.any());
    CHECK(!sut.none());
    CHECK(sut.count() == 4);

    CHECK(sut.test_and_reset(69));
    CHECK(!sut.test_and_reset(69));
    CHECK(!sut.test(69));

    CHECK(!sut.all());
    CHECK(sut.any());
    CHECK(!sut.none());
    CHECK(sut.count() == 3);

    sut.reset(30);
    sut.reset(50);

    CHECK(!sut.all());
    CHECK(sut.any());
    CHECK(!sut.none());
    CHECK(sut.count() == 3);
  }

  SUBCASE("Test all()") {
    CHECK(!sut.all());
    CHECK(!sut.any());
    CHECK(sut.none());
    CHECK(sut.count() == 0);

    for (int i = 0; i < 70; ++i) {
      sut.set(i);
    }

    CHECK(sut.all());
    CHECK(sut.any());
    CHECK(!sut.none());
    CHECK(sut.count() == 70);
  }

  SUBCASE("Test set() and reset()") {
    CHECK(!sut.all());
    CHECK(!sut.any());
    CHECK(sut.none());
    CHECK(sut.count() == 0);

    sut.set();

    CHECK(sut.all());
    CHECK(sut.any());
    CHECK(!sut.none());
    CHECK(sut.count() == 70);

    sut.reset();

    CHECK(!sut.all());
    CHECK(!sut.any());
    CHECK(sut.none());
    CHECK(sut.count() == 0);
  }
}

TEST_CASE("bitset, testing copying, moving, assigning") {
  using namespace citcpp::detail;

  bitset<std::uint64_t> sut(70);

  SUBCASE("Test copy constructor") {
    using namespace citcpp::detail;

    CHECK(!sut.all());
    CHECK(!sut.any());
    CHECK(sut.none());
    CHECK(sut.count() == 0);

    sut.set(1);
    sut.set(2);
    sut.set(3);
    sut.set(4);

    CHECK(!sut.all());
    CHECK(sut.any());
    CHECK(!sut.none());
    CHECK(sut.count() == 4);

    bitset<std::uint64_t> other(sut);

    CHECK(!other.all());
    CHECK(other.any());
    CHECK(!other.none());
    CHECK(other.count() == 4);

    CHECK(other.test(1));
    CHECK(other.test(2));
    CHECK(other.test(3));
    CHECK(other.test(4));

    CHECK(!sut.all());
    CHECK(sut.any());
    CHECK(!sut.none());
    CHECK(sut.count() == 4);
  }

  SUBCASE("Test move constructor") {
    using namespace citcpp::detail;

    CHECK(!sut.all());
    CHECK(!sut.any());
    CHECK(sut.none());
    CHECK(sut.count() == 0);

    sut.set(1);
    sut.set(2);
    sut.set(3);
    sut.set(4);

    CHECK(!sut.all());
    CHECK(sut.any());
    CHECK(!sut.none());
    CHECK(sut.count() == 4);

    bitset<std::uint64_t> other(std::move(sut));

    CHECK(!other.all());
    CHECK(other.any());
    CHECK(!other.none());
    CHECK(other.count() == 4);

    CHECK(other.test(1));
    CHECK(other.test(2));
    CHECK(other.test(3));
    CHECK(other.test(4));
  }

  SUBCASE("Test copy assignment") {
    using namespace citcpp::detail;

    CHECK(!sut.all());
    CHECK(!sut.any());
    CHECK(sut.none());
    CHECK(sut.count() == 0);

    sut.set(1);
    sut.set(2);
    sut.set(3);
    sut.set(4);

    CHECK(!sut.all());
    CHECK(sut.any());
    CHECK(!sut.none());
    CHECK(sut.count() == 4);

    bitset<std::uint64_t> other;
    other = sut;

    CHECK(!other.all());
    CHECK(other.any());
    CHECK(!other.none());
    CHECK(other.count() == 4);

    CHECK(other.test(1));
    CHECK(other.test(2));
    CHECK(other.test(3));
    CHECK(other.test(4));

    CHECK(!sut.all());
    CHECK(sut.any());
    CHECK(!sut.none());
    CHECK(sut.count() == 4);
  }

  SUBCASE("Test move assignment") {
    using namespace citcpp::detail;

    CHECK(!sut.all());
    CHECK(!sut.any());
    CHECK(sut.none());
    CHECK(sut.count() == 0);

    sut.set(1);
    sut.set(2);
    sut.set(3);
    sut.set(4);

    CHECK(!sut.all());
    CHECK(sut.any());
    CHECK(!sut.none());
    CHECK(sut.count() == 4);

    bitset<std::uint64_t> other;
    other = std::move(sut);

    CHECK(!other.all());
    CHECK(other.any());
    CHECK(!other.none());
    CHECK(other.count() == 4);

    CHECK(other.test(1));
    CHECK(other.test(2));
    CHECK(other.test(3));
    CHECK(other.test(4));
  }

  SUBCASE("Test swap()") {
    using namespace citcpp::detail;

    CHECK(!sut.all());
    CHECK(!sut.any());
    CHECK(sut.none());
    CHECK(sut.count() == 0);

    sut.set(1);
    sut.set(2);
    sut.set(3);
    sut.set(4);

    CHECK(!sut.all());
    CHECK(sut.any());
    CHECK(!sut.none());
    CHECK(sut.count() == 4);

    bitset<std::uint64_t> other;
    other.swap(sut);

    CHECK(!other.all());
    CHECK(other.any());
    CHECK(!other.none());
    CHECK(other.count() == 4);

    CHECK(other.test(1));
    CHECK(other.test(2));
    CHECK(other.test(3));
    CHECK(other.test(4));

    CHECK(sut.all());
    CHECK(!sut.any());
    CHECK(sut.none());
    CHECK(sut.count() == 0);
  }
}
