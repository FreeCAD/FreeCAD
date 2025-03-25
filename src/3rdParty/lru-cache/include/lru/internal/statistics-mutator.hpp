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

#ifndef LRU_STATISTICS_MUTATOR_HPP
#define LRU_STATISTICS_MUTATOR_HPP

#include <cassert>
#include <cstddef>
#include <memory>
#include <utility>

#include <lru/internal/optional.hpp>
#include <lru/statistics.hpp>

namespace LRU {
namespace Internal {

/// A mutable proxy interface to a statistics object.
///
/// The `StatisticsMutator` allows modification of the members of a statistics
/// object via a narrow interface, available only to internal classes. The point
/// of this is that while we don't want the user to be able to modify the hit or
/// miss count on a statistics object (it is "getter-only" in that sense), it's
/// also not ideal, from an encapsulation standpoint, to make the cache classes
/// (which do need to access and modify the hit and miss counts) friends of the
/// statistics. This is especially true since the caches should only need to
/// register hits or misses and not have to increment the count of total
/// accesses. As such, we really require a "package-level" interface that is not
/// visible to the end user, while at the same time providing an interface to
/// internal classes. The `StatisticsMutator` is a proxy/adapter class that
/// serves exactly this purpose. It is friends with the `Statistics` and can
/// thus access its members. At the same time the interface it defines is narrow
/// and provides only the necessary interface for the cache classes to register
/// hits and misses.
template <typename Key>
class StatisticsMutator {
 public:
  using StatisticsPointer = std::shared_ptr<Statistics<Key>>;

  /// Constructor.
  StatisticsMutator() noexcept = default;

  /// Constructor.
  ///
  /// \param stats A shared pointer lvalue reference.
  StatisticsMutator(const StatisticsPointer& stats)  // NOLINT(runtime/explicit)
      : _stats(stats) {
  }

  /// Constructor.
  ///
  /// \param stats A shared pointer rvalue reference to move into the
  ///                   mutator.
  StatisticsMutator(StatisticsPointer&& stats)  // NOLINT(runtime/explicit)
      : _stats(std::move(stats)) {
  }

  /// Registers a hit for the given key with the internal statistics.
  ///
  /// \param key The key to register a hit for.
  void register_hit(const Key& key) {
    assert(has_stats());

    _stats->_total_accesses += 1;
    _stats->_total_hits += 1;

    auto iterator = _stats->_key_map.find(key);
    if (iterator != _stats->_key_map.end()) {
      iterator->second.hits += 1;
    }
  }

  /// Registers a miss for the given key with the internal statistics.
  ///
  /// \param key The key to register a miss for.
  void register_miss(const Key& key) {
    assert(has_stats());

    _stats->_total_accesses += 1;

    auto iterator = _stats->_key_map.find(key);
    if (iterator != _stats->_key_map.end()) {
      iterator->second.misses += 1;
    }
  }

  /// \returns A reference to the statistics object.
  Statistics<Key>& get() noexcept {
    assert(has_stats());
    return *_stats;
  }

  /// \returns A const reference to the statistics object.
  const Statistics<Key>& get() const noexcept {
    assert(has_stats());
    return *_stats;
  }

  /// \returns A `shared_ptr` to the statistics object.
  StatisticsPointer& shared() noexcept {
    return _stats;
  }

  /// \returns A const `shared_ptr` to the statistics object.
  const StatisticsPointer& shared() const noexcept {
    return _stats;
  }

  /// \returns True if the mutator has a statistics object, else false.
  bool has_stats() const noexcept {
    return _stats != nullptr;
  }

  /// \copydoc has_stats()
  explicit operator bool() const noexcept {
    return has_stats();
  }

  /// Resets the internal statistics pointer.
  void reset() {
    _stats.reset();
  }

 private:
  /// A shared pointer to a statistics object.
  std::shared_ptr<Statistics<Key>> _stats;
};

}  // namespace Internal
}  // namespace LRU

#endif  // LRU_STATISTICS_MUTATOR_HPP
