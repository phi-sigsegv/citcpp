#ifndef DETAIL_COVERAGE_MAP_HPP_
#define DETAIL_COVERAGE_MAP_HPP_

#include <vector>
#include "bitset.hpp"
#include "binom_coeff_table.hpp"
#include "internal_model.hpp"
#include "datatypes_config.hpp"

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

    // Forward declare coverage_map iterators.
    class coverage_map_iterator_base;
    template<bool FIXED_LAST_PARAMETER>
      class coverage_map_iterator;

    class coverage_map_base
    {
      friend class coverage_map_iterator_base;
      template<bool FIXED_LAST_PARAMETER>
	friend class coverage_map_iterator;

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
      get_model () const;

      std::vector<bitset_with_num_ones>&
      get_coverage_map ();

      const std::vector<bitset_with_num_ones>&
      get_coverage_map () const;

      unsigned long long
      get_total_number_of_tuples () const;

    protected:
      const unsigned long long size_;
      const model &model_;
      const std::vector<unsigned int> &parameter_index_map_;
      const unsigned int n_;
      const unsigned int t_;
      std::vector<bitset_with_num_ones> cov_map_;
      unsigned long long total_num_tuples_;
    };

    // Forward declare value_combination_iterator.
    template<class T_VISITOR>
      class value_combination_iterator;

    class coverage_map_iterator_base
    {
      template<class T_VISITOR>
	friend class value_combination_iterator;

    public:
      typedef coverage_map_base::size_type size_type;

      coverage_map_iterator_base (coverage_map_base &cov_map) :
	  cov_map_ (cov_map), param_indices_ (cov_map.t_), value_indices_ (
	      cov_map.t_), bitset_index_ (0), bit_pos_ (0)
      {
      }

      coverage_map_iterator_base (const coverage_map_iterator_base &other) = default;
      coverage_map_iterator_base (coverage_map_iterator_base &&other) = default;

      ~coverage_map_iterator_base () = default;

      coverage_map_iterator_base&
      operator= (const coverage_map_iterator_base &other) = default;
      coverage_map_iterator_base&
      operator= (coverage_map_iterator_base &&other) = default;

      const strength_vector<unsigned int>&
      get_parameter_indices () const
      {
	return param_indices_;
      }

      bitset_with_num_ones&
      get_bitset ()
      {
	return cov_map_.cov_map_[bitset_index_];
      }

      const bitset_with_num_ones&
      get_bitset () const
      {
	return cov_map_.cov_map_[bitset_index_];
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
      template<class T_VISITOR>
	bool
	recursively_visit_all_parameter_combinations (
	    int start_idx_for_param_select, int current_param_idx_to_select,
	    T_VISITOR &visitor)
	{
	  if (current_param_idx_to_select < 0)
	    {
	      // We assume that the visitor is a functor accepting a reference to this iterator.
	      bool ret = visitor (*this);

	      ++bitset_index_;

	      return ret;
	    }

	  for (int j = start_idx_for_param_select;
	      j >= current_param_idx_to_select; --j)
	    {
	      param_indices_[current_param_idx_to_select] =
		  cov_map_.parameter_index_map_[j];

	      bool ret = recursively_visit_all_parameter_combinations (
		  j - 1, current_param_idx_to_select - 1, visitor);

	      if (!ret)
		{
		  return false;
		}
	    }

	  return true;
	}

    protected:
      coverage_map_base &cov_map_;
      strength_vector<unsigned int> param_indices_;
      strength_vector<int> value_indices_;
      size_type bitset_index_;
      size_type bit_pos_;
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
	operator() (coverage_map_iterator_base &cov_map_it)
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
	    coverage_map_iterator_base &cov_map_it, int current_index)
	{
	  strength_vector<int> &value_indices = cov_map_it.value_indices_;

	  if (current_index < 0)
	    {
	      // We assume that the visitor is a functor accepting a reference to this iterator.
	      // In addition
	      bool ret = visitor_ (cov_map_it);

	      ++cov_map_it.bit_pos_;

	      return ret;
	    }

	  // The current range goes from 0 to max_value[current_index]
	  unsigned int max_val =
	      cov_map_it.cov_map_.get_model ().get_parameters ()[cov_map_it.param_indices_[current_index]];

	  for (int i = max_val - 1; i >= 0; --i)
	    {
	      value_indices[current_index] = i;

	      bool ret = recursively_visit_all_value_combos_of_param_combo (
		  cov_map_it, current_index - 1);

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
      class coverage_map_iterator : public coverage_map_iterator_base
      {
	typedef coverage_map_iterator_base base_type;

      public:
	typedef coverage_map_iterator_base::size_type size_type;

	coverage_map_iterator (coverage_map_base &cov_map) :
	    base_type (cov_map)
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
	    for (int i = cov_map_.t_ - 1; i >= 0; --i)
	      {
		param_indices_[i] = cov_map_.parameter_index_map_[cov_map_.n_
		    - cov_map_.t_ + i];
	      }

	    bitset_index_ = 0;
	    bit_pos_ = 0;

	    recursively_visit_all_parameter_combinations (cov_map_.n_ - 1,
							  cov_map_.t_ - 1,
							  visitor);
	  }

	template<class T_VISITOR>
	  void
	  visit_all_tuples (bool skip_fully_covered_param_combo,
			    T_VISITOR &visitor)
	  {
	    for (int i = cov_map_.t_ - 1; i >= 0; --i)
	      {
		param_indices_[i] = cov_map_.parameter_index_map_[cov_map_.n_
		    - cov_map_.t_ + i];
	      }

	    bitset_index_ = 0;
	    bit_pos_ = 0;

	    value_combination_iterator<T_VISITOR> value_combo_it (
		skip_fully_covered_param_combo, visitor);

	    recursively_visit_all_parameter_combinations (cov_map_.n_ - 1,
							  cov_map_.t_ - 1,
							  value_combo_it);
	  }
      };

    template<>
      class coverage_map_iterator<true> : public coverage_map_iterator_base
      {
	typedef coverage_map_iterator_base base_type;

      public:
	typedef coverage_map_iterator_base::size_type size_type;

	coverage_map_iterator (coverage_map_base &cov_map) :
	    base_type (cov_map)
	{
	  param_indices_[cov_map_.t_ - 1] =
	      cov_map_.parameter_index_map_[cov_map_.n_ - 1];
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
	    for (int i = cov_map_.t_ - 1; i >= 0; --i)
	      {
		param_indices_[i] = cov_map_.parameter_index_map_[cov_map_.n_
		    - cov_map_.t_ + i];
	      }

	    bitset_index_ = 0;
	    bit_pos_ = 0;

	    recursively_visit_all_parameter_combinations (cov_map_.n_ - 2,
							  cov_map_.t_ - 2,
							  visitor);
	  }

	template<class T_VISITOR>
	  void
	  visit_all_tuples (bool skip_fully_covered_param_combo,
			    T_VISITOR &visitor)
	  {
	    for (int i = cov_map_.t_ - 1; i >= 0; --i)
	      {
		param_indices_[i] = cov_map_.parameter_index_map_[cov_map_.n_
		    - cov_map_.t_ + i];
	      }

	    bitset_index_ = 0;
	    bit_pos_ = 0;

	    value_combination_iterator<T_VISITOR> value_combo_it (
		skip_fully_covered_param_combo, visitor);

	    recursively_visit_all_parameter_combinations (cov_map_.n_ - 2,
							  cov_map_.t_ - 2,
							  value_combo_it);
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
      };

    typedef coverage_map<true> coverage_map_ipog;
  }
}

#endif /* DETAIL_COVERAGE_MAP_HPP_ */
