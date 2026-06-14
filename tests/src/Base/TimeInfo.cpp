#include <gtest/gtest.h>
#include <Base/TimeInfo.h>

TEST(TimeInfo, TestDefault)
{
    Base::TimeInfo ti;
    EXPECT_FALSE(ti.isNull());
}

TEST(TimeInfo, TestNull)
{
    Base::TimeInfo ti(Base::TimeInfo::null());
    EXPECT_TRUE(ti.isNull());
}

TEST(TimeInfo, TestCompare)
{
    Base::TimeInfo ti1;
    Base::TimeInfo ti2(ti1);
    ti2 += std::chrono::seconds(1);
    EXPECT_TRUE(ti1 == ti1);
    EXPECT_TRUE(ti1 != ti2);
    EXPECT_TRUE(ti1 < ti2);
    EXPECT_FALSE(ti1 > ti2);
    EXPECT_TRUE(ti1 <= ti1);
    EXPECT_TRUE(ti1 >= ti1);
}

TEST(TimeInfo, TestDiffTime)
{
    Base::TimeInfo ti1;
    Base::TimeInfo ti2(ti1);
    ti2 += std::chrono::seconds(1000);
    EXPECT_FLOAT_EQ(Base::TimeInfo::diffTimeF(ti1, ti2), 1000.0);
}
