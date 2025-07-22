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

#ifndef LRU_INTERNAL_TIMED_INFORMATION_HPP
#define LRU_INTERNAL_TIMED_INFORMATION_HPP

#include <cstddef>
#include <tuple>
#include <utility>

#include <lru/internal/definitions.hpp>
#include <lru/internal/information.hpp>
#include <lru/internal/utility.hpp>

namespace LRU {
namespace Internal {

/// The information object for timed caches.
///
/// TimedInformation differs from plain information only in that it stores the
/// creation time, to know when a key has expired.
///
/// \tparam Key The key type of the information.
/// \tparam Value The value type of the information.
template <typename Key, typename Value>
struct TimedInformation : public Information<Key, Value> {
  using super = Information<Key, Value>;
  using typename super::QueueIterator;
  using Timestamp = Internal::Timestamp;

  /// Constructor.
  ///
  /// \param value_ The value for the information.
  /// \param insertion_time_ The insertion timestamp of the key.
  /// \param order_ The order iterator for the information.
  TimedInformation(const Value& value_,
                   const Timestamp& insertion_time_,
                   QueueIterator order_ = QueueIterator())
  : super(value_, order_), insertion_time(insertion_time_) {
  }

  /// Constructor.
  ///
  /// Uses the current time as the insertion timestamp.
  ///
  /// \param value_ The value for the information.
  /// \param order_ The order iterator for the information.
  explicit TimedInformation(const Value& value_,
                            QueueIterator order_ = QueueIterator())
  : TimedInformation(value_, Internal::Clock::now(), order_) {
  }

  /// \copydoc Information::Information(QueueIterator,ValueArguments&&)
  template <typename... ValueArguments>
  TimedInformation(QueueIterator order_, ValueArguments&&... value_argument)
  : super(std::forward<ValueArguments>(value_argument)..., order_)
  , insertion_time(Internal::Clock::now()) {
  }

  /// \copydoc Information::Information(QueueIterator,const
  /// std::tuple<ValueArguments...>&)
  template <typename... ValueArguments>
  explicit TimedInformation(
      const std::tuple<ValueArguments...>& value_arguments,
      QueueIterator order_ = QueueIterator())
  : super(value_arguments, order_), insertion_time(Internal::Clock::now()) {
  }

  /// Compares this timed information for equality with another one.
  ///
  /// Additionally to key and value equality, the timed information requires
  /// that the insertion timestamps be equal.
  ///
  /// \param other The other timed information.
  /// \returns True if this information equals the other one, else false.
  bool operator==(const TimedInformation& other) const noexcept {
    if (super::operator!=(other)) return false;
    return this->insertion_time == other.insertion_time;
  }

  /// Compares this timed information for inequality with another one.
  ///
  /// \param other The other timed information.
  /// \returns True if this information does not equal the other one, else
  /// false.
  /// \see operator==()
  bool operator!=(const TimedInformation& other) const noexcept {
    return !(*this == other);
  }

  /// The time at which the key of the information was insterted into a cache.
  const Timestamp insertion_time;
};

}  // namespace Internal
}  // namespace LRU

#endif  // LRU_INTERNAL_TIMED_INFORMATION_HPP
