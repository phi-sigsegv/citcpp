#ifndef DETAIL_COVERAGE_MAP_HPP_
#define DETAIL_COVERAGE_MAP_HPP_

#include <vector>
#include <new>
#include "bitset.hpp"
#include "binom_coeff_table.hpp"
#include "internal_model.hpp"
#include "datatypes_config.hpp"
#include "function_ref.hpp"

namespace citcpp
{
  namespace detail
  {
    class bitset_with_num_ones : bitset_uint64
    {
    public:
      typedef bitset_uint64::size_type size_type;

    public:
      bitset_with_num_ones () :
	  bitset_uint64 (0), num_ones_ (0)
      {
      }

      bitset_with_num_ones (size_type num_bits) :
	  bitset_uint64 (num_bits), num_ones_ (0)
      {
      }

      bitset_with_num_ones (const bitset_with_num_ones &other) :
	  bitset_uint64 (other), num_ones_ (other.num_ones_)
      {
      }

      bitset_with_num_ones (bitset_with_num_ones &&other) :
	  bitset_uint64 (std::move (other)), num_ones_ (other.num_ones_)
      {
      }

      ~bitset_with_num_ones ()
      {
      }

      bitset_with_num_ones&
      operator= (const bitset_with_num_ones &other)
      {
	bitset_uint64::operator = (other);
	num_ones_ = other.num_ones_;

	return *this;
      }

      bitset_with_num_ones&
      operator= (bitset_with_num_ones &&other)
      {
	bitset_uint64::operator = (std::move (other));
	num_ones_ = other.num_ones_;

	return *this;
      }

      /**
       * Swaps this and the given other bitset.
       */
      void
      swap (bitset_with_num_ones &other)
      {
	bitset_uint64::swap (other);
	std::swap (num_ones_, other.num_ones_);
      }

      /**
       * Accesses the bit at the given position.
       * This method does no range checking. Passing an invalid position results in undefined behavior.
       */
      bool
      test (size_type bit_pos) const
      {
	return bitset_uint64::test (bit_pos);
      }

      /**
       * Accesses the bit at the given position.
       * This method does no range checking. Passing an invalid position results in undefined behavior.
       */
      bool
      operator[] (size_type bit_pos) const
      {
	return bitset_uint64::operator[] (bit_pos);
      }

      /**
       * Accesses the bit at the given position.
       * Unlike operator[] and test(), this method performs a range check and throws std::out_of_range
       * if the given position is not valid.
       */
      bool
      at (size_type bit_pos) const
      {
	return bitset_uint64::at (bit_pos);
      }

      /**
       * Checks if all of the bits are set to true.
       */
      bool
      all () const
      {
	return num_ones_ == size ();
      }

      /**
       * Checks if any bits are set to true.
       */
      bool
      any () const
      {
	return num_ones_ > 0;
      }

      /**
       * Checks if none of the bits are set to true.
       */
      bool
      none () const
      {
	return num_ones_ == 0;
      }

      /**
       * Returns the number of bits of this bitset.
       */
      size_type
      size () const
      {
	return bitset_uint64::size ();
      }

      /**
       * Accesses the bit at the given position and sets it to true.
       * This method returns the previous value stored at the
       * position.
       * This method does no range checking. Passing an invalid position results in undefined behavior.
       */
      bool
      test_and_set (size_type bit_pos)
      {
	const bool previous_value = bitset_uint64::test_and_set (bit_pos);

	if (!previous_value)
	  {
	    ++num_ones_;
	  }

	return previous_value;
      }

      /**
       * Accesses the bit at the given position and sets it to false.
       * This method returns the previous value stored at the
       * position.
       * This method does no range checking. Passing an invalid position results in undefined behavior.
       */
      bool
      test_and_reset (size_type bit_pos)
      {
	const bool previous_value = bitset_uint64::test_and_reset (bit_pos);

	if (previous_value)
	  {
	    --num_ones_;
	  }

	return previous_value;
      }

      /**
       * Returns the number of bits that are set to true.
       */
      size_type
      count () const
      {
	return num_ones_;
      }

      /**
       * Sets the bit at the given position to true.
       */
      void
      set (size_type bit_pos)
      {
	test_and_set (bit_pos);
      }

      /**
       * Sets all bits to true.
       */
      void
      set ()
      {
	bitset_uint64::set ();
	num_ones_ = size ();
      }

      /**
       * Sets the bit at the given position to false.
       */
      void
      reset (size_type bit_pos)
      {
	test_and_reset (bit_pos);
      }

