#include "binom_coeff_table.hpp"

namespace
{
  /**
   * @brief Initializes and returns a 2D vector containing precomputed binomial coefficients C(n, k).
   *
   * This function builds Pascal's triangle up to 'max_n' to allow for O(1) lookups
   * of binomial coefficients later. It's designed to be called once.
   *
   * @param max_n The maximum 'n' value for which binomial coefficients should be precomputed.
   * @return A std::vector<std::vector<long long>> where coeffs[n][k] stores C(n, k).
   */
  std::vector<std::vector<long long>>
  initialize_binomial_coefficients (unsigned int max_n)
  {
    // Resize the outer vector to hold up to max_n + 1 rows (for 0 to max_n)
    std::vector<std::vector<long long>> coeffs (max_n + 1);
    for (unsigned int i = 0; i <= max_n; ++i)
      {
	// Resize the inner vector for row 'i' to hold 'i + 1' elements (for 0 to i)
	coeffs[i].resize (i + 1);
	coeffs[i][0] = 1; // C(i, 0) is always 1
	if (i > 0)
	  {
	    coeffs[i][i] = 1; // C(i, i) is always 1
	  }
	// Calculate intermediate coefficients using the Pascal's identity: C(n, k) = C(n-1, k-1) + C(n-1, k)
	for (unsigned int j = 1; j < i; ++j)
	  {
	    coeffs[i][j] = coeffs[i - 1][j - 1] + coeffs[i - 1][j];
	    // Basic overflow check: if the sum overflows, it might become negative.
	    // Mark it as -1 to indicate overflow, though for typical N, K in combinatorial testing,
	    // long long is usually sufficient.
	    if (coeffs[i][j] < 0)
	      {
		coeffs[i][j] = -1; // Indicate overflow
	      }
	  }
      }
    return coeffs;
  }
}

namespace citcpp
{
  namespace detail
  {
    binom_coeff_table::binom_coeff_table (unsigned int max_n) :
	binom_coeffs_ (initialize_binomial_coefficients (max_n))
    {
    }

    long long
    binom_coeff_table::get_coefficient (unsigned int n, unsigned int k) const
    {
      return binom_coeffs_[n][k];
    }

    unsigned int
    binom_coeff_table::get_max_n () const
    {
      return binom_coeffs_.size ();
    }
  }
}
