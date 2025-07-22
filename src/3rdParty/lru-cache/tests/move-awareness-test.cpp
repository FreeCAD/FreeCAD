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

#include <string>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "lru/lru.hpp"
#include "tests/move-aware-dummies.hpp"

struct MoveAwarenessTest : public ::testing::Test {
  MoveAwarenessTest() {
    MoveAwareKey::reset();
    MoveAwareValue::reset();
  }

  LRU::Cache<MoveAwareKey, MoveAwareValue> cache;
};

TEST_F(MoveAwarenessTest, DoesNotMoveForInsert) {
  cache.insert("x", "y");

  // One construction (right there)
  ASSERT_EQ(MoveAwareKey::forwarding_count, 1);
  ASSERT_EQ(MoveAwareValue::forwarding_count, 1);

  ASSERT_EQ(MoveAwareKey::copy_count, 1);

  // Values only go into the map
  ASSERT_EQ(MoveAwareValue::copy_count, 1);

  // Do this at the end to avoid incrementing the counts
  ASSERT_EQ(cache["x"], "y");
}

TEST_F(MoveAwarenessTest, ForwardsValuesWell) {
  cache.emplace("x", "y");

  // One construction to make the key first
  EXPECT_GE(MoveAwareKey::forwarding_count, 1);
  EXPECT_GE(MoveAwareValue::forwarding_count, 1);

  EXPECT_EQ(MoveAwareKey::copy_count, 0);
  EXPECT_EQ(MoveAwareValue::copy_count, 0);

  ASSERT_EQ(cache["x"], "y");
}

TEST_F(MoveAwarenessTest, MovesSingleRValues) {
  cache.emplace(std::string("x"), std::string("y"));

  // Move constructions from the string
  EXPECT_EQ(MoveAwareKey::move_count, 1);
  EXPECT_EQ(MoveAwareValue::move_count, 1);

  EXPECT_EQ(MoveAwareKey::non_move_count, 0);
  EXPECT_EQ(MoveAwareValue::non_move_count, 0);

  EXPECT_EQ(MoveAwareKey::copy_count, 0);
  EXPECT_EQ(MoveAwareValue::copy_count, 0);

  ASSERT_EQ(cache["x"], "y");
}

TEST_F(MoveAwarenessTest, CopiesSingleLValues) {
  std::string x("x");
  std::string y("y");
  cache.emplace(x, y);

  // Move constructions from the string
  EXPECT_EQ(MoveAwareKey::non_move_count, 1);
  EXPECT_EQ(MoveAwareValue::non_move_count, 1);

  EXPECT_EQ(MoveAwareKey::move_count, 0);
  EXPECT_EQ(MoveAwareValue::move_count, 0);

  EXPECT_EQ(MoveAwareKey::copy_count, 0);
  EXPECT_EQ(MoveAwareValue::copy_count, 0);

  ASSERT_EQ(cache["x"], "y");
}

TEST_F(MoveAwarenessTest, MovesRValueTuples) {
  cache.emplace(std::piecewise_construct,
                std::forward_as_tuple(1, 3.14),
                std::forward_as_tuple(2, 2.718));

  // construct_from_tuple performs one move construction
  // (i.e. construction from rvalues)
  EXPECT_EQ(MoveAwareKey::move_count, 1);
  EXPECT_EQ(MoveAwareValue::move_count, 1);

  EXPECT_EQ(MoveAwareKey::non_move_count, 0);
  EXPECT_EQ(MoveAwareValue::non_move_count, 0);

  EXPECT_EQ(MoveAwareKey::copy_count, 0);
  EXPECT_EQ(MoveAwareValue::copy_count, 0);
}

TEST_F(MoveAwarenessTest, MovesLValueTuples) {
  int x = 1, z = 2;
  double y = 3.14, w = 2.718;

  cache.emplace(std::piecewise_construct,
                std::forward_as_tuple(x, y),
                std::forward_as_tuple(z, w));

  // construct_from_tuple will perfom one copy construction
  // (i.e. construction from lvalues)
  EXPECT_EQ(MoveAwareKey::non_move_count, 1);
  EXPECT_EQ(MoveAwareValue::non_move_count, 1);

  EXPECT_EQ(MoveAwareKey::move_count, 0);
  EXPECT_EQ(MoveAwareValue::move_count, 0);

  EXPECT_EQ(MoveAwareKey::copy_count, 0);
  EXPECT_EQ(MoveAwareValue::copy_count, 0);
}

TEST_F(MoveAwarenessTest, MovesElementsOutOfRValueRanges) {
  std::vector<std::pair<std::string, std::string>> range = {{"x", "y"}};
  cache.insert(std::move(range));

  // Move constructions from the string
  EXPECT_EQ(MoveAwareKey::move_count, 1);
  EXPECT_EQ(MoveAwareValue::move_count, 1);

  EXPECT_EQ(MoveAwareKey::non_move_count, 0);
  EXPECT_EQ(MoveAwareValue::non_move_count, 0);

  EXPECT_EQ(MoveAwareKey::copy_count, 0);
  EXPECT_EQ(MoveAwareValue::copy_count, 0);

  ASSERT_EQ(cache["x"], "y");
}
