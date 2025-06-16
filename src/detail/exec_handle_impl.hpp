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
    class exec_handle_impl : public exec_handle
    {
    public:
      exec_handle_impl () :
	  exec_handle (), num_combinations_to_cover_ (0), covered_combinations_ (
	      0), is_aborted_ (), test_set_ (), duration_msec_ (0), runnable_ (), thread_ ()
      {
      }

      exec_handle_impl (exec_handle_impl&&) = default;

      exec_handle_impl (const exec_handle_impl&) = delete;

      exec_handle_impl&
      operator= (exec_handle_impl&&) = default;

      exec_handle_impl&
      operator= (const exec_handle_impl&) = delete;

      ~exec_handle_impl ()
      {
	abort ();
	thread_.join ();
      }

    public:
      unsigned long long
      get_number_of_combinations_to_cover () const
      {
	return num_combinations_to_cover_;
      }

      unsigned long long
      get_number_of_covered_combinations () const
      {
	return covered_combinations_;
      }

      void
      abort ()
      {
	is_aborted_.test_and_set ();
      }

      std::future<test_set>
      get_test_set ()
      {
	return test_set_.get_future ();
      }

      unsigned int
      get_duration_in_milli_seconds () const
      {
	return duration_msec_;
      }

      void
      set_number_of_combinations_to_cover (
	  unsigned long long num_combinations_to_cover)
      {
	num_combinations_to_cover_ = num_combinations_to_cover;
      }

      void
      set_number_of_covered_combinations (
	  unsigned long long covered_combinations)
      {
	covered_combinations_ = covered_combinations;
      }

      void
      add_number_of_covered_combinations (
	  unsigned long long covered_combinations)
      {
	covered_combinations_.fetch_add (covered_combinations,
					 std::memory_order_acq_rel);
      }

      bool
      is_job_aborted ()
      {
	return is_aborted_.test ();
      }

      void
      set_test_set (test_set &&test_set)
      {
	test_set_.set_value (std::move (test_set));
      }

      void
      set_duration_in_milli_seconds (unsigned int duration_msec)
      {
	duration_msec_ = duration_msec;
      }

      /**
       * Sets the runnable to be called by the thread of this execution
       * handle. The thread will invoke the runnable right away,
       * as soon as this method is being called.
       */
      void
      set_runnable (std::unique_ptr<citcpp_algo_if> &&runnable)
      {
	runnable_ = std::move (runnable);
	thread_ = std::thread (&citcpp_algo_if::entry_point, runnable_.get (),
			       std::ref (*this));
      }

    public:
      std::atomic_ullong num_combinations_to_cover_;
      std::atomic_ullong covered_combinations_;
      std::atomic_flag is_aborted_;
      std::promise<test_set> test_set_;
      std::atomic_uint duration_msec_;
      std::unique_ptr<citcpp_algo_if> runnable_;
      std::thread thread_;
    };
  }
}

#endif /* EXEC_HANDLE_IMPL_HPP_ */
