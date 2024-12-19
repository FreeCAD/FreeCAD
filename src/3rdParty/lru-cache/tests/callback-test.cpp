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

#include <array>

#include "gtest/gtest.h"

#include "lru/lru.hpp"

using namespace LRU;

struct CallbackTest : public ::testing::Test {
  Cache<int, int> cache;
};

TEST_F(CallbackTest, HitCallbacksGetCalled) {
  std::array<int, 3> counts = {0, 0, 0};

  cache.hit_callback([&counts](auto& key, auto& value) { counts[key] += 1; });

  cache.emplace(0, 0);
  cache.emplace(1, 1);
  cache.emplace(2, 2);

  ASSERT_TRUE(cache.contains(0));
  EXPECT_EQ(counts[0], 1);
  EXPECT_EQ(counts[1], 0);
  EXPECT_EQ(counts[2], 0);

  cache.find(2);
  EXPECT_EQ(counts[0], 1);
  EXPECT_EQ(counts[1], 0);
  EXPECT_EQ(counts[2], 1);

  cache.lookup(1);
  EXPECT_EQ(counts[0], 1);
  EXPECT_EQ(counts[1], 1);
  EXPECT_EQ(counts[2], 1);

  cache.lookup(0);
  EXPECT_EQ(counts[0], 2);
  EXPECT_EQ(counts[1], 1);
  EXPECT_EQ(counts[2], 1);

  cache.contains(5);
  EXPECT_EQ(counts[0], 2);
  EXPECT_EQ(counts[1], 1);
  EXPECT_EQ(counts[2], 1);
}

TEST_F(CallbackTest, MissCallbacksGetCalled) {
  std::array<int, 3> counts = {0, 0, 0};

  cache.miss_callback([&counts](auto& key) { counts[key] += 1; });

  cache.emplace(0, 0);

  ASSERT_TRUE(cache.contains(0));
  EXPECT_EQ(counts[0], 0);
  EXPECT_EQ(counts[1], 0);
  EXPECT_EQ(counts[2], 0);

  cache.find(2);
  EXPECT_EQ(counts[0], 0);
  EXPECT_EQ(counts[1], 0);
  EXPECT_EQ(counts[2], 1);

  cache.find(1);
  EXPECT_EQ(counts[0], 0);
  EXPECT_EQ(counts[1], 1);
  EXPECT_EQ(counts[2], 1);

  cache.contains(1);
  EXPECT_EQ(counts[0], 0);
  EXPECT_EQ(counts[1], 2);
  EXPECT_EQ(counts[2], 1);
}

TEST_F(CallbackTest, AccessCallbacksGetCalled) {
  std::array<int, 3> counts = {0, 0, 0};

  cache.access_callback(
      [&counts](auto& key, bool found) { counts[key] += found ? 1 : -1; });

  cache.emplace(0, 0);

  ASSERT_TRUE(cache.contains(0));
  EXPECT_EQ(counts[0], 1);
  EXPECT_EQ(counts[1], 0);
  EXPECT_EQ(counts[2], 0);

  cache.find(2);
  EXPECT_EQ(counts[0], 1);
  EXPECT_EQ(counts[1], 0);
  EXPECT_EQ(counts[2], -1);

  cache.find(1);
  EXPECT_EQ(counts[0], 1);
  EXPECT_EQ(counts[1], -1);
  EXPECT_EQ(counts[2], -1);

  cache.contains(1);
  EXPECT_EQ(counts[0], 1);
  EXPECT_EQ(counts[1], -2);
  EXPECT_EQ(counts[2], -1);

  cache.find(0);
  EXPECT_EQ(counts[0], 2);
  EXPECT_EQ(counts[1], -2);
  EXPECT_EQ(counts[2], -1);
}

TEST_F(CallbackTest, CallbacksAreNotCalledAfterBeingCleared) {
  int hit = 0, miss = 0, access = 0;
  cache.hit_callback([&hit](auto&, auto&) { hit += 1; });
  cache.miss_callback([&miss](auto&) { miss += 1; });
  cache.access_callback([&access](auto&, bool) { access += 1; });

  cache.emplace(0, 0);

  cache.contains(0);
  cache.find(1);

  ASSERT_EQ(hit, 1);
  ASSERT_EQ(miss, 1);
  ASSERT_EQ(access, 2);

  cache.clear_all_callbacks();

  cache.contains(0);
  cache.find(1);
  cache.find(2);

  ASSERT_EQ(hit, 1);
  ASSERT_EQ(miss, 1);
  ASSERT_EQ(access, 2);
}
