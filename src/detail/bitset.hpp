#ifndef DETAIL_BITSET_HPP_
#define DETAIL_BITSET_HPP_

#include <cstddef>
#include <cstring>
#include <cstdint>
#include <limits>
#include <algorithm>
#include <type_traits>
#include <stdexcept>
#include <bit>

namespace citcpp
{
  namespace detail
  {
    template<typename T>
      struct all_ones_tmpl
      {
	static constexpr T value = static_cast<T> (-1);
      };

    template<typename T_FUNDAMENTAL_STORAGE_TYPE>
      class bitset
      {
      public:
	typedef T_FUNDAMENTAL_STORAGE_TYPE storage_type;
	typedef std::uint32_t size_type;

      public:
	bitset () :
	    bits_ (nullptr), size_ (0)
	{
	}

	bitset (size_type num_bits) :
	    bits_ (
		new storage_type[calculate_num_storage_blocks_from_num_bits (
		    num_bits)]
		  { }), size_ (num_bits)
	{
	}

	bitset (const bitset &other) :
	    bits_ (
		new storage_type[calculate_num_storage_blocks_from_num_bits (
		    other.size_)]
		  { }), size_ (other.size_)
	{
	  std::memcpy (
	      bits_,
	      other.bits_,
	      calculate_num_storage_blocks_from_num_bits (size_)
		  * sizeof(storage_type));
	}

	bitset (bitset &&other) :
	    bits_ (other.bits_), size_ (other.size_)
	{
	  other.size_ = 0;
	  other.bits_ = nullptr;
	}

	~bitset ()
	{
	  delete[] bits_;
	}

	bitset&
	operator= (const bitset &other)
	{
	  if (&other != this)
	    {
	      delete[] bits_;
	      size_ = other.size_;
	      bits_ =
		  new storage_type[calculate_num_storage_blocks_from_num_bits (
		      size_)];
	      std::memcpy (
		  bits_,
		  other.bits_,
		  calculate_num_storage_blocks_from_num_bits (size_)
		      * sizeof(storage_type));
	    }

	  return *this;
	}

	bitset&
	operator= (bitset &&other)
	{
	  if (&other != this)
	    {
	      delete[] bits_;
	      size_ = other.size_;
	      bits_ = other.bits_;
	      other.size_ = 0;
	      other.bits_ = nullptr;
	    }

	  return *this;
	}

	/**
	 * Swaps this and the given other bitset.
	 */
	void
	swap (bitset &other)
	{
	  std::swap (size_, other.size_);
	  std::swap (bits_, other.bits_);
	}

	/**
	 * Accesses the bit at the given position.
	 * This method does no range checking. Passing an invalid position results in undefined behavior.
	 */
	bool
	test (size_type bit_pos) const
	{
	  return (bits_[calculate_storage_block_index (bit_pos)]
	      & bit_mask (bit_pos)) != 0;
	}

	/**
	 * Accesses the bit at the given position.
	 * This method does no range checking. Passing an invalid position results in undefined behavior.
	 */
	bool
	operator[] (size_type bit_pos) const
	{
	  return test (bit_pos);
	}

	/**
	 * Accesses the bit at the given position.
	 * Unlike operator[] and test(), this method performs a range check and throws std::out_of_range
	 * if the given position is not valid.
	 */
	bool
	at (size_type bit_pos) const
	{
	  if (bit_pos >= size_)
	    {
	      throw std::out_of_range ("bitset::at out_of_range");
	    }

	  return test (bit_pos);
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
	  const size_type storage_block_index = calculate_storage_block_index (
	      bit_pos);
	  const storage_type mask = bit_mask (bit_pos);

	  const bool previous_value = (bits_[storage_block_index] & mask) != 0;

	  bits_[storage_block_index] |= mask;

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
	  const size_type storage_block_index = calculate_storage_block_index (
	      bit_pos);
	  const storage_type mask = bit_mask (bit_pos);

	  const bool previous_value = (bits_[storage_block_index] & mask) != 0;

	  bits_[storage_block_index] &= ~mask;

	  return previous_value;
	}

	/**
	 * Returns the number of bits of this bitset.
	 */
	size_type
	size () const
	{
	  return size_;
	}

	/**
	 * Checks if all of the bits are set to true.
	 */
	bool
	all () const
	{
	  if (size () == 0)
	    {
	      return true;
	    }

	  const size_type partial_storage_block_num_bits =
	      calculate_bit_index_mod_storage_width (size ());
	  const storage_type all_ones = all_ones_tmpl<storage_type>::value;
	  const size_type number_of_storage_blocks =
	      calculate_num_storage_blocks ();

	  if (partial_storage_block_num_bits == 0)
	    {
	      // We use each storage block completely.
	      for (size_type i = 0; i < number_of_storage_blocks; ++i)
		{
		  if (bits_[i] != all_ones)
		    {
		      return false;
		    }
		}
	    }
	  else
	    {
	      // First we check whether all storage blocks which we are using completely have all their bits set.
	      for (size_type i = 0; i < (number_of_storage_blocks - 1); ++i)
		{
		  if (bits_[i] != all_ones)
		    {
		      return false;
		    }
		}

	      const storage_type mask = (storage_type (1)
		  << partial_storage_block_num_bits) - 1;
	      if (bits_[number_of_storage_blocks - 1] != mask)
		{
		  return false;
		}
	    }
	  return true;
	}

