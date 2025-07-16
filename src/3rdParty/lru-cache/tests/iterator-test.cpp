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
#include <string>
#include <thread>

#include "gtest/gtest.h"
#include "lru/lru.hpp"

using namespace LRU;
using namespace std::chrono_literals;

struct IteratorTest : public ::testing::Test {
  using CacheType = Cache<std::string, int>;
  using UnorderedIterator = typename CacheType::UnorderedIterator;
  using UnorderedConstIterator = typename CacheType::UnorderedConstIterator;
  using OrderedIterator = typename CacheType::OrderedIterator;
  using OrderedConstIterator = typename CacheType::OrderedConstIterator;

  CacheType cache;
};

TEST_F(IteratorTest, UnorderedIteratorsAreCompatibleAsExpected) {
  cache.emplace("one", 1);

  // Move construction
  UnorderedIterator first(cache.unordered_begin());

  // Copy construction
  UnorderedIterator second(first);

  // Copy assignment
  UnorderedIterator third;
  third = second;

  // Move construction from non-const to const
  UnorderedConstIterator first_const(std::move(first));

  // Copy construction from non-const to const
  UnorderedConstIterator second_const(second);

  // Copy assignment
  UnorderedConstIterator third_const;
  third_const = third;
}

TEST_F(IteratorTest, OrderedIteratorsAreCompatibleAsExpected) {
  cache.emplace("one", 1);

  // Move construction
  OrderedIterator first(cache.ordered_begin());

  // Copy construction
  OrderedIterator second(first);

  // Copy assignment
  OrderedIterator third;
  third = second;

  // Move construction from non-const to const
  OrderedConstIterator first_const(std::move(first));

  // Copy construction from non-const to const
  OrderedConstIterator second_const(second);

  // Copy assignment
  OrderedConstIterator third_const;
  third_const = third;
}

TEST_F(IteratorTest, OrderedAndUnorderedAreComparable) {
  cache.emplace("one", 1);

  // Basic assumptions
  ASSERT_EQ(cache.unordered_begin(), cache.unordered_begin());
  ASSERT_EQ(cache.ordered_begin(), cache.ordered_begin());
  ASSERT_EQ(cache.unordered_end(), cache.unordered_end());
  ASSERT_EQ(cache.ordered_end(), cache.ordered_end());

  EXPECT_EQ(cache.unordered_begin(), cache.ordered_begin());

  // We need to ensure symmetry!
  EXPECT_EQ(cache.ordered_begin(), cache.unordered_begin());

  // This is an exceptional property we expect
  EXPECT_EQ(cache.unordered_end(), cache.ordered_end());
  EXPECT_EQ(cache.ordered_end(), cache.unordered_end());

  // These assumptions should hold because there is only one element
  // so the unordered iterator will convert to an ordered iterator, then
  // compare equal because both point to the same single element.
  EXPECT_EQ(cache.ordered_begin(), cache.unordered_begin());
  EXPECT_EQ(cache.unordered_begin(), cache.ordered_begin());

  cache.emplace("two", 1);

  // But then the usual assumptions should hold
  EXPECT_NE(cache.ordered_begin(), cache.find("two"));
  EXPECT_NE(cache.find("two"), cache.ordered_begin());
}

TEST_F(IteratorTest, TestConversionFromUnorderedToOrdered) {
  cache.emplace("one", 1);
  cache.emplace("two", 2);
  cache.emplace("three", 3);

  // Note: find() will always return end() - 1
  UnorderedIterator unordered = cache.find("one");

  ASSERT_EQ(unordered.key(), "one");
  ASSERT_EQ(unordered.value(), 1);

  OrderedIterator ordered(unordered);
  ordered = OrderedIterator(unordered);

  EXPECT_EQ(ordered.key(), "one");
  EXPECT_EQ(ordered.value(), 1);

  // Once it's ordered, the ordering shold be maintained
  --ordered;
  EXPECT_EQ(ordered.key(), "three");
  EXPECT_EQ(ordered.value(), 3);

  UnorderedConstIterator const_unordered = unordered;
  const_unordered = unordered;

  OrderedConstIterator const_ordered(std::move(const_unordered));
  const_ordered = OrderedConstIterator(std::move(const_unordered));

  // Just making sure this compiles
  const_ordered = --ordered;
  const_ordered = OrderedConstIterator(unordered);

  EXPECT_EQ(ordered.key(), "two");
  EXPECT_EQ(ordered.value(), 2);
}

TEST_F(IteratorTest, OrdereredIteratorsAreOrdered) {
  for (std::size_t i = 0; i < 100; ++i) {
    cache.emplace(std::to_string(i), i);
  }

  auto iterator = cache.ordered_begin();
  for (std::size_t i = 0; i < 100; ++i, ++iterator) {
    ASSERT_EQ(iterator.value(), i);
  }
}

TEST_F(IteratorTest, OrderedIteratorsDoNotChangeTheOrderOfElements) {
  cache.capacity(2);
  cache.insert({{"one", 1}});

  auto begin = cache.ordered_begin();

  cache.emplace("two", 2);

  // This here will cause a lookup, but it should not
  // change the order of elements
  ASSERT_EQ(begin->key(), "one");
  ASSERT_EQ((++begin)->key(), "two");
  ASSERT_EQ((--begin)->key(), "one");
  cache.emplace("three", 3);

  EXPECT_FALSE(cache.contains("one"));
  EXPECT_TRUE(cache.contains("two"));
  EXPECT_TRUE(cache.contains("three"));
}

