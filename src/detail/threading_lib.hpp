#ifndef THREADS_LIBRARY_H
#define THREADS_LIBRARY_H

#include <cstddef>
#include <thread>
#include <chrono>
#include <atomic>
#include <type_traits>
#include <utility>
#include <memory>
#include <tuple>
#include <utility>

#ifdef _MSC_VER
#define NOMINMAX
#include <windows.h>
#endif

#if defined(__APPLE__)
#include <pthread.h>
#endif



namespace threads {



namespace detail {



/**
 * This template stores a tuple of indexes.
 */
template< std::size_t... Indexes >
struct indexes_tuple
{
  typedef indexes_tuple< Indexes..., sizeof...(Indexes) > next;
};

/**
 * Builds an indexes_tuple< 0, 1, 2, ..., N-1 >.
 */
template< std::size_t N >
struct build_index_tuple
{
  typedef typename build_index_tuple< N - 1 >::type::next type;
};

template<>
struct build_index_tuple< 0 >
{
  typedef indexes_tuple<> type;
};



/**
 * This is a type copied from LLVM. It is an efficient, type-erasing, non-owning
 * reference to a callable. So similarily to std::function it wraps a callable
 * object, but does not store it in any way. It just refers to it, which means
 * that the lifetime of the callable must be long enough if it shall be
 * invoked via a function_ref object.
 *
 * WARNING: This function reference is easy to use incorrectly.
 * For example consider:
 * function_ref< int() > invoke_later( []{ return 42; } );
 * auto val = invoke_later();
 * Here the lambda is a temporary(!) whose address is taken in the constructor
 * of function_ref, therefore at the time invoke_later() is called, it is gone!
 * On the other hand this works because the lambda lives long enough:
 * void func( function_ref< int() > f );
 * func( []{ return 42; } );
 * And this also works:
 * auto lambda = []{ return 42; };
 * function_ref< int() > invoke_later( lambda );
 * auto val = invoke_later();
 * And this also works:
 * struct Functor { int operator()() { return 42; } };
 * Functor func;
 * function_ref< int() > invoke_later( func );
 * auto val = invoke_later();
 */
template< typename Fn >
class function_ref;

template< typename Ret, typename ...Params >
class function_ref< Ret(Params...) >
{
  private:
    Ret (*callback)( intptr_t callable, Params ...params ) = nullptr;
    intptr_t callable;

    template< typename Callable >
    static Ret callback_fn( intptr_t callable, Params ...params )
    {
      return ( *reinterpret_cast< Callable * >( callable ) )( std::forward< Params >( params )... );
    }

  public:
    function_ref() = default;
    function_ref( std::nullptr_t ) {}

    template< typename Callable >
    function_ref( Callable && callable,
                  typename std::enable_if< !std::is_same< typename std::remove_reference< Callable >::type,
                                                          function_ref >::value
                                         >::type * = nullptr )
        : callback( callback_fn< typename std::remove_reference< Callable >::type > )
        , callable( reinterpret_cast< intptr_t >( &callable ) )
    {}

    Ret operator()( Params ...params ) const
    {
      return callback( callable, std::forward< Params >( params )... );
    }

    operator bool() const
    {
      return callback;
    }
};



/**
 * This back-off strategy is a mixture of different back-off
 * mechanisms and implements incremental back-off.
 * An instance of this class keeps track of the "back-off state",
 * which is advanced on each call to backoff().
 */
class hybrid_backoff
{
  public:
    hybrid_backoff()
        : m_count( 0 )
    {}

    hybrid_backoff( const hybrid_backoff & ) = delete;
    hybrid_backoff & operator=( const hybrid_backoff & ) = delete;

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
    void backoff()
    {
      if ( m_count < 10 )
      {
#ifdef _MSC_VER
        // Visual C++ does not support inline assembly for ARM and x64 Architectures.
        // God knows why...
        // However, the following method ultimately expands to the pause instruction.
        YieldProcessor();
#else
        __asm__ __volatile__("pause;");
#endif
      }
      else if ( m_count < 20 )
      {
        for ( int i = 0; i < 50; ++i )
        {
#ifdef _MSC_VER
          YieldProcessor();
#else
          __asm__ __volatile__("pause;");
#endif
        }
      }
      else if ( m_count < 22 )
      {
        std::this_thread::yield();
      }
      else if ( m_count < 26 )
      {
        std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
      }
      else
      {
        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
      }

      ++m_count;
    }

    /**
     * This method resets the state of this back-off object.
     */
    void reset()
    {
      m_count = 0;
    }

  private:
    int m_count;
};



/**
 * This template provides something similar to std::array.
 * However, it can be configured to store its items in an
 * over-aligned way, making it possible to avoid false-sharing
 * if the alignment is chosen greater than or equal to the
 * size of a cache line. In addition this template also
 * allows to initialize all members with a single value
 * passed to its constructor.
 */
template< typename T_VALUE,
          std::size_t N,
          std::size_t T_ALIGNMENT >
class array_fixed_size
{
  private:
    struct alignas( T_ALIGNMENT ) aligned_value
    {
      T_VALUE m_value;
    };
    typedef array_fixed_size< T_VALUE, N, T_ALIGNMENT > this_type;

  public:
    template< bool is_const = false >
    class the_iterator
    {
        // This friend declaration is necessary in order to be able
        // to convert a non-const iterator into a const iterator,
        // while keeping members private.
        friend class the_iterator< true >;

        typedef typename std::conditional< is_const,
                                           const aligned_value *,
                                           aligned_value * >::type ptr_type;
        ptr_type m_ptr;

      public:
        typedef T_VALUE value_type;
        typedef std::ptrdiff_t difference_type;
        typedef typename std::conditional< is_const,
                                           const T_VALUE *,
                                           T_VALUE * >::type pointer;
        typedef typename std::conditional< is_const,
                                           const T_VALUE &,
                                           T_VALUE & >::type reference;
        typedef std::random_access_iterator_tag iterator_category;

        the_iterator() : m_ptr( nullptr ) {}
        the_iterator( ptr_type ptr ) : m_ptr( ptr ) {}
        the_iterator( const the_iterator< false > & other ) : m_ptr( other.m_ptr ) {}
        reference value() const { return m_ptr->m_value; }
        pointer value_ptr() const { return &( m_ptr->m_value ); }
        reference operator*() const { return value(); }
        pointer operator->() const { return value_ptr(); }
        the_iterator & operator++() { ++m_ptr; return *this; }
        the_iterator operator++( int ) { return the_iterator( m_ptr++ ); }
        the_iterator & operator--() { --m_ptr; return *this; }
        the_iterator operator--( int ) { return the_iterator( m_ptr-- ); }
        the_iterator & operator+=( difference_type n ) { m_ptr += n; return *this; }
        the_iterator & operator-=( difference_type n ) { m_ptr -= n; return *this; }
        the_iterator operator+( difference_type n ) const { return the_iterator( m_ptr+n ); }
        the_iterator operator-( difference_type n ) const { return the_iterator( m_ptr-n ); }
        friend the_iterator operator+( difference_type n, const the_iterator & it ) { return the_iterator( it.m_ptr+n ); }
        friend difference_type operator-( const the_iterator & lhs, const the_iterator & rhs ) { return lhs.m_ptr - rhs.m_ptr; }
        reference operator[]( difference_type n ) const { return m_ptr[n].m_value; }

        friend bool operator==( const the_iterator & lhs, const the_iterator & rhs ) { return lhs.m_ptr == rhs.m_ptr; }
        friend bool operator!=( const the_iterator & lhs, const the_iterator & rhs ) { return lhs.m_ptr != rhs.m_ptr; }
        friend bool operator<( const the_iterator & lhs, const the_iterator & rhs ) { return lhs.m_ptr < rhs.m_ptr; }
        friend bool operator>( const the_iterator & lhs, const the_iterator & rhs ) { return lhs.m_ptr > rhs.m_ptr; }
        friend bool operator<=( const the_iterator & lhs, const the_iterator & rhs ) { return lhs.m_ptr <= rhs.m_ptr; }
        friend bool operator>=( const the_iterator & lhs, const the_iterator & rhs ) { return lhs.m_ptr >= rhs.m_ptr; }
    };

  public:
    typedef T_VALUE value_type;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;
    typedef value_type & reference;
    typedef const value_type & const_reference;
    typedef value_type * pointer;
    typedef const value_type * const_pointer;
    typedef the_iterator< false > iterator;
    typedef the_iterator< true > const_iterator;
    typedef std::reverse_iterator< iterator > reverse_iterator;
    typedef std::reverse_iterator< const_iterator > const_reverse_iterator;

  public:
    array_fixed_size() = default;
    array_fixed_size( const this_type & ) = default;
    array_fixed_size( this_type && ) = default;
    ~array_fixed_size() = default;
    this_type & operator=( const this_type & ) = default;
    this_type & operator=( this_type&& ) = default;

    // Pass a single parameter down to all elements.
    template< typename Arg >
    array_fixed_size( Arg&& arg )
        : array_fixed_size( std::forward< Arg >( arg ),
                            typename build_index_tuple< N >::type{} )
    {}

    template< typename Arg, std::size_t... Indexes >
    array_fixed_size( Arg&& arg, indexes_tuple< Indexes... > )
        : m_array{{(static_cast< void >( Indexes ), std::forward< Arg >( arg ))}...}
    {}

    // Element access
    reference at( size_type pos ) { return m_array[pos].m_value; }
    const_reference at( size_type pos ) const { return m_array[pos].m_value; }
    reference operator[]( size_type pos ) { return m_array[pos].m_value; }
    const_reference operator[]( size_type pos ) const { return m_array[pos].m_value; }
    reference front() { return m_array[0].m_value; }
    const_reference front() const { return m_array[0].m_value; }
    reference back() { return m_array[N - 1].m_value; }
    const_reference back() const { return m_array[N - 1].m_value; }

    // Iterators
    iterator begin() { return &m_array[0]; }
    const_iterator begin() const { return &m_array[0]; }
    const_iterator cbegin() const { return &m_array[0]; }
    iterator end() { return ( &m_array[0] ) + N; }
    const_iterator end() const { return ( &m_array[0] ) + N; }
    const_iterator cend() const { return ( &m_array[0] ) + N; }
    reverse_iterator rbegin() { return reverse_iterator( end() ); }
    const_reverse_iterator rbegin() const { return const_reverse_iterator( end() ); }
    const_reverse_iterator crbegin() const { return const_reverse_iterator( cend() ); }
    reverse_iterator rend() { return reverse_iterator( begin() ); }
    const_reverse_iterator rend() const { return const_reverse_iterator( begin() ); }
    const_reverse_iterator crend() const { return const_reverse_iterator( cbegin() ); }

    constexpr bool empty() const noexcept { return size() != 0; }
    constexpr size_type size() const noexcept { return N; }
    constexpr size_type max_size() const noexcept { return N; }

    friend bool operator==( const this_type & lhs, const this_type & rhs )
    {
      for ( size_type i = 0; i < lhs.size(); ++i )
      {
        if ( lhs.m_array[i].m_value != rhs.m_array[i].m_value ) return false;
      }
      return true;
    }
    friend bool operator!=( const this_type & lhs, const this_type & rhs ) { return !( lhs == rhs ); }

  private:
    alignas( T_ALIGNMENT ) aligned_value m_array[N];
};



template< typename T_VALUE,
          std::size_t T_ALIGNMENT >
class array_fixed_size< T_VALUE, 0, T_ALIGNMENT >
{
  private:
    typedef array_fixed_size< T_VALUE, 0, T_ALIGNMENT > this_type;

  public:
    typedef T_VALUE value_type;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;
    typedef value_type & reference;
    typedef const value_type & const_reference;
    typedef value_type * pointer;
    typedef const value_type * const_pointer;
    typedef pointer iterator;
    typedef const_pointer const_iterator;
    typedef std::reverse_iterator< iterator > reverse_iterator;
    typedef std::reverse_iterator< const_iterator > const_reverse_iterator;

  public:
    array_fixed_size() = default;
    array_fixed_size( const this_type & ) = default;
    array_fixed_size( this_type && ) = default;
    ~array_fixed_size() = default;
    this_type & operator=( const this_type & ) = default;
    this_type & operator=( this_type&& ) = default;

    // Pass a single parameter down to all elements.
    template< typename Arg >
    array_fixed_size( Arg&& arg )
        : array_fixed_size( std::forward< Arg >( arg ),
                            typename build_index_tuple< 0 >::type{} )
    {}

