#ifndef DETAIL_COVERAGE_MAP_HPP_
#define DETAIL_COVERAGE_MAP_HPP_

#include <algorithm>
#include <citcpp/function_ref.hpp>
#include <vector>

#include "binom_coeff_table.hpp"
#include "bitset.hpp"
#include "datatypes_config.hpp"
#include "functor_executor.hpp"
#include "internal_model.hpp"
#include "shared_constants.hpp"

namespace citcpp {
namespace detail {

class coverage_map_second_level {
  public:
    typedef std::uint32_t size_type;

    coverage_map_second_level()
        : bitset_(0), param_indices_(), cov_num_ones_(0) {}

    coverage_map_second_level(size_type num_bits,
                              const param_vector& param_indices)
        : bitset_(num_bits << 1),
          param_indices_(param_indices),
          cov_num_ones_(0) {}

    coverage_map_second_level(const coverage_map_second_level& other) = default;
    coverage_map_second_level(coverage_map_second_level&& other) = default;

    ~coverage_map_second_level() {}

    coverage_map_second_level& operator=(
        const coverage_map_second_level& other) = default;
    coverage_map_second_level& operator=(coverage_map_second_level&& other) =
        default;

    /**
     * Swaps this and the given other bitset.
     */
    void swap(coverage_map_second_level& other) {
      std::swap(bitset_, other.bitset_);
      std::swap(param_indices_, other.param_indices_);
      std::swap(cov_num_ones_, other.cov_num_ones_);
    }

    /**
     * Checks if all values are marked as covered.
     */
    bool all_covered() const { return (cov_num_ones_ << 1) == bitset_.size(); }

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
        ++cov_num_ones_;
      }
      return previous_value;
    }

    const param_vector& get_parameter_indices() const { return param_indices_; }

  private:
    bitset_uint64 bitset_;
    param_vector param_indices_;
    size_type cov_num_ones_;
};

/**
 * This is a quite central data structure in combinatorial testing tools.
 * It keeps track of the coverage of the parameter combinations and their cross
 * product of value combinations.
 * Since the number of value combinations for all t-way combinations of
 * parameters can be quite huge, this data structure is optimized for efficient
 * memory representation. At the same time, operations on the data structure
 * need to be lighting fast, again due to the vast amount of t-tuples whose
 * coverage to track.
 *
 * The coverage map is able to keep track of tuple coverage
 * of t-wise combinations from n parameters (indices [0, ... ,n-1]).
 * Depending on the value of the parameter \a fixed_last_parameter, the last
 * parameter is fixed. Or in other words: We select combinations of length t-1
 * from the parameters [0, ... ,n-2], and extend those combinations by always
 * appending parameter n-1 to them.
 */
class coverage_map_base {
  public:
    coverage_map_base(unsigned int n, unsigned int t,
                      const internal_model& model,
                      const std::vector<unsigned int>& parameter_index_map,
                      const binom_coeff_table& binomial_coeffs,
                      bool fixed_last_parameter);

    coverage_map_base(const coverage_map_base& other) = default;
    coverage_map_base(coverage_map_base&& other) = default;

    ~coverage_map_base() = default;

    coverage_map_base& operator=(const coverage_map_base& other) = default;
    coverage_map_base& operator=(coverage_map_base&& other) = default;

    const internal_model& get_model() const { return model_; }

    const std::vector<unsigned int>& get_parameter_index_map() const {
      return parameter_index_map_;
    }

    unsigned int get_number_of_parameters_to_select_from() const { return n_; }

    unsigned int get_number_of_parameters_to_select() const { return t_; }

    unsigned long long get_total_number_of_tuples() const {
      return total_num_tuples_;
    }

  protected:
    const unsigned long long size_;
    const internal_model& model_;
    const std::vector<unsigned int>& parameter_index_map_;
    const unsigned int n_;
    const unsigned int t_;
    unsigned long long total_num_tuples_;
};

/**
 * This is a quite central data structure in combinatorial testing tools.
 * It keeps track of the coverage of the parameter combinations and their cross
 * product of value combinations.
 * Since the number of value combinations for all t-way combinations of
 * parameters can be quite huge, this data structure is optimized for efficient
 * memory representation. At the same time, operations on the data structure
 * need to be lighting fast, again due to the vast amount of t-tuples whose
 * coverage to track.
 *
 * The coverage map is able to keep track of tuple coverage
 * of t-wise combinations from n parameters (indices [0, ... ,n-1]).
 * Depending on the value of the parameter \a fixed_last_parameter, the last
 * parameter is fixed. Or in other words: We select combinations of length t-1
 * from the parameters [0, ... ,n-2], and extend those combinations by always
 * appending parameter n-1 to them.
 */
template <typename T_SECOND_LEVEL_TYPE>
class coverage_map_tmpl : public coverage_map_base {
    typedef coverage_map_base base_type;

  public:
    typedef std::vector<T_SECOND_LEVEL_TYPE>::size_type size_type;
    typedef T_SECOND_LEVEL_TYPE second_level_type;

    coverage_map_tmpl(unsigned int n, unsigned int t,
                      const internal_model& model,
                      const std::vector<unsigned int>& parameter_index_map,
                      const binom_coeff_table& binomial_coeffs,
                      bool fixed_last_parameter);

    coverage_map_tmpl(const coverage_map_tmpl& other) = default;
    coverage_map_tmpl(coverage_map_tmpl&& other) = default;

    ~coverage_map_tmpl() = default;

    coverage_map_tmpl& operator=(const coverage_map_tmpl& other) = default;
    coverage_map_tmpl& operator=(coverage_map_tmpl&& other) = default;

    std::vector<second_level_type>& get_coverage_map() { return cov_map_; }

    const std::vector<second_level_type>& get_coverage_map() const {
      return cov_map_;
    }

  protected:
    std::vector<second_level_type> cov_map_;
};

typedef coverage_map_tmpl<coverage_map_second_level> ipog_coverage_map;

}  // namespace detail
}  // namespace citcpp

#include "coverage_map.tpp"

#endif /* DETAIL_COVERAGE_MAP_HPP_ */
