#ifndef DETAIL_LIST_INTRUSIVE_HPP_
#define DETAIL_LIST_INTRUSIVE_HPP_

#include <type_traits>
#include <iterator>
#include <algorithm>

namespace citcpp
{
  namespace detail
  {
    struct sl_list_node_intrusive
    {
      sl_list_node_intrusive *next_node_;
    };

    /**
     * This is an intrusive list. An intrusive list is a list,
     * where the value types of the list have the necessary pointers
     * directly embedded in their type. This allows to construct a
     * list of values of that type without having to allocate
     * any extra memory for that list.
     */
    template<typename T_VALUE>
      class list_intrusive
      {
      private:
	typedef sl_list_node_intrusive node_type;
	typedef list_intrusive<T_VALUE> this_type;

      public:
	template<bool is_const = false>
	  class list_intrusive_iterator
	  {
	  private:
	    // This friend declaration is necessary in order to be able
	    // to convert a non-const iterator into a const iterator,
	    // while keeping members private.
	    friend class list_intrusive_iterator<true> ;

	    typedef list_intrusive<T_VALUE> list_intrusive_type;
	    typedef typename std::conditional<is_const,
		const typename list_intrusive::node_type,
		typename list_intrusive::node_type>::type node_base_type;
	    node_base_type *node_;

	  public:
	    typedef T_VALUE value_type;
	    typedef std::ptrdiff_t difference_type;
	    typedef typename std::conditional<is_const, const T_VALUE*, T_VALUE*>::type pointer;
	    typedef typename std::conditional<is_const, const T_VALUE&, T_VALUE&>::type reference;
	    typedef std::forward_iterator_tag iterator_category;

	    list_intrusive_iterator () :
		node_ (0)
	    {
	    }

	    explicit
	    list_intrusive_iterator (node_base_type *node) :
		node_ (node)
	    {
	    }

	    list_intrusive_iterator (
		const list_intrusive_iterator<false> &other) :
		node_ (other.node_)
	    {
	    }

	    bool
	    valid () const
	    {
	      return (!!node_);
	    }

	    reference
	    value () const
	    {
	      return *static_cast<pointer> (node_);
	    }
	    pointer
	    value_ptr () const
	    {
	      return static_cast<pointer> (node_);
	    }

	    reference
	    operator* () const
	    {
	      return *static_cast<pointer> (node_);
	    }

	    pointer
	    operator-> () const
	    {
	      return static_cast<pointer> (node_);
	    }

	    // This is the overload of the prefix increment
	    // operator.
	    list_intrusive_iterator&
	    operator++ ()
	    {
	      node_ = node_->next_node_;
	      return *this;
	    }

	    // This is the overload of the postfix increment
	    // operator.
	    list_intrusive_iterator
	    operator++ (int)
	    {
	      list_intrusive_iterator tmp (*this);
	      node_ = node_->next_node_;
	      return tmp;
	    }

	    friend bool
	    operator== (const list_intrusive_iterator &lhs,
			const list_intrusive_iterator &rhs)
	    {
	      return lhs.node_ == rhs.node_;
	    }

	    friend bool
	    operator!= (const list_intrusive_iterator &lhs,
			const list_intrusive_iterator &rhs)
	    {
	      return lhs.node_ != rhs.node_;
	    }
	  };

	typedef T_VALUE value_type;
	typedef value_type &reference;
	typedef const value_type &const_reference;
	typedef value_type *pointer;
	typedef const value_type *const_pointer;
	typedef std::ptrdiff_t difference_type;
	typedef list_intrusive_iterator<false> iterator;
	typedef list_intrusive_iterator<true> const_iterator;
	typedef unsigned int size_type;

	list_intrusive () :
	    dummy_ (), p_tail_ (&dummy_), size_ (0)
	{
	  dummy_.next_node_ = nullptr;
	}

	/**
	 * Too lazy to implement/ensuring that it is well-defined.
	 */
	list_intrusive (const this_type&) = delete;

