#ifndef EXEC_HANDLE_IPOG_HPP_
#define EXEC_HANDLE_IPOG_HPP_

#include "exec_handle.hpp"

namespace citcpp
{
  /**
   * This class provides means to monitor the progress of the execution
   * of the IPOG algorithm, as well as to terminate it, if construction
   * of a complete covering array shall be aborted and the intermediate results
   * shall be returned.
   * All methods can be safely called from an arbitrary thread.
   */
  class exec_handle_ipog : public virtual exec_handle
  {
  public:
    /**
     * The real destructor of this handle calls abort() and joins with
     * the executing thread, in order to ensure a clean termination
     * if this handle is destroyed without the client explicitly waiting
     * for the execution.
     */
    virtual
    ~exec_handle_ipog ()
    {
    }

    /**
     * Returns the number of parameters completely processed so far.
     */
    virtual unsigned int
    get_number_of_processed_parameters () const = 0;
  };
}

#endif /* EXEC_HANDLE_IPOG_HPP_ */