      /**
       * Sets all bits to false.
       */
      void
      reset ()
      {
	bitset_uint64::reset ();
	num_ones_ = 0;
      }

    private:
      size_type num_ones_;
    };

    class coverage_map_base
    {
    public:
      typedef std::vector<bitset_with_num_ones>::size_type size_type;
      typedef bitset_with_num_ones second_level_type;

      coverage_map_base (unsigned int n, unsigned int t, const model &model,
			 const std::vector<unsigned int> &parameter_index_map,
			 const binom_coeff_table &binomial_coeffs,
			 bool fixed_last_parameter);

      coverage_map_base (const coverage_map_base &other) = default;
      coverage_map_base (coverage_map_base &&other) = default;

      ~coverage_map_base () = default;

      coverage_map_base&
      operator= (const coverage_map_base &other) = default;
      coverage_map_base&
      operator= (coverage_map_base &&other) = default;

      const model&
      get_model () const
      {
	return model_;
      }

      const std::vector<unsigned int>&
      get_parameter_index_map () const
      {
	return parameter_index_map_;
      }

      const binom_coeff_table&
      get_binom_coeff_table () const
      {
	return binomial_coeffs_;
      }

      unsigned int
      get_number_of_parameters_to_select_from () const
      {
	return n_;
      }

      unsigned int
      get_number_of_parameters_to_select () const
      {
	return t_;
      }

      std::vector<bitset_with_num_ones>&
      get_coverage_map ()
      {
	return cov_map_;
      }

      const std::vector<bitset_with_num_ones>&
      get_coverage_map () const
      {
	return cov_map_;
      }

      unsigned long long
      get_total_number_of_tuples () const
      {
	return total_num_tuples_;
      }

    protected:
      const unsigned long long size_;
      const model &model_;
      const std::vector<unsigned int> &parameter_index_map_;
      const binom_coeff_table &binomial_coeffs_;
      const unsigned int n_;
      const unsigned int t_;
      std::vector<bitset_with_num_ones> cov_map_;
      unsigned long long total_num_tuples_;
    };

    // Forward declare value_combination_iterator.
    template<class T_VISITOR>
      class value_combination_iterator;

    class alignas( std::hardware_destructive_interference_size ) coverage_map_iterator_state
    {
      template<class T_VISITOR>
	friend class value_combination_iterator;

    public:
      typedef coverage_map_base::size_type size_type;

      coverage_map_iterator_state (coverage_map_base &cov_map) :
	  cov_map_ (cov_map), param_indices_ (
	      cov_map.get_number_of_parameters_to_select ()), value_indices_ (
	      cov_map.get_number_of_parameters_to_select ()), bitset_index_ (0), bit_pos_ (
	      0)
      {
      }

      coverage_map_iterator_state (const coverage_map_iterator_state &other) = default;
      coverage_map_iterator_state (coverage_map_iterator_state &&other) = default;

      ~coverage_map_iterator_state () = default;

      coverage_map_iterator_state&
      operator= (const coverage_map_iterator_state &other) = default;
      coverage_map_iterator_state&
      operator= (coverage_map_iterator_state &&other) = default;

      const coverage_map_base&
      get_coverage_map () const
      {
	return cov_map_;
      }

      const strength_vector<unsigned int>&
      get_parameter_indices () const
      {
	return param_indices_;
      }

      bitset_with_num_ones&
      get_bitset ()
      {
	return cov_map_.get_coverage_map ()[bitset_index_];
      }

      const bitset_with_num_ones&
      get_bitset () const
      {
	return cov_map_.get_coverage_map ()[bitset_index_];
      }

      const strength_vector<int>&
      get_value_indices () const
      {
	return value_indices_;
      }

      size_type
      get_bitpos () const
      {
	return bit_pos_;
      }

    protected:
      coverage_map_base &cov_map_;
      strength_vector<unsigned int> param_indices_;
      strength_vector<int> value_indices_;
      size_type bitset_index_;
      size_type bit_pos_;
    };

    class alignas( std::hardware_destructive_interference_size ) coverage_map_iterator_mutable_state : public coverage_map_iterator_state
    {
      typedef coverage_map_iterator_state base_type;

    public:
      coverage_map_iterator_mutable_state (coverage_map_base &cov_map) :
	  base_type (cov_map)
      {
      }

      coverage_map_iterator_mutable_state (
	  const coverage_map_iterator_mutable_state &other) = default;
      coverage_map_iterator_mutable_state (
	  coverage_map_iterator_mutable_state &&other) = default;

