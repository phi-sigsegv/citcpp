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
  IPOG
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
    bool replace_dont_care_values() const;

    /**
     * Returns the number of threads to use. The value 0 means that the
     * number of threads is chosen automatically.
     */
    unsigned int number_of_threads() const;

    /**
     * Returns the memory limit used for the constraint handler in terms
     * of gigabytes.
     */
    std::size_t constraint_handler_memory_limit_gb() const;

    /**
     * Returns the value separator to use in the produced test set, as well
     * as for parsing an optional test set that shall be extended.
     */
    const std::string& value_separator() const;

    /**
     * Returns the algorithm to be used for the covering array computation.
     */
    covering_array_computation_algorithm algorithm() const;

    /**
     * Sets whether don't care values shall be replaced in the produced
     * test set by concrete parameter values.
     */
    covering_array_computation_config& with_replace_dont_care_values(
        bool replace_dont_care_values);

    /**
     * Sets the number of threads that shall be used when computing the covering
     * array.
     * The value 0 means that the number of threads is chosen automatically.
     */
    covering_array_computation_config& with_number_of_threads(
        unsigned int number_of_threads);

    /**
     * Sets the memory limit used for the constraint handler in terms
     * of gigabytes.
     */
    covering_array_computation_config& with_constraint_handler_memory_limit_gb(
        std::size_t c_handler_memory_limit_gib);

    /**
     * Sets the value separator to use in the produced test set, as well
     * as for parsing an optional test set that shall be extended.
     */
    covering_array_computation_config& with_value_separator(
        std::string_view value_seperator);

    /**
     * Sets the algorithm to be used for the covering array computation.
     */
    covering_array_computation_config& with_algorithm(
        covering_array_computation_algorithm algo);

  private:
    bool replace_dont_care_values_;
    unsigned int number_of_threads_;
    std::size_t c_handler_memory_limit_gib_;
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
    std::size_t number_of_threads() const;

    /**
     * Returns the memory limit used for the constraint handler in terms
     * of gigabytes.
     */
    std::size_t constraint_handler_memory_limit_gb() const;

    /**
     * Returns the value separator to assume when parsing the test set whose
     * coverage to measure.
     */
    const std::string& value_separator() const;

    /**
     * Sets the number of threads that shall be used when measuring
     * coverage.
     * The value 0 means that the number of threads is chosen automatically.
     */
    coverage_measurement_config& with_number_of_threads(
        std::size_t number_of_threads);

    /**
     * Sets the memory limit used for the constraint handler in terms
     * of gigabytes.
     */
    coverage_measurement_config& with_constraint_handler_memory_limit_gb(
        std::size_t c_handler_memory_limit_gib);

    /**
     * Sets the value separator to assume when parsing the test set whose
     * coverage to measure.
     */
    coverage_measurement_config& with_value_separator(
        std::string_view value_seperator);

  private:
    std::size_t number_of_threads_;
    std::size_t c_handler_memory_limit_gib_;
    std::string value_seperator_;
};

}  // namespace citcpp

#endif /* CITCPP_CONFIG_HPP_ */
