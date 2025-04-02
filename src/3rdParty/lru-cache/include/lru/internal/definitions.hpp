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

#ifndef LRU_INTERNAL_DEFINITIONS_HPP
#define LRU_INTERNAL_DEFINITIONS_HPP

#include <chrono>
#include <cstddef>
#include <functional>
#include <list>
#include <tuple>
#include <unordered_map>

namespace LRU {
namespace Internal {

/// The default capacity for all caches.
const std::size_t DEFAULT_CAPACITY = 128;

/// The reference type use to store keys in the order queue.
template <typename T>
using Reference = std::reference_wrapper<T>;

/// Compares two References for equality.
///
/// This is necessary because `std::reference_wrapper` does not define any
/// operator overloads. We do need them, however (e.g. for container
/// comparison).
///
/// \param first The first reference to compare.
/// \param second The second reference to compare.
template <typename T, typename U>
bool operator==(const Reference<T>& first, const Reference<U>& second) {
  return first.get() == second.get();
}

/// Compares two References for inequality.
///
/// This is necessary because `std::reference_wrapper` does not define any
/// operator overloads. We do need them, however (e.g. for container
/// comparison).
///
/// \param first The first reference to compare.
/// \param second The second reference to compare.
template <typename T, typename U>
bool operator!=(const Reference<T>& first, const Reference<U>& second) {
  return !(first == second);
}

/// The default queue type used internally.
template <typename T>
using Queue = std::list<Reference<T>>;

/// The default map type used internally.
template <typename Key,
          typename Information,
          typename HashFunction,
          typename KeyEqual>
using Map = std::unordered_map<Key, Information, HashFunction, KeyEqual>;

/// The default clock used internally.
using Clock = std::chrono::steady_clock;

/// The default timestamp (time point) used internally.
using Timestamp = Clock::time_point;
}  // namespace Internal
}  // namespace LRU


#endif  // LRU_INTERNAL_DEFINITIONS_HPP