      ~coverage_map_iterator_mutable_state () = default;

      coverage_map_iterator_mutable_state&
      operator= (const coverage_map_iterator_mutable_state &other) = default;
      coverage_map_iterator_mutable_state&
      operator= (coverage_map_iterator_mutable_state &&other) = default;

      strength_vector<unsigned int>&
      get_parameter_indices ()
      {
	return param_indices_;
      }

      strength_vector<int>&
      get_value_indices ()
      {
	return value_indices_;
      }

      void
      set_bitset_index (base_type::size_type bitset_index)
      {
	bitset_index_ = bitset_index;
      }

      void
      set_bit_pos (base_type::size_type bit_pos)
      {
	bit_pos_ = bit_pos;
      }

      void
      inc_bitset_index (base_type::size_type bitset_index_increment)
      {
	bitset_index_ += bitset_index_increment;
      }

      void
      inc_bit_pos (base_type::size_type bit_pos_increment)
      {
	bit_pos_ += bit_pos_increment;
      }

      template<class T_VISITOR>
	bool
	recursively_visit_all_parameter_combinations (
	    int start_idx_for_param_select, int current_param_idx_to_select,
	    T_VISITOR &visitor)
	{
	  for (int j = start_idx_for_param_select;
	      j >= current_param_idx_to_select; --j)
	    {
	      param_indices_[current_param_idx_to_select] =
		  cov_map_.get_parameter_index_map ()[j];

	      bool ret = true;

	      if (current_param_idx_to_select == 0)
		{
		  // We assume that the visitor is a functor accepting a reference to this iterator state.
		  // However, we make sure that it can only see the immutable part of its API.
		  ret = visitor (
		      static_cast<coverage_map_iterator_state&> (*this));

		  ++bitset_index_;
		}
	      else
		{
		  ret = recursively_visit_all_parameter_combinations (
		      j - 1, current_param_idx_to_select - 1, visitor);
		}

	      if (!ret)
		{
		  return false;
		}
	    }

	  return true;
	}
    };

    template<class T_VISITOR>
      class value_combination_iterator
      {
      public:
	value_combination_iterator (bool skip_fully_covered_param_combo,
				    T_VISITOR &visitor) :
	    skip_fully_covered_param_combo_ (skip_fully_covered_param_combo), visitor_ (
		visitor)
	{
	}

	bool
	operator() (coverage_map_iterator_state &cov_map_it)
	{
	  if (!skip_fully_covered_param_combo_
	      || !cov_map_it.get_bitset ().all ())
	    {
	      cov_map_it.bit_pos_ = 0;

	      return recursively_visit_all_value_combos_of_param_combo (
		  cov_map_it, cov_map_it.value_indices_.size () - 1);
	    }

	  return true;
	}

      private:
	bool
	recursively_visit_all_value_combos_of_param_combo (
	    coverage_map_iterator_state &cov_map_it, int current_index)
	{
	  strength_vector<int> &value_indices = cov_map_it.value_indices_;

	  // The current range goes from 0 to max_value[current_index]
	  unsigned int max_val =
	      cov_map_it.cov_map_.get_model ().get_parameters ()[cov_map_it.get_parameter_indices ()[current_index]];

	  for (int i = max_val - 1; i >= 0; --i)
	    {
	      value_indices[current_index] = i;

	      bool ret = true;

	      if (current_index == 0)
		{
		  // We assume that the visitor is a functor accepting a reference to this iterator.
		  // In addition
		  ret = visitor_ (cov_map_it);

		  ++cov_map_it.bit_pos_;
		}
	      else
		{
		  ret = recursively_visit_all_value_combos_of_param_combo (
		      cov_map_it, current_index - 1);
		}

	      if (!ret)
		{
		  return false;
		}
	    }

	  return true;
	}

      private:
	bool skip_fully_covered_param_combo_;
	T_VISITOR &visitor_;
      };

    template<bool FIXED_LAST_PARAMETER>
      class coverage_map_iterator
      {
      public:
	coverage_map_iterator (coverage_map_base &cov_map) :
	    state_ (cov_map)
	{
	}

	coverage_map_iterator (const coverage_map_iterator &other) = default;
	coverage_map_iterator (coverage_map_iterator &&other) = default;

	~coverage_map_iterator () = default;

	coverage_map_iterator&
	operator= (const coverage_map_iterator &other) = default;
	coverage_map_iterator&
	operator= (coverage_map_iterator &&other) = default;

