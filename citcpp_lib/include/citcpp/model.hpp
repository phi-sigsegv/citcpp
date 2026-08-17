#ifndef INPUT_MODEL_HPP_
#define INPUT_MODEL_HPP_

#include <ostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "constraints.hpp"
#include "parameter.hpp"
#include "relation.hpp"

namespace citcpp {

/**
 * Represents an input model consisting of a list of parameters.
 */
class model {
  public:
    model();

    model(const model& other);
    model(model&& other) noexcept;

    ~model();

    model& operator=(const model& other);
    model& operator=(model&& other) noexcept;

    const std::string& get_name() const;

    void set_name(std::string_view name);

    const std::vector<parameter>& get_parameters() const;

    std::vector<parameter>& get_parameters();

    void add_parameter(parameter param);

    const std::vector<relation>& get_relations() const;

    std::vector<relation>& get_relations();

    void add_relation(relation r);

    const std::vector<std::shared_ptr<constraint>>& get_constraints() const;

    std::vector<std::shared_ptr<constraint>>& get_constraints();

    void add_constraint(std::shared_ptr<constraint> constraint);

    friend std::ostream& operator<<(std::ostream& os, const model& model);

  private:
    std::string name_;
    std::vector<parameter> parameters_;
    std::vector<relation> relations_;
    std::vector<std::shared_ptr<constraint>> constraints_;
};

}  // namespace citcpp

#endif /* INPUT_MODEL_HPP_ */
