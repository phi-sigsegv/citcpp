#ifndef INPUT_MODEL_HPP_
#define INPUT_MODEL_HPP_

#include <string>
#include <vector>

namespace citcpp
{
  /**
   * Represents a value of a parameter.
   */
  class ParameterValue
  {
  public:
    const std::string&
    getValue () const
    {
      return m_value;
    }

    void
    setValue (const std::string &value)
    {
      m_value = value;
    }

  private:
    std::string m_value;
  };

  /**
   * Represents a parameter, which has a set of possible values.
   */
  class Parameter
  {
  public:
    const std::vector<ParameterValue>&
    getValues () const
    {
      return m_values;
    }

    std::vector<ParameterValue>&
    getValues ()
    {
      return m_values;
    }

  private:
    std::vector<ParameterValue> m_values;
  };

  /**
   * Represents an input model consisting of a list of parameters.
   */
  class InputModel
  {
  public:
    const std::vector<Parameter>&
    getParameters () const
    {
      return m_parameters;
    }

    std::vector<Parameter>&
    getParameters ()
    {
      return m_parameters;
    }

  private:
    std::vector<Parameter> m_parameters;
  };
}

#endif /* INPUT_MODEL_HPP_ */