    template< typename Arg, std::size_t... Indexes >
    array_fixed_size( Arg&&, indexes_tuple< Indexes... > )
    {}

    // Element access
    reference at( size_type ) { return *static_cast< T_VALUE * >( nullptr ); }
    const_reference at( size_type ) const { return *static_cast< T_VALUE * >( nullptr ); }
    reference operator[]( size_type ) { return *static_cast< T_VALUE * >( nullptr ); }
    const_reference operator[]( size_type ) const { return *static_cast< T_VALUE * >( nullptr ); }
    reference front() { return *static_cast< T_VALUE * >( nullptr ); }
    const_reference front() const { return *static_cast< T_VALUE * >( nullptr ); }
    reference back() { return *static_cast< T_VALUE * >( nullptr ); }
    const_reference back() const { return *static_cast< T_VALUE * >( nullptr ); }

    // Iterators
    iterator begin() { return nullptr; }
    const_iterator begin() const { return nullptr; }
    const_iterator cbegin() const { return nullptr; }
    iterator end() { return nullptr; }
    const_iterator end() const { return nullptr; }
    const_iterator cend() const { return nullptr; }
    reverse_iterator rbegin() { return reverse_iterator( end() ); }
    const_reverse_iterator rbegin() const { return const_reverse_iterator( end() ); }
    const_reverse_iterator crbegin() const { return const_reverse_iterator( cend() ); }
    reverse_iterator rend() { return reverse_iterator( begin() ); }
    const_reverse_iterator rend() const { return const_reverse_iterator( begin() ); }
    const_reverse_iterator crend() const { return const_reverse_iterator( cbegin() ); }

    constexpr bool empty() const noexcept { return true; }
    constexpr size_type size() const noexcept { return 0; }
    constexpr size_type max_size() const noexcept { return 0; }

    friend bool operator==( const this_type &, const this_type & ) { return true; }
    friend bool operator!=( const this_type &, const this_type & ) { return false; }
};



#if defined(__APPLE__)
/*
 * The following provides a workaround for MacOS.
 * Unfortunately, Apple does NOT provide support
 * for thread-local storage in their toolchain at the moment.
 * So as a workaround, we use pthread-APIs to emulate
 * that functionality.
 */
class ThreadContext
{
  public:
    static void initMainThreadId()
    {
      static std::size_t main_thread_id = 0;
      setThreadId( main_thread_id );
    }

    static std::size_t getThreadId()
    {
      return *static_cast< std::size_t * >( pthread_getspecific( *threadIdPThreadKey() ) );
    }

    static void setThreadId( std::size_t & id )
    {
      pthread_setspecific( *threadIdPThreadKey(), &id );
    }

  private:
    static pthread_once_t * pThreadKeyInitOnce()
    {
      static pthread_once_t key_once = PTHREAD_ONCE_INIT;
      return &key_once;
    }

    static pthread_key_t * pThreadKey()
    {
      static pthread_key_t key;
      return &key;
    }

    static void initPThreadKey()
    {
      pthread_key_create( pThreadKey(), NULL );
    }

    static pthread_key_t * threadIdPThreadKey()
    {
      pthread_once( pThreadKeyInitOnce(), &ThreadContext::initPThreadKey );
      return pThreadKey();
    }
};
#else
class ThreadContext
{
  public:
    static void initMainThreadId()
    {
      std::size_t main_thread_id = 0;
      setThreadId( main_thread_id );
    }

    static std::size_t getThreadId()
    {
      return getThreadIdRef();
    }

    static void setThreadId( std::size_t & id )
    {
      getThreadIdRef() = id;
    }

  private:
    static std::size_t & getThreadIdRef()
    {
      static thread_local std::size_t thread_id;
      return thread_id;
    }
};
#endif



/**
 * This class provides a drop-in replacement for std::mutex.
 * Instead of a real mutex, this class implements a spinlock.
 * Locking amounts to atomically testing and setting a variable.
 * The back-off strategy is incremental deferring the calling
 * task more and more until it finally gets the lock.
 */
class SpinLock
{
  typedef SpinLock this_type;

  public:
    SpinLock()
        : m_locked()
    {
      m_locked.clear();
    }

    SpinLock( const this_type & ) = delete;
    SpinLock( this_type && ) = delete;
    this_type & operator=( const this_type & ) = delete;
    this_type & operator=( this_type && ) = delete;

    void lock()
    {
      detail::hybrid_backoff bkoff;
      while ( m_locked.test_and_set( std::memory_order_acquire ) )
      {
        // back-off before we retry.
        bkoff.backoff();
      }
    }

    void unlock()
    {
      m_locked.clear( std::memory_order_release );
    }

    bool try_lock()
    {
      return !m_locked.test_and_set( std::memory_order_acquire );
    }

  private:
    std::atomic_flag m_locked;
};



class concurrent_queue_intrusive_node
{
  template< typename T_VALUE,
            unsigned int T_CACHE_LINE_SIZE >
  friend class concurrent_queue_intrusive;

  template< typename T_TASK >
  friend class TaskListTmpl;

  typedef concurrent_queue_intrusive_node this_type;

  public:
    concurrent_queue_intrusive_node()
        : m_next_node( nullptr )
    {}

    concurrent_queue_intrusive_node( const this_type & other)
        : m_next_node( other.m_next_node.load( std::memory_order_relaxed ) )
    {}

    concurrent_queue_intrusive_node( this_type && other )
        : m_next_node( other.m_next_node.load( std::memory_order_relaxed ) )
    {}

    this_type & operator=( const this_type & other )
    {
      m_next_node.store( other.m_next_node.load( std::memory_order_relaxed ), std::memory_order_relaxed );

      return *this;
    }

    this_type & operator=( this_type && other )
    {
      m_next_node.store( other.m_next_node.load( std::memory_order_relaxed ), std::memory_order_relaxed );

      return *this;
    }

  private:
    std::atomic< concurrent_queue_intrusive_node * > m_next_node;
};



template< typename T_VALUE,
          unsigned int T_CACHE_LINE_SIZE >
class alignas( T_CACHE_LINE_SIZE ) concurrent_queue_intrusive
{
  typedef concurrent_queue_intrusive_node node_type;
  typedef concurrent_queue_intrusive< T_VALUE, T_CACHE_LINE_SIZE > this_type;

  public:
    typedef T_VALUE value_type;
    typedef value_type & reference;
    typedef const value_type & const_reference;
    typedef value_type * pointer;
    typedef const value_type * const_pointer;
    typedef std::ptrdiff_t difference_type;

  public:
    concurrent_queue_intrusive()
        : m_lock()
        , m_dummy()
        , m_head( &m_dummy )
        , m_tail( &m_dummy )
    {
      m_dummy.m_next_node = nullptr;
    }

    // Our instrusive containers are not copyable since
    // this is a dangerous operation.
    concurrent_queue_intrusive( const this_type & ) = delete;

    /**
     * Warning: Moving is expensive. We may need to walk through the entire
     * linked list to guarantee that no node point to the dummy node
     * of \a other.
     */
    concurrent_queue_intrusive( this_type && other )
        : m_lock()
        , m_dummy()
        , m_head( other.m_head.load( std::memory_order_relaxed ) )
        , m_tail( other.m_tail.load( std::memory_order_relaxed ) )
    {
      m_dummy.m_next_node.store( other.m_dummy.m_next_node.load( std::memory_order_relaxed ),
                                 std::memory_order_relaxed );
      other.clear();

      // head and/or tail might point to the dummy node.
      // If so, we let them point to our dummy node.
      if ( m_head.load( std::memory_order_relaxed ) == &other.m_dummy )
      {
        m_head.store( &m_dummy, std::memory_order_relaxed );
      }
      if ( m_tail.load( std::memory_order_relaxed ) == &other.m_dummy )
      {
        m_tail.store( &m_dummy, std::memory_order_relaxed );
      }

      // It might be the case that some node points to the
      // dummy node of other. If so, we need to re-direct its
      // next pointer to our dummy node.
      // This can only occur if head != dummy.
      node_type * node = m_head.load( std::memory_order_relaxed );
      if ( node != &m_dummy )
      {
        node_type * next_node = node->m_next_node.load( std::memory_order_relaxed );
        while ( next_node != &other.m_dummy )
        {
          node = next_node;
          next_node = node->m_next_node.load( std::memory_order_relaxed );
        }
        // When the loop finishes, node points to the dummy of other.
        // So we re-direct the next-node pointer. The next-node pointer of
        // dummy itself has been re-directed above.
        node->m_next_node.store( &m_dummy, std::memory_order_relaxed );
      }
    }

    // Our instrusive containers are not copyable since
    // this is a dangerous operation.
    this_type & operator=( const this_type & ) = delete;

    /**
     * Warning: Moving is expensive. We may need to walk through the entire
     * linked list to guarantee that no node point to the dummy node
     * of \a other.
     */
    this_type & operator=( this_type && other )
    {
      m_dummy.m_next_node.store( other.m_dummy.m_next_node.load( std::memory_order_relaxed ),
                                 std::memory_order_relaxed );
      m_head.store( other.m_head.load( std::memory_order_relaxed ), std::memory_order_relaxed );
      m_tail.store( other.m_tail.load( std::memory_order_relaxed ), std::memory_order_relaxed );

      other.clear();

      // head and/or tail might point to the dummy node.
      // If so, we let them point to our dummy node.
      if ( m_head.load( std::memory_order_relaxed ) == &other.m_dummy )
      {
        m_head.store( &m_dummy, std::memory_order_relaxed );
      }
      if ( m_tail.load( std::memory_order_relaxed ) == &other.m_dummy )
      {
        m_tail.store( &m_dummy, std::memory_order_relaxed );
      }

      // It might be the case that some node points to the
      // dummy node of other. If so, we need to re-direct its
      // next pointer to our dummy node.
      // This can only occur if head != dummy.
      node_type * node = m_head.load( std::memory_order_relaxed );
      if ( node != &m_dummy )
      {
        node_type * next_node = node->m_next_node.load( std::memory_order_relaxed );
        while ( next_node != &other.m_dummy )
        {
          node = next_node;
          next_node = node->m_next_node.load( std::memory_order_relaxed );
        }
        // When the loop finishes, node points to the dummy of other.
        // So we re-direct the next-node pointer. The next-node pointer of
        // dummy itself has been re-directed above.
        node->m_next_node.store( &m_dummy, std::memory_order_relaxed );
      }

      return *this;
    }

    void swap( this_type & other )
    {
      node_type * tmp = m_dummy.m_next_node.load( std::memory_order_relaxed );

      m_dummy.m_next_node.store( other.m_dummy.m_next_node.load( std::memory_order_relaxed ),
                                 std::memory_order_relaxed );
      other.m_dummy.m_next_node.store( tmp, std::memory_order_relaxed );

      tmp = m_head.load( std::memory_order_relaxed );
      m_head.store( other.m_head.load( std::memory_order_relaxed ), std::memory_order_relaxed );
      other.m_head.store( tmp, std::memory_order_relaxed );

      tmp = m_tail.load( std::memory_order_relaxed );
      m_tail.store( other.m_tail.load( std::memory_order_relaxed ), std::memory_order_relaxed );
      other.m_tail.store( tmp, std::memory_order_relaxed );

      if ( m_head.load( std::memory_order_relaxed ) == &other.m_dummy )
      {
        m_head.store( &m_dummy, std::memory_order_relaxed );
      }
      if ( other.m_head.load( std::memory_order_relaxed ) == &m_dummy )
      {
        other.m_head.store( &other.m_dummy, std::memory_order_relaxed );
      }
      if ( m_tail.load( std::memory_order_relaxed ) == &other.m_dummy )
      {
        m_tail.store( &m_dummy, std::memory_order_relaxed );
      }
      if ( other.m_tail.load( std::memory_order_relaxed ) == &m_dummy )
      {
        other.m_tail.store( &other.m_dummy, std::memory_order_relaxed );
      }

      // It might be the case that some node points to the
      // dummy node of other. If so, we need to re-direct its
      // next pointer to our dummy node.
      // This can only occur if head != dummy.
      node_type * node = m_head.load( std::memory_order_relaxed );
      if ( node != &m_dummy )
      {
        node_type * next_node = node->m_next_node.load( std::memory_order_relaxed );
        while ( next_node != &other.m_dummy )
        {
          node = next_node;
          next_node = node->m_next_node.load( std::memory_order_relaxed );
        }
        // When the loop finishes, node points to the dummy of other.
        // So we re-direct the next-node pointer. The next-node pointer of
        // dummy itself has been re-directed above.
        node->m_next_node.store( &m_dummy, std::memory_order_relaxed );
      }

      node = other.m_head.load( std::memory_order_relaxed );
      if ( node != &other.m_dummy )
      {
        node_type * next_node = node->m_next_node.load( std::memory_order_relaxed );
        while ( next_node != &m_dummy )
        {
          node = next_node;
          next_node = node->m_next_node.load( std::memory_order_relaxed );
        }
        // When the loop finishes, node points to the dummy of other.
        // So we re-direct the next-node pointer. The next-node pointer of
        // dummy itself has been re-directed above.
        node->m_next_node.store( &other.m_dummy, std::memory_order_relaxed );
      }
    }

