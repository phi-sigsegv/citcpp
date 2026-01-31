#ifndef CITCPP_CONFIG_HPP_
#define CITCPP_CONFIG_HPP_

#include <ostream>
#include <string>
#include <string_view>

namespace citcpp {

/**
 * This enumeration defines the possible results of a covering array
 * generation job.
 */
enum class covering_array_computation_algorithm {
  /**
   * An in-parameter-order-general algorithm similar to the one implemented
   * by the popular tool ACTS.
   */
  IPOG,
  /**
   * An in-parameter-order-general algorithm whose results are identical to
   * the ones achieved by IPOG, but which trades a much better memory
   * consumption against a slower run-time performance. This algorithm should be
   * used, if the given problem simply results in too much memory being used.
   */
  IPOG_OTF
};

std::ostream& operator<<(std::ostream& os,
                         const covering_array_computation_algorithm& algo);

/**
 * This class represents configuration settings for a coverage array
 * computation job.
 */
class covering_array_computation_config {
  public:
    covering_array_computation_config();

    /**
     * Returns whether don't care values shall be replaced in the produced
     * test set by concrete parameter values.
     */
    bool replace_dont_care_values() const { return replace_dont_care_values_; }

    /**
     * Returns the number of threads to use. The value 0 means that the
     * number of threads is chosen automatically.
     */
    unsigned int number_of_threads() const { return number_of_threads_; }

    /**
     * Returns the value separator to use in the produced test set, as well
     * as for parsing an optional test set that shall be extended.
     */
    const std::string& value_separator() const { return value_seperator_; }

    /**
     * Returns the algorithm to be used for the covering array computation.
     */
    covering_array_computation_algorithm algorithm() const { return algo_; }

    /**
     * Sets whether don't care values shall be replaced in the produced
     * test set by concrete parameter values.
     */
    covering_array_computation_config& with_replace_dont_care_values(
        bool replace_dont_care_values) {
      replace_dont_care_values_ = replace_dont_care_values;

      return *this;
    }

    /**
     * Sets the number of threads that shall be used when computing the covering
     * array.
     * The value 0 means that the number of threads is chosen automatically.
     */
    covering_array_computation_config& with_number_of_threads(
        unsigned int number_of_threads) {
      number_of_threads_ = number_of_threads;

      return *this;
    }

    /**
     * Sets the value separator to use in the produced test set, as well
     * as for parsing an optional test set that shall be extended.
     */
    covering_array_computation_config& with_value_separator(
        std::string_view value_seperator) {
      value_seperator_ = value_seperator;

      return *this;
    }

    /**
     * Sets the algorithm to be used for the covering array computation.
     */
    covering_array_computation_config& with_algorithm(
        covering_array_computation_algorithm algo) {
      algo_ = algo;

      return *this;
    }

  private:
    bool replace_dont_care_values_;
    unsigned int number_of_threads_;
    std::string value_seperator_;
    covering_array_computation_algorithm algo_;
};

/**
 * This class represents configuration settings for a coverage measurement job.
 */
class coverage_measurement_config {
  public:
    coverage_measurement_config();

    /**
     * Returns the number of threads to use. The value 0 means that the
     * number of threads is chosen automatically.
     */
    unsigned int number_of_threads() const { return number_of_threads_; }

    /**
     * Returns the value separator to assume when parsing the test set whose
     * coverage to measure.
     */
    const std::string& value_separator() const { return value_seperator_; }

    /**
     * Sets the number of threads that shall be used when measuring
     * coverage.
     * The value 0 means that the number of threads is chosen automatically.
     */
    coverage_measurement_config& with_number_of_threads(
        unsigned int number_of_threads) {
      number_of_threads_ = number_of_threads;

      return *this;
    }

    /**
     * Sets the value separator to assume when parsing the test set whose
     * coverage to measure.
     */
    coverage_measurement_config& with_value_separator(
        std::string_view value_seperator) {
      value_seperator_ = value_seperator;

      return *this;
    }

  private:
    unsigned int number_of_threads_;
    std::string value_seperator_;
};

}  // namespace citcpp

#endif /* CITCPP_CONFIG_HPP_ */
