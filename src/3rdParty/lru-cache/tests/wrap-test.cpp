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

#include "gtest/gtest.h"

#include "lru/lru.hpp"

TEST(WrapTest, CanWrapMutableAndNonMutableLambdas) {
  // This is purely to make sure both variants compile
  LRU::wrap([](int x) { return x; })(5);
  LRU::wrap([](int x) mutable { return x; })(5);
}

TEST(WrapTest, WrappingWorks) {
  auto f = [x = 0](int _) mutable {
    return ++x;
  };
  auto wrapped = LRU::wrap(f);

  EXPECT_EQ(wrapped(69), 1);
  EXPECT_EQ(wrapped(69), 1);
  EXPECT_EQ(wrapped(42), 2);
  EXPECT_EQ(wrapped(42), 2);
  EXPECT_EQ(wrapped(50), 3);
}

TEST(WrapTest, CanPassCapacityArgumentToWrap) {
  std::size_t call_count = 0;
  auto f = [&call_count](int _) { return call_count += 1; };

  auto wrapped1 = LRU::wrap(f, 1);

  wrapped1(1);
  EXPECT_EQ(call_count, 1);

  wrapped1(1);
  EXPECT_EQ(call_count, 1);

  auto wrapped2 = LRU::wrap(f, 0);

  wrapped1(1);
  EXPECT_EQ(call_count, 1);

  wrapped1(1);
  EXPECT_EQ(call_count, 1);
}


TEST(WrapTest, CanPassTimeArgumentToTimedCacheWrap) {
  using namespace std::chrono_literals;

  std::size_t call_count = 0;
  auto f = [&call_count](int _) { return call_count += 1; };

  auto wrapped1 = LRU::wrap<decltype(f), LRU::TimedCache>(f, 100ms);

  wrapped1(1);
  EXPECT_EQ(call_count, 1);

  wrapped1(1);
  EXPECT_EQ(call_count, 1);

  auto wrapped2 = LRU::timed_wrap(f, 0ms);

  wrapped1(1);
  EXPECT_EQ(call_count, 1);

  wrapped1(1);
  EXPECT_EQ(call_count, 1);
}
