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

#include <algorithm>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "gtest/gtest.h"

#include "lru/lru.hpp"

using namespace LRU;

struct CacheTest : public ::testing::Test {
  using CacheType = Cache<std::string, int>;

  template <typename Cache, typename Range>
  bool is_equal_to_range(const Cache& cache, const Range& range) {
    using std::begin;
    return std::equal(cache.ordered_begin(), cache.ordered_end(), begin(range));
  }

  CacheType cache;
};

TEST(CacheConstructionTest, IsConstructibleFromInitializerList) {
  Cache<std::string, int> cache = {
      {"one", 1}, {"two", 2}, {"three", 3},
  };

  EXPECT_FALSE(cache.is_empty());
  EXPECT_EQ(cache.size(), 3);
  EXPECT_EQ(cache["one"], 1);
  EXPECT_EQ(cache["two"], 2);
  EXPECT_EQ(cache["three"], 3);
}

TEST(CacheConstructionTest, IsConstructibleFromInitializerListWithCapacity) {
  // clang-format off
  Cache<std::string, int> cache(2, {
    {"one", 1}, {"two", 2}, {"three", 3},
  });
  // clang-format on

  EXPECT_FALSE(cache.is_empty());
  EXPECT_EQ(cache.size(), 2);
  EXPECT_FALSE(cache.contains("one"));
  EXPECT_EQ(cache["two"], 2);
  EXPECT_EQ(cache["three"], 3);
}

TEST(CacheConstructionTest, IsConstructibleFromRange) {
  const std::vector<std::pair<std::string, int>> range = {
      {"one", 1}, {"two", 2}, {"three", 3}};

  Cache<std::string, int> cache(range);

  EXPECT_FALSE(cache.is_empty());
  EXPECT_EQ(cache.size(), 3);
  EXPECT_EQ(cache["one"], 1);
  EXPECT_EQ(cache["two"], 2);
  EXPECT_EQ(cache["three"], 3);
}

TEST(CacheConstructionTest, IsConstructibleFromIterators) {
  std::vector<std::pair<std::string, int>> range = {
      {"one", 1}, {"two", 2}, {"three", 3}};

  Cache<std::string, int> cache(range.begin(), range.end());

  EXPECT_FALSE(cache.is_empty());
  EXPECT_EQ(cache.size(), 3);
  EXPECT_EQ(cache["one"], 1);
  EXPECT_EQ(cache["two"], 2);
  EXPECT_EQ(cache["three"], 3);
}

TEST(CacheConstructionTest, CapacityIsMaxOfInternalDefaultAndIteratorDistance) {
  std::vector<std::pair<std::string, int>> range = {
      {"one", 1}, {"two", 2}, {"three", 3}};

  Cache<std::string, int> cache(range.begin(), range.end());

  EXPECT_EQ(cache.capacity(), Internal::DEFAULT_CAPACITY);

  for (int i = 0; i < Internal::DEFAULT_CAPACITY; ++i) {
    range.emplace_back(std::to_string(i), i);
  }

  cache = std::move(range);
  EXPECT_EQ(cache.capacity(), range.size());

  Cache<std::string, int> cache2(range.begin(), range.end());
  EXPECT_EQ(cache2.capacity(), range.size());
}

TEST(CacheConstructionTest, UsesCustomHashFunction) {
  using MockHash = std::function<int(int)>;

  std::size_t mock_hash_call_count = 0;
  MockHash mock_hash = [&mock_hash_call_count](int value) {
    mock_hash_call_count += 1;
    return value;
  };

  Cache<int, int, decltype(mock_hash)> cache(128, mock_hash);

  EXPECT_EQ(mock_hash_call_count, 0);

  cache.contains(5);
  EXPECT_EQ(mock_hash_call_count, 1);
}

TEST(CacheConstructionTest, UsesCustomKeyEqual) {
  using MockCompare = std::function<bool(int, int)>;

  std::size_t mock_equal_call_count = 0;
  MockCompare mock_equal = [&mock_equal_call_count](int a, int b) {
    mock_equal_call_count += 1;
    return a == b;
  };

  Cache<int, int, std::hash<int>, decltype(mock_equal)> cache(
      128, std::hash<int>(), mock_equal);

  EXPECT_EQ(mock_equal_call_count, 0);

  cache.insert(5, 1);
  ASSERT_TRUE(cache.contains(5));
  EXPECT_EQ(mock_equal_call_count, 1);
}

