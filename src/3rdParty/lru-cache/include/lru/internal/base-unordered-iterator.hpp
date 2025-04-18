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

#ifndef BASE_UNORDERED_ITERATOR_HPP
#define BASE_UNORDERED_ITERATOR_HPP

#include <algorithm>
#include <iterator>
#include <type_traits>

#include <lru/entry.hpp>
#include <lru/internal/base-iterator.hpp>
#include <lru/internal/definitions.hpp>
#include <lru/internal/optional.hpp>
#include <lru/iterator-tags.hpp>


namespace LRU {

// Forward declaration.
template <typename, typename, typename, typename, typename>
class TimedCache;

namespace Internal {
template <typename Cache, typename UnderlyingIterator>
using BaseForBaseUnorderedIterator =
    BaseIterator<std::forward_iterator_tag,
                 decltype(UnderlyingIterator()->first),
                 decltype(UnderlyingIterator()->second.value),
                 Cache,
                 UnderlyingIterator>;

/// The base class for all const and non-const unordered iterators.
///
/// An unordered iterator is a wrapper around an `unordered_map` iterator with
/// ForwardIterator category. As such, it is (nearly) as fast to access the pair
/// as through the unordered iterator as through the map iterator directly.
/// However, the order of keys is unspecified. For this reason, unordered
/// iterators have the special property that they may be used to construct
/// ordered iterators, after which the order of insertion is respected.
///
/// \tparam Cache The type of the cache instances of the iterator point into.
/// \tparam UnderlyingIterator The underlying iterator class used to implement
///                            the iterator.
template <typename Cache, typename UnderlyingIterator>
class BaseUnorderedIterator
    : public BaseForBaseUnorderedIterator<Cache, UnderlyingIterator> {
 protected:
  using super = BaseForBaseUnorderedIterator<Cache, UnderlyingIterator>;
  using PRIVATE_BASE_ITERATOR_MEMBERS;
  // These are the key and value types the BaseIterator extracts
  using Key = typename super::KeyType;
  using Value = typename super::ValueType;

 public:
  using Tag = LRU::Tag::UnorderedIterator;
  using PUBLIC_BASE_ITERATOR_MEMBERS;

  /// Constructor.
  BaseUnorderedIterator() noexcept = default;

  /// \copydoc BaseIterator::BaseIterator(Cache,UnderlyingIterator)
  explicit BaseUnorderedIterator(Cache& cache,
                                 const UnderlyingIterator& iterator) noexcept
  : super(cache, iterator) {
  }

  /// Generalized copy constructor.
  ///
  /// Useful mainly for non-const to const conversion.
  ///
  /// \param other The iterator to copy from.
  template <typename AnyCache, typename AnyUnderlyingIterator>
  BaseUnorderedIterator(
      const BaseUnorderedIterator<AnyCache, AnyUnderlyingIterator>&
          other) noexcept
  : super(other) {
  }

  /// Copy constructor.
  BaseUnorderedIterator(const BaseUnorderedIterator& other) noexcept = default;

  /// Move constructor.
  BaseUnorderedIterator(BaseUnorderedIterator&& other) noexcept = default;

  /// Copy assignment operator.
  BaseUnorderedIterator&
  operator=(const BaseUnorderedIterator& other) noexcept = default;

  /// Move assignment operator.
  template <typename AnyCache, typename AnyUnderlyingIterator>
  BaseUnorderedIterator&
  operator=(BaseUnorderedIterator<AnyCache, AnyUnderlyingIterator>
                unordered_iterator) noexcept {
    swap(unordered_iterator);
    return *this;
  }

  /// Destructor.
  virtual ~BaseUnorderedIterator() = default;

  /// Compares this iterator for equality with another unordered iterator.
  ///
  /// \param other Another unordered iterator.
  /// \returns True if both iterators point to the same entry, else false.
  template <typename AnyCache, typename AnyIterator>
  bool
  operator==(const BaseUnorderedIterator<AnyCache, AnyIterator>& other) const
      noexcept {
    return this->_iterator == other._iterator;
  }

  /// Compares this iterator for inequality with another unordered iterator.
  ///
  /// \param other Another unordered iterator.
  /// \returns True if the iterators point to different entries, else false.
  template <typename AnyCache, typename AnyIterator>
  bool
  operator!=(const BaseUnorderedIterator<AnyCache, AnyIterator>& other) const
      noexcept {
    return !(*this == other);
  }

  /// Increments the iterator to the next entry.
  ///
  /// If the iterator already pointed to the end any number of increments
  /// before, behavior is undefined.
  ///
  /// \returns The resulting iterator.
  BaseUnorderedIterator& operator++() {
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
  BaseUnorderedIterator operator++(int) {
    auto previous = *this;
    ++*this;
    return previous;
  }

  /// \copydoc BaseIterator::operator*
  /// \details If the iterator is invalid, behavior is undefined. No exception
  /// handling is performed.
  Entry& operator*() noexcept override {
    if (!_entry.has_value()) {
      _entry.emplace(_iterator->first, _iterator->second.value);
    }

    return *_entry;
  }

  /// \returns A reference to the entry the iterator points to.
  /// \throws LRU::Error::InvalidIterator if the iterator is the end iterator.
  /// \throws LRU::Error::KeyExpired if the key pointed to by the iterator has
  /// expired.
  Entry& entry() override {
    if (!_entry.has_value()) {
      _entry.emplace(_iterator->first, _iterator->second.value);
    }

    _cache->throw_if_invalid(*this);
    return *_entry;
  }

  /// \returns A reference to the key the iterator points to.
  /// \throws LRU::Error::InvalidIterator if the iterator is the end iterator.
  /// \throws LRU::Error::KeyExpired if the key pointed to by the iterator has
  /// expired.
  const Key& key() override {
    return entry().key();
  }

  /// \returns A reference to the value the iterator points to.
  /// \throws LRU::Error::InvalidIterator if the iterator is the end iterator.
  /// \throws LRU::Error::KeyExpired if the key pointed to by the iterator has
  /// expired.
  Value& value() override {
    return entry().value();
  }

 protected:
  template <typename, typename, typename>
  friend class BaseOrderedIterator;

  template <typename, typename, typename, typename, typename>
  friend class LRU::TimedCache;
};
}  // namespace Internal
}  // namespace LRU

#endif  // BASE_UNORDERED_ITERATOR_HPP
