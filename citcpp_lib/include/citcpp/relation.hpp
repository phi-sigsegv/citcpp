#ifndef CITCPP_RELATION_HPP_
#define CITCPP_RELATION_HPP_

#include <ostream>
#include <string>
#include <string_view>
#include <vector>

#include "parameter.hpp"

namespace citcpp {

/**
 * Represents a relation, which is a set of parameters and an interaction
 * strength.
 */
class relation {
  public:
    const std::string& get_name() const;

    void set_name(std::string_view name);

    const std::vector<parameter_reference>& get_parameters() const;

    std::vector<parameter_reference>& get_parameters();

    void add_parameter(const parameter& parameter);

    void add_parameter(parameter_reference parameter);

    unsigned int get_interaction_strength() const;

    void set_interaction_strength(unsigned int interaction_strength);

    bool operator==(const relation& other) const;

    friend std::ostream& operator<<(std::ostream& os, const relation& rel);

  private:
    std::string name_{};
    std::vector<parameter_reference> parameters_{};
    unsigned int interaction_strength_{0};
};

}  // namespace citcpp

#endif /* CITCPP_RELATION_HPP_ */
