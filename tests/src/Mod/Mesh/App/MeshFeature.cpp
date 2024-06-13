#include "gtest/gtest.h"
#include <src/App/InitApplication.h>
#include <Mod/Mesh/App/MeshFeature.h>

class MeshFeatureTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {}

    void TearDown() override
    {}
};

// NOLINTBEGIN(cppcoreguidelines-*,readability-*)
TEST_F(MeshFeatureTest, getElementTypes)
{
    Mesh::Feature mf;
    std::vector<const char*> types = mf.getElementTypes();

    EXPECT_EQ(types.size(), 2);
    EXPECT_STREQ(types[0], "Mesh");
    EXPECT_STREQ(types[1], "Segment");
}

TEST_F(MeshFeatureTest, getComplexElementTypes)
{
    Mesh::MeshObject mf;
    std::vector<const char*> types = mf.getElementTypes();

    EXPECT_EQ(types.size(), 2);
    EXPECT_STREQ(types[0], "Mesh");
    EXPECT_STREQ(types[1], "Segment");
}
// NOLINTEND(cppcoreguidelines-*,readability-*)
