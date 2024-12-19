#include <gtest/gtest.h>
#include <Base/UniqueNameManager.h>

// NOLINTBEGIN(cppcoreguidelines-*,readability-*)
TEST(UniqueNameManager, TestUniqueName1)
{
    EXPECT_EQ(Base::UniqueNameManager().makeUniqueName("Body"), "Body");
}

TEST(UniqueNameManager, TestUniqueName2)
{
    Base::UniqueNameManager manager;
    manager.addExactName("Body");
    EXPECT_EQ(manager.makeUniqueName("Body", 1), "Body1");
}

TEST(UniqueNameManager, TestUniqueName3)
{
    Base::UniqueNameManager manager;
    manager.addExactName("Body");
    EXPECT_EQ(manager.makeUniqueName("Body", 3), "Body001");
}

TEST(UniqueNameManager, TestUniqueName4)
{
    Base::UniqueNameManager manager;
    manager.addExactName("Body001");
    EXPECT_EQ(manager.makeUniqueName("Body", 3), "Body002");
}

TEST(UniqueNameManager, TestUniqueName5)
{
    Base::UniqueNameManager manager;
    manager.addExactName("Body");
    manager.addExactName("Body001");
    EXPECT_EQ(manager.makeUniqueName("Body", 3), "Body002");
}

TEST(UniqueNameManager, TestUniqueName6)
{
    Base::UniqueNameManager manager;
    manager.addExactName("Body");
    manager.addExactName("Body001");
    EXPECT_EQ(manager.makeUniqueName("Body001", 3), "Body002");
}

TEST(UniqueNameManager, TestUniqueName7)
{
    Base::UniqueNameManager manager;
    manager.addExactName("Body");
    EXPECT_EQ(manager.makeUniqueName("Body001", 3), "Body001");
}

TEST(UniqueNameManager, TestUniqueName8)
{
    Base::UniqueNameManager manager;
    manager.addExactName("Body");
    EXPECT_EQ(manager.makeUniqueName("Body12345", 3), "Body001");
}

TEST(UniqueNameManager, TestUniqueName9)
{
    std::string name;
    Base::UniqueNameManager manager;
    manager.addExactName("Origin007");
    manager.addExactName("Origin");
    manager.addExactName("Origin008");
    manager.addExactName("Origin010");
    manager.addExactName("Origin011");
    manager.addExactName("Origin013");
    manager.addExactName("Origin016");

    name = manager.makeUniqueName("Origin", 3);
    manager.addExactName(name);

    name = manager.makeUniqueName("Origin", 3);
    manager.addExactName(name);

    name = manager.makeUniqueName("Origin", 3);
    EXPECT_NE(name, "Origin010");
}
// NOLINTEND(cppcoreguidelines-*,readability-*)
