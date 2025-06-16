#include <ranges>
#include "ipog_algorithm_uniform_strength.hpp"
#include "for_each_cross_product_elem.hpp"

namespace citcpp
{
  namespace detail
  {
    create_all_value_combinations_result
    create_all_value_combinations (
	unsigned int t, const model &model,
	const std::vector<unsigned int> &parameter_indices,
	citcpp::detail::test_set &test_set)
    {
      auto l_map_to_param_idx = [&parameter_indices]
      (int idx)
	{ return parameter_indices[idx];};

      auto l_map_to_num_param_values = [&model]
      (unsigned int param_idx)
	{ return model.get_parameters()[param_idx];};

      auto r_param_num_values = std::ranges::iota_view
	{ 0u, t } | std::views::transform (l_map_to_param_idx)
	  | std::views::transform (l_map_to_num_param_values);

      std::vector<unsigned int> param_num_values (r_param_num_values.begin (),
						  r_param_num_values.end ());

      create_all_value_combinations_result result
	{ 0 };

      for_each_cross_product_elem (
	  param_num_values,
	  [&model, &parameter_indices, &test_set, &result]
	  (const std::vector<unsigned int> &next_cross_product_elem)
	    {
	      // Initialize all values of the test with don't care.
	      test t(model.get_parameters().size(), -1);
	      t.set_num_dont_care_values(t.get_values().size());

	      // Replace the first t elements with the cross product element.
	      for (unsigned int index = 0; index < next_cross_product_elem.size(); ++index)
		{
		  t.get_values()[parameter_indices[index]] = next_cross_product_elem[index];
		}
	      t.set_num_dont_care_values(t.get_num_dont_care_values() - next_cross_product_elem.size());

	      test_set.get_list_of_tests().push_back(std::move(t));

	      ++result.num_created_combinations;
	    });

      return result;
    }
  }
}
