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

  // clang-format off
  cache.hit_callback([](auto& key, auto& value) {
    std::clog << "Hit for entry ("
              << key << ", " << value << ")"
              << std::endl;
  });

  cache.miss_callback([](auto& key) {
    std::clog << "Miss for " << key<< std::endl;
  });

  cache.access_callback([](auto& key, bool was_hit) {
    std::clog << "Access for " << key
              << " was a " << (was_hit ? "hit" : "miss")
              << std::endl;
  });
  // clang-format on

  auto value = fibonacci(n, cache);

  return value;
}

auto main() -> int {
  std::cout << fibonacci(10) << std::endl;
}
