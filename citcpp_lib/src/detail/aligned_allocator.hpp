#ifndef ALIGNED_ALLOCATOR_HPP_
#define ALIGNED_ALLOCATOR_HPP_

#include <cstdlib>
#include <memory>
#include <new>

// Handle Windows-specific alignment headers
#if defined(_MSC_VER) || defined(__MINGW32__)
#include <malloc.h>
#endif

template <typename T, std::size_t ALIGNMENT>
struct aligned_allocator {
    using value_type = T;

    aligned_allocator() noexcept = default;

    template <typename U>
    aligned_allocator(const aligned_allocator<U, ALIGNMENT>&) noexcept {}

    // Required for some older STL implementations,
    // though allocator_traits provides this by default since C++11.
    template <typename U>
    struct rebind {
        using other = aligned_allocator<U, ALIGNMENT>;
    };

    T* allocate(std::size_t n) {
      if (n == 0) return nullptr;

      // Safety check for overflow
      if (n > std::size_t(-1) / sizeof(T)) throw std::bad_array_new_length();

      std::size_t size = n * sizeof(T);
      void* ptr = nullptr;

#if defined(_MSC_VER) || defined(__MINGW32__)
      // Windows: size doesn't have to be a multiple of ALIGNMENT
      ptr = _aligned_malloc(size, ALIGNMENT);
#else
      // POSIX/C11: size MUST be a multiple of alignment
      if (std::size_t remainder = size % ALIGNMENT; remainder != 0) {
        size += (ALIGNMENT - remainder);
      }
      ptr = std::aligned_alloc(ALIGNMENT, size);
#endif

      if (!ptr) throw std::bad_alloc();
      return static_cast<T*>(ptr);
    }

    void deallocate(T* p, std::size_t) noexcept {
      if (!p) return;

#if defined(_MSC_VER) || defined(__MINGW32__)
      _aligned_free(p);
#else
      std::free(p);
#endif
    }
};

// Stateless allocators are equal if their alignment is the same
template <typename T, typename U, std::size_t ALIGNMENT>
bool operator==(const aligned_allocator<T, ALIGNMENT>&,
                const aligned_allocator<U, ALIGNMENT>&) noexcept {
  return true;
}

template <typename T, typename U, std::size_t ALIGNMENT>
bool operator!=(const aligned_allocator<T, ALIGNMENT>&,
                const aligned_allocator<U, ALIGNMENT>&) noexcept {
  return false;
}

#endif /* ALIGNED_ALLOCATOR_HPP_ */
