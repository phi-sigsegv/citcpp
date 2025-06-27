#ifndef DETAIL_COVERAGE_MAP_HPP_
#define DETAIL_COVERAGE_MAP_HPP_

#include <vector>
#include "bitset.hpp"
#include "binom_coeff_table.hpp"

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

    /**
     * This is a quite central data structure in combinatorial testing tools.
     * It keeps track of the coverage of the parameter combinations and their cross
     * product of value combinations.
     * Since the number of value combinations for all t-way combinations of parameters
     * can be quite huge, this data structure is optimized for efficient memory
     * representation. At the same time, operations on the data structure need to
     * be lighting fast, again due to the vast amount of t-tuples whose coverage to
     * track.
     */
    class coverage_map
    {
      typedef std::vector<bitset_with_num_ones> first_level_type;

    public:
      typedef typename first_level_type::size_type size_type;
      typedef bitset_with_num_ones second_level_type;

    public:
      /**
       * Creates a coverage map, which is able to keep track of tuple coverage
       * of t-wise combinations of n parameters.
       */
      coverage_map (unsigned int n, unsigned int t,
		    const binom_coeff_table &binomial_coeffs);

      coverage_map (const coverage_map &other) = default;
      coverage_map (coverage_map &&other) = default;

      ~coverage_map () = default;

      coverage_map&
      operator= (const coverage_map &other) = default;
      coverage_map&
      operator= (coverage_map &&other) = default;

      void
      swap (coverage_map &other);

      first_level_type&
      get_coverage_map ();

      const first_level_type&
      get_coverage_map () const;

    private:
      first_level_type cov_map_;
    };
  }
}

#endif /* DETAIL_COVERAGE_MAP_HPP_ */
