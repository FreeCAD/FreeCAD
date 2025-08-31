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

#ifndef LRU_INTERNAL_CALLBACK_MANAGER_HPP
#define LRU_INTERNAL_CALLBACK_MANAGER_HPP

#include <functional>
#include <vector>

#include <lru/entry.hpp>
#include <lru/internal/optional.hpp>

namespace LRU {
namespace Internal {

/// Manages hit, miss and access callbacks for a cache.
///
/// The callback manager implements the "publisher" of the observer pattern we
/// implement. It stores and calls three kinds of callbacks:
/// 1. Hit callbacks, taking a key and value after a cache hit.
/// 2. Miss callbacks, taking only a key, that was not found in a cache.
/// 3. Access callbacks, taking a key and a boolean indicating a hit or a miss.
///
/// Callbacks can be added, accessed and cleared.
template <typename Key, typename Value>
class CallbackManager {
 public:
  using HitCallback = std::function<void(const Key&, const Value&)>;
  using MissCallback = std::function<void(const Key&)>;
  using AccessCallback = std::function<void(const Key&, bool)>;

  using HitCallbackContainer = std::vector<HitCallback>;
  using MissCallbackContainer = std::vector<MissCallback>;
  using AccessCallbackContainer = std::vector<AccessCallback>;

  /// Calls all callbacks registered for a hit, with the given key and value.
  ///
  /// \param key The key for which a cache hit ocurred.
  /// \param value The value that was found for the key.
  void hit(const Key& key, const Value& value) {
    _call_each(_hit_callbacks, key, value);
    _call_each(_access_callbacks, key, true);
  }

  /// Calls all callbacks registered for a miss, with the given key.
  ///
  /// \param key The key for which a cache miss ocurred.
  void miss(const Key& key) {
    _call_each(_miss_callbacks, key);
    _call_each(_access_callbacks, key, false);
  }

  /// Registers a new hit callback.
  ///
  /// \param hit_callback The hit callback function to register with the
  ///                     manager.
  template <typename Callback>
  void hit_callback(Callback&& hit_callback) {
    _hit_callbacks.emplace_back(std::forward<Callback>(hit_callback));
  }

  /// Registers a new miss callback.
  ///
  /// \param miss_callback The miss callback function to register with the
  ///                      manager.
  template <typename Callback>
  void miss_callback(Callback&& miss_callback) {
    _miss_callbacks.emplace_back(std::forward<Callback>(miss_callback));
  }

  /// Registers a new access callback.
  ///
  /// \param access_callback The access callback function to register with the
  ///                        manager.
  template <typename Callback>
  void access_callback(Callback&& access_callback) {
    _access_callbacks.emplace_back(std::forward<Callback>(access_callback));
  }

  /// Clears all hit callbacks.
  void clear_hit_callbacks() {
    _hit_callbacks.clear();
  }

  /// Clears all miss callbacks.
  void clear_miss_callbacks() {
    _miss_callbacks.clear();
  }

  /// Clears all access callbacks.
  void clear_access_callbacks() {
    _access_callbacks.clear();
  }

  /// Clears all callbacks.
  void clear() {
    clear_hit_callbacks();
    clear_miss_callbacks();
    clear_access_callbacks();
  }

  /// \returns All hit callbacks.
  const HitCallbackContainer& hit_callbacks() const noexcept {
    return _hit_callbacks;
  }

  /// \returns All miss callbacks.
  const MissCallbackContainer& miss_callbacks() const noexcept {
    return _miss_callbacks;
  }

  /// \returns All access callbacks.
  const AccessCallbackContainer& access_callbacks() const noexcept {
    return _access_callbacks;
  }

 private:
  /// Calls each function in the given container with the given arguments.
  ///
  /// \param callbacks The container of callbacks to call.
  /// \param args The arguments to call the callbacks with.
  template <typename CallbackContainer, typename... Args>
  void _call_each(const CallbackContainer& callbacks, Args&&... args) {
    for (const auto& callback : callbacks) {
      callback(std::forward<Args>(args)...);
    }
  }

  /// The container of hit callbacks registered.
  HitCallbackContainer _hit_callbacks;

  /// The container of miss callbacks registered.
  MissCallbackContainer _miss_callbacks;

  /// The container of access callbacks registered.
  AccessCallbackContainer _access_callbacks;
};
}  // namespace Internal
}  // namespace LRU

#endif  // LRU_INTERNAL_CALLBACK_MANAGER_HPP
