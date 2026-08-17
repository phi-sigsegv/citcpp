#ifndef SPIN_LOCK_H
#define SPIN_LOCK_H

#include <atomic>
#include <chrono>
#include <cstddef>
#include <thread>

#ifdef _MSC_VER
#define NOMINMAX
#include <windows.h>
#endif

namespace threads {
namespace detail {

/**
 * This back-off strategy is a mixture of different back-off
 * mechanisms and implements incremental back-off.
 * An instance of this class keeps track of the "back-off state",
 * which is advanced on each call to backoff().
 */
class hybrid_backoff {
  public:
    hybrid_backoff() : m_count(0) {}

    ~hybrid_backoff() = default;

    hybrid_backoff(const hybrid_backoff&) = default;
    hybrid_backoff(hybrid_backoff&&) noexcept = default;

    hybrid_backoff& operator=(const hybrid_backoff&) = default;
    hybrid_backoff& operator=(hybrid_backoff&&) noexcept = default;

    /**
     * This method applies an incremental back-off strategy.
     * Each time this method is called, the calling thread
     * will be deferred even more:
     * First we simply execute a pause instruction, then
     * we execute a pause instruction multiple times, then
     * we yield the processor, then
     * we let the thread sleep for 1ms, and finally
     * we let the thread sleep for 10ms.
     */
    void backoff() {
      if (m_count < 10) {
#ifdef _MSC_VER
        // Visual C++ does not support inline assembly for ARM and x64
        // Architectures. God knows why... However, the following method
        // ultimately expands to the pause instruction.
        YieldProcessor();
#else
        __asm__ __volatile__("pause;");
#endif
      } else if (m_count < 20) {
        for (int i = 0; i < 50; ++i) {
#ifdef _MSC_VER
          YieldProcessor();
#else
          __asm__ __volatile__("pause;");
#endif
        }
      } else if (m_count < 22) {
        std::this_thread::yield();
      } else if (m_count < 26) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }

      ++m_count;
    }

    /**
     * This method resets the state of this back-off object.
     */
    void reset() { m_count = 0; }

  private:
    int m_count;
};

/**
 * This class provides a drop-in replacement for std::mutex.
 * Instead of a real mutex, this class implements a spinlock.
 * Locking amounts to atomically testing and setting a variable.
 * The back-off strategy is incremental deferring the calling
 * task more and more until it finally gets the lock.
 */
class SpinLock {
  public:
    SpinLock() : m_locked() { m_locked.clear(); }

    ~SpinLock() = default;

    SpinLock(const SpinLock&) = delete;
    SpinLock(SpinLock&&) = delete;
    SpinLock& operator=(const SpinLock&) = delete;
    SpinLock& operator=(SpinLock&&) = delete;

    void lock() {
      hybrid_backoff bkoff;
      while (m_locked.test_and_set(std::memory_order_acquire)) {
        // back-off before we retry.
        bkoff.backoff();
      }
    }

    void unlock() { m_locked.clear(std::memory_order_release); }

    bool try_lock() {
      return !m_locked.test_and_set(std::memory_order_acquire);
    }

  private:
    std::atomic_flag m_locked;
};

}  // namespace detail

/**
 * This class provides a drop-in replacement for std::mutex.
 * Instead of a real mutex, this class implements a spinlock.
 * Locking amounts to atomically testing and setting a variable.
 * The back-off strategy is incremental deferring the calling
 * task more and more until it finally gets the lock.
 */
class SpinLock {
  public:
    SpinLock() : impl_() {}

    ~SpinLock() = default;

    SpinLock(const SpinLock&) = delete;
    SpinLock(SpinLock&&) = delete;
    SpinLock& operator=(const SpinLock&) = delete;
    SpinLock& operator=(SpinLock&&) = delete;

    /**
     * Locks the mutex and returns.
     */
    void lock() { impl_.lock(); }

    /**
     * Unlocks the mutex and returns.
     */
    void unlock() { impl_.unlock(); }

    /**
     * Tries to lock the mutex and returns whether it
     * has successfully locked the mutex.
     *
     * @return true, if mutex locked, false otherwise
     */
    bool try_lock() { return impl_.try_lock(); }

  private:
    detail::SpinLock impl_;
};

}  // namespace threads

#endif  // SPIN_LOCK_H
