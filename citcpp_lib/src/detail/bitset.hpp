#ifndef DETAIL_BITSET_HPP_
#define DETAIL_BITSET_HPP_

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace citcpp {
namespace detail {

template <typename T>
struct all_ones_tmpl {
    static constexpr T value = static_cast<T>(-1);
};

template <typename T_FUNDAMENTAL_STORAGE_TYPE>
class bitset_tmpl {
  public:
    typedef T_FUNDAMENTAL_STORAGE_TYPE storage_type;
    typedef std::uint32_t size_type;

  public:
    bitset_tmpl() : bits_(nullptr), size_(0), num_ones_(0) {}

    bitset_tmpl(size_type num_bits)
        : bits_(new storage_type[calculate_num_storage_blocks_from_num_bits(
              num_bits)]{}),
          size_(num_bits),
          num_ones_(0) {}

    bitset_tmpl(const bitset_tmpl& other)
        : bits_(new storage_type[calculate_num_storage_blocks_from_num_bits(
              other.size_)]{}),
          size_(other.size_),
          num_ones_(other.num_ones_) {

      std::memcpy(bits_, other.bits_,
                  calculate_num_storage_blocks_from_num_bits(size_) *
                      sizeof(storage_type));
    }

    bitset_tmpl(bitset_tmpl&& other)
        : bits_(other.bits_), size_(other.size_), num_ones_(other.num_ones_) {
      other.bits_ = nullptr;
      other.size_ = 0;
      other.num_ones_ = 0;
    }

    ~bitset_tmpl() { delete[] bits_; }

    bitset_tmpl& operator=(const bitset_tmpl& other) {
      if (&other != this) {
        delete[] bits_;
        size_ = other.size_;
        size_type num_storage_blocks =
            calculate_num_storage_blocks_from_num_bits(size_);
        bits_ = new storage_type[num_storage_blocks];
        std::memcpy(bits_, other.bits_,
                    num_storage_blocks * sizeof(storage_type));
        num_ones_ = other.num_ones_;
      }

      return *this;
    }

    bitset_tmpl& operator=(bitset_tmpl&& other) {
      if (&other != this) {
        delete[] bits_;
        bits_ = other.bits_;
        size_ = other.size_;
        num_ones_ = other.num_ones_;
        other.bits_ = nullptr;
        other.size_ = 0;
        other.num_ones_ = 0;
      }

      return *this;
    }

    /**
     * Swaps this and the given other bitset.
     */
    void swap(bitset_tmpl& other) {
      std::swap(bits_, other.bits_);
      std::swap(size_, other.size_);
      std::swap(num_ones_, other.num_ones_);
    }

    /**
     * Accesses the bit at the given position.
     * This method does no range checking. Passing an invalid position
     * results in undefined behavior.
     */
    bool test(size_type bit_pos) const {
      return (this->bits_[calculate_storage_block_index(bit_pos)] &
              bit_mask(bit_pos)) != 0;
    }

    /**
     * Accesses the bit at the given position.
     * This method does no range checking. Passing an invalid position
     * results in undefined behavior.
     */
    bool operator[](size_type bit_pos) const { return test(bit_pos); }

    /**
     * Accesses the bit at the given position.
     * Unlike operator[] and test(), this method performs a range check and
     * throws std::out_of_range if the given position is not valid.
     */
    bool at(size_type bit_pos) const {
      if (bit_pos >= this->size_) {
        throw std::out_of_range("bitset::at out_of_range");
      }

      return test(bit_pos);
    }

    /**
     * Accesses the bit at the given position and sets it to true.
     * This method returns the previous value stored at the
     * position.
     * This method does no range checking. Passing an invalid position
     * results in undefined behavior.
     */
    bool test_and_set(size_type bit_pos) {
      const size_type storage_block_index =
          calculate_storage_block_index(bit_pos);
      const storage_type mask = bit_mask(bit_pos);

      const bool previous_value =
          (this->bits_[storage_block_index] & mask) != 0;
      if (!previous_value) {
        ++num_ones_;
      }

      this->bits_[storage_block_index] |= mask;

      return previous_value;
    }

    /**
     * Accesses the bit at the given position and sets it to false.
     * This method returns the previous value stored at the
     * position.
     * This method does no range checking. Passing an invalid position
     * results in undefined behavior.
     */
    bool test_and_reset(size_type bit_pos) {
      const size_type storage_block_index =
          calculate_storage_block_index(bit_pos);
      const storage_type mask = bit_mask(bit_pos);

      const bool previous_value =
          (this->bits_[storage_block_index] & mask) != 0;
      if (previous_value) {
        --num_ones_;
      }

      this->bits_[storage_block_index] &= ~mask;

      return previous_value;
    }

    /**
     * Returns the number of bits of this bitset.
     */
    size_type size() const { return this->size_; }

    /**
     * Checks if all of the bits are set to true.
     */
    bool all() const { return num_ones_ == size(); }

    /**
     * Checks if any bits are set to true.
     */
    bool any() const { return num_ones_ > 0; }

    /**
     * Checks if none of the bits are set to true.
     */
    bool none() const { return num_ones_ == 0; }

    /**
     * Returns the number of bits that are set to true.
     */
    size_type count() const { return num_ones_; }

    /**
     * Sets the bit at the given position to true.
     */
    void set(size_type bit_pos) {
      const size_type storage_block_index =
          calculate_storage_block_index(bit_pos);
      const storage_type mask = bit_mask(bit_pos);

      const bool previous_value =
          (this->bits_[storage_block_index] & mask) != 0;
      if (!previous_value) {
        ++num_ones_;
      }

      this->bits_[storage_block_index] |= mask;
    }

    /**
     * Sets all bits to true.
     */
    void set() {
      const storage_type all_ones = all_ones_tmpl<storage_type>::value;
      const size_type number_of_storage_blocks = calculate_num_storage_blocks();

      for (size_type i = 0; i < number_of_storage_blocks; ++i) {
        this->bits_[i] = all_ones;
      }

      num_ones_ = size();
    }

    /**
     * Sets the bit at the given position to false.
     */
    void reset(size_type bit_pos) {
      const size_type storage_block_index =
          calculate_storage_block_index(bit_pos);
      const storage_type mask = bit_mask(bit_pos);

      const bool previous_value =
          (this->bits_[storage_block_index] & mask) != 0;
      if (previous_value) {
        --num_ones_;
      }

      this->bits_[storage_block_index] &= ~mask;
    }

    /**
     * Sets all bits to false.
     */
    void reset() {
      const size_type number_of_storage_blocks = calculate_num_storage_blocks();
      const storage_type all_zeros = storage_type(0);

      for (size_type i = 0; i < number_of_storage_blocks; ++i) {
        this->bits_[i] = all_zeros;
      }

      num_ones_ = 0;
    }

    /**
     * Sets a new size for this bitset and resets its state.
     * Care must be taken to not set a size greater than the size
     * of the underlying storage.
     */
    void reset_with_new_size(size_type size) {
      size_ = size;
      reset();
    }

  private:
    static size_type calculate_num_storage_blocks_from_num_bits(
        size_type num_bits) {
      return num_bits / std::numeric_limits<storage_type>::digits +
             static_cast<size_type>(
                 num_bits % std::numeric_limits<storage_type>::digits != 0);
    }

    static size_type calculate_storage_block_index(size_type bit_pos) {
      return bit_pos / std::numeric_limits<storage_type>::digits;
    }

    static size_type calculate_bit_index_mod_storage_width(size_type bit_pos) {
      return static_cast<size_type>(bit_pos %
                                    std::numeric_limits<storage_type>::digits);
    }

    static storage_type bit_mask(size_type bit_pos) {
      return storage_type(1) << calculate_bit_index_mod_storage_width(bit_pos);
    }

    size_type calculate_num_storage_blocks() const {
      return calculate_num_storage_blocks_from_num_bits(this->size_);
    }

  private:
    static_assert(std::is_fundamental_v<T_FUNDAMENTAL_STORAGE_TYPE>,
                  "The underlying type must be a fundamental type");

    storage_type* bits_;
    size_type size_;
    size_type num_ones_;
};

using bitset_uint64 = bitset_tmpl<std::uint64_t>;

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_BITSET_HPP_ */
