// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <Inventor/SoDB.h>

#include <Mod/Part/Gui/SoBrepEdgeSet.h>


class SoBrepEdgeSetMappingTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        SoDB::init();
        if (PartGui::SoBrepEdgeSet::getClassTypeId().isBad()) {
            PartGui::SoBrepEdgeSet::initClass();
        }
    }

    void SetUp() override
    {
        edgeSet = new PartGui::SoBrepEdgeSet;
        edgeSet->ref();
    }

    void TearDown() override
    {
        edgeSet->unref();
    }

    PartGui::SoBrepEdgeSet* edgeSet {};
};

TEST_F(SoBrepEdgeSetMappingTest, sparseMapping)
{
    // Edge2 has no rendered polyline. Rendered lines 0, 1 and 2
    // correspond to topological Edge1, Edge3 and Edge4.
    edgeSet->setEdgeMapping({1, 3, 4});

    EXPECT_EQ(edgeSet->edgeIndexFromLine(0), 1);
    EXPECT_EQ(edgeSet->edgeIndexFromLine(1), 3);
    EXPECT_EQ(edgeSet->edgeIndexFromLine(2), 4);

    EXPECT_EQ(edgeSet->lineIndexFromEdge(1), 0);
    EXPECT_EQ(edgeSet->lineIndexFromEdge(3), 1);
    EXPECT_EQ(edgeSet->lineIndexFromEdge(4), 2);
}

TEST_F(SoBrepEdgeSetMappingTest, skippedEdgeHasNoRenderedLine)
{
    edgeSet->setEdgeMapping({1, 3, 4});

    // Regression case: Edge2 must not fall back to line 1,
    // since line 1 belongs to Edge3.
    EXPECT_EQ(edgeSet->lineIndexFromEdge(2), PartGui::SoBrepEdgeSet::InvalidLine);
}

TEST_F(SoBrepEdgeSetMappingTest, emptyMappingHasNoRenderedEdges)
{
    edgeSet->setEdgeMapping({});

    EXPECT_EQ(edgeSet->lineIndexFromEdge(1), PartGui::SoBrepEdgeSet::InvalidLine);
}

TEST_F(SoBrepEdgeSetMappingTest, edgeBeyondMappingHasNoRenderedLine)
{
    edgeSet->setEdgeMapping({1, 3, 4});

    EXPECT_EQ(edgeSet->lineIndexFromEdge(5), PartGui::SoBrepEdgeSet::InvalidLine);
    EXPECT_EQ(edgeSet->lineIndexFromEdge(0), PartGui::SoBrepEdgeSet::InvalidLine);
    EXPECT_EQ(edgeSet->lineIndexFromEdge(-1), PartGui::SoBrepEdgeSet::InvalidLine);
}

TEST_F(SoBrepEdgeSetMappingTest, lineBeyondMappingHasNoEdge)
{
    edgeSet->setEdgeMapping({1, 3, 4});

    EXPECT_EQ(edgeSet->edgeIndexFromLine(3), 0);
    EXPECT_EQ(edgeSet->edgeIndexFromLine(-1), 0);
}

TEST_F(SoBrepEdgeSetMappingTest, withoutMappingFallsBackToOneLinePerEdge)
{
    // Before the geometry is built there is no mapping, so the historical
    // assumption that line N is edge N + 1 is kept.
    EXPECT_EQ(edgeSet->edgeIndexFromLine(0), 1);
    EXPECT_EQ(edgeSet->edgeIndexFromLine(41), 42);
    EXPECT_EQ(edgeSet->lineIndexFromEdge(1), 0);
    EXPECT_EQ(edgeSet->lineIndexFromEdge(42), 41);
}

TEST_F(SoBrepEdgeSetMappingTest, mappingRoundTrips)
{
    const std::vector<int> lineToEdge {2, 5, 6, 9};
    edgeSet->setEdgeMapping(lineToEdge);

    for (int line = 0; line < static_cast<int>(lineToEdge.size()); ++line) {
        const int edge = edgeSet->edgeIndexFromLine(line);
        EXPECT_EQ(edge, lineToEdge[static_cast<size_t>(line)]);
        EXPECT_EQ(edgeSet->lineIndexFromEdge(edge), line);
    }
}

TEST_F(SoBrepEdgeSetMappingTest, remappingReplacesThePreviousMapping)
{
    edgeSet->setEdgeMapping({1, 3, 4});
    ASSERT_EQ(edgeSet->lineIndexFromEdge(3), 1);

    // A recompute can change which edges produce a polyline.
    edgeSet->setEdgeMapping({2, 3});

    EXPECT_EQ(edgeSet->lineIndexFromEdge(2), 0);
    EXPECT_EQ(edgeSet->lineIndexFromEdge(3), 1);
    EXPECT_EQ(edgeSet->lineIndexFromEdge(1), PartGui::SoBrepEdgeSet::InvalidLine);
    EXPECT_EQ(edgeSet->lineIndexFromEdge(4), PartGui::SoBrepEdgeSet::InvalidLine);
}
