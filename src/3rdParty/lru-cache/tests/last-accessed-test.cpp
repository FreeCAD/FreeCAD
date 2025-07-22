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


#include <iterator>
#include <string>
#include <unordered_map>

#include "gtest/gtest.h"
#include "lru/internal/last-accessed.hpp"

using namespace LRU::Internal;

struct LastAccessedTest : public ::testing::Test {
  using Map = std::unordered_map<std::string, int>;
  static Map map;
};

// clang-format off
LastAccessedTest::Map LastAccessedTest::map = {
  {"one", 1},
  {"two", 2},
  {"three", 3}
};
// clang-format on

TEST_F(LastAccessedTest, IsAssignableFromConstAndNonConst) {
  auto front = map.find("one");

  LastAccessed<std::string, int> last_accessed(front->first, front->second);

  ASSERT_EQ(last_accessed.key(), "one");
  ASSERT_EQ(last_accessed.information(), 1);

  last_accessed = map.find("two");

  EXPECT_EQ(last_accessed.key(), "two");
  EXPECT_EQ(last_accessed.information(), 2);

  last_accessed = map.find("three");

  EXPECT_EQ(last_accessed.key(), "three");
  EXPECT_EQ(last_accessed.information(), 3);
}

TEST_F(LastAccessedTest, IsComparableWithConstAndNonConstIterators) {
  auto front = map.find("one");
  LastAccessed<std::string, int> last_accessed(front->first, front->second);

  // non-const
  EXPECT_EQ(last_accessed, front);
  EXPECT_EQ(front, last_accessed);

  EXPECT_NE(map.find("two"), last_accessed);
  EXPECT_NE(last_accessed, map.find("three"));

  // const
  Map::const_iterator const_front = map.find("one");
  EXPECT_EQ(last_accessed, const_front);
  EXPECT_EQ(const_front, last_accessed);

  Map::const_iterator iterator = map.find("two");
  EXPECT_NE(iterator, last_accessed);

  iterator = map.find("three");
  EXPECT_NE(last_accessed, iterator);
}

TEST_F(LastAccessedTest, IsComparableToConstAndNonConstKeys) {
  using namespace std::string_literals;

  std::string key = "forty-two";
  int information = 42;

  LastAccessed<std::string, int> last_accessed(key, information);

  EXPECT_EQ(last_accessed, key);
  EXPECT_EQ(key, last_accessed);

  EXPECT_EQ(last_accessed, "forty-two"s);
  EXPECT_EQ("forty-two"s, last_accessed);

  const std::string& key_const_reference = key;

  EXPECT_EQ(key_const_reference, last_accessed);
  EXPECT_EQ(last_accessed, key_const_reference);

  EXPECT_NE(last_accessed, "asdf"s);
  EXPECT_NE(last_accessed, "foo"s);
  EXPECT_NE(last_accessed, "forty-three"s);
}
