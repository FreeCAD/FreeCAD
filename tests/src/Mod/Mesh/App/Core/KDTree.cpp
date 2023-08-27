#include "gtest/gtest.h"
#include <Mod/Mesh/App/Core/KDTree.h>

// NOLINTBEGIN(cppcoreguidelines-*,readability-*)

class KDTreeTest: public ::testing::Test
{
protected:
    void SetUp() override
    {
        points.emplace_back(0, 0, 0);
        points.emplace_back(0, 0, 1);
        points.emplace_back(0, 1, 0);
        points.emplace_back(0, 1, 1);
        points.emplace_back(1, 0, 0);
        points.emplace_back(1, 0, 1);
        points.emplace_back(1, 1, 0);
        points.emplace_back(1, 1, 1);
    }

    void TearDown() override
    {}

    const std::vector<Base::Vector3f>& GetPoints() const
    {
        return points;
    }

private:
    std::vector<Base::Vector3f> points;
};

TEST_F(KDTreeTest, TestKDTreeEmpty)
{
    MeshCore::MeshKDTree tree;
    EXPECT_EQ(tree.IsEmpty(), true);
}

TEST_F(KDTreeTest, TestKDTreeNearestEmpty)
{
    MeshCore::MeshKDTree tree;

    Base::Vector3f pnt;
    Base::Vector3f nor;
    float dist;
    EXPECT_EQ(tree.FindNearest(pnt, nor, dist), MeshCore::POINT_INDEX_MAX);
}

TEST_F(KDTreeTest, TestKDTreeNearest)
{
    MeshCore::MeshKDTree tree;
    tree.AddPoints(GetPoints());
    EXPECT_EQ(tree.IsEmpty(), false);

    Base::Vector3f nor;
    float dist;
    EXPECT_EQ(tree.FindNearest(Base::Vector3f(0.9F, 0.1F, 0.1F), nor, dist), 4);
}

TEST_F(KDTreeTest, TestKDTreeNearestMaxDist)
{
    MeshCore::MeshKDTree tree;
    tree.AddPoints(GetPoints());
    EXPECT_EQ(tree.IsEmpty(), false);

    Base::Vector3f nor;
    float dist;
    EXPECT_EQ(tree.FindNearest(Base::Vector3f(0.9F, 0.1F, 0.1F), 0.0F, nor, dist),
              MeshCore::POINT_INDEX_MAX);
}

TEST_F(KDTreeTest, TestKDTreeFindExact)
{
    MeshCore::MeshKDTree tree;
    tree.AddPoints(GetPoints());

    EXPECT_EQ(tree.FindExact(Base::Vector3f(0.1F, 0, 0)), MeshCore::POINT_INDEX_MAX);
    EXPECT_EQ(tree.FindExact(Base::Vector3f(0, 0, 0)), 0);
}

TEST_F(KDTreeTest, TestKDTreeFindRange)
{
    MeshCore::MeshKDTree tree;
    tree.AddPoints(GetPoints());

    std::vector<MeshCore::PointIndex> index;
    std::vector<MeshCore::PointIndex> result = {0, 4};
    tree.FindInRange(Base::Vector3f(0.5F, 0, 0), 0.6F, index);
    EXPECT_EQ(index, result);
}
// NOLINTEND(cppcoreguidelines-*,readability-*)
