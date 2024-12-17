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

#ifndef LRU_INTERNAL_HASH_HPP
#define LRU_INTERNAL_HASH_HPP

#include <cstddef>
#include <functional>
#include <tuple>

/// `std::hash` specialization to allow storing tuples as keys
/// in `std::unordered_map`.
///
/// Essentially hashes all tuple elements and jumbles the
/// individual hashes together.
namespace std {
template <typename... Ts>
struct hash<std::tuple<Ts...>> {
  using argument_type = std::tuple<Ts...>;
  using result_type = std::size_t;

  result_type operator()(const argument_type& argument) const {
    return hash_tuple(argument, std::make_index_sequence<sizeof...(Ts)>());
  }

 private:
  template <std::size_t I, std::size_t... Is>
  result_type
  hash_tuple(const argument_type& tuple, std::index_sequence<I, Is...>) const {
    auto value = std::get<I>(tuple);
    auto current = std::hash<decltype(value)>{}(value);
    auto seed = hash_tuple(tuple, std::index_sequence<Is...>());

    // http://www.boost.org/doc/libs/1_35_0/doc/html/boost/hash_combine_id241013.html
    return current + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  }

  result_type hash_tuple(const argument_type&, std::index_sequence<>) const {
    return 0;
  }
};
}  // namespace std

#endif  // LRU_INTERNAL_HASH_HPP
