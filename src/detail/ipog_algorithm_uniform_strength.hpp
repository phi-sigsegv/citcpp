#ifndef DETAIL_IPOG_ALGORITHM_UNIFORM_STRENGTH_HPP_
#define DETAIL_IPOG_ALGORITHM_UNIFORM_STRENGTH_HPP_

#include "internal_model.hpp"
#include "coverage_map.hpp"

namespace citcpp
{
  namespace detail
  {
    /**
     * This represents the result of the execution of function
     * create_all_value_combinations.
     */
    struct create_all_value_combinations_result
    {
      /**
       * The number of elements of the created cartesian product.
       */
      unsigned long long num_created_combinations;
    };

    /**
     * Create the full cartesian product of the values of t parameters from
     * the model and write them to the given test set.
     * The given parameter_indices specify the parameters whose values to combine.
     * The algorithm returns the number of elements of the created cartesian product.
     */
    create_all_value_combinations_result
    create_all_value_combinations (
	unsigned int t, const model &model,
	const std::vector<unsigned int> &parameter_indices,
	citcpp::detail::test_set &test_set);

    struct ipog_horizontal_extension_result
    {
      std::vector<list_intrusive<test>> value_to_row_mapping;
      unsigned long long num_new_covered_tuples;
    };

    ipog_horizontal_extension_result
    ipog_horizontal_extension (
	unsigned int current_param_idx, unsigned int num_params, unsigned int t,
	const model &model,
	const std::vector<unsigned int> &parameter_index_map,
	test_set &test_set, coverage_map &cov_map);
  }
}

#endif /* DETAIL_IPOG_ALGORITHM_UNIFORM_STRENGTH_HPP_ */
