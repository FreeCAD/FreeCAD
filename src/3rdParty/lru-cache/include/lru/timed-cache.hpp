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

#ifndef LRU_TIMED_CACHE_HPP
#define LRU_TIMED_CACHE_HPP

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <functional>
#include <iterator>
#include <list>
#include <stdexcept>
#include <unordered_map>
#include <utility>

#include <lru/error.hpp>
#include <lru/internal/base-cache.hpp>
#include <lru/internal/last-accessed.hpp>
#include <lru/internal/timed-information.hpp>

namespace LRU {
namespace Internal {
template <typename Key,
          typename Value,
          typename HashFunction,
          typename KeyEqual>
using TimedCacheBase = BaseCache<Key,
                                 Value,
                                 Internal::TimedInformation,
                                 HashFunction,
                                 KeyEqual,
                                 Tag::TimedCache>;
}  // namespace Internal


/// A timed LRU cache.
///
/// A timed LRU cache behaves like a regular LRU cache, but adds the concept of
/// "expiration". The cache now not only remembers the order of insertion, but
/// also the point in time at which each element was inserted into the cache.
/// The cache then has an additional "time to live" property, which designates
/// the time after which a key in the cache is said to be "expired". Once a key
/// has expired, the cache will behave as if the key were not present in the
/// cache at all and, for example, return false on calls to `contains()` or
/// throw on calls to `lookup()`.
///
/// \see LRU::Cache
template <typename Key,
          typename Value,
          typename Duration = std::chrono::duration<double, std::milli>,
          typename HashFunction = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>>
class TimedCache
    : public Internal::TimedCacheBase<Key, Value, HashFunction, KeyEqual> {
 private:
  using super = Internal::TimedCacheBase<Key, Value, HashFunction, KeyEqual>;
  using PRIVATE_BASE_CACHE_MEMBERS;

 public:
  using Tag = LRU::Tag::TimedCache;
  using PUBLIC_BASE_CACHE_MEMBERS;
  using super::ordered_end;
  using super::unordered_end;
  using typename super::size_t;

  /// \param time_to_live The time to live for keys in the cache.
  /// \copydoc BaseCache::BaseCache(size_t,const HashFunction&,const KeyEqual&)
  template <typename AnyDurationType = Duration>
  explicit TimedCache(const AnyDurationType& time_to_live,
                      size_t capacity = Internal::DEFAULT_CAPACITY,
                      const HashFunction& hash = HashFunction(),
                      const KeyEqual& equal = KeyEqual())
  : super(capacity, hash, equal)
  , _time_to_live(std::chrono::duration_cast<Duration>(time_to_live)) {
  }

  /// \param time_to_live The time to live for keys in the cache.
  /// \copydoc BaseCache::BaseCache(size_t,Iterator,Iterator,const
  /// HashFunction&,const
  /// KeyEqual&)
  template <typename Iterator, typename AnyDurationType = Duration>
  TimedCache(const AnyDurationType& time_to_live,
             size_t capacity,
             Iterator begin,
             Iterator end,
             const HashFunction& hash = HashFunction(),
             const KeyEqual& equal = KeyEqual())
  : super(capacity, begin, end, hash, equal)
  , _time_to_live(std::chrono::duration_cast<Duration>(time_to_live)) {
  }

  /// \param time_to_live The time to live for keys in the cache.
  /// \copydoc BaseCache::BaseCache(Iterator,Iterator,const HashFunction&,const
  /// KeyEqual&)
  template <typename Iterator, typename AnyDurationType = Duration>
  TimedCache(const AnyDurationType& time_to_live,
             Iterator begin,
             Iterator end,
             const HashFunction& hash = HashFunction(),
             const KeyEqual& equal = KeyEqual())
  : super(begin, end, hash, equal)
  , _time_to_live(std::chrono::duration_cast<Duration>(time_to_live)) {
  }

  /// \param time_to_live The time to live for keys in the cache.
  /// \copydoc BaseCache::BaseCache(Range,size_t,const HashFunction&,const
  /// KeyEqual&)
  template <typename Range,
            typename AnyDurationType = Duration,
            typename = Internal::enable_if_range<Range>>
  TimedCache(const AnyDurationType& time_to_live,
             size_t capacity,
             Range&& range,
             const HashFunction& hash = HashFunction(),
             const KeyEqual& equal = KeyEqual())
  : super(capacity, std::forward<Range>(range), hash, equal)
  , _time_to_live(std::chrono::duration_cast<Duration>(time_to_live)) {
  }

  /// \param time_to_live The time to live for keys in the cache.
  /// \copydoc BaseCache::BaseCache(Range,const HashFunction&,const
  /// KeyEqual&)
  template <typename Range,
            typename AnyDurationType = Duration,
            typename = Internal::enable_if_range<Range>>
  explicit TimedCache(const AnyDurationType& time_to_live,
                      Range&& range,
                      const HashFunction& hash = HashFunction(),
                      const KeyEqual& equal = KeyEqual())
  : super(std::forward<Range>(range), hash, equal)
  , _time_to_live(std::chrono::duration_cast<Duration>(time_to_live)) {
  }

  /// \param time_to_live The time to live for keys in the cache.
  /// \copydoc BaseCache::BaseCache(InitializerList,const HashFunction&,const
  /// KeyEqual&)
  template <typename AnyDurationType = Duration>
  TimedCache(const AnyDurationType& time_to_live,
             InitializerList list,
             const HashFunction& hash = HashFunction(),
             const KeyEqual& equal = KeyEqual())  // NOLINT(runtime/explicit)
      : super(list, hash, equal),
        _time_to_live(std::chrono::duration_cast<Duration>(time_to_live)) {
  }

  /// \param time_to_live The time to live for keys in the cache.
  /// \copydoc BaseCache::BaseCache(InitializerList,size_t,const
  /// HashFunction&,const
  /// KeyEqual&)
  template <typename AnyDurationType = Duration>
  TimedCache(const AnyDurationType& time_to_live,
             size_t capacity,
             InitializerList list,
             const HashFunction& hash = HashFunction(),
             const KeyEqual& equal = KeyEqual())  // NOLINT(runtime/explicit)
      : super(capacity, list, hash, equal),
        _time_to_live(std::chrono::duration_cast<Duration>(time_to_live)) {
  }

  /// \copydoc BaseCache::swap
  void swap(TimedCache& other) noexcept {
    using std::swap;

    super::swap(other);
    swap(_time_to_live, other._time_to_live);
  }

  /// Swaps the contents of one cache with another cache.
  ///
  /// \param first The first cache to swap.
  /// \param second The second cache to swap.
  friend void swap(TimedCache& first, TimedCache& second) noexcept {
    first.swap(second);
  }

  /// \copydoc BaseCache::find(const Key&)
  UnorderedIterator find(const Key& key) override {
    auto iterator = _map.find(key);
    if (iterator != _map.end()) {
      if (!_has_expired(iterator->second)) {
        _register_hit(key, iterator->second.value);
        _move_to_front(iterator->second.order);
        _last_accessed = iterator;
        return {*this, iterator};
      }
    }

    _register_miss(key);

    return end();
  }

  /// \copydoc BaseCache::find(const Key&) const
  UnorderedConstIterator find(const Key& key) const override {
    auto iterator = _map.find(key);
    if (iterator != _map.end()) {
      if (!_has_expired(iterator->second)) {
        _register_hit(key, iterator->second.value);
        _move_to_front(iterator->second.order);
        _last_accessed = iterator;
        return {*this, iterator};
      }
    }

    _register_miss(key);

    return cend();
  }

  // no front() because we may have to erase the
  // entire cache if everything happens to be expired

  /// \returns True if all keys in the cache have expired, else false.
  bool all_expired() const {
    // By the laws of predicate logic, any statement about any empty set is true
    if (is_empty()) return true;

    /// If the most-recently inserted key has expired, all others must have too.
    auto latest = _map.find(_order.back());
    return _has_expired(latest->second);
  }

  /// Erases all expired elements from the cache.
  ///
  /// \complexity O(N)
  /// \returns The number of elements erased.
  size_t clear_expired() {
    // We have to do a linear search here because linked lists do not
    // support O(log N) binary searches given their node-based nature.
    // Either way, in the worst case the entire cache has expired and
    // we would have to do O(N) erasures.

    if (is_empty()) return 0;

    auto iterator = _order.begin();
    size_t number_of_erasures = 0;

    while (iterator != _order.end()) {
      auto map_iterator = _map.find(*iterator);

      // If the current element hasn't expired, also all elements inserted
      // after will not have, so we can stop.
      if (!_has_expired(map_iterator->second)) break;

      _erase(map_iterator);

      iterator = _order.begin();
      number_of_erasures += 1;
    }

    return number_of_erasures;
  }

  /// \returns True if the given key is contained in the cache and has expired.
  /// \param key The key to test expiration for.
  bool has_expired(const Key& key) const noexcept {
    auto iterator = _map.find(key);
    return iterator != _map.end() && _has_expired(iterator->second);
  }

  /// \returns True if the key pointed to by the iterator has expired.
  /// \param ordered_iterator The ordered iterator to check.
  /// \details If this is the end iterator, this method returns false.
  bool has_expired(OrderedConstIterator ordered_iterator) const noexcept {
    if (ordered_iterator == ordered_end()) return false;
    auto iterator = _map.find(ordered_iterator->key());
    assert(iterator != _map.end());

    return _has_expired(iterator->second);
  }

  /// \returns True if the key pointed to by the iterator has expired.
  /// \param unordered_iterator The unordered iterator to check.
  /// \details If this is the end iterator, this method returns false.
  bool has_expired(UnorderedConstIterator unordered_iterator) const noexcept {
    if (unordered_iterator == unordered_end()) return false;
    assert(unordered_iterator._iterator != _map.end());

    return _has_expired(unordered_iterator._iterator->second);
  }

  /// \copydoc BaseCache::is_valid(UnorderedConstIterator)
  bool is_valid(UnorderedConstIterator unordered_iterator) const
      noexcept override {
    if (!super::is_valid(unordered_iterator)) return false;
    if (has_expired(unordered_iterator)) return false;
    return true;
  }

  /// \copydoc BaseCache::is_valid(OrderedConstIterator)
  bool is_valid(OrderedConstIterator ordered_iterator) const noexcept override {
    if (!super::is_valid(ordered_iterator)) return false;
    if (has_expired(ordered_iterator)) return false;
    return true;
  }

  /// \copydoc BaseCache::is_valid(UnorderedConstIterator)
  /// \throws LRU::Error::KeyExpired if the key pointed to by the iterator has
  /// expired.
  void
  throw_if_invalid(UnorderedConstIterator unordered_iterator) const override {
    super::throw_if_invalid(unordered_iterator);
    if (has_expired(unordered_iterator)) {
      throw LRU::Error::KeyExpired();
    }
  }

  /// \copydoc BaseCache::is_valid(OrderedConstIterator)
  /// \throws LRU::Error::KeyExpired if the key pointed to by the iterator has
  /// expired.
  void throw_if_invalid(OrderedConstIterator ordered_iterator) const override {
    super::throw_if_invalid(ordered_iterator);
    if (has_expired(ordered_iterator)) {
      throw LRU::Error::KeyExpired();
    }
  }

 private:
  using Clock = Internal::Clock;

  /// \returns True if the last accessed object is valid.
  /// \details Next to performing the base cache's action, this method also
  /// checks for expiration of the last accessed key.
  bool _last_accessed_is_ok(const Key& key) const noexcept override {
    if (!super::_last_accessed_is_ok(key)) return false;
    return !_has_expired(_last_accessed.information());
  }

  /// \copydoc _value_for_last_accessed() const
  Value& _value_for_last_accessed() override {
    auto& information = _last_accessed.information();
    if (_has_expired(information)) {
      throw LRU::Error::KeyExpired();
    } else {
      return information.value;
    }
  }

  /// Attempts to access the last accessed key's value.
  /// \throws LRU::Error::KeyExpired if the key has expired.
  /// \returns The value of the last accessed key.
  const Value& _value_for_last_accessed() const override {
    const auto& information = _last_accessed.information();
    if (_has_expired(information)) {
      throw LRU::Error::KeyExpired();
    } else {
      return information.value;
    }
  }

  /// Checks if a key has expired, given its information.
  ///
  /// \param information The information to check expiration with.
  /// \returns True if the key has expired, else false.
  bool _has_expired(const Information& information) const noexcept {
    auto elapsed = Clock::now() - information.insertion_time;
    return std::chrono::duration_cast<Duration>(elapsed) > _time_to_live;
  }

  /// The duration after which a key is said to be expired.
  Duration _time_to_live;
};

namespace Lowercase {
template <typename... Ts>
using timed_cache = TimedCache<Ts...>;
}  // namespace Lowercase

}  // namespace LRU

#endif  // LRU_TIMED_CACHE_HPP
