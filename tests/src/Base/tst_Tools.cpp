#include "gtest/gtest.h"
#include <Base/Tools.h>

TEST(BaseToolsSuite, TestUniqueName1)
{
    EXPECT_EQ(Base::Tools::getUniqueName("Body", {}), "Body");
}

TEST(BaseToolsSuite, TestUniqueName2)
{
    EXPECT_EQ(Base::Tools::getUniqueName("Body", {"Body"}, 1), "Body1");
}

TEST(BaseToolsSuite, TestUniqueName3)
{
    EXPECT_EQ(Base::Tools::getUniqueName("Body", {"Body"}, 3), "Body001");
}

TEST(BaseToolsSuite, TestUniqueName4)
{
    EXPECT_EQ(Base::Tools::getUniqueName("Body", {"Body001"}, 3), "Body002");
}

TEST(BaseToolsSuite, TestUniqueName5)
{
    EXPECT_EQ(Base::Tools::getUniqueName("Body", {"Body", "Body001"}, 3), "Body002");
}

TEST(BaseToolsSuite, TestUniqueName6)
{
    EXPECT_EQ(Base::Tools::getUniqueName("Body001", {"Body", "Body001"}, 3), "Body002");
}

TEST(BaseToolsSuite, TestUniqueName7)
{
    EXPECT_EQ(Base::Tools::getUniqueName("Body001", {"Body"}, 3), "Body002");
}

TEST(BaseToolsSuite, TestUniqueName8)
{
    EXPECT_EQ(Base::Tools::getUniqueName("Body12345", {"Body"}, 3), "Body12346");
}
