#include <gtest/gtest.h>

#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <HLRAlgo_Projector.hxx>
#include <HLRBRep_Algo.hxx>
#include <HLRBRep_HLRToShape.hxx>
#include <HLRBRep_PolyAlgo.hxx>
#include <HLRBRep_PolyHLRToShape.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <gp_Ax2.hxx>

#include "Mod/TechDraw/App/HLRExtractor.h"

namespace
{
int countEdges(const TopoDS_Shape& shape)
{
    int n = 0;
    for (TopExp_Explorer ex(shape, TopAbs_EDGE); ex.More(); ex.Next()) {
        ++n;
    }
    return n;
}
}  // namespace

class HLRExtractorTest: public ::testing::Test
{
protected:
    void SetUp() override
    {
        _box = BRepPrimAPI_MakeBox(10.0, 20.0, 30.0).Shape();
        _algo = new HLRBRep_Algo();
        _algo->Add(_box);

        // isometric-ish view so both visible and hidden edges exist
        gp_Ax2 viewAxis(gp_Pnt(0, 0, 0), gp_Dir(1, 1, 1));
        _algo->Projector(HLRAlgo_Projector(viewAxis));
        _algo->Update();
        _algo->Hide();
    }

    TopoDS_Shape _box;
    Handle(HLRBRep_Algo) _algo;
};

TEST_F(HLRExtractorTest, visibleHardMatchesOcc)
{
    HLRBRep_HLRToShape occ(_algo);
    const int expected = countEdges(occ.VCompound());

    HLRExtractor extractor(_algo);
    auto edges = extractor.extract(HLREdgeType::Hard, true);

    EXPECT_EQ(static_cast<int>(edges.size()), expected);
}

TEST_F(HLRExtractorTest, hiddenHardMatchesOcc)
{
    HLRBRep_HLRToShape occ(_algo);
    const int expected = countEdges(occ.HCompound());

    HLRExtractor extractor(_algo);
    auto edges = extractor.extract(HLREdgeType::Hard, false);

    EXPECT_EQ(static_cast<int>(edges.size()), expected);
    EXPECT_GT(edges.size(), 0U);
}

TEST_F(HLRExtractorTest, hardEdgesMapBackToSourceEdges)
{
    TopTools_IndexedMapOfShape boxEdges;
    TopExp::MapShapes(_box, TopAbs_EDGE, boxEdges);

    HLRExtractor extractor(_algo);
    for (const auto& hlrEdge : extractor.extract(HLREdgeType::Hard, true)) {
        EXPECT_EQ(hlrEdge.sourceType, "Edge");
        EXPECT_TRUE(boxEdges.Contains(hlrEdge.sourceShape));
    }
}

class PolyHLRExtractorTest: public ::testing::Test
{
protected:
    void SetUp() override
    {
        _box = BRepPrimAPI_MakeBox(10.0, 20.0, 30.0).Shape();

        BRepMesh_IncrementalMesh(_box, 0.10);

        _algo = new HLRBRep_PolyAlgo();
        _algo->Load(_box);

        gp_Ax2 viewAxis(gp_Pnt(0, 0, 0), gp_Dir(1, 1, 1));
        _algo->Projector(HLRAlgo_Projector(viewAxis));
        _algo->Update();
    }

    TopoDS_Shape _box;
    Handle(HLRBRep_PolyAlgo) _algo;
};


TEST_F(PolyHLRExtractorTest, visibleHardMatchesOcc)
{
    HLRBRep_PolyHLRToShape occ;
    occ.Update(_algo);
    const int expected = countEdges(occ.VCompound());

    PolyHLRExtractor extractor(_algo);
    auto edges = extractor.extract(HLREdgeType::Hard, true);

    EXPECT_EQ(static_cast<int>(edges.size()), expected);
}

TEST_F(PolyHLRExtractorTest, hiddenHardMatchesOcc)
{
    HLRBRep_PolyHLRToShape occ;
    occ.Update(_algo);
    const int expected = countEdges(occ.HCompound());

    PolyHLRExtractor extractor(_algo);
    auto edges = extractor.extract(HLREdgeType::Hard, false);

    EXPECT_EQ(static_cast<int>(edges.size()), expected);
    EXPECT_GT(edges.size(), 0U);
}

TEST_F(PolyHLRExtractorTest, hardEdgesMapBackToSourceEdges)
{
    TopTools_IndexedMapOfShape boxEdges;
    TopExp::MapShapes(_box, TopAbs_EDGE, boxEdges);

    PolyHLRExtractor extractor(_algo);
    for (const auto& hlrEdge : extractor.extract(HLREdgeType::Hard, true)) {
        EXPECT_EQ(hlrEdge.sourceType, "Edge");
        EXPECT_TRUE(boxEdges.Contains(hlrEdge.sourceShape));
    }
}