TEST_F(IteratorTest, UnorderedIteratorsDoNotChangeTheOrderOfElements) {
  cache.capacity(2);
  cache.insert({{"one", 1}});

  auto begin = cache.unordered_begin();

  cache.emplace("two", 2);

  ASSERT_EQ(begin->key(), "one");
  cache.emplace("three", 3);

  EXPECT_FALSE(cache.contains("one"));
  EXPECT_TRUE(cache.contains("two"));
  EXPECT_TRUE(cache.contains("three"));

  ASSERT_EQ(cache.back(), "two");
  ASSERT_EQ(cache.front(), "three");
}


TEST_F(IteratorTest, OrderedIteratorsThrowWhenAccessingExpiredElements) {
  TimedCache<int, int> timed_cache(0ms);

  timed_cache.emplace(1, 1);

  auto iterator = timed_cache.ordered_begin();

  EXPECT_THROW(iterator.entry(), LRU::Error::KeyExpired);
}

TEST_F(IteratorTest, UnorderedIteratorsThrowWhenAccessingExpiredElements) {
  TimedCache<int, int> timed_cache(0ms);

  timed_cache.emplace(1, 1);

  auto iterator = timed_cache.unordered_begin();

  EXPECT_THROW(iterator.entry(), LRU::Error::KeyExpired);
}

TEST_F(IteratorTest, IsValidReturnsTrueForValidIterators) {
  cache.emplace("one", 1);
  cache.emplace("two", 1);

  auto ordered_iterator = cache.ordered_begin();
  EXPECT_TRUE(cache.is_valid(ordered_iterator));
  EXPECT_TRUE(cache.is_valid(++ordered_iterator));

  auto unordered_iterator = cache.unordered_begin();
  EXPECT_TRUE(cache.is_valid(unordered_iterator));
  EXPECT_TRUE(cache.is_valid(++unordered_iterator));
}

TEST_F(IteratorTest, IsValidReturnsFalseForInvalidIterators) {
  TimedCache<int, int> timed_cache(0ms);

  EXPECT_FALSE(cache.is_valid(cache.ordered_begin()));
  EXPECT_FALSE(cache.is_valid(cache.ordered_end()));
  EXPECT_FALSE(cache.is_valid(cache.unordered_begin()));
  EXPECT_FALSE(cache.is_valid(cache.unordered_end()));

  timed_cache.emplace(1, 1);

  EXPECT_FALSE(cache.is_valid(cache.ordered_begin()));
  EXPECT_FALSE(cache.is_valid(cache.unordered_begin()));
}

TEST_F(IteratorTest, ThrowIfInvalidThrowsAsExpected) {
  EXPECT_THROW(cache.throw_if_invalid(cache.ordered_begin()),
               LRU::Error::InvalidIterator);
  EXPECT_THROW(cache.throw_if_invalid(cache.ordered_end()),
               LRU::Error::InvalidIterator);
  EXPECT_THROW(cache.throw_if_invalid(cache.unordered_begin()),
               LRU::Error::InvalidIterator);
  EXPECT_THROW(cache.throw_if_invalid(cache.unordered_end()),
               LRU::Error::InvalidIterator);

  TimedCache<int, int> timed_cache(0s, {{1, 1}});

  ASSERT_EQ(timed_cache.size(), 1);

  EXPECT_THROW(timed_cache.throw_if_invalid(timed_cache.ordered_begin()),
               LRU::Error::KeyExpired);
  EXPECT_THROW(timed_cache.throw_if_invalid(timed_cache.unordered_begin()),
               LRU::Error::KeyExpired);
}

TEST_F(IteratorTest, DereferencingNeverThrows) {
  TimedCache<int, int> timed_cache(1ms, {{1, 1}});

  // Test valid iterators.
  EXPECT_EQ(timed_cache.ordered_begin()->key(), 1);
  EXPECT_EQ(timed_cache.unordered_begin()->key(), 1);

  std::this_thread::sleep_for(1ms);

  // Test invalid iterators.
  *timed_cache.ordered_begin();
  *timed_cache.unordered_begin();
  timed_cache.ordered_begin()->key();
  timed_cache.unordered_begin()->key();
  timed_cache.ordered_begin()->value();
  timed_cache.unordered_begin()->value();
}

TEST_F(IteratorTest, CallingAccessThrowsForInvalidIterators) {
  TimedCache<int, int> timed_cache(1ms, {{1, 1}});

  // Test valid iterators.
  ASSERT_EQ(timed_cache.ordered_begin()->key(), 1);
  ASSERT_EQ(timed_cache.unordered_begin()->key(), 1);

  std::this_thread::sleep_for(1ms);

  // Test invalid iterators.
  EXPECT_THROW(timed_cache.ordered_begin().key(), LRU::Error::KeyExpired);
  EXPECT_THROW(timed_cache.unordered_begin().key(), LRU::Error::KeyExpired);
  EXPECT_THROW(timed_cache.ordered_begin().value(), LRU::Error::KeyExpired);
  EXPECT_THROW(timed_cache.unordered_begin().value(), LRU::Error::KeyExpired);
  EXPECT_THROW(timed_cache.ordered_end().key(), LRU::Error::InvalidIterator);
  EXPECT_THROW(timed_cache.unordered_end().key(), LRU::Error::InvalidIterator);
  EXPECT_THROW(timed_cache.ordered_end().value(), LRU::Error::InvalidIterator);
  EXPECT_THROW(timed_cache.unordered_end().value(),
               LRU::Error::InvalidIterator);
}
