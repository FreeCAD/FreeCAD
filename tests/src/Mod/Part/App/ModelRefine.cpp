// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <BRepAlgoAPI_BuilderAlgo.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <GProp_GProps.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <gp_Pnt.hxx>

#include <Mod/Part/App/modelRefine.h>

namespace
{

double volumeOf(const TopoDS_Shape& shape)
{
    GProp_GProps props;
    BRepGProp::VolumeProperties(shape, props);
    return props.Mass();
}

TopoDS_Shape refine(const TopoDS_Shape& shape)
{
    Part::BRepBuilderAPI_RefineModel mkRefine(shape);
    return mkRefine.Shape();
}

/// A cylinder fused with a box flush to its top, so refine has a coplanar pair to merge.
///
/// Without a merge FaceUniter::process returns before it ever reaches BRepLib_FuseEdges,
/// and nothing below can happen.
TopoDS_Shape cylinderWithCoplanarBox()
{
    const TopoDS_Shape cylinder = BRepPrimAPI_MakeCylinder(5.0, 10.0).Shape();
    const TopoDS_Shape box = BRepPrimAPI_MakeBox(gp_Pnt(-3.0, 3.0, 6.0), 6.0, 5.0, 4.0).Shape();
    return BRepAlgoAPI_Fuse(cylinder, box).Shape();
}

/// <shape> with <point> imprinted onto it, splitting whichever edge it lands on.
TopoDS_Shape imprintVertex(const TopoDS_Shape& shape, const gp_Pnt& point)
{
    TopTools_ListOfShape arguments;
    arguments.Append(shape);
    arguments.Append(BRepBuilderAPI_MakeVertex(point).Shape());

    BRepAlgoAPI_BuilderAlgo builder;
    builder.SetArguments(arguments);
    builder.SetNonDestructive(Standard_True);
    builder.Build();

    for (TopExp_Explorer xp(builder.Shape(), TopAbs_SOLID); xp.More(); xp.Next()) {
        return xp.Current();
    }
    return {};
}

}  // namespace

class ModelRefineTest: public ::testing::Test
{
protected:
    /// The cylinder's seam runs up +X at y = 0, so this point lands on it two thirds up.
    /// The vertex subdivides the seam while belonging to nothing else - 2 edges, 1 face.
    static constexpr double seamX = 5.0;
    static constexpr double seamZ = 6.0;
};

TEST_F(ModelRefineTest, testRefineKeepsASolidValidWhenAVertexSplitsASeam)
{
    // Arrange
    const TopoDS_Shape solid = imprintVertex(cylinderWithCoplanarBox(), gp_Pnt(seamX, 0.0, seamZ));
    ASSERT_FALSE(solid.IsNull());
    ASSERT_TRUE(BRepCheck_Analyzer(solid).IsValid());

    // Act
    const TopoDS_Shape refined = refine(solid);

    // Assert - BRepLib_FuseEdges rejoins the two seam halves into one edge, but the face
    // walks its seam twice and needs a pcurve for each pass
    EXPECT_TRUE(BRepCheck_Analyzer(refined).IsValid());
}

TEST_F(ModelRefineTest, testRefinePreservesVolumeWhenAVertexSplitsASeam)
{
    // Refine only merges faces lying on the same surface, so volume is an invariant.
    const TopoDS_Shape solid = imprintVertex(cylinderWithCoplanarBox(), gp_Pnt(seamX, 0.0, seamZ));
    ASSERT_FALSE(solid.IsNull());

    EXPECT_NEAR(volumeOf(refine(solid)), volumeOf(solid), 1e-6);
}

TEST_F(ModelRefineTest, testRefineLeavesNoEmptyFaceWhenAVertexSplitsASeam)
{
    const TopoDS_Shape solid = imprintVertex(cylinderWithCoplanarBox(), gp_Pnt(seamX, 0.0, seamZ));
    ASSERT_FALSE(solid.IsNull());

    for (TopExp_Explorer xp(refine(solid), TopAbs_FACE); xp.More(); xp.Next()) {
        GProp_GProps props;
        BRepGProp::SurfaceProperties(xp.Current(), props);
        EXPECT_GT(props.Mass(), 0.0);
    }
}

TEST_F(ModelRefineTest, testRefineIsUnharmedByAVertexOffTheSeam)
{
    // The control: same construction, same imprint, only the angle differs. It is what
    // makes the tests above point at the seam rather than at imprinting in general.
    const TopoDS_Shape solid = imprintVertex(cylinderWithCoplanarBox(), gp_Pnt(0.0, seamX, seamZ));
    ASSERT_FALSE(solid.IsNull());

    const TopoDS_Shape refined = refine(solid);

    EXPECT_TRUE(BRepCheck_Analyzer(refined).IsValid());
    EXPECT_NEAR(volumeOf(refined), volumeOf(solid), 1e-6);
}
