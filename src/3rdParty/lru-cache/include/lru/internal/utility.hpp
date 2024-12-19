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

#ifndef LRU_UTILITY_HPP
#define LRU_UTILITY_HPP

#include <cstddef>
#include <iterator>
#include <tuple>
#include <utility>

namespace LRU {
namespace Internal {

/// Generates an index sequence for a tuple.
///
/// \tparam Ts The types of the tuple (to deduce the size).
template <typename... Ts>
constexpr auto tuple_indices(const std::tuple<Ts...>&) {
  return std::make_index_sequence<sizeof...(Ts)>();
}

/// Applies (in the functional sense) a tuple to the constructor of a class.
///
/// \tparam T The type to construct.
/// \tparam Indices The indices into the tuple (generated from an index
///                 sequence).
/// \param args The tuple of arguments to construct the object with.
template <typename T, typename... Args, std::size_t... Indices>
constexpr T construct_from_tuple(const std::tuple<Args...>& arguments,
                                 std::index_sequence<Indices...>) {
  return T(std::forward<Args>(std::get<Indices>(arguments))...);
}

/// Applies (in the functional sense) a tuple to the constructor of a class.
///
/// \tparam T The type to construct.
/// \param args The tuple of arguments to construct the object with.
template <typename T, typename... Args>
constexpr T construct_from_tuple(const std::tuple<Args...>& args) {
  return construct_from_tuple<T>(args, tuple_indices(args));
}

/// Applies (in the functional sense) a tuple to the constructor of a class.
///
/// \tparam T The type to construct.
/// \param args The tuple of arguments to construct the object with.
template <typename T, typename... Args>
constexpr T construct_from_tuple(std::tuple<Args...>&& args) {
  return construct_from_tuple<T>(std::move(args), tuple_indices(args));
}

/// A type trait that disables a template overload if a type is not an iterator.
///
/// \tparam T the type to check.
template <typename T>
using enable_if_iterator = typename std::iterator_traits<T>::value_type;

/// A type trait that disables a template overload if a type is not a range.
///
/// \tparam T the type to check.
template <typename T>
using enable_if_range = std::pair<decltype(std::declval<T>().begin()),
                                  decltype(std::declval<T>().end())>;

/// A type trait that disables a template overload if a type is not an iterator
/// over a pair.
///
/// \tparam T the type to check.
template <typename T>
using enable_if_iterator_over_pair =
    std::pair<typename std::iterator_traits<T>::value_type::first_type,
              typename std::iterator_traits<T>::value_type::first_type>;


/// A type trait that disables a template overload if a type is not convertible
/// to a target type.
///
/// \tparam Target The type one wants to check against.
/// \tparam T The type to check.
template <typename Target, typename T>
using enable_if_same = std::enable_if_t<std::is_convertible<T, Target>::value>;

/// Base case for `static_all_of` (the neutral element of AND is true).
constexpr bool static_all_of() noexcept {
  return true;
}

/// Checks if all the given parameters evaluate to true.
///
/// \param head The first expression to check.
/// \param tail The remaining expression to check.
template <typename Head, typename... Tail>
constexpr bool static_all_of(Head&& head, Tail&&... tail) {
  // Replace with (ts && ...) when the time is right
  return std::forward<Head>(head) && static_all_of(std::forward<Tail>(tail)...);
}

/// Base case for `static_any_of` (the neutral element of OR is false).
constexpr bool static_any_of() noexcept {
  return false;
}

/// Checks if any the given parameters evaluate to true.
///
/// \param head The first expression to check.
/// \param tail The remaining expression to check.
/// \returns True if any of the given parameters evaluate to true.
template <typename Head, typename... Tail>
constexpr bool static_any_of(Head&& head, Tail&&... tail) {
  // Replace with (ts || ...) when the time is right
  return std::forward<Head>(head) || static_any_of(std::forward<Tail>(tail)...);
}

/// Checks if none the given parameters evaluate to true.
///
/// \param ts The expressions to check.
/// \returns True if any of the given parameters evaluate to true.
template <typename... Ts>
constexpr bool static_none_of(Ts&&... ts) {
  // Replace with (!ts && ...) when the time is right
  return !static_any_of(std::forward<Ts>(ts)...);
}

/// Checks if all the given types are convertible to the first type.
///
/// \tparam T the first type.
/// \tparam Ts The types to check against the first.
template <typename T, typename... Ts>
constexpr bool
    all_of_type = static_all_of(std::is_convertible<Ts, T>::value...);

/// Checks if none of the given types are convertible to the first type.
///
/// \tparam T the first type.
/// \tparam Ts The types to check against the first.
template <typename T, typename... Ts>
constexpr bool
    none_of_type = static_none_of(std::is_convertible<Ts, T>::value...);

/// Base case for `for_each`.
template <typename Function>
void for_each(Function) noexcept {
}

/// Calls a function for each of the given variadic arguments.
///
/// \param function The function to call for each argument.
/// \param head The first value to call the function with.
/// \param tail The remaining values to call the function with.
template <typename Function, typename Head, typename... Tail>
void for_each(Function function, Head&& head, Tail&&... tail) {
  function(std::forward<Head>(head));
  for_each(function, std::forward<Tail>(tail)...);
}

}  // namespace Internal
}  // namespace LRU

#endif  // LRU_UTILITY_HPP