    void clear()
    {
      m_dummy.m_next_node.store( nullptr, std::memory_order_relaxed );
      m_head.store( &m_dummy, std::memory_order_relaxed );
      m_tail.store( &m_dummy, std::memory_order_relaxed );
    }

    bool empty() const
    {
      node_type * head = m_head.load( std::memory_order_relaxed );
      return head == &m_dummy && !( head->m_next_node.load( std::memory_order_relaxed ) );
    }

    void push( reference entry )
    {
      entry.m_next_node.store( nullptr, std::memory_order_relaxed );
      node_type * prev_node = m_tail.exchange( &entry, std::memory_order_acq_rel );
      prev_node->m_next_node.store( &entry, std::memory_order_release );
    }

    void push( pointer entry )
    {
      entry->m_next_node.store( nullptr, std::memory_order_relaxed );
      node_type * prev_node = m_tail.exchange( entry, std::memory_order_acq_rel );
      prev_node->m_next_node.store( entry, std::memory_order_release );
    }

    void pushList( reference first_entry, reference last_entry )
    {
      last_entry.m_next_node.store( nullptr, std::memory_order_relaxed );
      node_type * prev_node = m_tail.exchange( &last_entry, std::memory_order_acq_rel );
      prev_node->m_next_node.store( &first_entry, std::memory_order_release );
    }

    void pushList( pointer first_entry, pointer last_entry )
    {
      last_entry->m_next_node.store( nullptr, std::memory_order_relaxed );
      node_type * prev_node = m_tail.exchange( last_entry, std::memory_order_acq_rel );
      prev_node->m_next_node.store( first_entry, std::memory_order_release );
    }

    bool pop( pointer & destination )
    {
      m_lock.lock();

      node_type * head = m_head.load( std::memory_order_relaxed );
      node_type * next = head->m_next_node.load( std::memory_order_acquire );

      if ( head == &m_dummy )
      {
        if ( !next )
        {
          // The queue is empty.
          m_lock.unlock();

          return false;
        }
        m_head.store( next, std::memory_order_relaxed );
        head = next;
        next = next->m_next_node.load( std::memory_order_acquire );
      }

      if ( next )
      {
        m_head.store( next, std::memory_order_relaxed );
        destination = static_cast< pointer >( head );

        m_lock.unlock();

        return true;
      }

      if ( head != m_tail.load( std::memory_order_acquire ) )
      {
        // The queue is empty
        m_lock.unlock();

        return false;
      }

      // We move the dummy node to the end of the queue.
      m_dummy.m_next_node.store( nullptr, std::memory_order_relaxed );
      node_type * prev_tail_node = m_tail.exchange( &m_dummy, std::memory_order_acq_rel );
      prev_tail_node->m_next_node.store( &m_dummy, std::memory_order_release );

      next = head->m_next_node.load( std::memory_order_acquire );
      if ( next )
      {
        m_head.store( next, std::memory_order_relaxed );
        destination = static_cast< pointer >( head );

        m_lock.unlock();

        return true;
      }

      // The queue is empty.
      m_lock.unlock();

      return false;
    }

    bool pop( reference destination )
    {
      return pop( &destination );
    }

    bool try_pop( pointer & destination )
    {
      if ( !m_lock.try_lock() )
      {
        return false;
      }

      node_type * head = m_head.load( std::memory_order_relaxed );
      node_type * next = head->m_next_node.load( std::memory_order_acquire );

      if ( head == &m_dummy )
      {
        if ( !next )
        {
          // The queue is empty.
          m_lock.unlock();

          return false;
        }
        m_head.store( next, std::memory_order_relaxed );
        head = next;
        next = next->m_next_node.load( std::memory_order_acquire );
      }

      if ( next )
      {
        m_head.store( next, std::memory_order_relaxed );
        destination = static_cast< pointer >( head );

        m_lock.unlock();

        return true;
      }

      if ( head != m_tail.load( std::memory_order_acquire ) )
      {
        // The queue is empty
        m_lock.unlock();

        return false;
      }

      // We move the dummy node to the end of the queue.
      m_dummy.m_next_node.store( nullptr, std::memory_order_relaxed );
      node_type * prev_tail_node = m_tail.exchange( &m_dummy, std::memory_order_acq_rel );
      prev_tail_node->m_next_node.store( &m_dummy, std::memory_order_release );

      next = head->m_next_node.load( std::memory_order_acquire );
      if ( next )
      {
        m_head.store( next, std::memory_order_relaxed );
        destination = static_cast< pointer >( head );

        m_lock.unlock();

        return true;
      }

      // The queue is empty.
      m_lock.unlock();

      return false;
    }

    bool try_pop( reference destination )
    {
      return try_pop( &destination );
    }

  private:
    alignas( T_CACHE_LINE_SIZE ) SpinLock m_lock;
    alignas( T_CACHE_LINE_SIZE ) node_type m_dummy;
    alignas( T_CACHE_LINE_SIZE ) std::atomic< node_type * > m_head;
    alignas( T_CACHE_LINE_SIZE ) std::atomic< node_type * > m_tail;
};



template< typename T_VALUE,
          std::size_t MAX_NUM_QUEUES,
          std::size_t T_CACHE_LINE_SIZE >
class alignas( T_CACHE_LINE_SIZE ) distributed_queue_intrusive
{
  typedef array_fixed_size< concurrent_queue_intrusive< T_VALUE, T_CACHE_LINE_SIZE >,
                            MAX_NUM_QUEUES,
                            T_CACHE_LINE_SIZE > array_type;
  typedef distributed_queue_intrusive< T_VALUE,
                                       MAX_NUM_QUEUES,
                                       T_CACHE_LINE_SIZE > this_type;

  public:
    typedef T_VALUE value_type;
    typedef value_type & reference;
    typedef const value_type & const_reference;
    typedef value_type * pointer;
    typedef const value_type * const_pointer;
    typedef std::ptrdiff_t difference_type;

    distributed_queue_intrusive( std::size_t num_queues = MAX_NUM_QUEUES )
        : m_queues()
        , m_num_queues( num_queues )
    {}

    // Our instrusive containers are not copyable since
    // this is a dangerous operation.
    distributed_queue_intrusive( const this_type & ) = delete;

    distributed_queue_intrusive( this_type && ) = default;

    // Our instrusive containers are not copyable since
    // this is a dangerous operation.
    this_type & operator=( const this_type & ) = delete;

    this_type & operator=( this_type && ) = default;

    void swap( this_type & other )
    {
      using std::swap;
      for ( std::size_t i = 0; i < MAX_NUM_QUEUES; ++i )
      {
        swap( m_queues[i], other.m_queues[i] );
      }
    }

    void push( T_VALUE & value )
    {
      m_queues[ThreadContext::getThreadId() % m_num_queues].push( value );
    }

    void push( T_VALUE * value )
    {
      m_queues[ThreadContext::getThreadId() % m_num_queues].push( value );
    }

    void push( std::size_t queue_hint, T_VALUE & value )
    {
      m_queues[queue_hint % m_num_queues].push( value );
    }

    void push( std::size_t queue_hint, T_VALUE * value )
    {
      m_queues[queue_hint % m_num_queues].push( value );
    }

    void pushList( T_VALUE & first_value, T_VALUE & last_value )
    {
      m_queues[ThreadContext::getThreadId() % m_num_queues].pushList( first_value, last_value );
    }

    void pushList( T_VALUE * first_value, T_VALUE * last_value )
    {
      m_queues[ThreadContext::getThreadId() % m_num_queues].pushList( first_value, last_value );
    }

    void pushList( std::size_t queue_hint, T_VALUE & first_value, T_VALUE & last_value )
    {
      m_queues[queue_hint % m_num_queues].pushList( first_value, last_value );
    }

    void pushList( std::size_t queue_hint, T_VALUE * first_value, T_VALUE * last_value )
    {
      m_queues[queue_hint % m_num_queues].pushList( first_value, last_value );
    }

    bool pop( T_VALUE * & destination )
    {
      std::size_t start = ThreadContext::getThreadId();

      for ( std::size_t i = 0; i < m_num_queues; ++i )
      {
        std::size_t index = ( start + i ) % m_num_queues;
        if ( m_queues[index].pop( destination ) )
        {
          // We have successfully executed the operation on a partial data structure.
          // So we are done at this point.
          return true;
        }
      }

      // Once we reach this point, we failed to execute the operation on any
      // of the partial data structures. So we return false here.
      return false;
    }

    bool pop( T_VALUE & destination )
    {
      return pop( &destination );
    }

    bool try_pop( T_VALUE * & destination )
    {
      std::size_t start = ThreadContext::getThreadId();

      for ( std::size_t i = 0; i < m_num_queues; ++i )
      {
        std::size_t index = ( start + i ) % m_num_queues;
        if ( m_queues[index].try_pop( destination ) )
        {
          // We have successfully executed the operation on a partial data structure.
          // So we are done at this point.
          return true;
        }
      }

      // Once we reach this point, we failed to execute the operation on any
      // of the partial data structures. So we return false here.
      return false;
    }

    bool try_pop( T_VALUE & destination )
    {
      return try_pop( &destination );
    }

    bool empty() const
    {
      for ( std::size_t i = 0; i < m_num_queues; ++i )
      {
        if ( !m_queues[i].empty() )
        {
          return false;
        }
      }

      // Once we reach this point, we have checked that all partial data structures
      // are empty.
      return true;
    }

    void clear()
    {
      for ( std::size_t i = 0; i < m_num_queues; ++i )
      {
        m_queues[i].clear();
      }
    }

  private:
    array_type m_queues;
    std::size_t m_num_queues;
};



/**
 * This class implements a list of tasks. This list
 * is intrusive, meaning no memory is allocated, but
 * instead a pointer is re-used that is anyway part of the
 * basic task classes.
 * The idea of this task list is to provide a means to remember
 * a list of created task, which are then spawned all at once.
 */
template< typename T_TASK >
class TaskListTmpl
{
  typedef TaskListTmpl< T_TASK > this_type;

  public:
    template< bool is_const = false >
    class TaskListIterator
    {
        // This friend declaration is necessary in order to be able
        // to convert a non-const iterator into a const iterator,
        // while keeping members private.
        friend class TaskListIterator< true >;

        friend class TaskListTmpl< T_TASK >;

        typedef typename std::conditional< is_const,
                                           const concurrent_queue_intrusive_node,
                                           concurrent_queue_intrusive_node >::type node_base_type;
        typedef typename std::conditional< is_const,
                                           const T_TASK,
                                           T_TASK >::type node_type;
        node_base_type * m_node;

      public:
        typedef T_TASK value_type;
        typedef std::ptrdiff_t difference_type;
        typedef typename std::conditional< is_const,
                                           const T_TASK *,
                                           T_TASK * >::type pointer;
        typedef typename std::conditional< is_const,
                                           const T_TASK &,
                                           T_TASK & >::type reference;
        typedef std::forward_iterator_tag iterator_category;

        TaskListIterator() : m_node( 0 ) {}
        explicit TaskListIterator( node_base_type * node ) : m_node( node ) {}
        TaskListIterator( const TaskListIterator< false > & other ) : m_node( other.m_node ) {}
        bool valid() const { return ( !!m_node ); }
        reference value() const { return *value_ptr(); }
        pointer value_ptr() const { return static_cast< pointer >( m_node ); }
        reference operator*() const { return value(); }
        pointer operator->() const { return value_ptr(); }
        // This is the overload of the prefix increment
        // operator.
        TaskListIterator & operator++()
        {
          m_node = m_node->m_next_node.load( std::memory_order_relaxed );
          return *this;
        }
        // This is the overload of the postfix increment
        // operator.
        TaskListIterator operator++( int )
        {
          TaskListIterator tmp( *this );
          m_node = m_node->m_next_node.load( std::memory_order_relaxed );
          return tmp;
        }

