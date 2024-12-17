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

#ifndef LRU_INTERNAL_OPTIONAL_HPP
#define LRU_INTERNAL_OPTIONAL_HPP

#ifndef __has_include
#define USE_LRU_OPTIONAL
#elif __has_include(<optional>)

#include <optional>

namespace LRU {
namespace Internal {
template <typename T>
using Optional = std::optional<T>;
}  // namespace Internal
}  // namespace LRU

#else
#define USE_LRU_OPTIONAL
#endif

#ifdef USE_LRU_OPTIONAL
#include <memory>
#include <stdexcept>

namespace LRU {
namespace Internal {

// A roll-your-own replacement of `std::optional`.
//
// This class is only to be used if `std::optional` is unavailable. It
// implements an optional type simply on top of a `unique_ptr`. It is
// API-compatible with `std::optional`, as required for our purposes.
template <typename T>
class Optional {
 public:
  /// Constructor.
  Optional() = default;

  /// Copy constructor.
  ///
  /// \param other The other optional object to copy from.
  Optional(const Optional& other) {
    if (other) emplace(*other);
  }

  /// Generalized copy constructor.
  ///
  /// \param other The other optional object to copy from.
  template <typename U,
            typename = std::enable_if_t<std::is_convertible<T, U>::value>>
  Optional(const Optional<U>& other) {
    if (other) emplace(*other);
  }

  /// Move constructor.
  ///
  /// \param other The other optional object to move into this one.
  Optional(Optional&& other) noexcept {
    swap(other);
  }

  /// Generalized move constructor.
  ///
  /// \param other The other optional object to move into this one.
  template <typename U,
            typename = std::enable_if_t<std::is_convertible<T, U>::value>>
  Optional(Optional<U>&& other) noexcept {
    if (other) {
      _value = std::make_unique<T>(std::move(*other));
    }
  }

  /// Assignment operator.
  ///
  /// \param other The other object to assign from.
  /// \returns The resulting optional instance.
  Optional& operator=(Optional other) noexcept {
    swap(other);
    return *this;
  }

  /// Swaps the contents of this optional with another one.
  ///
  /// \param other The other optional to swap with.
  void swap(Optional& other) {
    _value.swap(other._value);
  }

  /// Swaps the contents of two optionals.
  ///
  /// \param first The first optional to swap.
  /// \param second The second optional to swap.
  friend void swap(Optional& first, Optional& second) /* NOLINT */ {
    first.swap(second);
  }

  /// \returns True if the `Optional` has a value, else false.
  bool has_value() const noexcept {
    return static_cast<bool>(_value);
  }

  /// \copydoc has_value()
  explicit operator bool() const noexcept {
    return has_value();
  }

  /// \returns A pointer to the current value. Behavior is undefined if the
  /// optional has no value.
  T* operator->() {
    return _value.get();
  }

  /// \returns A const pointer to the current value. Behavior is undefined if
  /// the `Optional` has no value.
  const T* operator->() const {
    return _value.get();
  }

  /// \returns A const reference to the current value. Behavior is undefined if
  /// the `Optional` has no value.
  const T& operator*() const {
    return *_value;
  }

  /// \returns A reference to the current value. Behavior is undefined if
  /// the `Optional` has no value.
  T& operator*() {
    return *_value;
  }

  /// \returns A reference to the current value.
  /// \throws std::runtime_error If the `Optional` currently has no value.
  T& value() {
    if (!has_value()) {
      // Actually std::bad_optional_access
      throw std::runtime_error("optional has no value");
    }

    return *_value;
  }

  /// \returns A const reference to the current value.
  /// \throws std::runtime_error If the `Optional` currently has no value.
  const T& value() const {
    if (!has_value()) {
      // Actually std::bad_optional_access
      throw std::runtime_error("optional has no value");
    }

    return *_value;
  }

  /// \returns The current value, or the given argument if there is no value.
  /// \param default_value The value to return if this `Optional` currently has
  ///                      no value.
  template <class U>
  T value_or(U&& default_value) const {
    return *this ? **this : static_cast<T>(std::forward<U>(default_value));
  }

  /// Resets the `Optional` to have no value.
  void reset() noexcept {
    _value.reset();
  }

  /// Constructs the `Optional`'s value with the given arguments.
  ///
  /// \param args Arguments to perfeclty forward to the value's constructor.
  template <typename... Args>
  void emplace(Args&&... args) {
    _value = std::make_unique<T>(std::forward<Args>(args)...);
  }

 private:
  template <typename>
  friend class Optional;

  /// The value, as we implement it.
  std::unique_ptr<T> _value;
};
}  // namespace Internal
}  // namespace LRU

#endif

#endif  // LRU_INTERNAL_OPTIONAL_HPP
