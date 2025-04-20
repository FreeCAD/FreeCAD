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


#ifndef LRU_STATISTICS_HPP
#define LRU_STATISTICS_HPP

#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include <lru/error.hpp>
#include <lru/internal/utility.hpp>
#include <lru/key-statistics.hpp>

namespace LRU {
namespace Internal {
template <typename>
class StatisticsMutator;
}

/// Stores statistics about LRU cache utilization and efficiency.
///
/// The statistics object stores the number of misses and hits were recorded for
/// a cache in total. Furthemore, it is possibly to register a number of keys
/// for *monitoring*. For each of these keys, an additional hit and miss count
/// is maintained, that can keep insight into the utiliization of a particular
/// cache. Note that accesses only mean lookups -- insertions or erasures will
/// never signify an "access".
///
/// \tparam Key The type of the keys being monitored.
template <typename Key>
class Statistics {
 public:
  using size_t = std::size_t;
  using InitializerList = std::initializer_list<Key>;

  /// Constructor.
  Statistics() noexcept : _total_accesses(0), _total_hits(0) {
  }

  /// Constructor.
  ///
  /// \param keys Any number of keys to monitor.
  template <typename... Keys,
            typename = std::enable_if_t<Internal::all_of_type<Key, Keys...>>>
  explicit Statistics(Keys&&... keys) : Statistics() {
    // clang-format off
    Internal::for_each([this](auto&& key) {
      this->monitor(std::forward<decltype(key)>(key));
    }, std::forward<Keys>(keys)...);
    // clang-format on
  }

  /// Constructor.
  ///
  /// \param range A range of keys to monitor.
  template <typename Range, typename = Internal::enable_if_range<Range>>
  explicit Statistics(const Range& range)
  : Statistics(std::begin(range), std::end(range)) {
  }

  /// Constructor.
  ///
  /// \param begin The start iterator of a range of keys to monitor.
  /// \param end The end iterator of a range of keys to monitor.
  template <typename Iterator,
            typename = Internal::enable_if_iterator<Iterator>>
  Statistics(Iterator begin, Iterator end) : Statistics() {
    for (; begin != end; ++begin) {
      monitor(*begin);
    }
  }

  /// Constructor.
  ///
  /// \param list A list of keys to monitor.
  Statistics(InitializerList list)  // NOLINT(runtime/explicit)
      : Statistics(list.begin(), list.end()) {
  }

  /// \returns The total number of accesses (hits + misses) made to the cache.
  size_t total_accesses() const noexcept {
    return _total_accesses;
  }

  /// \returns The total number of hits made to the cache.
  size_t total_hits() const noexcept {
    return _total_hits;
  }

  /// \returns The total number of misses made to the cache.
  size_t total_misses() const noexcept {
    return total_accesses() - total_hits();
  }

  /// \returns The ratio of hits ($\in [0, 1]$) relative to all accesses.
  double hit_rate() const noexcept {
    return static_cast<double>(total_hits()) / total_accesses();
  }

  /// \returns The ratio of misses ($\in [0, 1]$) relative to all accesses.
  double miss_rate() const noexcept {
    return 1 - hit_rate();
  }

  /// \returns The number of hits for the given key.
  /// \param key The key to retrieve the hits for.
  /// \throws LRU::UnmonitoredKey if the key was not registered for monitoring.
  size_t hits_for(const Key& key) const {
    return stats_for(key).hits;
  }

  /// \returns The number of misses for the given key.
  /// \param key The key to retrieve the misses for.
  /// \throws LRU::UnmonitoredKey if the key was not registered for monitoring.
  size_t misses_for(const Key& key) const {
    return stats_for(key).misses;
  }

  /// \returns The number of accesses (hits + misses) for the given key.
  /// \param key The key to retrieve the accesses for.
  /// \throws LRU::UnmonitoredKey if the key was not registered for monitoring.
  size_t accesses_for(const Key& key) const {
    return stats_for(key).accesses();
  }

  /// \returns A `KeyStatistics` object for the given key.
  /// \param key The key to retrieve the stats for.
  /// \throws LRU::UnmonitoredKey if the key was not registered for monitoring.
  const KeyStatistics& stats_for(const Key& key) const {
    auto iterator = _key_map.find(key);
    if (iterator == _key_map.end()) {
      throw LRU::Error::UnmonitoredKey();
    }

    return iterator->second;
  }

  /// \copydoc stats_for()
  const KeyStatistics& operator[](const Key& key) const {
    return stats_for(key);
  }

  /// Registers the key for monitoring.
  ///
  /// If the key was already registered, this is a no-op (most importantly, the
  /// old statistics are __not__ wiped).
  ///
  /// \param key The key to register.
  void monitor(const Key& key) {
    // emplace does nothing if the key is already present
    _key_map.emplace(key, KeyStatistics());
  }

  /// Unregisters the given key from monitoring.
  ///
  /// \param key The key to unregister.
  /// \throws LRU::Error::UnmonitoredKey if the key was never registered for
  /// monitoring.
  void unmonitor(const Key& key) {
    auto iterator = _key_map.find(key);
    if (iterator == _key_map.end()) {
      throw LRU::Error::UnmonitoredKey();
    } else {
      _key_map.erase(iterator);
    }
  }

  /// Unregisters all keys from monitoring.
  void unmonitor_all() {
    _key_map.clear();
  }

  /// Clears all statistics for the given key, but keeps on monitoring it.
  ///
  /// \param key The key to reset.
  void reset_key(const Key& key) {
    auto iterator = _key_map.find(key);
    if (iterator == _key_map.end()) {
      throw LRU::Error::UnmonitoredKey();
    } else {
      iterator->second.reset();
    }
  }

  /// Clears the statistics of all keys, but keeps on monitoring it them.
  void reset_all() {
    for (auto& pair : _key_map) {
      _key_map.second.reset();
    }
  }

  /// \returns True if the given key is currently registered for monitoring,
  /// else false.
  /// \param key The key to check for.
  bool is_monitoring(const Key& key) const noexcept {
    return _key_map.count(key);
  }

  /// \returns The number of keys currnetly being monitored.
  size_t number_of_monitored_keys() const noexcept {
    return _key_map.size();
  }

  /// \returns True if currently any keys at all are being monitored, else
  /// false.
  bool is_monitoring_keys() const noexcept {
    return !_key_map.empty();
  }

 private:
  template <typename>
  friend class Internal::StatisticsMutator;

  using HitMap = std::unordered_map<Key, KeyStatistics>;

  /// The total number of accesses made for any key.
  size_t _total_accesses;

  /// The total number of htis made for any key.
  size_t _total_hits;

  /// The map to keep track of statistics for monitored keys.
  HitMap _key_map;
};

namespace Lowercase {
template <typename... Ts>
using statistics = Statistics<Ts...>;
}  // namespace Lowercase

}  // namespace LRU

#endif  // LRU_STATISTICS_HPP
