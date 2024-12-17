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

#ifndef LRU_INTERNAL_INFORMATION_HPP
#define LRU_INTERNAL_INFORMATION_HPP

#include <cstddef>
#include <tuple>
#include <utility>

#include <lru/internal/definitions.hpp>
#include <lru/internal/utility.hpp>

namespace LRU {
namespace Internal {

/// The value type of internal maps, used to store a value and iterator.
///
/// This information object is the basis of an LRU cache, which must associated
/// a value and such an order iterator with a key, such that the iterator may be
/// moved to the front of the order when the key is updated with a new value.
///
/// \tparam Key The key type of the information.
/// \tparam Value The value type of the information.
template <typename Key, typename Value>
struct Information {
  using KeyType = Key;
  using ValueType = Value;
  using QueueIterator = typename Internal::Queue<const Key>::const_iterator;

  /// Constructor.
  ///
  /// \param value_ The value for the information.
  /// \param order_ The order iterator for the information.
  explicit Information(const Value& value_,
                       QueueIterator order_ = QueueIterator())
  : value(value_), order(order_) {
  }

  /// Constructor.
  ///
  /// \param order_ The order iterator for the information.
  /// \param value_arguments Any number of arguments to perfectly forward to the
  ///                        value type's constructor.
  // template <typename... ValueArguments>
  // Information(QueueIterator order_, ValueArguments&&... value_arguments)
  // : value(std::forward<ValueArguments>(value_arguments)...), order(order_) {
  // }

  /// Constructor.
  ///
  /// \param order_ The order iterator for the information.
  /// \param value_arguments A tuple of arguments to perfectly forward to the
  ///                        value type's constructor.
  ///
  template <typename... ValueArguments>
  explicit Information(const std::tuple<ValueArguments...>& value_arguments,
                       QueueIterator order_ = QueueIterator())
  : Information(
        order_, value_arguments, Internal::tuple_indices(value_arguments)) {
  }

  /// Copy constructor.
  Information(const Information& other) = default;

  /// Move constructor.
  Information(Information&& other) = default;

  /// Copy assignment operator.
  Information& operator=(const Information& other) = default;

  /// Move assignment operator.
  Information& operator=(Information&& other) = default;

  /// Destructor.
  virtual ~Information() = default;

  /// Compares the information for equality with another information object.
  ///
  /// \param other The other information object to compare to.
  /// \returns True if key and value (not the iterator itself) of the two
  /// information objects are equal, else false.
  virtual bool operator==(const Information& other) const noexcept {
    if (this == &other) return true;
    if (this->value != other.value) return false;
    // We do not compare the iterator (because otherwise two containers
    // holding information would never be equal). We also do not compare
    // the key stored in the iterator, because keys will always have been
    // compared before this operator is called.
    return true;
  }

  /// Compares the information for inequality with another information object.
  ///
  /// \param other The other information object to compare for.
  /// \returns True if key and value (not the iterator itself) of the two
  /// information objects are unequal, else false.
  virtual bool operator!=(const Information& other) const noexcept {
    return !(*this == other);
  }

  /// The value of the information.
  Value value;

  /// The order iterator of the information.
  QueueIterator order;

 private:
  /// Implementation for the constructor taking a tuple of arguments for the
  /// value.
  ///
  /// \param order_ The order iterator for the information.
  /// \param value_argument The tuple of arguments to perfectly forward to the
  ///                       value type's constructor.
  /// \param _ An index sequence to access the elements of the tuple
  template <typename... ValueArguments, std::size_t... Indices>
  Information(const QueueIterator& order_,
              const std::tuple<ValueArguments...>& value_argument,
              std::index_sequence<Indices...> _)
  : value(std::forward<ValueArguments>(std::get<Indices>(value_argument))...)
  , order(order_) {
  }
};
}  // namespace Internal
}  // namespace LRU

#endif  // LRU_INTERNAL_INFORMATION_HPP