        friend bool operator==( const TaskListIterator & lhs,
                                const TaskListIterator & rhs )
        { return lhs.m_node == rhs.m_node; }
        friend bool operator!=( const TaskListIterator & lhs,
                                const TaskListIterator & rhs )
        { return lhs.m_node != rhs.m_node; }
    };

    typedef T_TASK value_type;
    typedef unsigned int size_type;
    typedef std::ptrdiff_t difference_type;
    typedef T_TASK & reference;
    typedef const T_TASK & const_reference;
    typedef T_TASK * pointer;
    typedef const T_TASK * const_pointer;
    typedef TaskListIterator< false > iterator;
    typedef TaskListIterator< true > const_iterator;

  public:
    TaskListTmpl()
        : m_head()
        , m_last( nullptr )
    {
      m_head.m_next_node.store( nullptr,
                                std::memory_order_relaxed );
    }

    // Our task list is not copyable since
    // this is a dangerous operation.
    TaskListTmpl( const this_type & ) = delete;

    TaskListTmpl( this_type && other )
        : m_head()
        , m_last( other.m_last )
    {
      m_head.m_next_node.store( other.m_head.m_next_node.load( std::memory_order_relaxed ),
                                std::memory_order_relaxed );
      other.clear();
    }

    // Our task list is not copyable since
    // this is a dangerous operation.
    this_type & operator=( const this_type & ) = delete;

    this_type & operator=( this_type && other )
    {
      m_head.m_next_node.store( other.m_head.m_next_node.load( std::memory_order_relaxed ),
                                std::memory_order_relaxed );
      m_last = other.m_last;
      other.clear();

      return *this;
    }

    void swap( this_type & other )
    {
      concurrent_queue_intrusive_node * tmp =
          m_head.m_next_node.load( std::memory_order_relaxed );
      m_head.m_next_node.store( other.m_head.m_next_node.load( std::memory_order_relaxed ),
                                std::memory_order_relaxed );
      other.m_head.m_next_node.store( tmp, std::memory_order_relaxed );

      T_TASK * tmp_last = m_last;
      m_last = other.m_last;
      other.m_last = tmp_last;
    }

    // Iterators
    iterator before_begin() { return iterator( &( m_head ) ); }
    const_iterator before_begin() const { return const_iterator( &( m_head ) ); }
    const_iterator cbefore_begin() const { return const_iterator( &( m_head ) ); }
    iterator begin() { return iterator( m_head.m_next_node.load( std::memory_order_relaxed ) ); }
    const_iterator begin() const { return const_iterator( m_head.m_next_node.load( std::memory_order_relaxed ) ); }
    const_iterator cbegin() const { return const_iterator( m_head.m_next_node.load( std::memory_order_relaxed ) ); }
    iterator end() { return iterator( 0 ); }
    const_iterator end() const { return const_iterator( 0 ); }
    const_iterator cend() const { return const_iterator( 0 ); }

    // Other functions
    reference front() { return *static_cast< T_TASK * >( m_head.m_next_node.load( std::memory_order_relaxed ) ); }
    const_reference front() const { return *static_cast< T_TASK * >( m_head.m_next_node.load( std::memory_order_relaxed ) ); }
    reference back() { return *m_last; }
    const_reference back() const { return *m_last; }
    bool empty() const { return m_head.m_next_node.load( std::memory_order_relaxed ) == nullptr; }

    void clear()
    {
      m_head.m_next_node.store( nullptr, std::memory_order_relaxed );
      m_last = nullptr;
    }

    iterator insert_after( const_iterator pos, reference value )
    {
      return iterator( insert_after_impl( pos.m_node, &value ) );
    }

    iterator insert_after( const_iterator pos, pointer value )
    {
      return iterator( insert_after_impl( pos.m_node, value ) );
    }

    iterator erase_after( const_iterator pos )
    {
      concurrent_queue_intrusive_node * i_non_const =
          const_cast< concurrent_queue_intrusive_node * >( pos.m_node );
      concurrent_queue_intrusive_node * next_node =
          i_non_const->m_next_node.load( std::memory_order_relaxed );
      concurrent_queue_intrusive_node * next_next_node =
          next_node->m_next_node.load( std::memory_order_relaxed );

      if ( !next_next_node )
      {
        if ( pos.m_node == &( m_head ) )
        {
          m_last = nullptr;
        }
        else
        {
          m_last = static_cast< T_TASK * >( i_non_const );
        }
      }

      i_non_const->m_next_node.store( next_next_node,
                                      std::memory_order_relaxed );

      return iterator( next_next_node );
    }

    void splice_after( const_iterator pos, this_type && other )
    {
      if ( other.empty() )
      {
        return;
      }

      concurrent_queue_intrusive_node * i_non_const =
          const_cast< concurrent_queue_intrusive_node * >( pos.m_node );
      concurrent_queue_intrusive_node * next_node =
          i_non_const->m_next_node.load( std::memory_order_relaxed );
      i_non_const->m_next_node.store( other.m_head.m_next_node.load( std::memory_order_relaxed ),
                                      std::memory_order_relaxed );

      if ( next_node )
      {
        while ( i_non_const->m_next_node.load( std::memory_order_relaxed ) )
        {
          i_non_const = i_non_const->m_next_node.load( std::memory_order_relaxed );
        }
        i_non_const->m_next_node.store( next_node, std::memory_order_relaxed );
      }
      else
      {
        m_last = other.m_last;
      }

      other.clear();
    }

    void push_front( reference value )
    {
      insert_after_impl( &( m_head ), &value );
    }

    void push_front( pointer value )
    {
      insert_after_impl( &( m_head ), value );
    }

    void splice_back( this_type && other )
    {
      if ( other.empty() )
      {
        return;
      }

      concurrent_queue_intrusive_node * node = m_last;
      if ( empty() )
      {
        node = &m_head;
      }

      node->m_next_node.store( other.m_head.m_next_node.load( std::memory_order_relaxed ),
                               std::memory_order_relaxed );
      m_last = other.m_last;

      other.clear();
    }

    void pop_front()
    {
      erase_after( const_iterator( &( m_head ) ) );
    }

  private:
    concurrent_queue_intrusive_node *
    insert_after_impl( const concurrent_queue_intrusive_node * i, pointer value )
    {
      concurrent_queue_intrusive_node * i_non_const =
          const_cast< concurrent_queue_intrusive_node * >( i );
      concurrent_queue_intrusive_node * next_node =
          i_non_const->m_next_node.load( std::memory_order_relaxed );
      value->m_next_node.store( next_node,
                                std::memory_order_relaxed );
      i_non_const->m_next_node.store( value, std::memory_order_relaxed );

      if ( !next_node )
      {
        m_last = value;
      }

      return value;
    }

  private:
    concurrent_queue_intrusive_node m_head;
    T_TASK * m_last;
};



template< typename T_THREAD_POOL,
          typename T_TASK >
class StructuredTaskGroup
{
  typedef StructuredTaskGroup< T_THREAD_POOL, T_TASK > this_type;

  public:
    StructuredTaskGroup( T_THREAD_POOL * pool_ptr )
        : m_waiting_refcount( 0 )
        , m_pool_ptr( pool_ptr )
    {}

    /**
     * Synchronization handles are not copy constructible.
     */
    StructuredTaskGroup( const this_type & ) = delete;
    StructuredTaskGroup( this_type && other )
        : m_waiting_refcount( 0 )
        , m_pool_ptr( other.m_pool_ptr )
    {}

    ~StructuredTaskGroup()
    {
      wait();
    }

    /**
     * Synchronization handles are not assignable
     */
    this_type & operator=( const this_type & ) = delete;
    this_type & operator=( this_type && other )
    {
      m_waiting_refcount.store( 0, std::memory_order_release );
      m_pool_ptr = other.m_pool_ptr;

      return *this;
    }

    void spawn( T_TASK * task )
    {
      m_waiting_refcount.fetch_add( 1, std::memory_order_acq_rel );
      task->m_waiting_refcount = &m_waiting_refcount;
      m_pool_ptr->enqueueTask( task );
    }

    void spawn( std::size_t queue_hint, T_TASK * task )
    {
      m_waiting_refcount.fetch_add( 1, std::memory_order_acq_rel );
      task->m_waiting_refcount = &m_waiting_refcount;
      m_pool_ptr->enqueueTask( queue_hint, task );
    }

    void spawn( TaskListTmpl< T_TASK > & task_list )
    {
      if ( !task_list.empty() )
      {
        for ( T_TASK & task : task_list )
        {
          m_waiting_refcount.fetch_add( 1, std::memory_order_acq_rel );
          task.m_waiting_refcount = &m_waiting_refcount;
        }
      }

      m_pool_ptr->enqueueTaskList( task_list );
    }

    void spawn( std::size_t queue_hint, TaskListTmpl< T_TASK > & task_list )
    {
      if ( !task_list.empty() )
      {
        for ( T_TASK & task : task_list )
        {
          m_waiting_refcount.fetch_add( 1, std::memory_order_acq_rel );
          task->m_waiting_refcount = &m_waiting_refcount;
        }
      }

      m_pool_ptr->enqueueTaskList( queue_hint, task_list );
    }

    void wait()
    {
      hybrid_backoff bkoff;
      while ( m_waiting_refcount.load( std::memory_order_acquire ) > 0 )
      {
        if ( m_pool_ptr->stealTask() )
        {
          bkoff.reset();
        }
        else
        {
          bkoff.backoff();
        }
      }
    }

    void incrementNumberOfAwaitedTaskCompletions( int num_addition_tasks )
    {
      m_waiting_refcount.fetch_add( num_addition_tasks, std::memory_order_acq_rel );
    }

    void spawn_and_wait( T_TASK * task )
    {
      m_waiting_refcount.fetch_add( 1, std::memory_order_acq_rel );
      task->m_waiting_refcount = &m_waiting_refcount;

      do
      {
        task->m_pool_ptr = m_pool_ptr;
        task->execute();

        T_TASK * next_task = nullptr;
        if ( task->m_successor_task && task->m_successor_task->decrementRefCount() <= 0 )
        {
          // The successor task is ready to be executed.
          // We take the successor task as our next
          // task to execute.
          next_task = task->m_successor_task;
        }

        if ( task->m_waiting_refcount )
        {
          task->m_waiting_refcount->fetch_sub( 1, std::memory_order_acq_rel );
        }

        task = next_task;
      } while( task );

      wait();
    }

    T_THREAD_POOL * threadPool() const
    {
      return m_pool_ptr;
    }

    void reset()
    {
      m_waiting_refcount.store( 0, std::memory_order_acq_rel );
    }

  private:
    std::atomic_int m_waiting_refcount;
    mutable T_THREAD_POOL * m_pool_ptr;
};



template< typename T_THREAD_POOL,
          typename T_DERIVED >
class StructuredTask : public concurrent_queue_intrusive_node
{
  template< typename X_THREAD_POOL,
            typename X_TASK >
  friend class StructuredTaskGroup;

  template< typename X_QUEUE,
            typename X_TASK >
  friend class StructuredWorkStealingThreadPoolWorker;

  template< std::size_t X_MAX_NUM_THREADS,
            unsigned int X_CACHE_LINE_SIZE,
            typename X_TASK >
  friend class StructuredWorkStealingThreadPool;

  typedef StructuredTask< T_THREAD_POOL, T_DERIVED > this_type;
  typedef T_THREAD_POOL pool_type;
  typedef unsigned int T_STATUS_MASK_TYPE;

  static const T_STATUS_MASK_TYPE TERMINATION_MASK = 1;

  public:
    StructuredTask()
        : m_pool_ptr( nullptr )
        , m_task_status( 0 )
        , m_refcount( 0 )
        , m_successor_task( nullptr )
        , m_waiting_refcount( nullptr )
        , m_func_ref()
    {}

    template< class T_CALLABLE >
    StructuredTask( T_CALLABLE & callable )
        : StructuredTask()
    {
      setCallable( callable );
    }

    StructuredTask( const this_type & other )
        : m_pool_ptr( other.m_pool_ptr )
        , m_task_status( other.m_task_status )
        , m_refcount( other.m_refcount.load( std::memory_order_relaxed ) )
        , m_successor_task( other.m_successor_task )
        , m_waiting_refcount( other.m_waiting_refcount )
        , m_func_ref( other.m_func_ref )
    {}

    StructuredTask( this_type && other )
        : m_pool_ptr( other.m_pool_ptr )
        , m_task_status( other.m_task_status )
        , m_refcount( other.m_refcount.load( std::memory_order_relaxed ) )
        , m_successor_task( other.m_successor_task )
        , m_waiting_refcount( other.m_waiting_refcount )
        , m_func_ref( std::move( other.m_func_ref ) )
    {}