	template<class T_VISITOR>
	  void
	  visit_all_parameter_combinations (T_VISITOR &visitor)
	  {
	    for (int i =
		state_.get_coverage_map ().get_number_of_parameters_to_select ()
		    - 1; i >= 0; --i)
	      {
		state_.get_parameter_indices ()[i] =
		    state_.get_coverage_map ().get_parameter_index_map ()[state_.get_coverage_map ().get_number_of_parameters_to_select_from ()
			- state_.get_coverage_map ().get_number_of_parameters_to_select ()
			+ i];
	      }

	    state_.set_bitset_index (0);
	    state_.set_bit_pos (0);

	    state_.recursively_visit_all_parameter_combinations (
		state_.get_coverage_map ().get_number_of_parameters_to_select_from ()
		    - 1,
		state_.get_coverage_map ().get_number_of_parameters_to_select ()
		    - 1,
		visitor);
	  }

	template<class T_VISITOR>
	  void
	  visit_all_tuples (bool skip_fully_covered_param_combo,
			    T_VISITOR &visitor)
	  {
	    for (int i =
		state_.get_coverage_map ().get_number_of_parameters_to_select ()
		    - 1; i >= 0; --i)
	      {
		state_.get_parameter_indices ()[i] =
		    state_.get_coverage_map ().get_parameter_index_map ()[state_.get_coverage_map ().get_number_of_parameters_to_select_from ()
			- state_.get_coverage_map ().get_number_of_parameters_to_select ()
			+ i];
	      }

	    state_.set_bitset_index (0);
	    state_.set_bit_pos (0);

	    value_combination_iterator<T_VISITOR> value_combo_it (
		skip_fully_covered_param_combo, visitor);

	    state_.recursively_visit_all_parameter_combinations (
		state_.get_coverage_map ().get_number_of_parameters_to_select_from ()
		    - 1,
		state_.get_coverage_map ().get_number_of_parameters_to_select ()
		    - 1,
		value_combo_it);
	  }

      private:
	coverage_map_iterator_mutable_state state_;
      };

    template<>
      class coverage_map_iterator<true>
      {
      public:
	coverage_map_iterator (coverage_map_base &cov_map) :
	    state_ (cov_map)
	{
	}

	coverage_map_iterator (const coverage_map_iterator &other) = default;
	coverage_map_iterator (coverage_map_iterator &&other) = default;

	~coverage_map_iterator () = default;

	coverage_map_iterator&
	operator= (const coverage_map_iterator &other) = default;
	coverage_map_iterator&
	operator= (coverage_map_iterator &&other) = default;

	template<class T_VISITOR>
	  void
	  visit_all_parameter_combinations (T_VISITOR &visitor)
	  {
	    for (int i =
		state_.get_coverage_map ().get_number_of_parameters_to_select ()
		    - 1; i >= 0; --i)
	      {
		state_.get_parameter_indices ()[i] =
		    state_.get_coverage_map ().get_parameter_index_map ()[state_.get_coverage_map ().get_number_of_parameters_to_select_from ()
			- state_.get_coverage_map ().get_number_of_parameters_to_select ()
			+ i];
	      }

	    state_.set_bitset_index (0);
	    state_.set_bit_pos (0);

	    state_.recursively_visit_all_parameter_combinations (
		state_.get_coverage_map ().get_number_of_parameters_to_select_from ()
		    - 2,
		state_.get_coverage_map ().get_number_of_parameters_to_select ()
		    - 2,
		visitor);
	  }

	template<class T_VISITOR>
	  void
	  visit_all_tuples (bool skip_fully_covered_param_combo,
			    T_VISITOR &visitor)
	  {
	    for (int i =
		state_.get_coverage_map ().get_number_of_parameters_to_select ()
		    - 1; i >= 0; --i)
	      {
		state_.get_parameter_indices ()[i] =
		    state_.get_coverage_map ().get_parameter_index_map ()[state_.get_coverage_map ().get_number_of_parameters_to_select_from ()
			- state_.get_coverage_map ().get_number_of_parameters_to_select ()
			+ i];
	      }

	    state_.set_bitset_index (0);
	    state_.set_bit_pos (0);

	    value_combination_iterator<T_VISITOR> value_combo_it (
		skip_fully_covered_param_combo, visitor);

	    state_.recursively_visit_all_parameter_combinations (
		state_.get_coverage_map ().get_number_of_parameters_to_select_from ()
		    - 2,
		state_.get_coverage_map ().get_number_of_parameters_to_select ()
		    - 2,
		value_combo_it);
	  }

