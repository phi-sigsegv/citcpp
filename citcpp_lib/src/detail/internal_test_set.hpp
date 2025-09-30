#ifndef DETAIL_INTERNAL_TESTSET_HPP_
#define DETAIL_INTERNAL_TESTSET_HPP_

#include <list>
#include <vector>

#include "list_intrusive.hpp"

namespace citcpp {
namespace detail {

class test;

class test_list_intrusive_integ : public sl_list_node_intrusive {
    typedef sl_list_node_intrusive base_type;

  public:
    test_list_intrusive_integ(test* test) : base_type(), test_(test) {}

    test_list_intrusive_integ(const test_list_intrusive_integ& other)
        : base_type(other), test_(other.test_) {}

    test_list_intrusive_integ(test_list_intrusive_integ&& other)
        : base_type(std::move(other)), test_(other.test_) {}

    test_list_intrusive_integ& operator=(
        const test_list_intrusive_integ& other) {
      base_type::operator=(other);
      test_ = other.test_;

      return *this;
    }

    test_list_intrusive_integ& operator=(
        test_list_intrusive_integ&& other) noexcept {
      base_type::operator=(std::move(other));
      test_ = other.test_;

      return *this;
    }

    bool is_linked_up() const {
      return prev_node_ != nullptr || next_node_ != nullptr;
    }

    test& get_test() { return *test_; }

    const test& get_test() const { return *test_; }

  private:
    test* test_;
};

class test {
    typedef std::vector<int> values_list_type;

  public:
    typedef typename values_list_type::value_type value_type;
    typedef typename values_list_type::allocator_type allocator_type;
    typedef typename values_list_type::size_type size_type;
    typedef typename values_list_type::difference_type difference_type;
    typedef typename values_list_type::reference reference;
    typedef typename values_list_type::const_reference const_reference;
    typedef typename values_list_type::pointer pointer;
    typedef typename values_list_type::const_pointer const_pointer;
    typedef typename values_list_type::iterator iterator;
    typedef typename values_list_type::const_iterator const_iterator;
    typedef typename values_list_type::reverse_iterator reverse_iterator;
    typedef typename values_list_type::const_reverse_iterator
        const_reverse_iterator;

  public:
    test() noexcept
        : values_(),
          value_partition_il_node_(this),
          vertical_ext_il_node_(this) {}

    explicit test(size_type count)
        : values_(count),
          value_partition_il_node_(this),
          vertical_ext_il_node_(this) {}

    test(size_type count, const int& value)
        : values_(count, value),
          value_partition_il_node_(this),
          vertical_ext_il_node_(this) {}

    template <class InputIt>
    test(InputIt first, InputIt last)
        : values_(first, last),
          value_partition_il_node_(this),
          vertical_ext_il_node_(this) {}

    test(const test& other)
        : values_(other.values_),
          value_partition_il_node_(this),
          vertical_ext_il_node_(this) {}

    test(test&& other)
        : values_(std::move(other.values_)),
          value_partition_il_node_(this),
          vertical_ext_il_node_(this) {}

    test(std::initializer_list<int> init)
        : values_(std::move(init)),
          value_partition_il_node_(this),
          vertical_ext_il_node_(this) {}

    test& operator=(const test& other) {
      values_ = other.values_;

      return *this;
    }

    test& operator=(test&& other) noexcept {
      values_ = std::move(other.values_);

      return *this;
    }

    test& operator=(std::initializer_list<value_type> ilist) {
      values_ = ilist;

      return *this;
    }

    values_list_type& get_values() { return values_; }

    const values_list_type& get_values() const { return values_; }

    test_list_intrusive_integ& get_value_partition_intrusive_list_node() {
      return value_partition_il_node_;
    }

    test_list_intrusive_integ& get_vertical_extension_intrusive_list_node() {
      return vertical_ext_il_node_;
    }

  private:
    values_list_type values_;
    test_list_intrusive_integ value_partition_il_node_;
    test_list_intrusive_integ vertical_ext_il_node_;
};

/**
 * This class represents a produced test set.
 */
class internal_test_set {
  public:
    const std::list<test>& get_list_of_tests() const { return testset_; }

    std::list<test>& get_list_of_tests() { return testset_; }

  private:
    std::list<test> testset_;
};

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_INTERNAL_TESTSET_HPP_ */
