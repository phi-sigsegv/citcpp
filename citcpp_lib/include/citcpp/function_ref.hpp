#ifndef FUNCTION_REF_HPP_
#define FUNCTION_REF_HPP_

#include <cstdint>
#include <utility>

namespace citcpp {

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
template <typename Fn>
class function_ref;

template <typename Ret, typename... Params>
class function_ref<Ret(Params...)> {
  private:
    Ret (*callback)(intptr_t callable, Params... params) = nullptr;
    intptr_t callable;

    template <typename Callable>
    static Ret callback_fn(intptr_t callable, Params... params) {
      return (*reinterpret_cast<Callable*>(callable))(
          std::forward<Params>(params)...);
    }

  public:
    function_ref() = default;
    function_ref(std::nullptr_t) {}

    template <typename Callable>
    function_ref(
        Callable&& callable,
        typename std::enable_if<
            !std::is_same<typename std::remove_reference<Callable>::type,
                          function_ref>::value>::type* = nullptr)
        : callback(callback_fn<typename std::remove_reference<Callable>::type>),
          callable(reinterpret_cast<intptr_t>(&callable)) {}

    Ret operator()(Params... params) const {
      return callback(callable, std::forward<Params>(params)...);
    }

    operator bool() const { return callback; }
};

}  // namespace citcpp

#endif /* FUNCTION_REF_HPP_ */
