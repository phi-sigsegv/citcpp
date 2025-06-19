#include <algorithm>
#include "coverage_map.hpp"

namespace citcpp
{
  namespace detail
  {
    coverage_map::coverage_map (unsigned int n, unsigned int t,
				const binom_coeff_table &binomial_coeffs) :
	cov_map_ (binomial_coeffs.get_coefficient (n, t))
    {
    }

    void
    coverage_map::swap (coverage_map &other)
    {
      std::swap (cov_map_, other.cov_map_);
    }

    coverage_map::first_level_type&
    coverage_map::get_coverage_map ()
    {
      return cov_map_;
    }

    const coverage_map::first_level_type&
    coverage_map::get_coverage_map () const
    {
      return cov_map_;
    }
  }
}
