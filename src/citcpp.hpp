#ifndef CITCPP_HPP_
#define CITCPP_HPP_

#include <memory>

#include "exec_handle.hpp"
#include "input_model.hpp"

namespace citcpp
{
  /**
   * This enumeration defines the supported algorithms for computation of
   * a covering test set.
   */
  enum class Algorithm
  {
    IPOG
  };

  /**
   * This is the main class which can be used to trigger the calculation
   * of a test set computation.
   */
  class CitCpp
  {
  public:
    CitCpp (const InputModel &input_model);

    ~CitCpp ();

    /**
     * Triggers execution of the calculation of a covering test set.
     * This returns immediately to the caller with a handle object,
     * which can then be used to monitor the progress of the execution
     * or to terminate it, as well as to obtain the final results.
     */
    std::unique_ptr<IExecHandle>
    computeCoveringTestSet (unsigned int t, Algorithm alg) const;

  private:
    class impl;
    std::unique_ptr<impl> impl_;
  };
}

#endif /* CITCPP_HPP_ */
