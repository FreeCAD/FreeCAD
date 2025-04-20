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

#ifndef LRU_INTERNAL_LAST_ACCESSED_HPP
#define LRU_INTERNAL_LAST_ACCESSED_HPP

#include <algorithm>
#include <functional>
#include <iterator>

#include <lru/internal/utility.hpp>

namespace LRU {
namespace Internal {

/// Provides a simple iterator-compatible pointer object for a key and
/// information.
///
/// The easisest idea for this class, theoretically, would be to just store an s
/// iterator to the internal cache map (i.e. template the class on the iterator
/// type). However, the major trouble with that approach is that this class
/// should be 100% *mutable*, as in "always non-const", so that  keys and
/// informations
/// we store for fast access can be (quickly) retrieved as either const or
/// non-const (iterators for example). This is not possible, since the
/// const-ness of `const_iterators` are not the usual idea of const in C++,
/// meaning especially it cannot be cast away with a `const_cast` as is required
/// for the mutability. As such, we *must* store the plain keys and
/// informations.
/// This, however, means that iterators cannot be stored efficiently, since a
/// new hash table lookup would be required to go from a key to its iterator.
/// However, since the main use case of this class is to avoid a second lookup
/// in the usual `if (cache.contains(key)) return cache.lookup(key)`, which is
/// not an issue for iterators since they can be compared to the `end` iterator
/// in constant time (equivalent to the call to `contains()`).
///
/// WARNING: This class stores *pointers* to keys and informations. As such
/// lifetime
/// of the pointed-to objects must be cared for by the user of this class.
///
/// \tparam Key The type of key being accessed.
/// \tparam InformationType The type of information being accessed.
/// \tparam KeyEqual The type of the key comparison function.
template <typename Key,
          typename InformationType,
          typename KeyEqual = std::equal_to<Key>>
class LastAccessed {
 public:
  /// Constructor.
  ///
  /// \param key_equal The function to compare keys with.
  explicit LastAccessed(const KeyEqual& key_equal = KeyEqual())
  : _key(nullptr)
  , _information(nullptr)
  , _is_valid(false)
  , _key_equal(key_equal) {
  }

  /// Constructor.
  ///
  /// \param key The key to store a reference to.
  /// \param information The information to store a reference to.
  /// \param key_equal The function to compare keys with.
  LastAccessed(const Key& key,
               const InformationType& information,
               const KeyEqual& key_equal = KeyEqual())
  : _key(const_cast<Key*>(&key))
  , _information(const_cast<InformationType*>(&information))
  , _is_valid(true)
  , _key_equal(key_equal) {
  }

  /// Constructor.
  ///
  /// \param iterator An iterator pointing to a key and information to use for
  ///                constructing the instance.
  /// \param key_equal The function to compare keys with.
  template <typename Iterator>
  explicit LastAccessed(Iterator iterator,
                        const KeyEqual& key_equal = KeyEqual())
  : LastAccessed(iterator->first, iterator->second, key_equal) {
  }

  /// Copy assignment operator for iterators.
  ///
  /// \param iterator An iterator pointing to a key and value to use for the
  ///                 `LastAccessed` instance.
  /// \return The resulting `LastAccessed` instance.
  template <typename Iterator>
  LastAccessed& operator=(Iterator iterator) {
    _key = const_cast<Key*>(&(iterator->first));
    _information = const_cast<InformationType*>(&(iterator->second));
    _is_valid = true;

    return *this;
  }

  /// Compares a `LastAccessed` object for equality with a key.
  ///
  /// \param last_accessed The `LastAccessed` instance to compare.
  /// \param key The key instance to compare.
  /// \returns True if the key of the `LastAccessed` object's key equals the
  /// given key, else false.
  friend bool
  operator==(const LastAccessed& last_accessed, const Key& key) noexcept {
    if (!last_accessed._is_valid) return false;
    return last_accessed._key_equal(key, last_accessed.key());
  }

  /// \copydoc operator==(const LastAccessed&,const Key&)
  friend bool
  operator==(const Key& key, const LastAccessed& last_accessed) noexcept {
    return last_accessed == key;
  }

  /// Compares a `LastAccessed` object  for equality with an iterator.
  ///
  /// \param last_accessed The `LastAccessed` instance to compare.
  /// \param iterator The iterator to compare with.
  /// \returns True if the `LastAccessed` object's key equals that of the
  /// iterator, else false.
  template <typename Iterator, typename = enable_if_iterator<Iterator>>
  friend bool
  operator==(const LastAccessed& last_accessed, Iterator iterator) noexcept {
    /// Fast comparisons to an iterator (not relying on implicit conversion)
    return last_accessed == iterator->first;
  }

  /// \copydoc operator==(const LastAccessed&,Iterator)
  template <typename Iterator, typename = enable_if_iterator<Iterator>>
  friend bool
  operator==(Iterator iterator, const LastAccessed& last_accessed) noexcept {
    return last_accessed == iterator;
  }

  /// Compares a `LastAccessed` object for inequality with something.
  ///
  /// \param last_accessed The `LastAccessed` instance to compare.
  /// \param other Something else to compare to.
  /// \returns True if the key of the `LastAccessed` object's key does not equal
  /// the given other object's key, else false.
  template <typename T>
  friend bool
  operator!=(const LastAccessed& last_accessed, const T& other) noexcept {
    return !(last_accessed == other);
  }

  /// \copydoc operator!=(const LastAccessed&,const T&)
  template <typename T>
  friend bool
  operator!=(const T& other, const LastAccessed& last_accessed) noexcept {
    return !(other == last_accessed);
  }

  /// \returns The last accessed key.
  Key& key() noexcept {
    assert(is_valid());
    return *_key;
  }

  /// \returns The last accessed key.
  const Key& key() const noexcept {
    assert(is_valid());
    return *_key;
  }

  /// \returns The last accessed information.
  InformationType& information() noexcept {
    assert(is_valid());
    return *_information;
  }

  /// \returns The last accessed information.
  const InformationType& information() const noexcept {
    assert(is_valid());
    return *_information;
  }

  /// \returns The last accessed information.
  auto& iterator() noexcept {
    assert(is_valid());
    return _information->order;
  }

  /// \returns The last accessed value.
  auto& value() noexcept {
    assert(is_valid());
    return _information->value;
  }

  /// \returns The last accessed value.
  const auto& value() const noexcept {
    assert(is_valid());
    return _information->value;
  }

  /// \returns True if the key and information of the instance may be accessed,
  /// else false.
  bool is_valid() const noexcept {
    return _is_valid;
  }

  /// \copydoc is_valid()
  explicit operator bool() const noexcept {
    return is_valid();
  }

  /// Invalidates the instance.
  void invalidate() noexcept {
    _is_valid = false;
    _key = nullptr;
    _information = nullptr;
  }

  /// \returns The key comparison function used.
  const KeyEqual& key_equal() const noexcept {
    return _key_equal;
  }

 private:
  /// A pointer to the key that was last accessed (if any).
  Key* _key;

  /// A pointer to the information that was last accessed (if any).
  InformationType* _information;

  /// True if the key and information pointers are valid, else false.
  bool _is_valid;

  /// The function used to compare keys.
  KeyEqual _key_equal;
};
}  // namespace Internal
}  // namespace LRU

#endif  // LRU_INTERNAL_LAST_ACCESSED_HPP
