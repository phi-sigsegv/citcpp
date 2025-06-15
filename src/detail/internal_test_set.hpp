#ifndef DETAIL_INTERNAL_TESTSET_HPP_
#define DETAIL_INTERNAL_TESTSET_HPP_

#include <vector>
#include <list>
#include "list_intrusive.hpp"

namespace citcpp
{
  namespace detail
  {
    class test : sl_list_node_intrusive
    {
      typedef sl_list_node_intrusive base_type;
      typedef std::vector<int> vector_type;

    public:
      typedef typename vector_type::value_type value_type;
      typedef typename vector_type::allocator_type allocator_type;
      typedef typename vector_type::size_type size_type;
      typedef typename vector_type::difference_type difference_type;
      typedef typename vector_type::reference reference;
      typedef typename vector_type::const_reference const_reference;
      typedef typename vector_type::pointer pointer;
      typedef typename vector_type::const_pointer const_pointer;
      typedef typename vector_type::iterator iterator;
      typedef typename vector_type::const_iterator const_iterator;
      typedef typename vector_type::reverse_iterator reverse_iterator;
      typedef typename vector_type::const_reverse_iterator const_reverse_iterator;

    public:
      test () noexcept :
	  base_type (), values_ ()
      {
      }

      explicit
      test (size_type count) :
	  base_type (), values_ (count)
      {
      }

      test (size_type count, const int &value) :
	  base_type (), values_ (count, value)
      {
      }

      template<class InputIt>
	test (InputIt first, InputIt last) :
	    base_type (), values_ (first, last)
	{
	}

      test (const test &other) :
	  base_type (other), values_ (other.values_)
      {
      }

      test (test &&other) :
	  base_type (std::move (other)), values_ (std::move (other.values_))
      {
      }

      test (std::initializer_list<int> init) :
	  base_type (), values_ (std::move (init))
      {
      }

      test&
      operator= (const test &other)
      {
	base_type::operator= (other);
	values_ = other.values_;

	return *this;
      }

      test&
      operator= (test &&other) noexcept
      {
	base_type::operator= (std::move (other));
	values_ = std::move (other.values_);

	return *this;
      }

      test&
      operator= (std::initializer_list<value_type> ilist)
      {
	values_ = ilist;

	return *this;
      }

      std::vector<int>&
      get_values ()
      {
	return values_;
      }

      const std::vector<int>&
      get_values () const
      {
	return values_;
      }

    private:
      std::vector<int> values_;
    };

    /**
     * This class represents a produced test set.
     */
    class test_set
    {
    public:
      const std::list<test>&
      get_list_of_tests () const
      {
	return testset_;
      }

      std::list<test>&
      get_list_of_tests ()
      {
	return testset_;
      }

    private:
      std::list<test> testset_;
    };
  }
}

#endif /* DETAIL_INTERNAL_TESTSET_HPP_ */