      private:
	coverage_map_iterator_mutable_state state_;
      };

    template<bool FIXED_LAST_PARAMETER>
      class coverage_map_parallel_iterator
      {
      public:
	coverage_map_parallel_iterator (coverage_map_base &cov_map,
					thread_pool &tp) :
	    cov_map_ (cov_map), per_thread_data_ (
		tp.get_num_workers (),
		coverage_map_iterator_mutable_state (cov_map)), tp_ (tp), iterate_tasks_ (), visitor_ ()
	{
	  unsigned long long bitset_start_index = 0;

	  for (int i = cov_map_.get_number_of_parameters_to_select_from () - 1;
	      i >= (int) cov_map_.get_number_of_parameters_to_select () - 1;
	      --i)
	    {
	      iterate_tasks_.emplace_back (this, i, bitset_start_index);

	      bitset_start_index +=
		  cov_map_.get_binom_coeff_table ().get_coefficient (
		      i, cov_map_.get_number_of_parameters_to_select () - 1);
	    }
	}

	coverage_map_parallel_iterator (
	    const coverage_map_parallel_iterator &other) = default;
	coverage_map_parallel_iterator (coverage_map_parallel_iterator &&other) = default;

	~coverage_map_parallel_iterator () = default;

	coverage_map_parallel_iterator&
	operator= (const coverage_map_parallel_iterator &other) = default;
	coverage_map_parallel_iterator&
	operator= (coverage_map_parallel_iterator &&other) = default;

	unsigned int
	get_num_workers () const
	{
	  return tp_.get_num_workers ();
	}

	unsigned int
	get_worker_id () const
	{
	  return tp_.get_worker_id ();
	}

	template<class T_VISITOR>
	  void
	  visit_all_parameter_combinations (T_VISITOR &visitor)
	  {
	    if (cov_map_.get_number_of_parameters_to_select () < 2)
	      {
		coverage_map_iterator_mutable_state &iter_data =
		    per_thread_data_[0];

		for (int i = cov_map_.get_number_of_parameters_to_select () - 1;
		    i >= 0; --i)
		  {
		    iter_data.get_parameter_indices ()[i] =
			cov_map_.get_parameter_index_map ()[cov_map_.get_number_of_parameters_to_select_from ()
			    - cov_map_.get_number_of_parameters_to_select () + i];
		  }

		iter_data.set_bitset_index (0);
		iter_data.set_bit_pos (0);

		iter_data.recursively_visit_all_parameter_combinations (
		    cov_map_.get_number_of_parameters_to_select_from () - 1,
		    cov_map_.get_number_of_parameters_to_select () - 1,
		    visitor);
	      }
	    else
	      {
		visitor_ = visitor;

		task_group tg (tp_.createTaskGroup ());
		int i = 0;
		for (iterate_task &task : iterate_tasks_)
		  {
		    task.reset ();
		    tg.spawn (i, &task);
		    ++i;
		  }
		tg.wait ();
	      }
	  }

	template<class T_VISITOR>
	  void
	  visit_all_tuples (bool skip_fully_covered_param_combo,
			    T_VISITOR &visitor)
	  {
	    if (cov_map_.get_number_of_parameters_to_select () < 2)
	      {
		coverage_map_iterator_mutable_state &iter_data =
		    per_thread_data_[0];

		for (int i = cov_map_.get_number_of_parameters_to_select () - 1;
		    i >= 0; --i)
		  {
		    iter_data.get_parameter_indices ()[i] =
			cov_map_.get_parameter_index_map ()[cov_map_.get_number_of_parameters_to_select_from ()
			    - cov_map_.get_number_of_parameters_to_select () + i];
		  }

		iter_data.set_bitset_index (0);
		iter_data.set_bit_pos (0);

		value_combination_iterator<T_VISITOR> value_combo_it (
		    skip_fully_covered_param_combo, visitor);

		iter_data.recursively_visit_all_parameter_combinations (
		    cov_map_.get_number_of_parameters_to_select_from () - 1,
		    cov_map_.get_number_of_parameters_to_select () - 1,
		    value_combo_it);
	      }
	    else
	      {
		value_combination_iterator<T_VISITOR> value_combo_it (
		    skip_fully_covered_param_combo, visitor);

		visitor_ = value_combo_it;

		task_group tg (tp_.createTaskGroup ());
		int i = 0;
		for (iterate_task &task : iterate_tasks_)
		  {
		    task.reset ();
		    tg.spawn (i, &task);
		    ++i;
		  }
		tg.wait ();
	      }
	  }

