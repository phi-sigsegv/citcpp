#ifndef DETAIL_COVERAGE_MAP_HPP_
#define DETAIL_COVERAGE_MAP_HPP_

#include <vector>
#include "bitset.hpp"
#include "binom_coeff_table.hpp"

namespace citcpp
{
  namespace detail
  {
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

      std::vector<bitset_uint64>&
      get_coverage_map ();

      const std::vector<bitset_uint64>&
      get_coverage_map () const;

    private:
      std::vector<bitset_uint64> cov_map_;
    };
  }
}

#endif /* DETAIL_COVERAGE_MAP_HPP_ */
