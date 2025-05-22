#ifndef EXEC_HANDLE_IMPL_HPP_
#define EXEC_HANDLE_IMPL_HPP_

#include <atomic>
#include <thread>
#include <functional>

#include "citcpp_algo_if.hpp"
#include "../exec_handle.hpp"

namespace citcpp
{
  namespace detail
  {
    class ExecHandleImpl : public IExecHandle
    {
    public:
      ExecHandleImpl () :
	  m_num_combinations_to_cover (0), m_covered_combinations (), m_is_aborted (), m_test_set (), m_runnable (), m_thread ()
      {
      }

      ~ExecHandleImpl ()
      {
	abort ();
	m_thread.join ();
      }

    public:
      unsigned long
      getNumberOfCombinationsToCover () const
      {
	return m_num_combinations_to_cover;
      }

      unsigned long
      getNumberOfCoveredCombinations () const
      {
	return m_covered_combinations;
      }

      void
      abort ()
      {
	m_is_aborted.test_and_set ();
      }

      std::future<TestSet>
      getTestSet ()
      {
	return m_test_set.get_future ();
      }

      bool
      isJobAborted ()
      {
	return m_is_aborted.test ();
      }

      void
      setTestSet (TestSet &&test_set)
      {
	m_test_set.set_value (std::move (test_set));
      }

      /**
       * Sets the runnable to be called by the thread of this execution
       * handle. The thread will invoke the runnable right away,
       * as soon as this method is being called.
       */
      void
      setRunnable (std::unique_ptr<ICitCppAlgo> &&runnable)
      {
	m_runnable = std::move (runnable);
	m_thread = std::thread (&ICitCppAlgo::entryPoint, m_runnable.get (),
				std::ref (*this));
      }

    public:
      int m_num_combinations_to_cover;
      std::atomic_long m_covered_combinations;
      std::atomic_flag m_is_aborted;
      std::promise<TestSet> m_test_set;
      std::unique_ptr<ICitCppAlgo> m_runnable;
      std::thread m_thread;
    };
  }
}

#endif /* EXEC_HANDLE_IMPL_HPP_ */