TEST_F(CacheTest, ContainsAfterInsertion) {
  ASSERT_TRUE(cache.is_empty());

  for (std::size_t i = 1; i <= 100; ++i) {
    const auto key = std::to_string(i);
    cache.insert(key, i);
    EXPECT_EQ(cache.size(), i);
    EXPECT_TRUE(cache.contains(key));
  }

  EXPECT_FALSE(cache.is_empty());
}

TEST_F(CacheTest, ContainsAfteEmplacement) {
  ASSERT_TRUE(cache.is_empty());

  for (std::size_t i = 1; i <= 100; ++i) {
    const auto key = std::to_string(i);
    cache.emplace(key, i);
    EXPECT_EQ(cache.size(), i);
    EXPECT_TRUE(cache.contains(key));
  }

  EXPECT_FALSE(cache.is_empty());
}

TEST_F(CacheTest, RemovesLRUElementWhenFull) {
  cache.capacity(2);
  ASSERT_EQ(cache.capacity(), 2);

  cache.emplace("one", 1);
  cache.emplace("two", 2);
  ASSERT_EQ(cache.size(), 2);
  ASSERT_TRUE(cache.contains("one"));
  ASSERT_TRUE(cache.contains("two"));


  cache.emplace("three", 3);
  EXPECT_EQ(cache.size(), 2);
  EXPECT_TRUE(cache.contains("two"));
  EXPECT_TRUE(cache.contains("three"));
  EXPECT_FALSE(cache.contains("one"));
}

TEST_F(CacheTest, LookupReturnsTheRightValue) {
  for (std::size_t i = 1; i <= 10; ++i) {
    const auto key = std::to_string(i);
    cache.emplace(key, i);
    ASSERT_EQ(cache.size(), i);
    EXPECT_EQ(cache.lookup(key), i);
    EXPECT_EQ(cache[key], i);
  }
}

TEST_F(CacheTest, LookupOnlyThrowsWhenKeyNotFound) {
  cache.emplace("one", 1);

  ASSERT_EQ(cache.size(), 1);
  EXPECT_EQ(cache.lookup("one"), 1);

  EXPECT_THROW(cache.lookup("two"), LRU::Error::KeyNotFound);
  EXPECT_THROW(cache.lookup("three"), LRU::Error::KeyNotFound);

  cache.emplace("two", 2);
  EXPECT_EQ(cache.lookup("two"), 2);
}

TEST_F(CacheTest, SizeIsUpdatedProperly) {
  ASSERT_EQ(cache.size(), 0);

  for (std::size_t i = 1; i <= 10; ++i) {
    cache.emplace(std::to_string(i), i);
    // Use ASSERT and not EXPECT to terminate the loop early
    ASSERT_EQ(cache.size(), i);
  }

  for (std::size_t i = 10; i >= 1; --i) {
    ASSERT_EQ(cache.size(), i);
    cache.erase(std::to_string(i));
    // Use ASSERT and not EXPECT to terminate the loop early
  }

  EXPECT_EQ(cache.size(), 0);
}

TEST_F(CacheTest, SpaceLeftWorks) {
  cache.capacity(10);
  ASSERT_EQ(cache.size(), 0);

  for (std::size_t i = 10; i >= 1; --i) {
    EXPECT_EQ(cache.space_left(), i);
    cache.emplace(std::to_string(i), i);
  }

  EXPECT_EQ(cache.space_left(), 0);
}

TEST_F(CacheTest, IsEmptyWorks) {
  ASSERT_TRUE(cache.is_empty());
  cache.emplace("one", 1);
  EXPECT_FALSE(cache.is_empty());
  cache.clear();
  EXPECT_TRUE(cache.is_empty());
}

TEST_F(CacheTest, IsFullWorks) {
  ASSERT_FALSE(cache.is_full());
  cache.capacity(0);
  ASSERT_TRUE(cache.is_full());

  cache.capacity(2);
  cache.emplace("one", 1);
  EXPECT_FALSE(cache.is_full());
  cache.emplace("two", 1);
  EXPECT_TRUE(cache.is_full());

  cache.clear();
  EXPECT_FALSE(cache.is_full());
}

