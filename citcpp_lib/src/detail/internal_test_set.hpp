#ifndef DETAIL_INTERNAL_TESTSET_HPP_
#define DETAIL_INTERNAL_TESTSET_HPP_

#include <list>
#include <vector>

#include "list_intrusive.hpp"

namespace citcpp {
namespace detail {

class test : public sl_list_node_intrusive {
    typedef sl_list_node_intrusive base_type;
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
    test() noexcept : base_type(), values_(), num_dont_care_values_(0) {}

    explicit test(size_type count)
        : base_type(), values_(count), num_dont_care_values_(0) {}

    test(size_type count, const int& value)
        : base_type(), values_(count, value), num_dont_care_values_(0) {}

    template <class InputIt>
    test(InputIt first, InputIt last)
        : base_type(), values_(first, last), num_dont_care_values_(0) {}

    test(const test& other)
        : base_type(other),
          values_(other.values_),
          num_dont_care_values_(other.num_dont_care_values_) {}

    test(test&& other)
        : base_type(std::move(other)),
          values_(std::move(other.values_)),
          num_dont_care_values_(other.num_dont_care_values_) {}

    test(std::initializer_list<int> init)
        : base_type(), values_(std::move(init)), num_dont_care_values_(0) {}

    test& operator=(const test& other) {
      base_type::operator=(other);
      values_ = other.values_;
      num_dont_care_values_ = other.num_dont_care_values_;

      return *this;
    }

    test& operator=(test&& other) noexcept {
      base_type::operator=(std::move(other));
      values_ = std::move(other.values_);
      num_dont_care_values_ = other.num_dont_care_values_;

      return *this;
    }

    test& operator=(std::initializer_list<value_type> ilist) {
      values_ = ilist;

      return *this;
    }

    values_list_type& get_values() { return values_; }

    const values_list_type& get_values() const { return values_; }

    unsigned int get_num_dont_care_values() const {
      return num_dont_care_values_;
    }

    void set_num_dont_care_values(unsigned int num_dont_care_values) {
      num_dont_care_values_ = num_dont_care_values;
    }

  private:
    values_list_type values_;
    unsigned int num_dont_care_values_;
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
