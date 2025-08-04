#ifndef CITCPP_CONFIG_HPP_
#define CITCPP_CONFIG_HPP_

namespace citcpp {

class covering_array_computation_config {
  public:
    covering_array_computation_config()
        : replace_dont_care_values_(true), multithreading_enabled_(true) {}

    bool replace_dont_care_values() const { return replace_dont_care_values_; }

    bool multithreading_enabled() const { return multithreading_enabled_; }

    covering_array_computation_config& with_replace_dont_care_values(
        bool replace_dont_care_values) {
      replace_dont_care_values_ = replace_dont_care_values;

      return *this;
    }

    covering_array_computation_config& with_multithreading_enabled(
        bool multithreading_enabled) {
      multithreading_enabled_ = multithreading_enabled;

      return *this;
    }

  private:
    bool replace_dont_care_values_;
    bool multithreading_enabled_;
};

}  // namespace citcpp

#endif /* CITCPP_CONFIG_HPP_ */
