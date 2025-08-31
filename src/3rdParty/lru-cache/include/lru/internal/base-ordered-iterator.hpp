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

#ifndef BASE_ORDERED_ITERATOR_HPP
#define BASE_ORDERED_ITERATOR_HPP

#include <algorithm>
#include <functional>
#include <iterator>
#include <type_traits>

#include <lru/entry.hpp>
#include <lru/error.hpp>
#include <lru/internal/base-iterator.hpp>
#include <lru/internal/base-unordered-iterator.hpp>
#include <lru/internal/definitions.hpp>
#include <lru/internal/optional.hpp>
#include <lru/iterator-tags.hpp>

namespace LRU {
namespace Internal {

template <typename Key, typename Value, typename Cache>
using BaseForBaseOrderedIterator =
    BaseIterator<std::bidirectional_iterator_tag,
                 Key,
                 Value,
                 Cache,
                 typename Queue<Key>::const_iterator>;

/// The base class for all const and non-const ordered iterators.
///
/// Ordered iterators are bidirectional iterators that iterate over the keys of
/// a cache in the order in which they were inserted into the cache. As they
/// only iterate over the keys, they must perform hash lookups to retrieve the
/// value the first time they are dereferenced. This makes them slightly less
/// efficient than unordered iterators. However, they also have the additional
/// property that they may be constructed from unordered iterators, and that
/// they can be decremented.
///
/// \tparam Key The key type over which instances of the iterator iterate.
/// \tparam Value The value type over which instances of the iterator iterate.
/// \tparam Cache The type of the cache instances of the iterator point into.
template <typename Key, typename Value, typename Cache>
class BaseOrderedIterator
    : public BaseForBaseOrderedIterator<Key, Value, Cache> {
 protected:
  using super = BaseForBaseOrderedIterator<Key, Value, Cache>;
  using PRIVATE_BASE_ITERATOR_MEMBERS;
  using UnderlyingIterator = typename Queue<Key>::const_iterator;

 public:
  using Tag = LRU::Tag::OrderedIterator;
  using PUBLIC_BASE_ITERATOR_MEMBERS;

  /// Constructor.
  BaseOrderedIterator() noexcept = default;

  /// \copydoc BaseIterator::BaseIterator(Cache,UnderlyingIterator)
  BaseOrderedIterator(Cache& cache, UnderlyingIterator iterator)
  : super(cache, iterator) {
  }

  /// Generalized copy constructor.
  ///
  /// \param other The ordered iterator to contruct from.
  template <typename AnyKey, typename AnyValue, typename AnyCache>
  BaseOrderedIterator(
      const BaseOrderedIterator<AnyKey, AnyValue, AnyCache>& other)
  : super(other) {
  }

  /// Generalized move constructor.
  ///
  /// \param other The ordered iterator to move into this one.
  template <typename AnyKey, typename AnyValue, typename AnyCache>
  BaseOrderedIterator(BaseOrderedIterator<AnyKey, AnyValue, AnyCache>&& other)
  : super(std::move(other)) {
  }

  /// Generalized conversion copy constructor.
  ///
  /// \param unordered_iterator The unordered iterator to construct from.
  template <
      typename AnyCache,
      typename UnderlyingIterator,
      typename = std::enable_if_t<
          std::is_same<std::decay_t<AnyCache>, std::decay_t<Cache>>::value>>
  BaseOrderedIterator(const BaseUnorderedIterator<AnyCache, UnderlyingIterator>&
                          unordered_iterator) {
    // Atomicity
    _throw_if_at_invalid(unordered_iterator);
    _cache = unordered_iterator._cache;
    _iterator = unordered_iterator._iterator->second.order;
  }

  /// Generalized conversion move constructor.
  ///
  /// \param unordered_iterator The unordered iterator to move-construct from.
  template <
      typename AnyCache,
      typename UnderlyingIterator,
      typename = std::enable_if_t<
          std::is_same<std::decay_t<AnyCache>, std::decay_t<Cache>>::value>>
  BaseOrderedIterator(BaseUnorderedIterator<AnyCache, UnderlyingIterator>&&
                          unordered_iterator) {
    // Atomicity
    _throw_if_at_invalid(unordered_iterator);
    _cache = std::move(unordered_iterator._cache);
    _entry = std::move(unordered_iterator._entry);
    _iterator = std::move(unordered_iterator._iterator->second.order);
  }

  /// Copy constructor.
  BaseOrderedIterator(const BaseOrderedIterator& other) = default;

  /// Move constructor.
  BaseOrderedIterator(BaseOrderedIterator&& other) = default;

  /// Copy assignment operator.
  BaseOrderedIterator& operator=(const BaseOrderedIterator& other) = default;

  /// Move assignment operator.
  BaseOrderedIterator& operator=(BaseOrderedIterator&& other) = default;

  /// Destructor.
  virtual ~BaseOrderedIterator() = default;

  /// Checks for equality between this iterator and another ordered iterator.
  ///
  /// \param other The other ordered iterator.
  /// \returns True if both iterators point to the same entry, else false.
  bool operator==(const BaseOrderedIterator& other) const noexcept {
    return this->_iterator == other._iterator;
  }

  /// Checks for inequality between this iterator another ordered iterator.
  ///
  /// \param other The other ordered iterator.
  /// \returns True if the iterators point to different entries, else false.
  bool operator!=(const BaseOrderedIterator& other) const noexcept {
    return !(*this == other);
  }

  /// Checks for inequality between this iterator and another unordered
  /// iterator.
  ///
  /// \param other The other unordered iterator.
  /// \returns True if both iterators point to the end of the same cache, else
  /// the result of comparing with the unordered iterator, converted to an
  /// ordered iterator.
  template <typename AnyCache, typename AnyUnderlyingIterator>
  bool operator==(
      const BaseUnorderedIterator<AnyCache, AnyUnderlyingIterator>& other) const
      noexcept {
    if (this->_cache != other._cache) return false;

    // The past-the-end iterators of the same cache should compare equal.
    // This is an exceptional guarantee we make. This is also the reason
    // why we can't rely on the conversion from unordered to ordered iterators
    // because construction of an ordered iterator from the past-the-end
    // unordered iterator will fail (with an InvalidIteratorConversion error)
    if (other == other._cache->unordered_end()) {
      return *this == this->_cache->ordered_end();
    }

    // Will call the other overload
    return *this == static_cast<BaseOrderedIterator>(other);
  }

  /// Checks for equality between an unordered iterator and an ordered iterator.
  ///
  /// \param first The unordered iterator.
  /// \param second The ordered iterator.
  /// \returns True if both iterators point to the end of the same cache, else
  /// the result of comparing with the unordered iterator, converted to an
  /// ordered iterator.
  template <typename AnyCache, typename AnyUnderlyingIterator>
  friend bool operator==(
      const BaseUnorderedIterator<AnyCache, AnyUnderlyingIterator>& first,
      const BaseOrderedIterator& second) noexcept {
    return second == first;
  }

  /// Checks for inequality between an unordered
  /// iterator and an ordered iterator.
  ///
  /// \param first The ordered iterator.
  /// \param second The unordered iterator.
  /// \returns True if the iterators point to different entries, else false.
  template <typename AnyCache, typename AnyUnderlyingIterator>
  friend bool
  operator!=(const BaseOrderedIterator& first,
             const BaseUnorderedIterator<AnyCache, AnyUnderlyingIterator>&
                 second) noexcept {
    return !(first == second);
  }

  /// Checks for inequality between an unordered
  /// iterator and an ordered iterator.
  ///
  /// \param first The unordered iterator.
  /// \param second The ordered iterator.
  /// \returns True if the iterators point to different entries, else false.
  template <typename AnyCache, typename AnyUnderlyingIterator>
  friend bool operator!=(
      const BaseUnorderedIterator<AnyCache, AnyUnderlyingIterator>& first,
      const BaseOrderedIterator& second) noexcept {
    return second != first;
  }

  /// Increments the iterator to the next entry.
  ///
  /// If the iterator already pointed to the end any number of increments
  /// before, behavior is undefined.
  ///
  /// \returns The resulting iterator.
  BaseOrderedIterator& operator++() {
    ++_iterator;
    _entry.reset();
    return *this;
  }

  /// Increments the iterator and returns a copy of the previous one.
  ///
  /// If the iterator already pointed to the end any number of increments
  /// before, behavior is undefined.
  ///
  /// \returns A copy of the previous iterator.
  BaseOrderedIterator operator++(int) {
    auto previous = *this;
    ++*this;
    return previous;
  }

  /// Decrements the iterator to the previous entry.
  ///
  /// \returns The resulting iterator.
  BaseOrderedIterator& operator--() {
    --_iterator;
    _entry.reset();
    return *this;
  }

  /// Decrements the iterator and returns a copy of the  previous entry.
  ///
  /// \returns The previous iterator.
  BaseOrderedIterator operator--(int) {
    auto previous = *this;
    --*this;
    return previous;
  }

  Entry& operator*() noexcept override {
    return _maybe_lookup();
  }

  /// \returns A reference to the entry the iterator points to.
  /// \details If the iterator is invalid, behavior is undefined.
  Entry& entry() override {
    _cache->throw_if_invalid(*this);
    return _maybe_lookup();
  }

  /// \returns A reference to the value the iterator points to.
  /// \details If the iterator is invalid, behavior is undefined.
  Value& value() override {
    return entry().value();
  }

  /// \returns A reference to the key the iterator points to.
  /// \details If the iterator is invalid, behavior is undefined.
  const Key& key() override {
    // No lookup required
    _cache->throw_if_invalid(*this);
    return *_iterator;
  }

 protected:
  template <typename, typename, typename>
  friend class BaseOrderedIterator;

  /// Looks up the entry for a key if this was not done already.
  ///
  /// \returns The entry, which was possibly newly looked up.
  Entry& _maybe_lookup() {
    if (!_entry.has_value()) {
      _lookup();
    }

    return *_entry;
  }

  /// Looks up the entry for a key and sets the internal entry member.
  void _lookup() {
    auto iterator = _cache->_map.find(*_iterator);
    _entry.emplace(iterator->first, iterator->second.value);
  }

 private:
  /// Throws an exception if the given unordered iterator is invalid.
  ///
  /// \param unordered_iterator The iterator to check.
  /// \throws LRU::Error::InvalidIteratorConversion if the iterator is invalid.
  template <typename UnorderedIterator>
  void _throw_if_at_invalid(const UnorderedIterator& unordered_iterator) {
    // For atomicity of the copy assignment, we assign the cache pointer only
    // after this check in the copy/move constructor and use the iterator's
    // cache. If an exception is thrown, the state of the ordered iterator is
    // unchanged compared to before the assignment.
    if (unordered_iterator == unordered_iterator._cache->unordered_end()) {
      throw LRU::Error::InvalidIteratorConversion();
    }
  }
};

}  // namespace Internal
}  // namespace LRU

#endif  // BASE_ORDERED_ITERATOR_HPP
