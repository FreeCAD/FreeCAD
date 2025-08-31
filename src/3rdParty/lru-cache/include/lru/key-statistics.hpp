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


#ifndef LRU_KEY_STATISTICS_HPP
#define LRU_KEY_STATISTICS_HPP

#include <cstddef>

namespace LRU {

/// Stores statistics for a single key.
///
/// The statistics stored are the total number of hits and the total number of
/// misses. The total number of acccesses (the sum of hits and misses) may be
/// accessed as well.
struct KeyStatistics {
  using size_t = std::size_t;

  /// Constructor.
  ///
  /// \param hits_ The initial number of hits for the key.
  /// \param misses_ The initial number of misses for the key.
  explicit KeyStatistics(size_t hits_ = 0, size_t misses_ = 0)
  : hits(hits_), misses(misses_) {
  }

  /// \returns The total number of accesses made for the key.
  /// \details This is the sum of the hits and misses.
  size_t accesses() const noexcept {
    return hits + misses;
  }

  /// Resets the statistics for a key (sets them to zero).
  void reset() {
    hits = 0;
    misses = 0;
  }

  /// The number of hits for the key.
  size_t hits;

  /// The number of misses for the key.
  size_t misses;
};

}  // namespace LRU

#endif  // LRU_KEY_STATISTICS_HPP
