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

#include <iostream>

#include "lru/lru.hpp"

using Cache = LRU::Cache<std::uint64_t, std::uint64_t>;

std::uint64_t fibonacci(std::uint64_t n, Cache& cache) {
  if (n < 2) return 1;

  // We std::uint64_ternally keep track of the last accessed key, meaning a
  // `contains(key)` + `lookup(key)` sequence will involve only a single hash
  // table lookup.
  if (cache.contains(n)) return cache[n];

  auto value = fibonacci(n - 1, cache) + fibonacci(n - 2, cache);

  // Caches are 100% move-aware and we have implemented
  // `unordered_map` style emplacement and insertion.
  cache.emplace(n, value);

  return value;
}

std::uint64_t fibonacci(std::uint64_t n) {
  // Use a capacity of 100 (after 100 insertions, the next insertion will evict
  // the least-recently inserted element). The default capacity is 128. Note
  // that for fibonacci, a capacity of 2 is sufficient (and ideal).
  Cache cache(100);
  cache.monitor(2, 3, 4, 5);
  auto value = fibonacci(n, cache);

  for (auto i : {2, 3, 4, 5}) {
    auto stats = cache.stats().stats_for(i);
    // clang-format off
    std::cout << "Statistics for " << i << ": "
              << stats.hits << " hit(s), "
              << stats.misses << " miss(es)."
              << std::endl;
  }

  // You'll notice we'll always have n - 1 misses, for each time we access
  // one of the numbers in [0, n] for the first time.
  std::cout << "Overall: "
            << cache.stats().total_hits() << " hit(s), "
            << cache.stats().total_misses() << " miss(es)."
            << std::endl;
  // clang-format on

  return value;
}

auto main() -> int {
  // The last number that fits into a 64 bit unsigned number
  std::cout << fibonacci(92) << std::endl;
}