    virtual ~StructuredTask() {}

    this_type & operator=( const this_type & other )
    {
      m_pool_ptr = other.m_pool_ptr;
      m_task_status = other.m_task_status;
      m_refcount.store( other.m_refcount.load( std::memory_order_relaxed ), std::memory_order_relaxed );
      m_successor_task = other.m_successor_task;
      m_waiting_refcount = other.m_waiting_refcount;
      m_func_ref = other.m_func_ref;

      return *this;
    }

    this_type & operator=( this_type && other )
    {
      m_pool_ptr = other.m_pool_ptr;
      m_task_status = other.m_task_status;
      m_refcount.store( other.m_refcount.load( std::memory_order_relaxed ), std::memory_order_relaxed );
      m_successor_task = other.m_successor_task;
      m_waiting_refcount = other.m_waiting_refcount;
      m_func_ref = std::move( other.m_func_ref );

      return *this;
    }

    void spawn( T_DERIVED * task ) const
    {
      m_pool_ptr->enqueueTask( task );
    }

    void spawn( std::size_t queue_hint, T_DERIVED * task ) const
    {
      m_pool_ptr->enqueueTask( queue_hint, task );
    }

    void spawn( TaskListTmpl< T_DERIVED > & task_list ) const
    {
      m_pool_ptr->enqueueTaskList( task_list );
    }

    void spawn( std::size_t queue_hint, TaskListTmpl< T_DERIVED > & task_list ) const
    {
      m_pool_ptr->enqueueTaskList( queue_hint, task_list );
    }

    T_DERIVED * getSuccessorTask() const
    {
      return m_successor_task;
    }

    void setSuccessorTask( T_DERIVED * successor_task )
    {
      if ( m_successor_task )
      {
        m_successor_task->m_refcount.fetch_sub( 1, std::memory_order_acq_rel );
      }

      m_successor_task = successor_task;

      if ( m_successor_task )
      {
        m_successor_task->m_refcount.fetch_add( 1, std::memory_order_acq_rel );
      }
    }

    void moveWaitingConditionTo( T_DERIVED * new_task_to_wait_for )
    {
      if ( new_task_to_wait_for != this )
      {
        new_task_to_wait_for->m_waiting_refcount = m_waiting_refcount;
        m_waiting_refcount = nullptr;
      }
    }

    void extendWaitingConditionToNextExecutionInstance()
    {
      if ( m_waiting_refcount )
      {
        m_waiting_refcount->fetch_add( 1, std::memory_order_acq_rel );
      }
    }

    void reset()
    {
      m_pool_ptr = nullptr;
      m_task_status = 0;
      m_refcount.store( 0, std::memory_order_acq_rel );
      m_successor_task = nullptr;
      m_waiting_refcount = nullptr;
    }

    template< class T_CALLABLE >
    void setCallable( T_CALLABLE & callable )
    {
      setCallableImpl( callable );
    }

  protected:
    int decrementRefCount()
    {
      return m_refcount.fetch_sub( 1, std::memory_order_acq_rel ) - 1;
    }

    int addRefCount( int count )
    {
      return m_refcount.fetch_add( count, std::memory_order_acq_rel ) + count;
    }

  private:
    void execute()
    {
      m_func_ref();
    }

    template< class T_CALLABLE >
    void setCallableImpl( T_CALLABLE & callable )
    {
      m_func_ref = detail::function_ref< void() >( std::forward< T_CALLABLE >( callable ) );
    }

  protected:
    mutable pool_type * m_pool_ptr;
    T_STATUS_MASK_TYPE m_task_status;
    std::atomic_int m_refcount;
    T_DERIVED * m_successor_task;
    std::atomic_int * m_waiting_refcount;
    detail::function_ref< void() > m_func_ref;
};



template< typename T_QUEUE,
          typename T_TASK >
class StructuredWorkStealingThreadPoolWorker
{
  typedef StructuredWorkStealingThreadPoolWorker< T_QUEUE,
                                                  T_TASK > this_type;
  typedef T_QUEUE task_queue_type;
  typedef T_TASK task_type;

  public:
    StructuredWorkStealingThreadPoolWorker()
        : m_queue( nullptr )
        , m_thread_id( 0 )
        , m_thread()
    {}

    ~StructuredWorkStealingThreadPoolWorker()
    {
      joinWithThread();
    }

    // Disable copying and moving
    StructuredWorkStealingThreadPoolWorker( const this_type & ) = delete;
    StructuredWorkStealingThreadPoolWorker( this_type && ) = delete;
    this_type & operator=( const this_type & ) = delete;
    this_type & operator=( this_type && ) = delete;

    void startWorker( task_queue_type * queue,
                      std::size_t thread_id )
    {
      m_queue = queue;
      m_thread_id = thread_id;
      m_thread = std::thread( &this_type::run, this );
    }

    void joinWithThread()
    {
      if ( m_thread.joinable() )
      {
        m_thread.join();
      }
    }

  private:
    void run()
    {
      ThreadContext::setThreadId( m_thread_id );

      task_type * task;
      hybrid_backoff bkoff;

      while ( true )
      {
        bkoff.reset();
        while ( !m_queue->pop( task ) )
        {
          // Either the queue is empty or it was contended. Back-off before we retry.
          bkoff.backoff();
        }
        if ( task->m_task_status & task_type::TERMINATION_MASK )
        {
          // We received a signal to terminate.
          return;
        }
        else
        {
          do
          {
            task->execute();

            task_type * next_task = nullptr;
            if ( task->m_successor_task && task->m_successor_task->decrementRefCount() <= 0 )
            {
              // The successor task is ready to be executed.
              // We take the successor task as our next
              // task to execute.
              next_task = task->m_successor_task;
              next_task->m_pool_ptr = task->m_pool_ptr;
            }

            if ( task->m_waiting_refcount )
            {
              task->m_waiting_refcount->fetch_sub( 1, std::memory_order_acq_rel );
            }

            task = next_task;
          } while( task );
        }
      }
    }

  private:
    task_queue_type * m_queue;
    std::size_t m_thread_id;
    std::thread m_thread;
};



template< std::size_t MAX_NUM_THREADS,
          unsigned int T_CACHE_LINE_SIZE,
          typename T_TASK >
class StructuredWorkStealingThreadPool
{
  public:
    typedef StructuredWorkStealingThreadPool< MAX_NUM_THREADS,
                                              T_CACHE_LINE_SIZE,
                                              T_TASK > this_type;
    typedef T_TASK task_type;
    typedef distributed_queue_intrusive< task_type,
                                         MAX_NUM_THREADS,
                                         T_CACHE_LINE_SIZE > task_queue_type;
    typedef StructuredWorkStealingThreadPoolWorker< task_queue_type,
                                                    task_type > worker_type;

  public:
    StructuredWorkStealingThreadPool( std::size_t num_threads )
        : m_task_queue( std::min( num_threads, MAX_NUM_THREADS ) )
        , m_num_threads( std::min( num_threads, MAX_NUM_THREADS ) )
        , m_termination_tasks()
        , m_workers()
        , m_workers_started()
    {
      m_workers_started.clear( std::memory_order_release );
      // Initialize the thread ID of the calling main thread.
      ThreadContext::initMainThreadId();

      for( std::size_t i = 0; i < MAX_NUM_THREADS; ++i )
      {
        m_termination_tasks[i].m_task_status |= task_type::TERMINATION_MASK;
      }
    }

    // Disable copying and moving
    StructuredWorkStealingThreadPool( const this_type & ) = delete;
    StructuredWorkStealingThreadPool( this_type && ) = delete;
    this_type & operator=( const this_type & ) = delete;
    this_type & operator=( this_type && ) = delete;

    ~StructuredWorkStealingThreadPool()
    {
      shutdown();
    }

    void shutdown()
    {
      // Send a termination message to all workers.
      for( std::size_t i = 0; i < m_num_threads - 1; ++i )
      {
        m_task_queue.push( i, m_termination_tasks[i] );
      }

      for( std::size_t i = 0; i < m_num_threads - 1; ++i )
      {
        m_workers[i].joinWithThread();
      }

      m_task_queue.clear();
      m_workers_started.clear( std::memory_order_release );
    }

    void enqueueTask( task_type * task )
    {
      task->m_pool_ptr = this;
      m_task_queue.push( task );
      startWorkers();
    }

    void enqueueTask( std::size_t queue_hint, task_type * task )
    {
      task->m_pool_ptr = this;
      m_task_queue.push( queue_hint, task );
      startWorkers();
    }

    void enqueueTaskList( TaskListTmpl< task_type > & task_list )
    {
      if ( !task_list.empty() )
      {
        for ( task_type & task : task_list )
        {
          task.m_pool_ptr = this;
        }

        m_task_queue.pushList( task_list.front(), task_list.back() );
        startWorkers();
      }
    }

    void enqueueTaskList( std::size_t queue_hint, TaskListTmpl< task_type > & task_list )
    {
      if ( !task_list.empty() )
      {
        for ( task_type & task : task_list )
        {
          task.m_pool_ptr = this;
        }

        m_task_queue.pushList( queue_hint, task_list.front(), task_list.back() );
        startWorkers();
      }
    }

    bool stealTask()
    {
      task_type * task;

      if ( m_task_queue.pop( task ) )
      {
        if ( task->m_task_status & task_type::TERMINATION_MASK )
        {
          // We received a signal to terminate.
          return false;
        }
        else
        {
          do
          {
            task->execute();

            task_type * next_task = nullptr;
            if ( task->m_successor_task && task->m_successor_task->decrementRefCount() <= 0 )
            {
              // The successor task is ready to be executed.
              // We take the successor task as our next
              // task to execute.
              next_task = task->m_successor_task;
              next_task->m_pool_ptr = task->m_pool_ptr;
            }

            if ( task->m_waiting_refcount )
            {
              task->m_waiting_refcount->fetch_sub( 1, std::memory_order_acq_rel );
            }

            task = next_task;
          } while( task );

          return true;
        }
      }

      return false;
    }

  private:
    void startWorkers()
    {
      if ( !m_workers_started.test_and_set( std::memory_order_acquire ) )
      {
        for( std::size_t i = 0; i < m_num_threads - 1; ++i )
        {
          m_workers[i].startWorker( &m_task_queue, i + 1 );
        }
      }
    }

  private:
    task_queue_type m_task_queue;
    const std::size_t m_num_threads;
    array_fixed_size< task_type,
                      MAX_NUM_THREADS,
                      T_CACHE_LINE_SIZE > m_termination_tasks;
    array_fixed_size< worker_type,
                      MAX_NUM_THREADS - 1,
                      T_CACHE_LINE_SIZE > m_workers;
    std::atomic_flag m_workers_started;
};



template< bool B,
          typename T = void >
using enable_if_t = typename std::enable_if< B, T >::type;



template< typename T_CALLABLE,
          typename ...T_ARGS >
struct TakesArgumentsHelper
{
  typedef std::true_type yes_type;
  typedef std::false_type no_type;

  // Declare a value of type FUNC and try to call it
  // using the argument types provided.
  template< typename X_CALLABLE >
  static yes_type test( decltype( std::declval< X_CALLABLE & >()( std::declval< T_ARGS >()... ) )* )
  {
    return yes_type{};
  }

  template< typename FUNC >
  static no_type test(...)
  {
    return no_type{};
  }

  static constexpr bool value = std::is_same< decltype( test< T_CALLABLE >( nullptr ) ), yes_type >::value;
};

} // namespace detail










/**
 * This class provides a drop-in replacement for std::mutex.
 * Instead of a real mutex, this class implements a spinlock.
 * Locking amounts to atomically testing and setting a variable.
 * The back-off strategy is incremental deferring the calling
 * task more and more until it finally gets the lock.
 */
class SpinLock : private detail::SpinLock
{
  typedef detail::SpinLock base_type;
  typedef SpinLock this_type;

  public:
    SpinLock() : base_type() {}

    SpinLock( const this_type & ) = delete;
    SpinLock( this_type && ) = delete;
    this_type & operator=( const this_type & ) = delete;
    this_type & operator=( this_type && ) = delete;

    /**
     * Locks the mutex and returns.
     */
    void lock() { base_type::lock(); }

    /**
     * Unlocks the mutex and returns.
     */
    void unlock() { base_type::unlock(); }

