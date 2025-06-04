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

#ifndef LRU_CACHE_HPP
#define LRU_CACHE_HPP

#include <cassert>
#include <chrono>
#include <cstddef>
#include <functional>
#include <iterator>
#include <list>
#include <stdexcept>
#include <unordered_map>

#include <lru/cache-tags.hpp>
#include <lru/error.hpp>
#include <lru/internal/base-cache.hpp>
#include <lru/internal/information.hpp>
#include <lru/internal/last-accessed.hpp>

namespace LRU {
namespace Internal {
template <typename Key,
          typename Value,
          typename HashFunction,
          typename KeyEqual>
using UntimedCacheBase = Internal::BaseCache<Key,
                                             Value,
                                             Internal::Information,
                                             HashFunction,
                                             KeyEqual,
                                             Tag::BasicCache>;
}  // namespace Internal

/// A basic LRU cache implementation.
///
/// An LRU cache is a fixed-size cache that remembers the order in which
/// elements were inserted into it. When the size of the cache exceeds its
/// capacity, the "least-recently-used" (LRU) element is erased. In our
/// implementation, usage is defined as insertion, but not lookup. That is,
/// looking up an element does not move it to the "front" of the cache (making
/// the operation faster). Only insertions (and erasures) can change the order
/// of elements. The capacity of the cache can be modified at any time.
///
/// \see LRU::TimedCache
template <typename Key,
          typename Value,
          typename HashFunction = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>>
class Cache
    : public Internal::UntimedCacheBase<Key, Value, HashFunction, KeyEqual> {
 private:
  using super = Internal::UntimedCacheBase<Key, Value, HashFunction, KeyEqual>;
  using PRIVATE_BASE_CACHE_MEMBERS;

 public:
  using PUBLIC_BASE_CACHE_MEMBERS;
  using typename super::size_t;

  /// \copydoc BaseCache::BaseCache(size_t,const HashFunction&,const KeyEqual&)
  /// \detailss The capacity defaults to an internal constant, currently 128.
  explicit Cache(size_t capacity = Internal::DEFAULT_CAPACITY,
                 const HashFunction& hash = HashFunction(),
                 const KeyEqual& equal = KeyEqual())
  : super(capacity, hash, equal) {
  }

  /// \copydoc BaseCache(size_t,Iterator,Iterator,const HashFunction&,const
  /// KeyEqual&)
  template <typename Iterator>
  Cache(size_t capacity,
        Iterator begin,
        Iterator end,
        const HashFunction& hash = HashFunction(),
        const KeyEqual& equal = KeyEqual())
  : super(capacity, begin, end, hash, equal) {
  }

  /// \copydoc BaseCache(Iterator,Iterator,const HashFunction&,const
  /// KeyEqual&)
  template <typename Iterator>
  Cache(Iterator begin,
        Iterator end,
        const HashFunction& hash = HashFunction(),
        const KeyEqual& equal = KeyEqual())
  : super(begin, end, hash, equal) {
  }

  /// Constructor.
  ///
  /// \param capacity The capacity of the cache.
  /// \param range A range to construct the cache with.
  /// \param hash The hash function to use for the internal map.
  /// \param key_equal The key equality function to use for the internal map.
  template <typename Range, typename = Internal::enable_if_range<Range>>
  Cache(size_t capacity,
        Range&& range,
        const HashFunction& hash = HashFunction(),
        const KeyEqual& equal = KeyEqual())
  : super(capacity, std::forward<Range>(range), hash, equal) {
  }

  /// Constructor.
  ///
  /// \param range A range to construct the cache with.
  /// \param hash The hash function to use for the internal map.
  /// \param key_equal The key equality function to use for the internal map.
  template <typename Range, typename = Internal::enable_if_range<Range>>
  explicit Cache(Range&& range,
                 const HashFunction& hash = HashFunction(),
                 const KeyEqual& equal = KeyEqual())
  : super(std::forward<Range>(range), hash, equal) {
  }

  /// \copydoc BaseCache(InitializerList,const HashFunction&,const
  /// KeyEqual&)
  Cache(InitializerList list,
        const HashFunction& hash = HashFunction(),
        const KeyEqual& equal = KeyEqual())  // NOLINT(runtime/explicit)
      : super(list, hash, equal) {
  }

  /// \copydoc BaseCache(size_t,InitializerList,const HashFunction&,const
  /// KeyEqual&)
  Cache(size_t capacity,
        InitializerList list,
        const HashFunction& hash = HashFunction(),
        const KeyEqual& equal = KeyEqual())  // NOLINT(runtime/explicit)
      : super(capacity, list, hash, equal) {
  }

  /// \copydoc BaseCache::find(const Key&)
  UnorderedIterator find(const Key& key) override {
    auto iterator = _map.find(key);
    if (iterator != _map.end()) {
      _register_hit(key, iterator->second.value);
      _move_to_front(iterator->second.order);
      _last_accessed = iterator;
    } else {
      _register_miss(key);
    }

    return {*this, iterator};
  }

  /// \copydoc BaseCache::find(const Key&) const
  UnorderedConstIterator find(const Key& key) const override {
    auto iterator = _map.find(key);
    if (iterator != _map.end()) {
      _register_hit(key, iterator->second.value);
      _move_to_front(iterator->second.order);
      _last_accessed = iterator;
    } else {
      _register_miss(key);
    }

    return {*this, iterator};
  }

  /// \returns The most-recently inserted element.
  const Key& front() const noexcept {
    if (is_empty()) {
      throw LRU::Error::EmptyCache("front");
    } else {
      // The queue is reversed for natural order of iteration.
      return _order.back();
    }
  }

  /// \returns The least-recently inserted element.
  const Key& back() const noexcept {
    if (is_empty()) {
      throw LRU::Error::EmptyCache("back");
    } else {
      // The queue is reversed for natural order of iteration.
      return _order.front();
    }
  }
};

namespace Lowercase {
template <typename... Ts>
using cache = Cache<Ts...>;
}  // namespace Lowercase

}  // namespace LRU

#endif  // LRU_CACHE_HPP
