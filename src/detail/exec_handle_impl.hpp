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
	  IExecHandle (), num_combinations_to_cover_ (0), covered_combinations_ (), is_aborted_ (), test_set_ (), runnable_ (), thread_ ()
      {
      }

      ExecHandleImpl (ExecHandleImpl&&) = default;

      ExecHandleImpl (const ExecHandleImpl&) = delete;

      ExecHandleImpl&
      operator= (ExecHandleImpl&&) = default;

      ExecHandleImpl&
      operator= (const ExecHandleImpl&) = delete;

      ~ExecHandleImpl ()
      {
	abort ();
	thread_.join ();
      }

    public:
      unsigned long long
      getNumberOfCombinationsToCover () const
      {
	return num_combinations_to_cover_;
      }

      unsigned long long
      getNumberOfCoveredCombinations () const
      {
	return covered_combinations_;
      }

      void
      abort ()
      {
	is_aborted_.test_and_set ();
      }

      std::future<TestSet>
      getTestSet ()
      {
	return test_set_.get_future ();
      }

      void
      setNumberOfCombinationsToCover (
	  unsigned long long num_combinations_to_cover)
      {
	num_combinations_to_cover_ = num_combinations_to_cover;
      }

      void
      setNumberOfCoveredCombinations (unsigned long long covered_combinations)
      {
	covered_combinations_ = covered_combinations;
      }

      bool
      isJobAborted ()
      {
	return is_aborted_.test ();
      }

      void
      setTestSet (TestSet &&test_set)
      {
	test_set_.set_value (std::move (test_set));
      }

      /**
       * Sets the runnable to be called by the thread of this execution
       * handle. The thread will invoke the runnable right away,
       * as soon as this method is being called.
       */
      void
      setRunnable (std::unique_ptr<ICitCppAlgo> &&runnable)
      {
	runnable_ = std::move (runnable);
	thread_ = std::thread (&ICitCppAlgo::entryPoint, runnable_.get (),
			       std::ref (*this));
      }

    public:
      std::atomic_ullong num_combinations_to_cover_;
      std::atomic_ullong covered_combinations_;
      std::atomic_flag is_aborted_;
      std::promise<TestSet> test_set_;
      std::unique_ptr<ICitCppAlgo> runnable_;
      std::thread thread_;
    };
  }
}

#endif /* EXEC_HANDLE_IMPL_HPP_ */