    /**
     * Tries to lock the mutex and returns whether it
     * has successfully locked the mutex.
     *
     * @return true, if mutex locked, false otherwise
     */
    bool try_lock() { return base_type::try_lock(); }
};



/**
 * This back-off strategy is a mixture of different back-off
 * mechanisms and implements incremental back-off.
 * An instance of this class keeps track of the "back-off state",
 * which is advanced on each call to backoff().
 */
class hybrid_backoff : private detail::hybrid_backoff
{
  typedef detail::hybrid_backoff base_type;
  typedef hybrid_backoff this_type;

  public:
    hybrid_backoff() : base_type() {}

    hybrid_backoff( const this_type & ) = delete;
    hybrid_backoff & operator=( const this_type & ) = delete;

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
    void backoff() { base_type::backoff(); }

    /**
     * This method resets the state of this back-off object.
     */
    void reset() { base_type::reset(); }
};



/**
 * This class provides some static methods to query
 * the unique ID of a thread. Each thread of a thread pool
 * will be assigned a unique ID. These IDs will form a
 * contiguous range starting from 0. This allows using them
 * as indexes of an array.
 */
class ThreadContext
{
  public:
    /**
     * This static method returns the ID of the calling thread.
     *
     * @return the unique ID of the calling thread
     */
    static std::size_t getThreadId()
    {
      return detail::ThreadContext::getThreadId();
    }
};



/**
 * This template provides a thread-local storage. It owns a
 * static array of fixed size (the maximum number of threads).
 * This thread local storage can only be used in conjunction
 * with the thread pool classes, because it relies on properly
 * set thread IDs, which the thread pool classes do.
 *
 * The template provides an iterator that can be used to iterate
 * over all thread specific values held by it. The member
 * function #local() returns a reference to the thread-specific
 * value of the calling thread. This allows the threads to modify
 * their local values without any synchronization. Later on the
 * values can be aggregated by iterating over them.
 *
 * The template parameter T_CACHE_LINE_SIZE can be used to
 * configure the way the thread-local values are stored.
 */
template< typename T_VALUE,
          unsigned int T_MAX_NUM_THREADS,
          std::size_t T_CACHE_LINE_SIZE = 64 >
class ThreadSpecificValueArray
{
  private:
    typedef detail::array_fixed_size< T_VALUE,
                                      T_MAX_NUM_THREADS,
                                      T_CACHE_LINE_SIZE > array_type;
    typedef ThreadSpecificValueArray< T_VALUE,
                                      T_MAX_NUM_THREADS,
                                      T_CACHE_LINE_SIZE > this_type;

  public:
    using value_type = typename array_type::value_type;
    using size_type = typename array_type::size_type;
    using difference_type = typename array_type::difference_type;
    using reference = typename array_type::reference;
    using const_reference = typename array_type::const_reference;
    using pointer = typename array_type::pointer;
    using const_pointer = typename array_type::const_pointer;
    using iterator = typename array_type::iterator;
    using const_iterator = typename array_type::const_iterator;
    using reverse_iterator = typename array_type::reverse_iterator;
    using const_reverse_iterator = typename array_type::const_reverse_iterator;

  public:
    ThreadSpecificValueArray() = default;
    ThreadSpecificValueArray( const this_type & ) = default;
    ThreadSpecificValueArray( this_type && ) = default;

    // Pass a single parameter down to all partial data structures.
    template< typename Arg >
    ThreadSpecificValueArray( Arg&& arg )
        : m_thread_local_values( std::forward< Arg >( arg ) )
    {}

    this_type & operator= ( const this_type & ) = default;
    this_type & operator=( this_type && ) = default;

    reference local()
    {
      return m_thread_local_values[ThreadContext::getThreadId()];
    }
    const_reference local() const
    {
      return m_thread_local_values[ThreadContext::getThreadId()];
    }

    // Element access
    reference at( size_type pos ) { return m_thread_local_values.at( pos ); }
    const_reference at( size_type pos ) const { return m_thread_local_values.at( pos ); }
    reference operator[]( size_type pos ) { return m_thread_local_values[pos]; }
    const_reference operator[]( size_type pos ) const { return m_thread_local_values[pos]; }
    reference front() { return m_thread_local_values.front(); }
    const_reference front() const { return m_thread_local_values.front(); }
    reference back() { return m_thread_local_values.back(); }
    const_reference back() const { return m_thread_local_values.back(); }

    // Iterators
    iterator begin() { return m_thread_local_values.begin(); }
    const_iterator begin() const { return m_thread_local_values.begin(); }
    const_iterator cbegin() const { return m_thread_local_values.cbegin(); }
    iterator end() { return m_thread_local_values.end(); }
    const_iterator end() const { return m_thread_local_values.end(); }
    const_iterator cend() const { return m_thread_local_values.cend(); }
    reverse_iterator rbegin() { return m_thread_local_values.rbegin(); }
    const_reverse_iterator rbegin() const { return m_thread_local_values.rbegin(); }
    const_reverse_iterator crbegin() const { return m_thread_local_values.crbegin(); }
    reverse_iterator rend() { return m_thread_local_values.rend(); }
    const_reverse_iterator rend() const { return m_thread_local_values.rend(); }
    const_reverse_iterator crend() const { return m_thread_local_values.crend(); }

    constexpr bool empty() const noexcept { return m_thread_local_values.empty(); }
    constexpr size_type size() const noexcept { return m_thread_local_values.size(); }

  private:
    array_type m_thread_local_values;
};





/**
 * This class template provides a work stealing thread
 * pool. Tasks can be defined by creating objects of the nested
 * class \a Task and setting some callable to be executed by them.
 * Such tasks can then be enqueued on
 * this thread pool for execution by some of its threads.
 * This thread pool is in many ways similar to Intel TBBs
 * task scheduler and also provides means to synchronize
 * the execution of tasks. The documentations of the nested
 * classes Task, TaskGroup and Future illustrate these features.
 *
 * The parameters of this template allow to control the maximum
 * number of threads that can be spawned by the thread pool
 * and to configure the size of a cache line of the target
 * CPU. The size of a cache line is often 64 bytes on modern
 * CPUs. The purpose of this template parameter is to avoid
 * false sharing by forcing thread-local data of two different
 * threads to never be placed in the same cache line.
 *
 * Note that the worker threads created by this template do
 * busy waiting, polling the task queue for new tasks to be
 * executed. However, an incremental back-off strategy is applied
 * that ultimately causes the worker threads to wait for some
 * time before looking at the task queue again. See
 * hybrid_backoff::backoff() for a description of that strategy.
 * One could argue that explicitly putting threads in a waiting
 * condition and waking them once there is work to do is a better
 * use of CPU time, but this causes a bottleneck since it requires
 * some kind of global lock or semaphore. Therefore, you should
 * only use this thread pool template, if you can keep the worker
 * threads busy executing something, otherwise you may just waste
 * CPU time.
 *
 * WARNING: Do NOT construct a thread pool from within a thread
 * spawned by another instance of this thread pool template! Otherwise
 * the unique thread IDs get messed up. This is because upon
 * construction of a thread pool, the calling thread always
 * gets assigned the ID 0 and all spawned worker threads N get
 * a thread ID ranging from 1 .. N. Thus, if constructing another
 * thread pool from the context of a thread with ID 1, the thread
 * will have the ID 0 later on. Thus, you end up with at least two
 * threads in the first thread pool having the ID 0. This causes
 * undefined behavior.
 * On the other hand, constructing multiple thread pools from the context
 * of the same thread (say the main thread of you program) is fine.
 * However, in that case some threads from different thread pools may
 * have the same ID, because the assigned thread IDs are only unique
 * with respect to their associated thread pool. That means you must
 * be careful to not mix the thread pools, meaning tasks executed on a
 * thread pool A must not spawn tasks on a thread pool B. Further, instances
 * of the ThreadSpecificValueArray template must not be accessed by tasks/threads
 * associated to different thread pools. This is because in that case
 * different threads having the same ID access the same data, which
 * leads to race conditions.
 */
template< std::size_t MAX_NUM_THREADS,
          unsigned int T_CACHE_LINE_SIZE = 64 >
class WorkStealingThreadPool
{
  typedef WorkStealingThreadPool< MAX_NUM_THREADS,
                                  T_CACHE_LINE_SIZE > pool_this_type;

  public:
    // Forward declaration.
    class Task;

    /**
     * A task group is a concept tracking the execution
     * state of a set of tasks spawned through it. It sole
     * purpose is to provide a similar functionality like
     * joining with threads, but on the level of tasks.
     * This is achieved by the #wait() method of this
     * class: Calling #wait() synchronizes the caller with
     * the point in time, at which all tasks spawned through
     * the task group have finished their execution.
     * Technically this is implemented by the task group
     * tracking the number of tasks spawned via it. Each of
     * these tasks notifies the task group upon its completion.
     *
     * Note that a task can transfer the waiting condition
     * of the task group that has spawned it to some other
     * task. This causes the other task to notify the task
     * group upon its completion. That way task graphs with
     * continuation tasks can be designed.
     */
    class TaskGroup
    {
      typedef TaskGroup this_type;

      public:
        using Task = pool_this_type::Task;
        using TaskList = detail::TaskListTmpl< Task >;

      public:
        TaskGroup( detail::StructuredWorkStealingThreadPool< MAX_NUM_THREADS,
                                                             T_CACHE_LINE_SIZE,
                                                             Task > * pool )
            : m_impl( pool )
        {}

        TaskGroup( const this_type & ) = delete;
        TaskGroup( this_type && other )
            : m_impl( std::move( other.m_impl ) )
        {}

        /**
         * The caller of the destructor synchronizes, waiting until
         * all tasks spawed through the task group have been completed.
         */
        ~TaskGroup() {}

        this_type & operator=( const this_type & ) = delete;
        this_type & operator=( this_type && other )
        {
          m_impl = std::move( other.m_impl );

          return *this;
        }

        /**
         * This method spawn \a task. That means \a task is inserted into
         * the task queue of the associated thread pool.
         * Further, the waiting condition of \a task is initialized such
         * that this task group waits for the completion of \a task.
         *
         * @param task the task to spawn
         */
        void spawn( Task * task )
        {
          m_impl.spawn( task );
        }

        /**
         * This method spawn \a task. That means \a task is inserted into
         * the task queue of the associated thread pool.
         * Further, the waiting condition of \a task is initialized such
         * that this task group waits for the completion of \a task.
         * The parameter \a queue_hint is a hint for insertion into the
         * task queue. Actually, the task queue consists of multiple queues.
         * Each thread is associated with its own queue. Only if it does not
         * find any task inside its own queue, it tries to steal a task from
         * another queue. So if the caller wants to distribute tasks among
         * the worker threads, it can use \a queue_hint to do so.
         * The queue where the task gets inserted into is determined by
         * "queue_hint % num_queues". So a strategy to distribute tasks among
         * these queues might look like:
         * queue_hint = 0;
         * while( true ) { task_group.spawn( queue_hint++, task ); }
         *
         * @param queue_hint the hint for insertion into the task queue
         * @param task the task to spawn
         */
        void spawn( std::size_t queue_hint, Task * task )
        {
          m_impl.spawn( queue_hint, task );
        }

        /**
         * This method spawns a list of tasks \a task_list. That means the
         * tasks in the list are atomically inserted into the task queue of the
         * associated thread pool. Further, the waiting condition of each task
         * is initialized such that this task group waits for the completion
         * of all tasks of \a task_list.
         *
         * @param task_list the list of tasks to spawn
         */
        void spawn( TaskList & task_list )
        {
          m_impl.spawn( task_list );
        }

        /**
         * This method spawns a list of tasks \a task_list. That means the
         * tasks in the list are atomically inserted into the task queue of the
         * associated thread pool. Further, the waiting condition of each task
         * is initialized such that this task group waits for the completion
         * of all tasks of \a task_list. The parameter \a queue_hint is a hint
         * for insertion into the task queue. Actually, the task queue consists
         * of multiple queues. Each thread is associated with its own queue.
         * Only if it does not find any task inside its own queue, it tries to
         * steal a task from another queue. So if the caller wants to distribute
         * tasks among the worker threads, it can use \a queue_hint to do so.
         * The queue where the task gets inserted into is determined by
         * "queue_hint % num_queues". So a strategy to distribute tasks among
         * these queues might look like:
         * queue_hint = 0;
         * while( true ) { task_group.spawn( queue_hint++, task_list ); }
         *
         * @param queue_hint the hint for insertion into the task queue
         * @param task_list the list of tasks to spawn
         */
        void spawn( std::size_t queue_hint, TaskList & task_list )
        {
          m_impl.spawn( queue_hint, task_list );
        }

