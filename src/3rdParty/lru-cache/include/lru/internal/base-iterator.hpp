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

#ifndef LRU_INTERNAL_BASE_ITERATOR_HPP
#define LRU_INTERNAL_BASE_ITERATOR_HPP

#include <algorithm>
#include <iterator>

#include <lru/entry.hpp>
#include <lru/internal/optional.hpp>

#define PUBLIC_BASE_ITERATOR_MEMBERS \
  typename super::Entry;             \
  using typename super::KeyType;     \
  using typename super::ValueType;

#define PRIVATE_BASE_ITERATOR_MEMBERS \
  super::_iterator;                   \
  using super::_entry;                \
  using super::_cache;


namespace LRU {
namespace Internal {

/// The base class for all (ordered and unordered) iterators.
///
/// All iterators over our LRU caches store a reference to the cache they point
/// into, an underlying iterator they adapt (e.g. a map iterator or list
/// iterator) as well as a entry, a reference to which is returned when
/// dereferencing the iterator.
///
/// \tparam IteratorTag A standard iterator category tag.
/// \tparam Key The key type over which instances of the iterator iterate.
/// \tparam Value The value type over which instances of the iterator iterate.
/// \tparam Cache The type of the cache instances of the iterator point into.
/// \tparam UnderlyingIterator The underlying iterator class used to implement
///                            the iterator.
template <typename IteratorTag,
          typename Key,
          typename Value,
          typename Cache,
          typename UnderlyingIterator>
class BaseIterator : public std::iterator<IteratorTag, LRU::Entry<Key, Value>> {
 public:
  using KeyType = Key;
  using ValueType =
      std::conditional_t<std::is_const<Cache>::value, const Value, Value>;
  using Entry = LRU::Entry<KeyType, ValueType>;

  /// Default constructor.
  BaseIterator() noexcept : _cache(nullptr) {
  }

  /// Constructor.
  ///
  /// \param cache The cache this iterator points into.
  /// \param iterator The underlying iterator to adapt.
  BaseIterator(Cache& cache, const UnderlyingIterator& iterator) noexcept
  : _iterator(iterator), _cache(&cache) {
  }

  /// Copy constructor.
  ///
  /// Differs from the default copy constructor in that it does not copy the
  /// entry.
  ///
  /// \param other The base iterator to copy.
  BaseIterator(const BaseIterator& other) noexcept
  : _iterator(other._iterator), _cache(other._cache) {
    // Note: we do not copy the entry, as it would require a new allocation.
    // Since iterators are often taken by value, this may incur a high cost.
    // As such we delay the retrieval of the entry to the first call to entry().
  }

  /// Copy assignment operator.
  ///
  /// Differs from the default copy assignment
  /// operator in that it does not copy the entry.
  ///
  /// \param other The base iterator to copy.
  /// \return The base iterator instance.
  BaseIterator& operator=(const BaseIterator& other) noexcept {
    if (this != &other) {
      _iterator = other._iterator;
      _cache = other._cache;
      _entry.reset();
    }
    return *this;
  }

  /// Move constructor.
  BaseIterator(BaseIterator&& other) noexcept = default;

  /// Move assignment operator.
  BaseIterator& operator=(BaseIterator&& other) noexcept = default;

  /// Generalized copy constructor.
  ///
  /// Mainly necessary for non-const to const conversion.
  ///
  /// \param other The base iterator to copy from.
  template <typename AnyIteratorTag,
            typename AnyKeyType,
            typename AnyValueType,
            typename AnyCacheType,
            typename AnyUnderlyingIteratorType>
  BaseIterator(const BaseIterator<AnyIteratorTag,
                                  AnyKeyType,
                                  AnyValueType,
                                  AnyCacheType,
                                  AnyUnderlyingIteratorType>& other)
  : _iterator(other._iterator), _entry(other._entry), _cache(other._cache) {
  }

  /// Generalized move constructor.
  ///
  /// Mainly necessary for non-const to const conversion.
  ///
  /// \param other The base iterator to move into this one.
  template <typename AnyIteratorTag,
            typename AnyKeyType,
            typename AnyValueType,
            typename AnyCacheType,
            typename AnyUnderlyingIteratorType>
  BaseIterator(BaseIterator<AnyIteratorTag,
                            AnyKeyType,
                            AnyValueType,
                            AnyCacheType,
                            AnyUnderlyingIteratorType>&& other) noexcept
  : _iterator(std::move(other._iterator))
  , _entry(std::move(other._entry))
  , _cache(std::move(other._cache)) {
  }

  /// Destructor.
  virtual ~BaseIterator() = default;

  /// Swaps this base iterator with another one.
  ///
  /// \param other The other iterator to swap with.
  void swap(BaseIterator& other) noexcept {
    // Enable ADL
    using std::swap;

    swap(_iterator, other._iterator);
    swap(_entry, other._entry);
    swap(_cache, other._cache);
  }

  /// Swaps two base iterator.
  ///
  /// \param first The first iterator to swap.
  /// \param second The second iterator to swap.
  friend void swap(BaseIterator& first, BaseIterator& second) noexcept {
    first.swap(second);
  }

  /// \returns A reference to the current entry pointed to by the iterator.
  virtual Entry& operator*() noexcept = 0;

  /// \returns A pointer to the current entry pointed to by the iterator.
  Entry* operator->() noexcept {
    return &(**this);
  }

  /// \copydoc operator*()
  virtual Entry& entry() = 0;

  /// \returns A reference to the value of the entry currently pointed to by the
  /// iterator.
  virtual ValueType& value() = 0;

  /// \returns A reference to the key of the entry currently pointed to by the
  /// iterator.
  virtual const Key& key() = 0;

 protected:
  template <typename, typename, typename, typename, typename>
  friend class BaseIterator;

  /// The underlying iterator this iterator class adapts.
  UnderlyingIterator _iterator;

  /// The entry optionally being stored.
  Optional<Entry> _entry;

  /// A pointer to the cache this iterator points into.
  /// Pointer and not reference because it's cheap to copy.
  /// Pointer and not `std::reference_wrapper` because the class needs to be
  /// default-constructible.
  Cache* _cache;
};
}  // namespace Internal
}  // namespace LRU

#endif  // LRU_INTERNAL_BASE_ITERATOR_HPP
