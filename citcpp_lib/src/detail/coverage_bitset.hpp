#ifndef DETAIL_COVERAGE_BITSET_HPP_
#define DETAIL_COVERAGE_BITSET_HPP_

#include <atomic>

#include "bitset.hpp"

namespace citcpp {
namespace detail {

template <typename T_BITSET>
class coverage_bitset_tmpl {
  public:
    typedef std::uint32_t size_type;

    coverage_bitset_tmpl() : bitset_(0), cov_num_ones_(0), valid_num_ones_(0) {}

    coverage_bitset_tmpl(size_type num_bits)
        : bitset_(num_bits << 1), cov_num_ones_(0), valid_num_ones_(0) {}

    coverage_bitset_tmpl(const coverage_bitset_tmpl& other)
        : bitset_(other.bitset_),
          cov_num_ones_(other.cov_num_ones_.load()),
          valid_num_ones_(other.valid_num_ones_.load()) {}

    coverage_bitset_tmpl(coverage_bitset_tmpl&& other)
        : bitset_(std::move(other.bitset_)),
          cov_num_ones_(other.cov_num_ones_.load()),
          valid_num_ones_(other.valid_num_ones_.load()) {}

    ~coverage_bitset_tmpl() {}

    coverage_bitset_tmpl& operator=(const coverage_bitset_tmpl& other) {
      if (this != &other) {
        bitset_ = other.bitset_;
        cov_num_ones_ = other.cov_num_ones_.load();
        valid_num_ones_ = other.valid_num_ones_.load();
      }
      return *this;
    }

    coverage_bitset_tmpl& operator=(coverage_bitset_tmpl&& other) {
      if (this != &other) {
        bitset_ = std::move(other.bitset_);
        cov_num_ones_ = other.cov_num_ones_.load();
        valid_num_ones_ = other.valid_num_ones_.load();
      }
      return *this;
    }

    /**
     * Swaps this and the given other bitset.
     */
    void swap(coverage_bitset_tmpl& other) {
      std::swap(bitset_, other.bitset_);
      size_type this_cov = cov_num_ones_.load();
      cov_num_ones_.store(other.cov_num_ones_.load());
      other.cov_num_ones_.store(this_cov);
      size_type this_valid = valid_num_ones_.load();
      valid_num_ones_.store(other.valid_num_ones_.load());
      other.valid_num_ones_.store(this_valid);
    }

    /**
     * Returns the number of values being tracked.
     */
    size_type size() const {
      const size_type size = bitset_.size() >> 1;
      return size;
    }

    /**
     * Returns the number of values that are marked covered.
     */
    size_type count_covered() const {
      return cov_num_ones_.load(std::memory_order_relaxed);
    }

    /**
     * Checks if all values are marked as covered.
     */
    bool all_covered() const {
      return (cov_num_ones_.load(std::memory_order_relaxed) << 1) ==
             bitset_.size();
    }

    /**
     * Accesses the bit at the given position that represents
     * coverage of a value combination.
     * This method does no range checking. Passing an invalid position
     * results in undefined behavior.
     */
    bool is_marked_covered(size_type bit_pos) const {
      return bitset_.test(bit_pos << 1);
    }

    /**
     * Accesses the bit at the given position that represents
     * coverage of a value combination and sets it to true.
     * This method returns the previous value stored at the
     * position.
     * This method does no range checking. Passing an invalid position
     * results in undefined behavior.
     */
    bool test_and_set_covered(size_type bit_pos) {
      const size_type prev_num_ones = bitset_.count();
      const bool previous_value = bitset_.test_and_set(bit_pos << 1);
      if (bitset_.count() > prev_num_ones) {
        cov_num_ones_.fetch_add(1, std::memory_order_relaxed);
      }
      return previous_value;
    }

    /**
     * Accesses the bit at the given position that represents
     * coverage of a value combination and sets it to true.
     * This method does no range checking. Passing an invalid position
     * results in undefined behavior.
     */
    void set_covered(size_type bit_pos) {
      const size_type prev_num_ones = bitset_.count();
      bitset_.set(bit_pos << 1);
      if (bitset_.count() > prev_num_ones) {
        cov_num_ones_.fetch_add(1, std::memory_order_relaxed);
      }
    }

    /**
     * Returns the number of values that are marked valid.
     */
    size_type count_valid() const {
      return valid_num_ones_.load(std::memory_order_relaxed);
    }

    /**
     * Checks if all values are marked as valid.
     */
    bool all_valid() const {
      return (valid_num_ones_.load(std::memory_order_relaxed) << 1) ==
             bitset_.size();
    }

    /**
     * Accesses the bit at the given position that represents
     * validity of a value combination.
     * This method does no range checking. Passing an invalid position
     * results in undefined behavior.
     */
    bool is_valid(size_type bit_pos) const {
      return bitset_.test((bit_pos << 1) + 1);
    }

    /**
     * Sets the bit at the given position that represents
     * validity of a value combination.
     * This method does no range checking. Passing an invalid position
     * results in undefined behavior.
     */
    void set_valid(size_type bit_pos) {
      const size_type prev_num_ones = bitset_.count();
      bitset_.set((bit_pos << 1) + 1);
      if (bitset_.count() > prev_num_ones) {
        valid_num_ones_.fetch_add(1, std::memory_order_relaxed);
      }
    }

    /**
     * Sets the bits for all value combinations to valid.
     */
    void set_all_valid() {
      const size_type size = this->size();
      for (size_type i = 0; i < size; ++i) {
        set_valid(i);
      }
    }

    /**
     * Returns a reference to the underlying bitset.
     */
    const T_BITSET& get_bitset() const { return bitset_; }

    /**
     * Returns a reference to the underlying bitset.
     */
    T_BITSET& get_bitset() { return bitset_; }

  private:
    T_BITSET bitset_;
    std::atomic<size_type> cov_num_ones_;
    std::atomic<size_type> valid_num_ones_;
};

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_COVERAGE_BITSET_HPP_ */
