#ifndef CITCPP_CONFIG_HPP_
#define CITCPP_CONFIG_HPP_

namespace citcpp
{
  class covering_array_computation_config
  {
  public:
    covering_array_computation_config () :
	replace_dont_care_values_ (true)
    {
    }

    covering_array_computation_config (
	const covering_array_computation_config &other) = default;

    covering_array_computation_config (
	covering_array_computation_config &&other) = default;

    covering_array_computation_config&
    operator= (const covering_array_computation_config &other) = default;

    covering_array_computation_config&
    operator= (covering_array_computation_config &&other) = default;

    bool
    replace_dont_care_values () const
    {
      return replace_dont_care_values_;
    }

    covering_array_computation_config&
    with_replace_dont_care_values (bool replace_dont_care_values)
    {
      replace_dont_care_values_ = replace_dont_care_values;

      return *this;
    }

  private:
    bool replace_dont_care_values_;
  };
}

#endif /* CITCPP_CONFIG_HPP_ */
