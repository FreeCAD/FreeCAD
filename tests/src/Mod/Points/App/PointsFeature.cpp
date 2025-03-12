#include "gtest/gtest.h"
#include <src/App/InitApplication.h>
#include <Mod/Points/App/PointsFeature.h>

class PointsFeatureTest: public ::testing::Test
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
TEST_F(PointsFeatureTest, getElementTypes)
{
    Points::Feature pf;
    std::vector<const char*> types = pf.getElementTypes();

    EXPECT_EQ(types.size(), 0);
}

TEST_F(PointsFeatureTest, getComplexElementTypes)
{
    Points::PointKernel pk;
    std::vector<const char*> types = pk.getElementTypes();

    EXPECT_EQ(types.size(), 0);
}
// NOLINTEND(cppcoreguidelines-*,readability-*)
