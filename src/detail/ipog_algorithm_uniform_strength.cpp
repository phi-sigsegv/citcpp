#include <ranges>
#include "ipog_algorithm_uniform_strength.hpp"
#include "for_each_cross_product_elem.hpp"

namespace
{
  unsigned int
  ipog_horizontal_select_best_value (
      unsigned int current_param_idx, unsigned int strength,
      const citcpp::detail::model &model,
      const std::vector<unsigned int> &parameter_index_map,
      const citcpp::detail::test &test,
      const citcpp::detail::coverage_map &cov_map)
  {
    using namespace citcpp::detail;
  }

  unsigned long long
  ipog_horizontal_update_coverage_map (
      unsigned int current_param_idx, unsigned int strength,
      const citcpp::detail::model &model,
      const std::vector<unsigned int> &parameter_index_map,
      const citcpp::detail::test &test, citcpp::detail::coverage_map &cov_map)
  {
    using namespace citcpp::detail;
  }
}

namespace citcpp
{
  namespace detail
  {
    create_all_value_combinations_result
    create_all_value_combinations (
	unsigned int strength, const model &model,
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
	{ 0u, strength } | std::views::transform (l_map_to_param_idx)
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

    ipog_horizontal_extension_result
    ipog_horizontal_extension (
	unsigned int current_param_idx, unsigned int strength,
	const model &model,
	const std::vector<unsigned int> &parameter_index_map,
	test_set &test_set, coverage_map &cov_map)
    {
      // First initialize the

      ipog_horizontal_extension_result result
	{ std::vector<list_intrusive<test>> (
	    model.get_parameters ()[parameter_index_map[current_param_idx]]), 0 };

      for (test &t : test_set.get_list_of_tests ())
	{
	  unsigned int selected_value = ipog_horizontal_select_best_value (
	      current_param_idx, strength, model, parameter_index_map, t,
	      cov_map);

	  // Now that we have selected the value with most coverage, we set it in the
	  // test accordingly.
	  t.set_num_dont_care_values (t.get_num_dont_care_values () - 1);
	  t.get_values ()[current_param_idx] = selected_value;

	  // The last step consists in updating the coverage map.
	  unsigned long long num_new_covered_tuples =
	      ipog_horizontal_update_coverage_map (current_param_idx, strength,
						   model, parameter_index_map,
						   t, cov_map);

	  // Keep track of how many tuples we have covered in addition.
	  result.num_new_covered_tuples += num_new_covered_tuples;

	  // Maintain a mapping from values of the current parameter to the tests.
	  result.value_to_row_mapping[selected_value].push_back (t);
	}

      return result;
    }
  }
}
