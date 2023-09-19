#include "gtest/gtest.h"
#include <Mod/Points/App/Points.h>

// NOLINTBEGIN(cppcoreguidelines-*,readability-*)
TEST(Points, TestDefault)
{
    Points::PointKernel kernel;
    std::vector<Points::PointKernel::value_type> points;
    kernel.setBasicPoints(points);
    EXPECT_EQ(kernel.size(), 0);
}
// NOLINTEND(cppcoreguidelines-*,readability-*)