TEST_F(CacheTest, CapacityCanBeAdjusted) {
  cache.capacity(10);

  ASSERT_EQ(cache.capacity(), 10);

  for (std::size_t i = 0; i < 10; ++i) {
    cache.emplace(std::to_string(i), i);
  }

  ASSERT_EQ(cache.size(), 10);

  cache.emplace("foo", 0xdeadbeef);
  EXPECT_EQ(cache.size(), 10);

  cache.capacity(11);
  ASSERT_EQ(cache.capacity(), 11);

  cache.emplace("bar", 0xdeadbeef);
  EXPECT_EQ(cache.size(), 11);

  cache.capacity(5);
  EXPECT_EQ(cache.capacity(), 5);
  EXPECT_EQ(cache.size(), 5);

  cache.capacity(0);
  EXPECT_EQ(cache.capacity(), 0);
  EXPECT_EQ(cache.size(), 0);

  cache.capacity(128);
  EXPECT_EQ(cache.capacity(), 128);
  EXPECT_EQ(cache.size(), 0);
}

TEST_F(CacheTest, EraseErasesAndReturnsTrueWhenElementContained) {
  cache.emplace("one", 1);
  ASSERT_TRUE(cache.contains("one"));

  EXPECT_TRUE(cache.erase("one"));
  EXPECT_FALSE(cache.contains("one"));
}

TEST_F(CacheTest, EraseReturnsFalseWhenElementNotContained) {
  ASSERT_FALSE(cache.contains("one"));
  EXPECT_FALSE(cache.erase("one"));
}

TEST_F(CacheTest, ClearRemovesAllElements) {
  ASSERT_TRUE(cache.is_empty());

  cache.emplace("one", 1);
  EXPECT_FALSE(cache.is_empty());

  cache.clear();
  EXPECT_TRUE(cache.is_empty());
}

TEST_F(CacheTest, ShrinkAdjustsSizeWell) {
  cache.emplace("one", 1);
  cache.emplace("two", 2);

  ASSERT_EQ(cache.size(), 2);

  cache.shrink(1);

  EXPECT_EQ(cache.size(), 1);

  cache.emplace("three", 2);
  cache.emplace("four", 3);

  ASSERT_EQ(cache.size(), 3);

  cache.shrink(1);

  EXPECT_EQ(cache.size(), 1);

  cache.shrink(0);

  EXPECT_TRUE(cache.is_empty());
}

TEST_F(CacheTest, ShrinkDoesNothingWhenRequestedSizeIsGreaterThanCurrent) {
  cache.emplace("one", 1);
  cache.emplace("two", 2);

  ASSERT_EQ(cache.size(), 2);

  cache.shrink(50);

  EXPECT_EQ(cache.size(), 2);
}

TEST_F(CacheTest, ShrinkRemovesLRUElements) {
  cache.emplace("one", 1);
  cache.emplace("two", 2);
  cache.emplace("three", 3);

  ASSERT_EQ(cache.size(), 3);

  cache.shrink(2);

  EXPECT_EQ(cache.size(), 2);
  EXPECT_FALSE(cache.contains("one"));
  EXPECT_TRUE(cache.contains("two"));
  EXPECT_TRUE(cache.contains("three"));

  cache.shrink(1);

  EXPECT_EQ(cache.size(), 1);
  EXPECT_FALSE(cache.contains("one"));
  EXPECT_FALSE(cache.contains("two"));
  EXPECT_TRUE(cache.contains("three"));
}

TEST_F(CacheTest, CanInsertIterators) {
  using Range = std::vector<std::pair<std::string, int>>;
  Range range = {{"one", 1}, {"two", 2}, {"three", 3}};

  EXPECT_EQ(cache.insert(range.begin(), range.end()), 3);
  EXPECT_TRUE(is_equal_to_range(cache, range));

  Range range2 = {{"one", 1}, {"four", 4}};

  EXPECT_EQ(cache.insert(range2.begin(), range2.end()), 1);
  // clang-format off
  EXPECT_TRUE(is_equal_to_range(cache, Range({
    {"two", 2}, {"three", 3}, {"one", 1}, {"four", 4}
  })));
  // clang-format on
}

TEST_F(CacheTest, CanInsertRange) {
  std::vector<std::pair<std::string, int>> range = {
      {"one", 1}, {"two", 2}, {"three", 3}};

  cache.insert(range);
  EXPECT_TRUE(is_equal_to_range(cache, range));
}

