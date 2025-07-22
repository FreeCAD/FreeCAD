/// The MIT License (MIT)
/// Copyright (c) 2016 Peter Goldsborough
///
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the "Software"), to
/// deal in the Software without restriction, including without limitation the
/// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
/// sell copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
///
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
///
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
/// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
/// IN THE SOFTWARE.

#ifndef LRU_WRAP_HPP
#define LRU_WRAP_HPP

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

#include <lru/cache.hpp>
#include <lru/internal/hash.hpp>
#include <lru/internal/utility.hpp>

namespace LRU {

/// Wraps a function with a "shallow" LRU cache.
///
/// Given a function, this function will return a new function, where
/// "top-level" calls are cached. With "top-level" or "shallow", we mean
/// that recursive calls to the same function are not cached, since those
/// will call the original function symbol, not the wrapped one.
///
/// \tparam CacheType The cache template class to use.
/// \param original_function The function to wrap.
/// \param args Any arguments to forward to the cache.
/// \returns A new function with a shallow LRU cache.
template <typename Function,
          template <typename...> class CacheType = Cache,
          typename... Args>
auto wrap(Function original_function, Args&&... args) {
  return [
    original_function,
    cache_args = std::forward_as_tuple(std::forward<Args>(args)...)
  ](auto&&... arguments) mutable {
    using Arguments = std::tuple<std::decay_t<decltype(arguments)>...>;
    using ReturnType = decltype(
        original_function(std::forward<decltype(arguments)>(arguments)...));

    static_assert(!std::is_void<ReturnType>::value,
                  "Return type of wrapped function must not be void");

    static auto cache =
        Internal::construct_from_tuple<CacheType<Arguments, ReturnType>>(
            cache_args);

    auto key = std::make_tuple(arguments...);
    auto iterator = cache.find(key);

    if (iterator != cache.end()) {
      return iterator->second;
    }

    auto value =
        original_function(std::forward<decltype(arguments)>(arguments)...);
    cache.emplace(key, value);

    return value;
  };
}

/// Wraps a function with a "shallow" LRU timed cache.
///
/// Given a function, this function will return a new function, where
/// "top-level" calls are cached. With "top-level" or "shallow", we mean
/// that recursive calls to the same function are not cached, since those
/// will call the original function symbol, not the wrapped one.
///
/// \param original_function The function to wrap.
/// \param args Any arguments to forward to the cache.
/// \returns A new function with a shallow LRU cache.
template <typename Function, typename Duration, typename... Args>
auto timed_wrap(Function original_function, Duration duration, Args&&... args) {
  return wrap<Function, TimedCache>(
      original_function, duration, std::forward<Args>(args)...);
}

}  //  namespace LRU

#endif  // LRU_WRAP_HPP