        /**
         * This method synchronizes the caller with
         * the point in time, at which all tasks spawned through
         * the task group have finished their execution. This means
         * the call of this method returns after all tasks spawned
         * through the task group have finished their execution.
         *
         * Note that a task can transfer the waiting condition
         * of the task group that has spawned it to some other
         * task. This causes the other task to notify the task
         * group upon its completion. That way task graphs with
         * continuation tasks can be designed.
         */
        void wait()
        {
          m_impl.wait();
        }

        /**
         * As documented in the class description of TaskGroup,
         * waiting for the tasks of a task group to complete is
         * implemented by the task group tracking the number of
         * tasks spawned via it. Each of these tasks notifies the
         * task group upon its completion. This method can be
         * used to increase the number of task completions the
         * task group waits for. This can be useful if e.g. you
         * spawned some task A via a task group and you want the
         * task group to also wait for additional execution
         * instances of A that are triggered by setting A as the
         * successor task of some other task.
         *
         * @param num_addition_tasks
         */
        void incrementNumberOfAwaitedTaskCompletions( int num_addition_tasks )
        {
          m_impl.incrementNumberOfAwaitedTaskCompletions( num_addition_tasks );
        }

        /**
         * This method executes \a task in the context of the caller
         * and afterwards waits until all other tasks spawned through
         * the task group have finished their execution. Thus, the
         * effect of this method is similar to calling #spawn(task)
         * followed by calling #wait(). However, there is a slight
         * performance improvement because \a task is not put into
         * the task queue of the thread pool associated with the
         * task group.
         *
         * @param task the task to execute before waiting
         */
        void spawn_and_wait( Task * task )
        {
          m_impl.spawn_and_wait( task );
        }

        /**
         * This method creates and returns a task group that uses
         * the same thread pool associated to this task group. A task group
         * allows for waiting on the completion of all tasks spawned
         * through it.
         *
         * @return a task group
         */
        TaskGroup createTaskGroup() const
        {
          return TaskGroup( m_impl.threadPool() );
        }

        /**
         * This method creates and returns an empty task list
         * where tasks can be inserted into, before spawning the
         * the whole list of tasks.
         * Note that task lists are quite efficient. No memory
         * is allocated, but instead a pointer is re-used to
         * link up tasks, that is anyway part of this task class.
         *
         * @return an empty task list
         */
        TaskList createTaskList() const
        {
          return TaskList();
        }

        /**
         * Resets the task group to a state as if it has just been constructed.
         * The association with the thread pool is kept as is.
         */
        void reset()
        {
          m_impl.reset();
        }

      private:
        detail::StructuredTaskGroup< detail::StructuredWorkStealingThreadPool< MAX_NUM_THREADS,
                                                                               T_CACHE_LINE_SIZE,
                                                                               Task >,
                                     Task > m_impl;
    };



    /**
     * This task class is heavily inspired by Intel TBB library.
     * The basic idea being that you can create a task tree structure,
     * which evolves over time.
     * In this tree each task \a A points to its successor task \a B
     * that is waiting for \a A to complete. A null pointer means that
     * a task has no successor task. Each task has a refcount variable,
     * which counts the number of tasks that have it as a successor.
     * This variable provides the means to decide when a successor task
     * should be executed. However, in contrast to Intel TBB, the refcount
     * variable cannot be manipulated directly. Instead it is managed
     * implicitly when #setSuccessorTask() is called, meaning the refcount
     * of the successor task gets incremented. Note that in contrast
     * to Intel TBB, waiting for some task to complete its execution is
     * realized by a different refcount variable and thus is completely
     * orthogonal to the successor task logic. The only means to wait for
     * some task to complete its execution is to spawn it through some
     * TaskGroup tg and call tg.wait() afterwards, which will wait until
     * all tasks spawned through it have completed their execution.
     *
     * A task carries a reference to some callable to be executed by
     * it. You can pass a reference to that callable at task construction
     * or later on. However, be aware that the task only has a reference
     * to that callalbe and does not store it in any way. So it is your
     * responsibility to ensure its proper lifetime.
     *
     * The next task to be executed by the thread that has executed
     * a task is chosen according to the first applicable rule below:
     * 1. The successor task of this task, if this task was the last
     *    completed predecessor.
     * 2. A task from the queue of the worker thread.
     * 3. A task from the queue of another worker thread.
     * Rules 1 can be used to achieve a task execution ordering. In contrast
     * there is no ordering guarantee for tasks put in the task queue
     * of some worker thread.
     *
     * Note that in contrast to Intel TBB any aspects of memory allocation must
     * be done by the client of this library. That means, the client
     * is responsible for allocating memory for tasks, ensuring that they
     * have a proper lifetime and deallocating memory once tasks are not
     * needed anymore.
     *
     *
     *
     *
     *
     * The following are some examples on how this library can be used. These
     * examples are similar to Intel TBBs catalog of recommended patterns,
     * discussing proper alternatives, where this library does not provide
     * certain features that Intel TBB does:
     *
     * T_THREAD_POOL thread_pool( ... );
     * T t( ... );
     * t.setCallable( t ):
     * T_THREAD_POOL::TaskGroup tg = thread_pool.createTaskGroup();
     * tg.spawn_and_wait( &t );
     *
     *
     *
     * Blocking style with k children.
     * void operator()()
     * {
     *   TaskGroup tg = createTaskGroup();
     *   U t_k( ... ); tg.spawn( &t_k );
     *   ...
     *   U t_2( ... ); tg.spawn( &t_2 );
     *   U t_1( ... ); tg.spawn_and_wait( &t_1 );
     * }
     * This pattern starts with the invocation of task t, which has a function
     * call operator.
     * Once executed, t creates a task group and creates a set of k tasks.
     * The call tg.spawn_and_wait( &t_1 ) combines spawning and waiting.
     * A slightly less efficient alternative would be to spawn all tasks
     * and call tg.wait() afterwards or simply let tg go out of scope,
     * which will automatically call wait() in its destructor.
     * As the T::operator()() synchronizes with the completion of t_1 - t_k,
     * it is fine to allocate t_1 - t_k on the stack when T::operator()() is called.
     * Before T::operator()() returns, at which point t_1 - t_k are destroyed, t_1 - t_k
     * have been executed.
     * Note that it is critical to spawn the tasks through a TaskGroup, otherwise
     * T::operator()() will return before t_1 - t_k have been executed.
     * Note that it is also possible to create a task list containing t_1 - t_k
     * and then spawning that task list.
     *
     * Intel TBB supports recycling a task as a continuation. The meaning is that
     * the task t is not destroyed and is called again once all of its predecessors
     * have been completed. This style is unsupported, but the blocking style can be
     * used in place. Instead of returning after the synchronization on
     * all predecessor tasks, you may just write some code that executes afterwards.
     *
     *
     *
     * Creating a continuation task.
     * void operator()()
     * {
     *   // create the continuation task.
     *   C c = new C( ... );
     *   moveWaitingConditionTo( c );
     *   auto tl = createTaskList();
     *   U t_k = new U( ... ); t_k->setSuccessorTask( c ); tl.push_front( t_k );
     *   ...
     *   U t_1 = new U( ... ); t_1->setSuccessorTask( c ); tl.push_front( t_1 );
     *   spawn( tl );
     * }
     *
     * This pattern starts with the invocation of task t.
     * Once executed, t creates a continuation task c, moves its own waiting condition
     * to c and creates a set of k tasks, setting c as their successor task. However, a task
     * is NOT spawned directly after being created. The reason is that
     * otherwise c might be executed as a successor task BEFORE ALL its
     * predecessors have been linked up. For instance, if t_k would be spawned
     * right away, then it might be executed by some other thread BEFORE t_1 has
     * been constructed. As t_1 has not been constructed yet, c also does not know
     * it as predecessor yet. Therefore, spawning t_k right away can lead to premature
     * execution of the continuation task c. In order to prevent this, a task list
     * is created instead, where each predecessor of task c is added to. Then that
     * list is spawned as a whole. Once all tasks t_1 - t_k are completed, task c gets executed.
     * This can happen before or after t.operator()() returns: It is not defined.
     * Moving the waiting condition of t to c causes the call
     * tg.spawn_and_wait( &t ) to wait until c has finished executing. If the
     * waiting condition would not be moved, then tg.spawn_and_wait( &t ) would
     * return after t.operator()() returns, independently of whether c has
     * been completed.
     *
     *
     *
     * Intel TBB supports a recycling style where the job of one of the tasks
     * t_1 - t_k can be executed by t. In that style t is not deallocated
     * once t.execute() returns. Instead the state of t is updated to reflect
     * the job that would otherwise be executed by say t_1. Then t.execute()
     * returns and is immediately re-entered. While this library can also
     * be used in such a way, it makes little sense to do so. What you should
     * do is the following: Instead of returning from t.execute() after having
     * updated the state of t, one can just compute the job that t_1 would
     * compute otherwise and then return. The following realizes this:
     * void operator()()
     * {
     *   // create the continuation task.
     *   C c = new C( ... );
     *   moveWaitingConditionTo( c );
     *   setSuccessorTask( c );
     *   U t_k = new U( ... ); t_k->setSuccessorTask( c ); spawn( t_k );
     *   ...
     *   U t_2 = new U( ... ); t_2->setSuccessorTask( c ); spawn( t_2 );
     *
     *   ... update state and execute what otherwise t_1 would compute ...
     * }
     * This pattern starts with the invocation of task t.
     * Once executed, t creates a continuation task c, moves its own waiting condition
     * to c, sets c as its new successor and creates a set of k-1 tasks, setting
     * c as their successor task. The predecessor tasks can be spawned directly,
     * because t has set c as its successor BEFORE the other predecessors of
     * c are spawned. Once all tasks t_2 - t_k are completed AND t.operator()()
     * has returned (which also executes something more after having spawned
     * t_2 - t_k), task c gets executed.
     * Moving the waiting condition of t to c causes the call
     * tg.spawn_and_wait( &t ) to wait until c has finished executing. If the
     * waiting condition would not be moved, then tg.spawn_and_wait( &t ) would
     * return after t.operator()() returns, independently of whether c has
     * been completed.
     *
     *
     *
     * Letting the main thread work, while some tasks are running.
     * {
     *   T_THREAD_POOL thread_pool( ... );
     *   T_THREAD_POOL::TaskGroup tg = thread_pool.createTaskGroup();
     *   U t_k( ... ); tg.spawn( &t_k );
     *   ...
     *   U t_1( ... ); tg.spawn( &t_1 );
     *   ... do some other work ...
     *   tg.wait();
     * }
     * In this pattern a TaskGroup tg is created. Then a set of k tasks is created
     * and spawned through tg. Afterwards the main thread does some other work
     * and later on synchronizes with the all spawned task, effectively
     * waiting for t_1 - t_k to complete. Instead of calling tg.wait(),
     * one could also just let tg go out of scope. This has the same effect.
     */
    class Task : public detail::StructuredTask< detail::StructuredWorkStealingThreadPool< MAX_NUM_THREADS,
                                                                                          T_CACHE_LINE_SIZE,
                                                                                          Task >,
                                                Task >
    {
      typedef detail::StructuredTask< detail::StructuredWorkStealingThreadPool< MAX_NUM_THREADS,
                                                                                T_CACHE_LINE_SIZE,
                                                                                Task >,
                                      Task > base_type;
      typedef Task this_type;

      public:
        using TaskGroup = pool_this_type::TaskGroup;
        using TaskList = detail::TaskListTmpl< this_type >;

      public:
        Task()
            : base_type()
        {}

        template< class T_CALLABLE >
        Task( T_CALLABLE & callable )
            : Task( callable )
        {}

        Task( const this_type & ) = default;
        Task( this_type && ) = default;

        virtual ~Task() {}

        this_type & operator=( const this_type & ) = default;
        this_type & operator=( this_type && ) = default;

        /**
         * This method spawn \a task. That means \a task is inserted into
         * the task queue of the associated thread pool.
         * Calling this function is equivalent to calling enqueueTask( task )
         * on the associated thread pool.
         *
         * WARNING: Only call this method AFTER this task has been spawned.
         * Otherwise calling this method results in undefined behavior,
         * because this task does not know its associated thread pool, yet.
         *
         * @param task the task to spawn
         */
        void spawn( this_type * task ) const
        {
          base_type::spawn( task );
        }

        /**
         * This method spawn \a task. That means \a task is inserted into
         * the task queue of the associated thread pool.
         * Calling this function is equivalent to calling
         * enqueueTask( queue_hint, task ) on the associated thread pool.
         * The parameter \a queue_hint is a hint for insertion into the
         * task queue. Actually, the task queue consists of multiple queues.
         * Each thread is associated with its own queue. Only if it does not
         * find any task inside its own queue, it tries to steal a task from
         * another queue. So if the caller wants to distribute tasks among
         * the worker threads, it can use \a queue_hint to do so.
         * The queue where the task gets inserted into is determined by
         * "queue_hint % num_queues". So a strategy to distribute tasks among
         * these queues might look like:
         * queue_hint = 0;
         * while( true ) { spawn( queue_hint++, task ); }
         *
         * WARNING: Only call this method AFTER this task has been spawned.
         * Otherwise calling this method results in undefined behavior,
         * because this task does not know its associated thread pool, yet.
         *
         * @param queue_hint the hint for insertion into the task queue
         * @param task the task to spawn
         */
        void spawn( std::size_t queue_hint, this_type * task ) const
        {
          base_type::spawn( queue_hint, task );
        }

        /**
         * This method spawns a list of tasks \a task_list. That means the
         * tasks in the list are atomically inserted into the task queue of the
         * associated thread pool. Calling this function is equivalent to
         * calling enqueueTaskList( task_list ) on the associated thread pool.
         *
         * WARNING: Only call this method AFTER this task has been spawned.
         * Otherwise calling this method results in undefined behavior,
         * because this task does not know its associated thread pool, yet.
         *
         * @param task_list the list of tasks to spawn
         */
        void spawn( TaskList & task_list ) const
        {
          base_type::spawn( task_list );
        }

        /**
         * This method spawns a list of tasks \a task_list. That means the
         * tasks in the list are atomically inserted into the task queue of the
         * associated thread pool. Calling this function is equivalent to
         * calling enqueueTaskList( queue_hint, task_list ) on the associated
         * thread pool. The parameter \a queue_hint is a hint for insertion
         * into the task queue. Actually, the task queue consists of multiple
         * queues. Each thread is associated with its own queue.
         * Only if it does not find any task inside its own queue, it tries to
         * steal a task from another queue. So if the caller wants to distribute
         * tasks among the worker threads, it can use \a queue_hint to do so.
         * The queue where the task gets inserted into is determined by
         * "queue_hint % num_queues". So a strategy to distribute tasks among
         * these queues might look like:
         * queue_hint = 0;
         * while( true ) { spawn( queue_hint++, task_list ); }
         *
         * WARNING: Only call this method AFTER this task has been spawned.
         * Otherwise calling this method results in undefined behavior,
         * because this task does not know its associated thread pool, yet.
         *
         * @param queue_hint the hint for insertion into the task queue
         * @param task_list the list of tasks to spawn
         */
        void spawn( std::size_t queue_hint, TaskList & task_list ) const
        {
          base_type::spawn( queue_hint, task_list );
        }

        /**
         * Returns the successor task of this task. If upon completion this task
         * is the last predecessor of its successor task, then that successor
         * task gets executed.
         *
         * @return the successor task of this task
         */
        this_type * getSuccessorTask() const
        {
          return base_type::getSuccessorTask();
        }

        /**
         * Sets the successor task of this task. If upon completion this task is
         * the last predecessor of \a successor_task, then \a successor_task
         * gets executed.
         *
         * Note that this method is not thread-safe and must only be called
         * either on a task which was not spawned yet or from within the
         * #execute() method in the context of the executing thread.
         *
         * @param successor_task the new successor task of this task.
         */
        void setSuccessorTask( this_type * successor_task )
        {
          base_type::setSuccessorTask( successor_task );
        }

        /**
         * Moves any waiting condition associated with this task to
         * the other task \a new_task_to_wait_for. A task has a
         * waiting condition, if it has been spawned via a TaskGroup.
         * A task group allows to wait for all the tasks spawned
         * through it to be completed (the call of the execute method
         * by some worker thread returns). If this task has been spawed
         * through a task group \a tg, this method causes a call to
         * tg.wait() to wait until \a new_task_to_wait_for has been
         * completed instead of this task. With this feature one can
         * design task graphs with task continuations like e.g.
         * in Intel TBB.
         *
         * @param new_task_to_wait_for the other task
         */
        void moveWaitingConditionTo( this_type * new_task_to_wait_for )
        {
          base_type::moveWaitingConditionTo( new_task_to_wait_for );
        }

        /**
         * If this task has been spawned via some task group \a tg,
         * then calling tg.wait() would synchronize with the point
         * in time when this task completes. There exist use-cases
         * where you may want to let the same task instance execute
         * multiple times: For example, while executing a task you
         * may want to configure its re-execution by setting it as
         * the successor task of some other task(s). This method
         * supports such use-cases by extending the waiting condition
         * of that task to its next execution instance. That means,
         * in the example above, tg.wait() would then synchronize with
         * the point in time when the re-execution of the task completes.
         */
        void extendWaitingConditionToNextExecutionInstance()
        {
          base_type::extendWaitingConditionToNextExecutionInstance();
        }

        /**
         * This method creates and returns a task group that uses
         * the same thread pool associated to this task. A task group
         * allows for waiting on the completion of all tasks spawned
         * through it.
         *
         * @return a task group
         */
        TaskGroup createTaskGroup() const
        {
          return TaskGroup( this->m_pool_ptr );
        }

        /**
         * This method creates and returns an empty task list
         * where tasks can be inserted into, before spawning the
         * the whole list of tasks.
         * Note that task lists are quite efficient. No memory
         * is allocated, but instead a pointer is re-used to
         * link up tasks, that is anyway part of this task class.
         *
         * @return an empty task list
         */
        TaskList createTaskList() const
        {
          return TaskList();
        }

        /**
         * Resets the task to a state as if it has just been constructed.
         */
        void reset()
        {
          base_type::reset();
        }
    };

