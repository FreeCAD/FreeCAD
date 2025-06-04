# lru-cache

[![GitHub license](https://img.shields.io/github/license/mashape/apistatus.svg?style=flat-square)](http://goldsborough.mit-license.org) [![Build Status](https://travis-ci.org/goldsborough/lru-cache.svg?branch=master)](https://travis-ci.org/goldsborough/lru-cache)

A feature complete LRU cache implementation in C++.

## Description

A *least recently used* (LRU) cache is a fixed size cache that behaves just like a regular lookup table, but remembers the order in which elements are accessed. Once its (user-defined) capacity is reached, it uses this information to replace the least recently used element with a newly inserted one. This is ideal for caching function return values, where fast lookup of complex computations is favorable, but a memory blowup from caching all `(input, output)` pairs is to be avoided.

We provide two implementations of an LRU cache: one has only the basic functionality described above, and another can be additionally supplied with a *time to live*. This is useful, for example, when caching resources on a server, where cache entries should be invalidated automatically after a certain amount of time, because they are no longer "fresh".

Additionally, all our caches can be connected to *statistics* objects, that keep track of cache hits and misses for all keys and, upon request, individual keys (similar to `functools.lru_cache` in Python). You can also register arbitrary callbacks for hits, misses or accesses in general.

## Basic Usage

The two main classes we provide are `LRU::Cache` and `LRU::TimedCache`. A basic usage example of these may look like so:

__`LRU::Cache`__
```C++
#include <iostream>
#include "lru/lru.hpp"

using Cache = LRU::Cache<int, int>;

int fibonacci(int n, Cache& cache) {
  if (n < 2) return 1;

  // We internally keep track of the last accessed key, meaning a
  // `contains(key)` + `lookup(key)` sequence will involve only a single hash
  // table lookup.
  if (cache.contains(n)) return cache.lookup(n);

  auto value = fibonacci(n - 1, cache) + fibonacci(n - 2, cache);

  // Caches are 100% move-aware and we have implemented
  // `unordered_map` style emplacement and insertion.
  cache.emplace(n, value);

  return value;
}

int fibonacci(int n) {
  // Use a capacity of 100 (after 100 insertions, the next insertion will evict
  // the least-recently accessed element). The default capacity is 128.
  Cache cache(100);
  return fibonacci(n, cache);
}
```

__`LRU::TimedCache`__
```C++
#include <chrono>
#include <iostream>

#include "lru/lru.hpp"

using namespace std::chrono_literals;

using Cache = LRU::TimedCache<int, int>;

int fibonacci(int n, Cache& cache) {
  if (n < 2) return 1;
  if (cache.contains(n)) return cache[n];

  auto value = fibonacci(n - 1, cache) + fibonacci(n - 2, cache);
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
```

## Extended Usage

Our caches bring along many exciting features including statistics monitoring, function wrapping, arbitrary callbacks as well as ordered and unordered iteration.

### Ordered and Unordered Iteration

Both the `LRU::Cache` and `LRU::TimedCache` can be iterated over in two ways: ordered or unordered fashion (where the "order" refers to the order of insertion). The default iterators returned by `begin()`, `cbegin()`, `end()` etc. are *unordered* and mostly similar to `unordered_map` iterators (with some nice non-standard sugar):

```C++
LRU::Cache<int, int> cache = {{1, 2}, {2, 3}, {3, 4}};

int sum = 0;
for (const auto& pair : cache) {
  sum += pair.first; // Standards compliant (good for templates)
  sum += pair.value(); // But sugar on top (also key())
}

auto iterator = cache.begin();           // These two lines
auto iterator = cache.unordered_begin(); // are the same
auto iterator = cache.ordered_end();     // This is something different
```

Unordered iterators are implemented directly over internal map iterators and thus have access to the key and value of a pointed-to entry.

Ordered iterators respect the order of insertion. They differ in a few ways from unordered iterators:

1. They are bidirectional, while unordered iterators are forward iterators.
2. They provide fast access only to the `key()`. Accessing the value requires a hash table lookup the first time an iterator is dereferenced.
3. They can be constructed from unordered iterators! This means writing something like `typename LRU::Cache<int, int>::OrderedIterator i(unordered_iterator)` will work and is fast.

Dreferencing an iterator will not change the order of elements in the cache.

### Statistics

Our caches can be associated with statistics objects, that monitor hits and misses. There are a few ways to create and use them. First of all, let's say you only wanted to record hits and misses for all keys and didn't care about any particular key. The simplest way to do this is to simply call:

```cpp
cache.monitor();
```

This allows you to call `cache.stats()`, which returns an `LRU::Statistics` object. It's interface is quite clear, allowing you to write stuff like:

```cpp
cache.stats().total_hits(); // Hits for any key
cache.stats().total_misses(); // Misses for any key
cache.stats().hit_rate(); // Hit rate in [0, 1]
```

Note that a hit or miss only refers to lookup (i.e. methods `contains()`, `find()`, `lookup()` and `operator[]`) but not insertion via `emplace()` or `insert()`.

#### Sharing Statistics

Already here, one idea might be that we have two functions, each with their own cache, but we'd like them to share statistics. This is easy to do. Simply create the `Statistics` object as a `std::shared_ptr` and plug it into `cache.monitor()` for as many caches as you like:

```cpp
auto stats = std::make_shared<LRU::Statistics<std::string>>();

cache1.monitor(stats);
cache2.monitor(stats);

// Both affect the same statistics object
cache1.lookup("key");
cache2.lookup("foo");

assert(&cache1.stats() == &cache2.stats()); // Ok

std::shared_ptr<Statistics<std::string>> stats2 = cache1.shared_stats();
```

#### Monitoring specific keys

One of the more interesting features of our statistics API is the ability to monitor hits and misses for a specific set of keys. Say we were writing a web server accepting HTTP requests and wanted to cache resources (I assume that's something people would do). Because our website changes in some way every hour, we'll use a timed cache with a time-to-live of one hour. We're also particularly interested in how many cache hits we get for `index.html`. For this, it's good to know that the empty `monitor()` call we made further up is actually a method accepting variadic arguments to forward to the constructor of an internal statistics object (the empty `monitor()` calls the default constructor). One constructor of `Statistics` takes a number of keys to monitor in particular. So calling `monitor(key1, key2, ...)` will set up monitoring for those keys. We could then something like this:

```cpp
#include <string>
#include <chrono>

#include "lru/lru.hpp"

using namespace std::chrono_literals;

struct MyWebServer {

  // We pass 1h to let the cache know that resources are to be invalidated after one hour.
  MyWebServer() : cache(1h) {
    cache.monitor("index.html");
  }

  std::string get(const std::string& resource_name) {
    std::string resource;
    if (cache.contains(resource_name)) {
      resource = cache.lookup(resource_name);
    } else {
      resource = fetch_expensively(resource_name);
      cache.insert(resource_name, resource);
    }

    return resource;
  }

  LRU::TimedCache<std::string, std::string> cache;
};
```

Later on, we can use methods like `hits_for("index.html")`, `misses_for("index.html")` or `stats_for("index.html")` on `cache.stats()` to find out how many hits or misses we got for our monitored resource. Note that `stats_for(key)` returns a lightweight `struct` holding hit and miss information about a particular key.

### Callbacks

Next to registering statistics, we also allow hook in arbitrary callbacks. The three kinds of callbacks that may be registered are:

1. Hit callbacks, taking a key and value after a cache hit (registered with `hit_callback()`).
2. Miss callbacks, taking only a key, that was not found in a cache (registered with `miss_callback()`).
3. Access callbacks, taking a key and a boolean indicating a hit or a miss (registered with `access_callback()`).

Usage could look something like this:

```cpp
LRU::Cache<int, int> cache;

cache.hit_callback([](const auto& key, const auto& value) {
  std::clog << "Hit for entry ("
            << key << ", " << value << ")"
            << std::endl;
});

cache.miss_callback([](const auto& key) {
  std::clog << "Miss for " << key<< std::endl;
});

// Roll your own statistics
std::size_t miss_count = 0;
cache.miss_callback([&miss_count](auto&) {
  miss_count += 1;
});

cache.access_callback([](const auto& key, bool was_hit) {
  std::clog << "Access for " << key
            << " was a " << (was_hit ? "hit" : "miss")
            << std::endl;
});
```

Note that just like with statistics, these callbacks will only get invoked for lookup and not insertion.

### Wrapping

We provide utility functions `LRU::wrap` and `LRU::timed_wrap` that take a function and return a new function, with a (timed) cache attached to it. Feels like Python. Just faster.

```cpp
#include "lru/lru.hpp"

int my_expensive_function(int, char, double) {
  // ...
}

auto my_cached_expensive_function = LRU::wrap(my_expensive_function);

my_cached_expensive_function(1, 'a', 3.14);
```

Next to the function to wrap, `LRU::wrap` and `LRU::timed_wrap` take any number of arguments to forward to the constructor of the internal cache:

```cpp
// Use a capacity of 100
auto new_function = LRU::wrap(old_function, 1000);

// Use a time-to-live of 100 milliseconds
auto new_function = LRU::timed_wrap(old_function, 100ms);
```

Note that this will *not* cache recursive calls, since we cannot override the actual function symobl. As such we refer to this as "shallow memoization".

### Lowercase Names

Not everyone has the same taste. We get that. For this reason, for every public `CamelCase` type name, we've defined a `lower_case` (C++ standard style) alias. You can make these visible by including `lru/lowercase.hpp` instead of `lru/lru.hpp`:

```cpp
#include <chrono>
#include <iostream>

#include "lru/lowercase.hpp"

void print(lru::tag::basic_cache) {
  std::cout << "basic cache" << '\n';
}

void print(lru::tag::timed_cache) {
  std::cout << "timed cache" << '\n';
}

auto main() -> int {
  using namespace std::chrono_literals;

  lru::cache<int, int> cache;
  lru::timed_cache<int, int> timed_cache(100ms);

  print(cache.tag());
  print(timed_cache.tag());

  lru::cache<int, int>::ordered_const_iterator iterator(cache.begin());

  lru::statistics<int> stats;
}

```

## Documentation

We have 100% public and private documentation coverage with a decent effort behind it. As such we ask you to RTFM to see the full interface we provide (it is a superset of `std::unordered_map`, minus the new node interface). Documentation can be generated with [Doxygen](http://www.stack.nl/~dimitri/doxygen/) by running the `doxygen` command inside the `docs/` folder.

Also do check out all the examples in the `examples/` folder!

## LICENSE

This project is released under the [MIT License](http://goldsborough.mit-license.org). For more information, see the LICENSE file.

## Authors

[Peter Goldsborough](http://goldsborough.me) + [cat](https://goo.gl/IpUmJn) :heart:

Thanks to [@engelmarkus](https://github.com/engelmarkus) for technical and emotional support.
