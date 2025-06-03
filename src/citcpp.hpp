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
  enum class algorithm
  {
    IPOG
  };

  /**
   * Triggers execution of the calculation of a covering test set.
   * This returns immediately to the caller with a handle object,
   * which can then be used to monitor the progress of the execution
   * or to terminate it, as well as to obtain the final results.
   */
  std::unique_ptr<exec_handle>
  compute_covering_test_set (const input_model &input_model, unsigned int t,
			     algorithm alg);
}

#endif /* CITCPP_HPP_ */
