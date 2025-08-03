#ifndef DETAIL_BINOM_COEFF_TABLE_HPP_
#define DETAIL_BINOM_COEFF_TABLE_HPP_

#include <vector>

namespace citcpp
{
  namespace detail
  {
    /**
     * This class encapsulates a table of binomial coefficients C(n, k) up to
     * a maximum n. The purpose is to allows for a fast O(1) lookup of these.
     */
    class binom_coeff_table
    {
    public:
      binom_coeff_table (unsigned int max_n);

      /**
       * Returns the binomial coefficient C(n, k).
       */
      long long
      get_coefficient (unsigned int n, unsigned int k) const;

      /**
       * Returns the maximum n for which this table contains coefficients.
       */
      unsigned int
      get_max_n () const;

    private:
      std::vector<std::vector<long long>> binom_coeffs_;
    };
  }
}

#endif /* DETAIL_BINOM_COEFF_TABLE_HPP_ */
