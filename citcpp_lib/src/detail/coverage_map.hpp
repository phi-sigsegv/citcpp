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

class coverage_map_second_level : public bitset_uint64 {
  public:
    coverage_map_second_level() : bitset_uint64() {}

    coverage_map_second_level(size_type num_bits,
                              const param_vector& param_indices)
        : bitset_uint64(num_bits), param_indices_(param_indices) {}

    coverage_map_second_level(const coverage_map_second_level& other)
        : bitset_uint64(other), param_indices_(other.param_indices_) {}

    coverage_map_second_level(coverage_map_second_level&& other)
        : bitset_uint64(std::move(other)),
          param_indices_(std::move(other.param_indices_)) {}

    ~coverage_map_second_level() {}

    coverage_map_second_level& operator=(
        const coverage_map_second_level& other) {
      if (this != &other) {
        bitset_uint64::operator=(other);
        param_indices_ = other.param_indices_;
      }

      return *this;
    }

    coverage_map_second_level& operator=(coverage_map_second_level&& other) {
      if (this != &other) {
        bitset_uint64::operator=(std::move(other));
        param_indices_ = std::move(other.param_indices_);
      }

      return *this;
    }

    /**
     * Swaps this and the given other bitset.
     */
    void swap(coverage_map_second_level& other) {
      bitset_uint64::swap(other);
      std::swap(param_indices_, other.param_indices_);
    }

    const param_vector& get_parameter_indices() const { return param_indices_; }

  private:
    param_vector param_indices_;
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
    typedef std::vector<coverage_map_second_level>::size_type size_type;
    typedef coverage_map_second_level second_level_type;

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

    std::vector<coverage_map_second_level>& get_coverage_map() {
      return cov_map_;
    }

    const std::vector<coverage_map_second_level>& get_coverage_map() const {
      return cov_map_;
    }

    unsigned long long get_total_number_of_tuples() const {
      return total_num_tuples_;
    }

  protected:
    const unsigned long long size_;
    const internal_model& model_;
    const std::vector<unsigned int>& parameter_index_map_;
    const unsigned int n_;
    const unsigned int t_;
    std::vector<coverage_map_second_level> cov_map_;
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
class coverage_map : public coverage_map_base {
    typedef coverage_map_base base_type;

  public:
    coverage_map(unsigned int n, unsigned int t, const internal_model& model,
                 const std::vector<unsigned int>& parameter_index_map,
                 const binom_coeff_table& binomial_coeffs,
                 bool fixed_last_parameter)
        : base_type(n, t, model, parameter_index_map, binomial_coeffs,
                    fixed_last_parameter) {}

    coverage_map(const coverage_map& other) = default;
    coverage_map(coverage_map&& other) = default;

    ~coverage_map() = default;

    coverage_map& operator=(const coverage_map& other) = default;
    coverage_map& operator=(coverage_map&& other) = default;
};

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_COVERAGE_MAP_HPP_ */
