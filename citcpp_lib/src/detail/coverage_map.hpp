#ifndef DETAIL_COVERAGE_MAP_HPP_
#define DETAIL_COVERAGE_MAP_HPP_

#include <cstdint>
#include <memory>
#include <vector>

#include "binom_coeff_table.hpp"
#include "bitset.hpp"
#include "datatypes_config.hpp"
#include "internal_model.hpp"
#include "internal_test_set.hpp"

namespace citcpp {
namespace detail {

class ipog_coverage_map;

/**
 * The state shared by an ipog_coverage_map and the views on its parameter
 * combinations. It is heap-allocated, so that its address stays stable when
 * the coverage map is moved.
 */
struct ipog_coverage_map_storage {
    // Coverage and validity bits of all tuples, in the physical layout
    // described at ipog_coverage_map (plus one extra word, so that 64-bit
    // windows can be read and written at any bit position).
    std::vector<std::uint64_t> covered;
    std::vector<std::uint64_t> valid;
    // Number of tuples per value of the fixed last parameter.
    std::uint64_t tuples_per_value = 0;
    // Per parameter combination and per earlier parameter of it (in ascending
    // position): the domain size and the physical index weight of its value.
    std::vector<std::uint32_t> combo_domain_sizes;
    std::vector<std::uint64_t> combo_value_weights;
};

/**
 * One parameter combination of an ipog_coverage_map: t-1 earlier parameters
 * plus the fixed last parameter. Its tuples are addressed by a logical index:
 * the parameters' values in radix notation, with the first parameter most
 * significant and the last parameter least significant. (This is the order in
 * which the combinations and their tuples are visited, e.g. by the vertical
 * extension.) The logical index is mapped to the physical layout internally.
 */
class ipog_coverage_map_param_combo {
  public:
    typedef std::uint32_t size_type;

    const param_vector& get_parameter_indices() const { return param_indices_; }

    /**
     * Returns the number of tuples of this parameter combination.
     */
    size_type size() const { return size_; }

    /**
     * Returns the number of tuples that are marked covered.
     */
    size_type count_covered() const { return num_covered_; }

    /**
     * Checks if all tuples are marked as covered.
     */
    bool all_covered() const { return num_covered_ == size_; }

    bool is_marked_covered(size_type logical_index) const {
      const std::uint64_t i = physical_index(logical_index);
      return (storage_->covered[i >> 6] >> (i & 63)) & 1;
    }

    /**
     * Marks the tuple as covered and returns whether it was covered before.
     */
    bool test_and_set_covered(size_type logical_index) {
      const std::uint64_t i = physical_index(logical_index);
      const std::uint64_t mask = std::uint64_t(1) << (i & 63);
      std::uint64_t& word = storage_->covered[i >> 6];
      if (word & mask) {
        return true;
      }
      word |= mask;
      ++num_covered_;
      return false;
    }

    bool is_valid(size_type logical_index) const {
      const std::uint64_t i = physical_index(logical_index);
      return (storage_->valid[i >> 6] >> (i & 63)) & 1;
    }

    void set_valid(size_type logical_index) {
      const std::uint64_t i = physical_index(logical_index);
      storage_->valid[i >> 6] |= std::uint64_t(1) << (i & 63);
    }

  private:
    friend class ipog_coverage_map;

    std::uint64_t physical_index(size_type logical_index) const {
      const std::size_t s = param_indices_.size() - 1;
      const std::uint32_t* domain_sizes =
          storage_->combo_domain_sizes.data() + first_element_;
      const std::uint64_t* weights =
          storage_->combo_value_weights.data() + first_element_;
      std::uint64_t rest = logical_index;
      const std::uint64_t last_value = rest % last_domain_size_;
      rest /= last_domain_size_;
      std::uint64_t index = base_ + last_value * storage_->tuples_per_value;
      for (std::size_t k = s; k-- > 0;) {
        index += (rest % domain_sizes[k]) * weights[k];
        rest /= domain_sizes[k];
      }
      return index;
    }