TEST_F(CacheTest, CanInsertList) {
  std::initializer_list<std::pair<std::string, int>> list = {
      {"one", 1}, {"two", 2}, {"three", 3}};

  // Do it like this, just to verify that template deduction fails if only
  // the range function exists and no explicit overload for the initializer list
  cache.insert({{"one", 1}, {"two", 2}, {"three", 3}});
  EXPECT_TRUE(is_equal_to_range(cache, list));
}

TEST_F(CacheTest, ResultIsCorrectForInsert) {
  auto result = cache.insert("one", 1);

  EXPECT_TRUE(result.was_inserted());
  EXPECT_TRUE(result);

  EXPECT_EQ(result.iterator(), cache.begin());

  result = cache.insert("one", 1);

  EXPECT_FALSE(result.was_inserted());
  EXPECT_FALSE(result);

  EXPECT_EQ(result.iterator(), cache.begin());
}

TEST_F(CacheTest, ResultIsCorrectForEmplace) {
  auto result = cache.emplace("one", 1);

  EXPECT_TRUE(result.was_inserted());
  EXPECT_TRUE(result);

  EXPECT_EQ(result.iterator(), cache.begin());

  result = cache.emplace("one", 1);

  EXPECT_FALSE(result.was_inserted());
  EXPECT_FALSE(result);

  EXPECT_EQ(result.iterator(), cache.begin());
}

TEST_F(CacheTest, CapacityIsSameAfterCopy) {
  cache.capacity(100);
  auto cache2 = cache;

  EXPECT_EQ(cache.capacity(), cache2.capacity());
}

TEST_F(CacheTest, CapacityIsSameAfterMove) {
  cache.capacity(100);
  auto cache2 = std::move(cache);

  EXPECT_EQ(cache2.capacity(), 100);
}

TEST_F(CacheTest, ComparisonOperatorWorks) {
  ASSERT_EQ(cache, cache);

  auto cache2 = cache;
  EXPECT_EQ(cache, cache2);

  cache.emplace("one", 1);
  cache2.emplace("one", 1);
  EXPECT_EQ(cache, cache2);

  cache.emplace("two", 2);
  cache2.emplace("two", 2);
  EXPECT_EQ(cache, cache2);

  cache.erase("two");
  EXPECT_NE(cache, cache2);
}

TEST_F(CacheTest, SwapWorks) {
  auto cache2 = cache;

  cache.emplace("one", 1);
  cache2.emplace("two", 2);

  ASSERT_TRUE(cache.contains("one"));
  ASSERT_TRUE(cache2.contains("two"));

  cache.swap(cache2);

  EXPECT_FALSE(cache.contains("one"));
  EXPECT_TRUE(cache.contains("two"));
  EXPECT_FALSE(cache2.contains("two"));
  EXPECT_TRUE(cache2.contains("one"));
}

TEST_F(CacheTest, SizeStaysZeroWhenCapacityZero) {
  cache.capacity(0);

  ASSERT_EQ(cache.capacity(), 0);
  ASSERT_EQ(cache.size(), 0);

  auto result = cache.insert("one", 1);

  EXPECT_EQ(cache.capacity(), 0);
  EXPECT_EQ(cache.size(), 0);
  EXPECT_FALSE(result.was_inserted());
  EXPECT_EQ(result.iterator(), cache.end());

  result = cache.emplace("two", 2);

  EXPECT_EQ(cache.capacity(), 0);
  EXPECT_EQ(cache.size(), 0);
  EXPECT_FALSE(result.was_inserted());
  EXPECT_EQ(result.iterator(), cache.end());
}

TEST_F(CacheTest, LookupsMoveElementsToFront) {
  cache.capacity(2);
  cache.insert({{"one", 1}, {"two", 2}});

  // The LRU principle mandates that lookups place
  // accessed elements to the front.

  typename CacheType::OrderedIterator iterator(cache.find("one"));
  cache.emplace("three", 3);

  EXPECT_TRUE(cache.contains("one"));
  EXPECT_FALSE(cache.contains("two"));
  EXPECT_TRUE(cache.contains("three"));
  EXPECT_EQ(std::prev(cache.ordered_end()).key(), "three");
  EXPECT_EQ(cache.front(), "three");
  EXPECT_EQ(cache.back(), "one");

  ASSERT_EQ(cache.lookup("one"), 1);
  EXPECT_EQ(std::prev(cache.ordered_end()).key(), "one");
  EXPECT_EQ(cache.ordered_begin().key(), "three");
  EXPECT_EQ(cache.front(), "one");
  EXPECT_EQ(cache.back(), "three");
}
