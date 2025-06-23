#ifndef INPUT_MODEL_HPP_
#define INPUT_MODEL_HPP_

#include <string>
#include <vector>

namespace citcpp
{
  /**
   * Represents a value of a parameter.
   */
  class parameter_value
  {
  public:
    parameter_value (const std::string &value) :
	value_ (value)
    {
    }

    operator const std::string & ()
    {
      return get_value ();
    }

    const std::string&
    get_value () const
    {
      return value_;
    }

    void
    set_value (const std::string &value)
    {
      value_ = value;
    }

    bool
    operator== (const parameter_value &other) const
    {
      return value_ == other.value_;
    }

  private:
    std::string value_;
  };

  /**
   * Represents a parameter, which has a set of possible values.
   */
  class parameter
  {
  public:
    const std::string&
    get_name () const
    {
      return name_;
    }

    std::string&
    get_name ()
    {
      return name_;
    }

    void
    set_name (const std::string &name)
    {
      name_ = name;
    }

    parameter&
    name (const std::string &name)
    {
      name_ = name;
      return *this;
    }

    const std::vector<parameter_value>&
    get_values () const
    {
      return values_;
    }

    std::vector<parameter_value>&
    get_values ()
    {
      return values_;
    }

    void
    set_values (const std::vector<parameter_value> &values)
    {
      values_ = values;
    }

    parameter&
    values (const std::vector<parameter_value> &values)
    {
      values_ = values;
      return *this;
    }

    void
    add_value (const parameter_value &value)
    {
      values_.push_back (value);
    }

    void
    add_value (parameter_value &&value)
    {
      values_.push_back (std::move (value));
    }

    bool
    operator== (const parameter &other) const
    {
      return name_ == other.name_ && values_ == other.values_;
    }

  private:
    std::string name_;
    std::vector<parameter_value> values_;
  };

  /**
   * Represents an input model consisting of a list of parameters.
   */
  class input_model
  {
  public:
    const std::vector<parameter>&
    get_parameters () const
    {
      return parameters_;
    }

    std::vector<parameter>&
    get_parameters ()
    {
      return parameters_;
    }

    void
    add_parameter (const parameter &parameter)
    {
      parameters_.push_back (parameter);
    }

    void
    add_parameter (parameter &&parameter)
    {
      parameters_.push_back (std::move (parameter));
    }

    bool
    operator== (const input_model &other) const
    {
      return parameters_ == other.parameters_;
    }

  private:
    std::vector<parameter> parameters_;
  };
}

#endif /* INPUT_MODEL_HPP_ */