      private:
	typedef coverage_map_parallel_iterator<FIXED_LAST_PARAMETER> iterator_type;

	class iterate_task : public thread_pool::Task
	{
	private:
	  typedef thread_pool::Task base_type;
	  typedef iterate_task this_type;

	public:
	  iterate_task () = delete;

	  iterate_task (iterator_type *iterator, int max_param_to_select_from,
			unsigned long long bitset_start_index) :
	      base_type (), iterator_ (iterator), max_param_to_select_from_ (
		  max_param_to_select_from), bitset_start_index_ (
		  bitset_start_index)
	  {
	    setCallable (*this);
	  }

	  iterate_task (const this_type&) = delete;

	  iterate_task (this_type &&other) :
	      base_type (std::move (other)), iterator_ (other.iterator_), max_param_to_select_from_ (
		  other.max_param_to_select_from_), bitset_start_index_ (
		  other.bitset_start_index_)
	  {
	    setCallable (*this);
	  }

	  virtual
	  ~iterate_task ()
	  {
	  }

	  this_type&
	  operator= (const this_type&) = delete;

	  this_type&
	  operator= (this_type&&) = delete;

	  void
	  operator () ()
	  {
	    coverage_map_iterator_mutable_state &iter_data =
		iterator_->per_thread_data_[iterator_->get_worker_id ()];
	    for (int j =
		iterator_->cov_map_.get_number_of_parameters_to_select () - 1;
		j >= 0; --j)
	      {
		iter_data.get_parameter_indices ()[j] =
		    iterator_->cov_map_.get_parameter_index_map ()[max_param_to_select_from_
			- (iterator_->cov_map_.get_number_of_parameters_to_select ()
			    - 1) + j];
	      }

	    iter_data.set_bitset_index (bitset_start_index_);

	    iter_data.recursively_visit_all_parameter_combinations (
		max_param_to_select_from_ - 1,
		iterator_->cov_map_.get_number_of_parameters_to_select () - 2,
		iterator_->visitor_);
	  }

	private:
	  iterator_type *iterator_;
	  int max_param_to_select_from_;
	  unsigned long long bitset_start_index_;
	};

	friend class iterate_task;

      private:
	coverage_map_base &cov_map_;
	std::vector<coverage_map_iterator_mutable_state> per_thread_data_;
	thread_pool &tp_;
	std::vector<iterate_task> iterate_tasks_;
	function_ref<bool
	(coverage_map_iterator_state&)> visitor_;
      };

    template<>
      class coverage_map_parallel_iterator<true>
      {
      public:
	coverage_map_parallel_iterator (coverage_map_base &cov_map,
					thread_pool &tp) :
	    cov_map_ (cov_map), per_thread_data_ (
		tp.get_num_workers (),
		coverage_map_iterator_mutable_state (cov_map)), tp_ (tp), iterate_tasks_ (), visitor_ ()
	{
	  unsigned long long bitset_start_index = 0;

	  for (int i = cov_map_.get_number_of_parameters_to_select_from () - 2;
	      i >= (int) cov_map_.get_number_of_parameters_to_select () - 2;
	      --i)
	    {
	      iterate_tasks_.emplace_back (this, i, bitset_start_index);

	      bitset_start_index +=
		  cov_map_.get_binom_coeff_table ().get_coefficient (
		      i, cov_map_.get_number_of_parameters_to_select () - 2);
	    }
	}

	coverage_map_parallel_iterator (
	    const coverage_map_parallel_iterator &other) = default;
	coverage_map_parallel_iterator (coverage_map_parallel_iterator &&other) = default;

	~coverage_map_parallel_iterator () = default;

	coverage_map_parallel_iterator&
	operator= (const coverage_map_parallel_iterator &other) = default;
	coverage_map_parallel_iterator&
	operator= (coverage_map_parallel_iterator &&other) = default;

	unsigned int
	get_num_workers () const
	{
	  return tp_.get_num_workers ();
	}

	unsigned int
	get_worker_id () const
	{
	  return tp_.get_worker_id ();
	}

