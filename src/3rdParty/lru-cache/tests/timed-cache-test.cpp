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
#include <initializer_list>
#include <string>
#include <thread>
#include <utility>

#include "gtest/gtest.h"
#include "lru/lru.hpp"

using namespace LRU;
using namespace std::chrono_literals;

TEST(TimedCacheTest, ContainsRespectsExpiration) {
  TimedCache<int, int> cache(2ms);

  cache.insert(1, 2);
  ASSERT_EQ(cache.size(), 1);
  ASSERT_TRUE(cache.contains(1));
  ASSERT_EQ(cache[1], 2);

  std::this_thread::sleep_for(1ms);

  EXPECT_TRUE(cache.contains(1));

  std::this_thread::sleep_for(1ms);

  EXPECT_FALSE(cache.contains(1));
}

TEST(TimedCacheTest, KnowsWhenAllKeysHaveExpired) {
  TimedCache<int, int> cache(2ms);

  ASSERT_TRUE(cache.all_expired());

  cache = {{1, 2}, {2, 3}};

  ASSERT_EQ(cache.size(), 2);
  ASSERT_FALSE(cache.all_expired());

  std::this_thread::sleep_for(1ms);

  cache.insert(3, 4);

  std::this_thread::sleep_for(1ms);

  EXPECT_FALSE(cache.all_expired());
  EXPECT_FALSE(cache.contains(1));
  EXPECT_FALSE(cache.contains(2));
  EXPECT_TRUE(cache.contains(3));

  std::this_thread::sleep_for(1ms);

  EXPECT_TRUE(cache.all_expired());
  EXPECT_FALSE(cache.contains(1));
  EXPECT_FALSE(cache.contains(2));
  EXPECT_FALSE(cache.contains(3));
}

TEST(TimedCacheTest, CleanExpiredRemovesExpiredElements) {
  TimedCache<int, int> cache(2ms, 128, {{1, 2}, {2, 3}});

  ASSERT_EQ(cache.size(), 2);
  ASSERT_FALSE(cache.all_expired());

  std::this_thread::sleep_for(1ms);

  cache.insert(3, 4);
  ASSERT_EQ(cache.size(), 3);

  std::this_thread::sleep_for(1ms);

  ASSERT_EQ(cache.size(), 3);

  cache.clear_expired();

  EXPECT_EQ(cache.size(), 1);
  EXPECT_FALSE(cache.contains(1));
  EXPECT_FALSE(cache.contains(2));
  EXPECT_TRUE(cache.contains(3));

  std::this_thread::sleep_for(1ms);

  cache.clear_expired();

  EXPECT_FALSE(cache.contains(3));
  EXPECT_EQ(cache.size(), 0);
  EXPECT_TRUE(cache.is_empty());
}

TEST(TimedCacheTest, LookupThrowsWhenKeyExpired) {
  TimedCache<int, int> cache(2ms, 128, {{1, 2}});

  ASSERT_EQ(cache.lookup(1), 2);

  std::this_thread::sleep_for(2ms);

  ASSERT_THROW(cache.lookup(1), LRU::Error::KeyExpired);
}

TEST(TimedCacheTest, HasExpiredReturnsFalseForNonContainedKeys) {
  TimedCache<int, int> cache(2ms);

  EXPECT_FALSE(cache.has_expired(1));
  EXPECT_FALSE(cache.has_expired(2));
}

TEST(TimedCacheTest, HasExpiredReturnsFalseForContainedButNotExpiredKeys) {
  TimedCache<int, int> cache(100ms);

  cache.emplace(1, 1);
  cache.emplace(2, 2);

  EXPECT_FALSE(cache.has_expired(1));
  EXPECT_FALSE(cache.has_expired(2));
}

TEST(TimedCacheTest, HasExpiredReturnsTrueForContainedAndExpiredKeys) {
  TimedCache<int, int> cache(2ms);

  cache.emplace(1, 1);

  std::this_thread::sleep_for(1ms);

  cache.emplace(2, 2);

  EXPECT_FALSE(cache.has_expired(1));

  std::this_thread::sleep_for(1ms);

  EXPECT_TRUE(cache.has_expired(1));
  EXPECT_FALSE(cache.has_expired(2));

  std::this_thread::sleep_for(3ms);

  EXPECT_TRUE(cache.has_expired(1));
  EXPECT_TRUE(cache.has_expired(2));
}

TEST(TimedCacheTest, LookupsMoveElementsToFront) {
  TimedCache<std::string, int> cache(1s);

  cache.capacity(2);
  cache.insert({{"one", 1}, {"two", 2}});

  // The LRU principle mandates that lookups place
  // accessed elements to the front. So when we look at
  // one it should move to the front.

  auto iterator = cache.find("one");
  cache.emplace("three", 3);

  EXPECT_TRUE(cache.contains("one"));
  EXPECT_FALSE(cache.contains("two"));
  EXPECT_TRUE(cache.contains("three"));
  EXPECT_EQ(++iterator, cache.end());
}