	list_intrusive (this_type &&other) :
	    dummy_ (other.dummy_), p_tail_ (other.p_tail_), size_ (other.size_)
	{
	  other.dummy_.next_node_ = nullptr;
	  if (p_tail_ == &other.dummy_)
	    {
	      p_tail_ = &dummy_;
	    }
	  other.size_ = 0;
	}

	/**
	 * Too lazy to implement/ensuring that it is well-defined.
	 */
	this_type&
	operator= (const this_type&) = delete;

	this_type&
	operator= (this_type &&other)
	{
	  dummy_ = other.dummy_;
	  other.dummy_.next_node_ = nullptr;
	  p_tail_ = other.p_tail_;
	  if (p_tail_ == &other.dummy_)
	    {
	      p_tail_ = &dummy_;
	    }
	  size_ = other.size_;
	  other.size_ = 0;

	  return *this;
	}

	void
	swap (this_type &other)
	{
	  using std::swap;
	  swap (dummy_.next_node_, other.dummy_.next_node_);
	  swap (p_tail_, other.p_tail_);
	  swap (size_, other.size_);
	  if (p_tail_ == &other.dummy_)
	    {
	      p_tail_ = &dummy_;
	    }
	  if (other.p_tail_ == &dummy_)
	    {
	      other.p_tail_ = &other.dummy_;
	    }
	}

	// Iterators
	iterator
	begin ()
	{
	  return iterator (dummy_.next_node_);
	}

	const_iterator
	begin () const
	{
	  return const_iterator (dummy_.next_node_);
	}

	const_iterator
	cbegin () const
	{
	  return const_iterator (dummy_.next_node_);
	}

	iterator
	end ()
	{
	  return iterator (0);
	}

	const_iterator
	end () const
	{
	  return const_iterator (0);
	}

	const_iterator
	cend () const
	{
	  return const_iterator (0);
	}

	void
	clear ()
	{
	  dummy_.next_node_ = nullptr;
	  p_tail_ = &dummy_;
	  size_ = 0;
	}

	bool
	empty () const
	{
	  return dummy_.next_node_ == nullptr;
	}

	reference
	front ()
	{
	  return *static_cast<pointer> (dummy_.next_node_);
	}

	const_reference
	front () const
	{
	  return *static_cast<pointer> (dummy_.next_node_);
	}

	reference
	back ()
	{
	  return *static_cast<pointer> (p_tail_);
	}

	const_reference
	back () const
	{
	  return *static_cast<pointer> (p_tail_);
	}

	void
	push_front (reference entry)
	{
	  entry.next_node_ = dummy_.next_node_;
	  dummy_.next_node_ = &entry;
	  if (p_tail_ == &dummy_)
	    {
	      p_tail_ = &entry;
	    }
	  ++size_;
	}

	void
	push_front (pointer entry)
	{
	  entry->next_node_ = dummy_.next_node_;
	  dummy_.next_node_ = entry;
	  if (p_tail_ == &dummy_)
	    {
	      p_tail_ = entry;
	    }
	  ++size_;
	}

	void
	push_back (reference entry)
	{
	  entry.next_node_ = nullptr;
	  p_tail_->next_node_ = &entry;
	  p_tail_ = &entry;
	  ++size_;
	}

	void
	push_back (pointer entry)
	{
	  entry->next_node_ = nullptr;
	  p_tail_->next_node_ = entry;
	  p_tail_ = entry;
	  ++size_;
	}

	void
	pop_front ()
	{
	  --size_;
	  if (p_tail_ == dummy_.next_node_)
	    {
	      p_tail_ = &dummy_;
	    }
	  dummy_.next_node_ = dummy_.next_node_->next_node_;
	}

	size_type
	size () const
	{
	  return size_;
	}

      private:
	node_type dummy_;
	node_type *p_tail_;
	size_type size_;
      };
  }
}

#endif /* DETAIL_LIST_INTRUSIVE_HPP_ */