	template<class T_VISITOR>
	  void
	  visit_all_parameter_combinations (T_VISITOR &visitor)
	  {
	    if (cov_map_.get_number_of_parameters_to_select () < 3)
	      {
		coverage_map_iterator_mutable_state &iter_data =
		    per_thread_data_[0];

		for (int i = cov_map_.get_number_of_parameters_to_select () - 1;
		    i >= 0; --i)
		  {
		    iter_data.get_parameter_indices ()[i] =
			cov_map_.get_parameter_index_map ()[cov_map_.get_number_of_parameters_to_select_from ()
			    - cov_map_.get_number_of_parameters_to_select () + i];
		  }

		iter_data.set_bitset_index (0);
		iter_data.set_bit_pos (0);

		iter_data.recursively_visit_all_parameter_combinations (
		    cov_map_.get_number_of_parameters_to_select_from () - 2,
		    cov_map_.get_number_of_parameters_to_select () - 2,
		    visitor);
	      }
	    else
	      {
		visitor_ = visitor;

		task_group tg (tp_.createTaskGroup ());
		int i = 0;
		for (iterate_task &task : iterate_tasks_)
		  {
		    task.reset ();
		    tg.spawn (i, &task);
		    ++i;
		  }
		tg.wait ();
	      }
	  }

	template<class T_VISITOR>
	  void
	  visit_all_tuples (bool skip_fully_covered_param_combo,
			    T_VISITOR &visitor)
	  {
	    if (cov_map_.get_number_of_parameters_to_select () < 3)
	      {
		coverage_map_iterator_mutable_state &iter_data =
		    per_thread_data_[0];

		for (int i = cov_map_.get_number_of_parameters_to_select () - 1;
		    i >= 0; --i)
		  {
		    iter_data.get_parameter_indices ()[i] =
			cov_map_.get_parameter_index_map ()[cov_map_.get_number_of_parameters_to_select_from ()
			    - cov_map_.get_number_of_parameters_to_select () + i];
		  }

		iter_data.set_bitset_index (0);
		iter_data.set_bit_pos (0);

		value_combination_iterator<T_VISITOR> value_combo_it (
		    skip_fully_covered_param_combo, visitor);

		iter_data.recursively_visit_all_parameter_combinations (
		    cov_map_.get_number_of_parameters_to_select_from () - 2,
		    cov_map_.get_number_of_parameters_to_select () - 2,
		    value_combo_it);
	      }
	    else
	      {
		value_combination_iterator<T_VISITOR> value_combo_it (
		    skip_fully_covered_param_combo, visitor);

		visitor_ = value_combo_it;

		task_group tg (tp_.createTaskGroup ());
		int i = 0;
		for (iterate_task &task : iterate_tasks_)
		  {
		    task.reset ();
		    tg.spawn (i, &task);
		    ++i;
		  }
		tg.wait ();
	      }
	  }

      private:
	typedef coverage_map_parallel_iterator<true> iterator_type;

	class iterate_task : public thread_pool::Task
	{
	private:
	  typedef thread_pool::Task base_type;
	  typedef iterate_task this_type;

	public:
	  iterate_task () = delete;

	  iterate_task (iterator_type *iterator, int max_param_to_select_from,
			unsigned long long bitset_start_index) :
	      base_type (), iterator_ (iterator), max_param_to_select_from_ (
		  max_param_to_select_from), bitset_start_index_ (
		  bitset_start_index)
	  {
	    setCallable (*this);
	  }

	  iterate_task (const this_type&) = delete;

	  iterate_task (this_type &&other) :
	      base_type (std::move (other)), iterator_ (other.iterator_), max_param_to_select_from_ (
		  other.max_param_to_select_from_), bitset_start_index_ (
		  other.bitset_start_index_)
	  {
	    setCallable (*this);
	  }

	  virtual
	  ~iterate_task ()
	  {
	  }

	  this_type&
	  operator= (const this_type&) = delete;

	  this_type&
	  operator= (this_type&&) = delete;

	  void
	  operator () ()
	  {
	    coverage_map_iterator_mutable_state &iter_data =
		iterator_->per_thread_data_[iterator_->get_worker_id ()];
	    iter_data.get_parameter_indices ()[iterator_->cov_map_.get_number_of_parameters_to_select ()
		- 1] =
		iterator_->cov_map_.get_parameter_index_map ()[iterator_->cov_map_.get_number_of_parameters_to_select_from ()
		    - 1];
	    for (int j =
		iterator_->cov_map_.get_number_of_parameters_to_select () - 2;
		j >= 0; --j)
	      {
		iter_data.get_parameter_indices ()[j] =
		    iterator_->cov_map_.get_parameter_index_map ()[max_param_to_select_from_
			- (iterator_->cov_map_.get_number_of_parameters_to_select ()
			    - 2) + j];
	      }

	    iter_data.set_bitset_index (bitset_start_index_);

	    iter_data.recursively_visit_all_parameter_combinations (
		max_param_to_select_from_ - 1,
		iterator_->cov_map_.get_number_of_parameters_to_select () - 3,
		iterator_->visitor_);
	  }

