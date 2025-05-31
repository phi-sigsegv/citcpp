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
    ParameterValue (const std::string &value) :
	value_ (value)
    {
    }

    operator const std::string & ()
    {
      return getValue ();
    }

    const std::string&
    getValue () const
    {
      return value_;
    }

    void
    setValue (const std::string &value)
    {
      value_ = value;
    }

  private:
    std::string value_;
  };

  /**
   * Represents a parameter, which has a set of possible values.
   */
  class Parameter
  {
  public:
    const std::string&
    getName () const
    {
      return name_;
    }

    std::string&
    getName ()
    {
      return name_;
    }

    void
    setName (const std::string &name)
    {
      name_ = name;
    }

    const std::vector<ParameterValue>&
    getValues () const
    {
      return values_;
    }

    std::vector<ParameterValue>&
    getValues ()
    {
      return values_;
    }

  private:
    std::string name_;
    std::vector<ParameterValue> values_;
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
      return parameters_;
    }

    std::vector<Parameter>&
    getParameters ()
    {
      return parameters_;
    }

  private:
    std::vector<Parameter> parameters_;
  };
}

#endif /* INPUT_MODEL_HPP_ */
