#ifndef DETAIL_COMBINATION_HPP_
#define DETAIL_COMBINATION_HPP_

#include <tuple>
#include <vector>

namespace citcpp
{
  namespace detail
  {
    // Represents a t-way combination as a tuple of a list of parameter indices
    // and value indices.
    // Both, parameter and value indices must be sorted vectors in order to ensure
    // a canonical representation.
    using pv_combination = std::tuple<std::vector<unsigned int>, std::vector<unsigned int>>;

    // This is a custom hash function for combinations, such
    // that they can be used in hash-based containers.
    struct combination_hash
    {
      size_t
      operator() (const pv_combination &combo) const
      {
	std::hash<unsigned int> int_hash;

	std::size_t hash_val = 0;
	const auto &param_indices = std::get<0> (combo);
	const auto &value_indices = std::get<1> (combo);
	for (unsigned int index : param_indices)
	  {
	    hash_val ^= int_hash (index) + 0x9e3779b9 + (hash_val << 6)
		+ (hash_val >> 2);
	  }
	for (unsigned int value : value_indices)
	  {
	    hash_val ^= int_hash (value) + 0x9e3779b9 + (hash_val << 6)
		+ (hash_val >> 2);
	  }
	return hash_val;
      }
    };

    // This is a custom equality operator for combinations, such
    // that they can be used in hash-based containers.
    struct combination_equal
    {
      bool
      operator() (const pv_combination &c1, const pv_combination &c2) const
      {
	return std::get<0> (c1) == std::get<0> (c2)
	    && std::get<1> (c1) == std::get<1> (c2);
      }
    };
  }
}

#endif /* DETAIL_COMBINATION_HPP_ */