	private:
	  iterator_type *iterator_;
	  int max_param_to_select_from_;
	  unsigned long long bitset_start_index_;
	};

	friend class iterate_task;

      private:
	coverage_map_base &cov_map_;
	std::vector<coverage_map_iterator_mutable_state> per_thread_data_;
	thread_pool &tp_;
	std::vector<iterate_task> iterate_tasks_;
	function_ref<bool
	(coverage_map_iterator_state&)> visitor_;
      };

    /**
     * This is a quite central data structure in combinatorial testing tools.
     * It keeps track of the coverage of the parameter combinations and their cross
     * product of value combinations.
     * Since the number of value combinations for all t-way combinations of parameters
     * can be quite huge, this data structure is optimized for efficient memory
     * representation. At the same time, operations on the data structure need to
     * be lighting fast, again due to the vast amount of t-tuples whose coverage to
     * track.
     *
     * The coverage map is able to keep track of tuple coverage
     * of t-wise combinations from n parameters (indices [0, ... ,n-1]).
     */
    template<bool FIXED_LAST_PARAMETER>
      class coverage_map : public coverage_map_base
      {
	typedef coverage_map_base base_type;
	friend class coverage_map_iterator<false> ;

      public:
	typedef std::vector<bitset_with_num_ones>::size_type size_type;
	typedef bitset_with_num_ones second_level_type;

	coverage_map (unsigned int n, unsigned int t, const model &model,
		      const std::vector<unsigned int> &parameter_index_map,
		      const binom_coeff_table &binomial_coeffs) :
	    base_type (n, t, model, parameter_index_map, binomial_coeffs, false)
	{
	}

	coverage_map (const coverage_map &other) = default;
	coverage_map (coverage_map &&other) = default;

	~coverage_map () = default;

	coverage_map&
	operator= (const coverage_map &other) = default;
	coverage_map&
	operator= (coverage_map &&other) = default;

	coverage_map_iterator<false>
	create_iterator ()
	{
	  return coverage_map_iterator<false> (*this);
	}

	coverage_map_parallel_iterator<false>
	create_parallel_iterator (thread_pool &tp)
	{
	  return coverage_map_parallel_iterator<false> (*this, tp);
	}
      };

    /**
     * This is a quite central data structure in combinatorial testing tools.
     * It keeps track of the coverage of the parameter combinations and their cross
     * product of value combinations.
     * Since the number of value combinations for all t-way combinations of parameters
     * can be quite huge, this data structure is optimized for efficient memory
     * representation. At the same time, operations on the data structure need to
     * be lighting fast, again due to the vast amount of t-tuples whose coverage to
     * track.
     *
     * The coverage map is able to keep track of tuple coverage
     * of t-wise combinations from n parameters (indices [0, ... ,n-1]), but
     * the last parameter is fixed. Or in other words: We select combinations of
     * length t-1 from the parameters [0, ... ,n-2], and extend those combinations by
     * always prepending parameter n-1 to them.
     */
    template<>
      class coverage_map<true> : public coverage_map_base
      {
	typedef coverage_map_base base_type;
	friend class coverage_map_iterator<true> ;

      public:
	typedef std::vector<bitset_with_num_ones>::size_type size_type;
	typedef bitset_with_num_ones second_level_type;

	coverage_map (unsigned int n, unsigned int t, const model &model,
		      const std::vector<unsigned int> &parameter_index_map,
		      const binom_coeff_table &binomial_coeffs) :
	    base_type (n, t, model, parameter_index_map, binomial_coeffs, true)
	{
	}

	coverage_map (const coverage_map &other) = default;
	coverage_map (coverage_map &&other) = default;

	~coverage_map () = default;

	coverage_map&
	operator= (const coverage_map &other) = default;
	coverage_map&
	operator= (coverage_map &&other) = default;

	coverage_map_iterator<true>
	create_iterator ()
	{
	  return coverage_map_iterator<true> (*this);
	}

	coverage_map_parallel_iterator<true>
	create_parallel_iterator (thread_pool &tp)
	{
	  return coverage_map_parallel_iterator<true> (*this, tp);
	}
      };

    typedef coverage_map<true> coverage_map_ipog;
  }
}

#endif /* DETAIL_COVERAGE_MAP_HPP_ */