    ipog_coverage_map_storage* storage_ = nullptr;
    param_vector param_indices_;
    // Physical index of the tuple with all values 0.
    std::uint64_t base_ = 0;
    // Offset of this combination's entries in the storage's combo arrays.
    std::uint64_t first_element_ = 0;
    std::uint32_t last_domain_size_ = 1;
    size_type size_ = 0;
    size_type num_covered_ = 0;
};

/**
 * This is a quite central data structure in combinatorial testing tools.
 * It keeps track of the coverage of the parameter combinations and their cross
 * product of value combinations (tuples), for the IPOG algorithm: of the
 * parameters at positions [0, ... ,n-1] of a relation, it tracks the
 * combinations of t-1 parameters from [0, ... ,n-2] (the "earlier" ones),
 * each extended by parameter n-1 (the "last" one, the one being added by
 * IPOG).
 *
 * The combinations are available in a fixed order as views
 * (get_coverage_map()), for per-combination access. The hot operations of the
 * horizontal extension (add_gains(), cover()) work on the whole map at once.
 *
 * Physical layout (a mixed-radix combinatorial number system over the earlier
 * parameters): let d_q be the domain size of the parameter at position q, and
 * E_k(j) the number of k-tuples (k parameters with one value each) over the
 * positions [0, j). A tuple with earlier positions i_1 < ... < i_{t-1}, values
 * x_1, ..., x_{t-1}, and value v of the last parameter has the index
 *
 *   v * E_{t-1}(n-1) + sum_k ( E_k(i_k) + x_k * E_{k-1}(i_k) ).
 *
 * The k = 1 term, E_1(i) + x = off(i) + x, is the "value slot" of (i, x).
 * Fixing v and the positions and values of all earlier parameters but the
 * smallest one leaves the tuples for all slots of the positions below the
 * second smallest one, which are contiguous: a "row". A test is represented by
 * the bitset of its value slots, so that the tuples of a row that are
 * contained in the test are found by one word operation per 64 slots.
 */
class ipog_coverage_map {
  public:
    typedef std::vector<ipog_coverage_map_param_combo>::size_type size_type;
    typedef ipog_coverage_map_param_combo second_level_type;

    /**
     * Creates the coverage map for the parameters at positions [0, n-1] of
     * the given parameter index map and strength t. The last parameter must be
     * fixed (fixed_last_parameter = true). The combinations are ordered as the
     * original IPOG implementation enumerated them.
     */
    ipog_coverage_map(unsigned int n, unsigned int t,
                      const internal_model& model,
                      const std::vector<unsigned int>& parameter_index_map,
                      const binom_coeff_table& binomial_coeffs,
                      bool fixed_last_parameter);

    ipog_coverage_map(const ipog_coverage_map& other) = delete;
    ipog_coverage_map(ipog_coverage_map&& other) noexcept = default;
    ~ipog_coverage_map() = default;
    ipog_coverage_map& operator=(const ipog_coverage_map& other) = delete;
    ipog_coverage_map& operator=(ipog_coverage_map&& other) = delete;

    const internal_model& get_model() const { return *model_; }

    const std::vector<unsigned int>& get_parameter_index_map() const {
      return *parameter_index_map_;
    }

    unsigned int get_number_of_parameters_to_select_from() const { return n_; }

    unsigned int get_number_of_parameters_to_select() const { return t_; }

    unsigned long long get_total_number_of_tuples() const {
      return total_num_tuples_;
    }

    std::vector<second_level_type>& get_coverage_map() { return combos_; }

    const std::vector<second_level_type>& get_coverage_map() const {
      return combos_;
    }

    /**
     * Adds to gain_per_value[v], for every value v of the last parameter that
     * is set in valid_values, the number of uncovered tuples with value v
     * whose earlier values are all assigned in the given test.
     */
    void add_gains(const test& t, const bitset_uint64& valid_values,
                   std::vector<unsigned long long>& gain_per_value) const;

    /**
     * Marks the tuples with the given value of the last parameter whose earlier
     * values are all assigned in the given test as covered, and the tuples
     * with any value set in valid_values as valid. Returns the number of
     * tuples that were not covered before.
     */
    unsigned long long cover(const test& t, int last_value,
                             const bitset_uint64& valid_values);

  private:
    // Builds the slot bitset of the test (see the class comment) in
    // test_slots_, and the list of earlier positions with assigned values in
    // assigned_positions_.
    void load_test(const test& t) const;

    // Calls f(row_base, row_length, rank_base) for every row of the loaded
    // test, where row_base is the physical index of the row's first slot for
    // value 0 of the last parameter, and a tuple of the row in a slot of
    // position i belongs to the combination with index rank_base - i.
    template <typename F>
    void for_each_row(F&& f) const;

    const internal_model* model_;
    const std::vector<unsigned int>* parameter_index_map_;
    const binom_coeff_table* binomial_coeffs_;
    unsigned int n_;
    unsigned int t_;
    unsigned long long total_num_tuples_ = 0;
    std::unique_ptr<ipog_coverage_map_storage> storage_;
    std::vector<second_level_type> combos_;

    // E_[k][j]: number of k-tuples over the earlier positions [0, j).
    std::vector<std::vector<std::uint64_t>> E_;
    // Value slot -> earlier position.
    std::vector<std::uint32_t> slot_position_;
    unsigned int last_domain_size_ = 1;

    // Scratch data of load_test().
    mutable std::vector<std::uint64_t> test_slots_;
    mutable std::vector<int> test_values_;
    mutable std::vector<unsigned int> assigned_positions_;
};

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_COVERAGE_MAP_HPP_ */