	/**
	 * Checks if any bits are set to true.
	 */
	bool
	any () const
	{
	  for (size_type i = 0; i < calculate_num_storage_blocks (); ++i)
	    {
	      if (bits_[i])
		return true;
	    }

	  return false;
	}

	/**
	 * Checks if none of the bits are set to true.
	 */
	bool
	none () const
	{
	  return !any ();
	}

	/**
	 * Returns the number of bits that are set to true.
	 */
	size_type
	count () const
	{
	  const size_type partial_storage_block_num_bits =
	      calculate_bit_index_mod_storage_width (size ());
	  const size_type number_of_storage_blocks =
	      calculate_num_storage_blocks ();

	  size_type num_ones = 0;

	  if (partial_storage_block_num_bits == 0)
	    {
	      // We use each storage block completely.
	      for (size_type i = 0; i < number_of_storage_blocks; ++i)
		{
		  num_ones += std::popcount (bits_[i]);
		}
	    }
	  else
	    {
	      // First we check whether all storage blocks which we are using completely have all their bits set.
	      for (size_type i = 0; i < (number_of_storage_blocks - 1); ++i)
		{
		  num_ones += std::popcount (bits_[i]);
		}

	      const storage_type mask = (storage_type (1)
		  << partial_storage_block_num_bits) - 1;

	      num_ones += std::popcount (
		  bits_[number_of_storage_blocks - 1] & mask);
	    }

	  return num_ones;
	}

	/**
	 * Sets the bit at the given position to true.
	 *
	 * @return *this
	 */
	bitset&
	set (size_type bit_pos)
	{
	  const size_type storage_block_index = calculate_storage_block_index (
	      bit_pos);
	  const storage_type mask = bit_mask (bit_pos);

	  bits_[storage_block_index] |= mask;

	  return *this;
	}

	/**
	 * Sets all bits to true.
	 *
	 * @return *this
	 */
	bitset&
	set ()
	{
	  const storage_type all_ones = all_ones_tmpl<storage_type>::value;
	  const size_type number_of_storage_blocks =
	      calculate_num_storage_blocks ();

	  for (size_type i = 0; i < number_of_storage_blocks; ++i)
	    {
	      bits_[i] = all_ones;
	    }

	  set_unused_bits_to_zero (number_of_storage_blocks);

	  return *this;
	}

	/**
	 * Sets the bit at the given position to false.
	 *
	 * @return *this
	 */
	bitset&
	reset (size_type bit_pos)
	{
	  bits_[calculate_storage_block_index (bit_pos)] &= ~bit_mask (bit_pos);

	  return *this;
	}

	/**
	 * Sets all bits to false.
	 *
	 * @return *this
	 */
	bitset&
	reset ()
	{
	  const size_type number_of_storage_blocks =
	      calculate_num_storage_blocks ();
	  const storage_type all_zeros = storage_type (0);

	  for (size_type i = 0; i < number_of_storage_blocks; ++i)
	    {
	      bits_[i] = all_zeros;
	    }

	  return *this;
	}

	/**
	 * Flips the bit at the given position.
	 *
	 * @return *this
	 */
	bitset&
	flip (size_type bit_pos)
	{
	  bits_[calculate_storage_block_index (bit_pos)] ^= bit_mask (bit_pos);

	  return *this;
	}

	/**
	 * Flips all bits (like operator~ but in place).
	 *
	 * @return *this
	 */
	bitset&
	flip ()
	{
	  const size_type number_of_storage_blocks =
	      calculate_num_storage_blocks ();

	  for (size_type i = 0; i < number_of_storage_blocks; ++i)
	    {
	      bits_[i] = ~bits_[i];
	    }

	  set_unused_bits_to_zero (number_of_storage_blocks);

	  return *this;
	}

      private:
	static size_type
	calculate_num_storage_blocks_from_num_bits (size_type num_bits)
	{
	  return num_bits / std::numeric_limits<storage_type>::digits
	      + static_cast<size_type> (num_bits
		  % std::numeric_limits<storage_type>::digits != 0);
	}

	static size_type
	calculate_storage_block_index (size_type bit_pos)
	{
	  return bit_pos / std::numeric_limits<storage_type>::digits;
	}

	static size_type
	calculate_bit_index_mod_storage_width (size_type bit_pos)
	{
	  return static_cast<size_type> (bit_pos
	      % std::numeric_limits<storage_type>::digits);
	}

	static storage_type
	bit_mask (size_type bit_pos)
	{
	  return storage_type (1)
	      << calculate_bit_index_mod_storage_width (bit_pos);
	}

	size_type
	calculate_num_storage_blocks () const
	{
	  return calculate_num_storage_blocks_from_num_bits (size_);
	}

	void
	set_unused_bits_to_zero (size_type number_of_storage_blocks)
	{
	  const size_type partial_storage_block_num_bits =
	      calculate_bit_index_mod_storage_width (size ());

	  if (partial_storage_block_num_bits != 0)
	    {
	      bits_[number_of_storage_blocks - 1] &= (storage_type (1)
		  << partial_storage_block_num_bits) - 1;
	    }
	}

      private:
	static_assert(std::is_fundamental_v<T_FUNDAMENTAL_STORAGE_TYPE>, "The underlying type must be a fundamental type");

	storage_type *bits_;
	size_type size_;
      };

    using bitset_uint64 = bitset<std::uint64_t>;
  }
}

#endif /* DETAIL_BITSET_HPP_ */
