#include "gtest/gtest.h"
#include <Base/Tools.h>

TEST(BaseToolsSuite, TestUniqueName1)
{
    EXPECT_STREQ(Base::Tools::getUniqueName("Body", {}).c_str(), "Body");
}

TEST(BaseToolsSuite, TestUniqueName2)
{
    EXPECT_STREQ(Base::Tools::getUniqueName("Body", {"Body"}, 1).c_str(), "Body1");
}

TEST(BaseToolsSuite, TestUniqueName3)
{
    EXPECT_STREQ(Base::Tools::getUniqueName("Body", {"Body"}, 3).c_str(), "Body001");
}

TEST(BaseToolsSuite, TestUniqueName4)
{
    EXPECT_STREQ(Base::Tools::getUniqueName("Body", {"Body001"}, 3).c_str(), "Body002");
}

TEST(BaseToolsSuite, TestUniqueName5)
{
    EXPECT_STREQ(Base::Tools::getUniqueName("Body", {"Body001"}, 3).c_str(), "Body002");
}

TEST(BaseToolsSuite, TestUniqueName6)
{
    EXPECT_STREQ(Base::Tools::getUniqueName("Body001", {"Body", "Body001"}, 3).c_str(), "Body002");
}