    typedef typename Task::TaskList TaskList;



  public:
    /**
     * This creates a work stealing thread pool that uses
     * \a min( num_threads, MAX_NUM_THREADS ) number of threads
     * in order to execute the tasks enqueued on it. The
     * threads will be created when the first task is enqueued
     * on this thread pool.
     *
     * Note that the number of threads includes the main thread
     * of execution that constructs the thread pool. That means
     * if \a num_threads is equal to 1, then no worker thread is
     * spawned and enqueued tasks are only executed by the main
     * thread of execution.
     *
     * @param num_threads the number of threads to be used by the thread pool
     */
    WorkStealingThreadPool( std::size_t num_threads )
        : m_pool_impl( num_threads )
    {}

    // Disable copying and moving
    WorkStealingThreadPool( const pool_this_type & ) = delete;
    WorkStealingThreadPool( pool_this_type && ) = delete;
    pool_this_type & operator=( const pool_this_type & ) = delete;
    pool_this_type & operator=( pool_this_type && ) = delete;

    /**
     * This method calls #shutdown() and destroys the thread pool.
     */
    ~WorkStealingThreadPool()
    {}

    /**
     * This method shuts down the thread pool. It enqueues N special
     * tasks, where N + 1 is the number of threads this thread pool has
     * been constructed with. These special tasks when dequeued by a
     * worker thread, cause it to exit from its main execution loop.
     * The thread of execution calling this method blocks until all
     * worker threads have been terminated. Afterwards the state of this
     * thread pool is as if it has just been constructed, meaning you
     * may again start to enqueue some tasks, which starts the worker
     * threads again.
     *
     * WARNING: This method does not kill any worker thread. You must
     * make sure that all tasks have been executed, meaning the worker
     * threads must be idle. Thus, the sole purpose of this method is
     * to prevent having a thread pool with worker threads that do nothing
     * but to consume CPU time.
     */
    void shutdown()
    {
      m_pool_impl.shutdown();
    }

    /**
     * This method enqueues \a task. That means \a task is inserted into
     * the task queue of this thread pool. If this is the first
     * task to be enqueued, then this pool starts its worker threads.
     *
     * @param task the task to insert
     */
    void enqueueTask( Task * task )
    {
      m_pool_impl.enqueueTask( task );
    }

    /**
     * This method enqueues \a task. That means \a task is inserted into
     * the task queue of this thread pool. If this is the first
     * task to be enqueued, then this pool starts its worker threads.
     * The parameter \a queue_hint is a hint for insertion into the task
     * queue. Actually, the task queue consists of multiple queues. Each
     * thread is associated with its own queue. Only if it does not find
     * any task inside its own queue, it tries to steal a task from another queue.
     * So if the caller wants to distribute tasks among the worker threads,
     * it can use \a queue_hint to do so. The queue where the task gets
     * inserted into is determined by "queue_hint % num_queues". So a
     * strategy to distribute tasks among these queues might look like:
     * queue_hint = 0;
     * while( true ) { thread_pool.enqueueTask( queue_hint++, task ); }
     *
     * @param queue_hint the hint for insertion into the task queue
     * @param task the task to insert
     */
    void enqueueTask( std::size_t queue_hint, Task * task )
    {
      m_pool_impl.enqueueTask( queue_hint, task );
    }

    /**
     * This method enqueues a list of tasks \a task_list. That means the tasks
     * in the list are atomically inserted into the task queue of this thread pool.
     * If these are the first tasks to be enqueued, then this pool starts its
     * worker threads.
     *
     * @param task_list the list of tasks to enqueue
     */
    void enqueueTaskList( TaskList & task_list )
    {
      m_pool_impl.enqueueTaskList( task_list );
    }

    /**
     * This method enqueues a list of tasks \a task_list. That means the tasks
     * in the list are atomically inserted into the task queue of this thread pool.
     * If these are the first tasks to be enqueued, then this pool starts its
     * worker threads. The parameter \a queue_hint is a hint for insertion into
     * the task queue. Actually, the task queue consists of multiple queues. Each
     * thread is associated with its own queue. Only if it does not find
     * any task inside its own queue, it tries to steal a task from another queue.
     * So if the caller wants to distribute tasks among the worker threads,
     * it can use \a queue_hint to do so. The queue where the task gets
     * inserted into is determined by "queue_hint % num_queues". So a
     * strategy to distribute tasks among these queues might look like:
     * queue_hint = 0;
     * while( true ) { thread_pool.enqueueTaskList( queue_hint++, task_list ); }
     *
     * @param queue_hint the hint for insertion into the task queue
     * @param task_list the list of tasks to enqueue
     */
    void enqueueTaskList( std::size_t queue_hint, TaskList & task_list )
    {
      m_pool_impl.enqueueTaskList( queue_hint, task_list );
    }

    /**
     * This method tries to dequeue a task from the task queue of this
     * thread pool and executing it. If a task has been dequeued and executed,
     * true is returned. Otherwise false is returned.
     * If the dequeued task has a successor task to be executed, then it
     * will also be executed (if the dequeued task is the last predecessor
     * and enableWaiting() has not been called on its successor). This continues
     * until a task was executed, which has no successor task or the successor
     * task is not ready to be executed.
     *
     * Note: The intent of this method is to provide a mechanism for an
     * external thread to steal some work from this thread pool. For example,
     * this can be useful if you want the main thread of a program to
     * participate in getting the tasks done. While this could also be
     * achieved via waiting for a root task, doing it via this method
     * allows you to do some other stuff inbetween successive calls of
     * this method. Of course this means that you have to take care about
     * proper synchronization yourself.
     *
     * @return whether at least one task was dequeued and executed
     */
    bool stealTask()
    {
      return m_pool_impl.stealTask();
    }

    /**
     * This method creates and returns a task group that uses
     * this thread pool. A task group allows for waiting on the
     * completion of all tasks spawned through it.
     *
     * @return a task group
     */
    TaskGroup createTaskGroup()
    {
      return TaskGroup( &m_pool_impl );
    }

    /**
     * This method creates and returns an empty task list
     * where tasks can be inserted into, before spawning the
     * the whole list of tasks.
     * Note that task lists are quite efficient. No memory
     * is allocated, but instead a pointer is re-used to
     * link up tasks, that is anyway part of the task class.
     *
     * @return an empty task list
     */
    TaskList createTaskList() const
    {
      return TaskList();
    }

    /**
     * Creates a task that executes the given callable.
     *
     * @param callable the callable to execute by the task
     * @return the created task
     */
    template< class T_CALLABLE >
    Task
    makeTask( T_CALLABLE & callable )
    {
      return { callable };
    }

  private:
    detail::StructuredWorkStealingThreadPool< MAX_NUM_THREADS,
                                              T_CACHE_LINE_SIZE,
                                              Task > m_pool_impl;
};



} // namespace threads


#endif // THREADS_LIBRARY_H
