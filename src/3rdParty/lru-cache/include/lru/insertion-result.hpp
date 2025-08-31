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

#ifndef LRU_INSERTION_RESULT_HPP
#define LRU_INSERTION_RESULT_HPP

#include <algorithm>
#include <type_traits>
#include <utility>

namespace LRU {

/// The result of an insertion into a cache.
///
/// This is a semantically nicer alternative to a generic `std::pair`, as is
/// returned by `std::unordered_map` or so. It still has the same static
/// interface as the `std::pair` (with `first` and `second` members), but adds
/// nicer `was_inserted()` and `iterator()` accessors.
///
/// \tparam Iterator The class of the iterator contained in the result.
template <typename Iterator>
struct InsertionResult final {
  using IteratorType = Iterator;

  /// Constructor.
  ///
  /// \param result Whether the result was successful.
  /// \param iterator The iterator pointing to the inserted or updated key.
  InsertionResult(bool result, Iterator iterator)
  : first(result), second(iterator) {
  }

  /// \returns True if the key was newly inserted, false if it was only updated.
  bool was_inserted() const noexcept {
    return first;
  }

  /// \returns The iterator pointing to the inserted or updated key.
  Iterator iterator() const noexcept {
    return second;
  }

  /// \copydoc was_inserted
  explicit operator bool() const noexcept {
    return was_inserted();
  }

  /// Whether the result was successful.
  bool first;

  /// The iterator pointing to the inserted or updated key.
  Iterator second;
};

}  // namespace LRU


#endif  // LRU_INSERTION_RESULT_HPP
