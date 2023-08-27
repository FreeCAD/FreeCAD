#include "gtest/gtest.h"
#include <Base/TimeInfo.h>

TEST(TimeInfo, TestDefault)
{
    Base::TimeInfo ti;
    EXPECT_EQ(ti.isNull(), false);
}

TEST(TimeInfo, TestNull)
{
    Base::TimeInfo ti(Base::TimeInfo::null());
    EXPECT_EQ(ti.isNull(), true);
}

TEST(TimeInfo, TestCompare)
{
    Base::TimeInfo ti1;
    Base::TimeInfo ti2;
    ti2.setTime_t(ti1.getSeconds() + 1);
    EXPECT_EQ(ti1 == ti1, true);
    EXPECT_EQ(ti1 != ti2, true);
    EXPECT_EQ(ti1 < ti2, true);
    EXPECT_EQ(ti1 > ti2, false);
    EXPECT_EQ(ti1 <= ti1, true);
    EXPECT_EQ(ti1 >= ti1, true);
}

TEST(TimeInfo, TestDiffTime)
{
    Base::TimeInfo ti1;
    Base::TimeInfo ti2;
    ti2.setTime_t(ti1.getSeconds() + 1);
    EXPECT_EQ(Base::TimeInfo::diffTimeF(ti1, ti2), 1.0);
}
