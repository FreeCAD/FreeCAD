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

#include <chrono>
#include <iostream>

#include "lru/lru.hpp"

using namespace std::chrono_literals;

using Cache = LRU::TimedCache<int, int>;

int fibonacci(int n, Cache& cache) {
  if (n < 2) return 1;

  // We internally keep track of the last accessed key, meaning a
  // `contains(key)` + `lookup(key)` sequence will involve only a single hash
  // table lookup.
  if (cache.contains(n)) return cache[n];

  auto value = fibonacci(n - 1, cache) + fibonacci(n - 2, cache);

  // Caches are 100% move-aware and we have implemented
  // `unordered_map` style emplacement and insertion.
  cache.emplace(n, value);

  return value;
}

int fibonacci(int n) {
  // Use a time to live of 100ms. This means that 100ms after insertion, a key
  // will be said to have "expired" and `contains(key)` will return false.
  Cache cache(100ms);
  return fibonacci(n, cache);
}

auto main() -> int {
  std::cout << fibonacci(32) << std::endl;
}
