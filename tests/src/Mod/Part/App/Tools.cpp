// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <IMeshTools_Parameters.hxx>
#include <Poly_Triangulation.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>

#include <App/Application.h>
#include <Base/Tools.h>
#include <Mod/Part/App/Tools.h>
#include <src/App/InitApplication.h>

namespace
{

TopoDS_Edge findEdgeWithoutCurve(const TopoDS_Shape& shape)
{
    for (TopExp_Explorer xp(shape, TopAbs_EDGE); xp.More(); xp.Next()) {
        const TopoDS_Edge& edge = TopoDS::Edge(xp.Current());
        double first = 0.0;
        double last = 0.0;
        if (BRep_Tool::Curve(edge, first, last).IsNull()) {
            return edge;
        }
    }
    return {};
}

}  // namespace

class PartToolsTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    // A countersink cut into a board: runs to its apex, and BRepMesh chokes on it.
    static TopoDS_Shape loadCountersinkCone()
    {
        TopoDS_Shape shape;
        BRep_Builder builder;
        const std::string path =
            App::Application::getHomePath() + "/tests/brepfiles/countersink_cone.brep";
        BRepTools::Read(shape, path.c_str(), builder);
        return shape;
    }

    static void meshLikeTheViewProvider(const TopoDS_Shape& shape)
    {
        IMeshTools_Parameters params;
        params.Deflection = Part::Tools::getDeflection(shape, 0.2);
        params.Relative = Standard_False;
        params.Angle = Base::toRadians(28.65);
        params.InParallel = Standard_True;
        params.AllowQualityDecrease = Standard_True;

        BRepTools::Clean(shape, Standard_True);
        BRepMesh_IncrementalMesh(shape, params);
    }
};

TEST_F(PartToolsTest, testAnUnmeshableFaceLeavesItsDegenerateEdgeBare)
{
    // The only route that hands polygonOfEdge a degenerate edge. Asserted, so
    // that an OCCT mesher fix fails here rather than silently gutting the test.
    const TopoDS_Shape face = loadCountersinkCone();
    ASSERT_FALSE(face.IsNull());
    meshLikeTheViewProvider(face);

    TopLoc_Location loc;
    ASSERT_TRUE(BRep_Tool::Triangulation(TopoDS::Face(face), loc).IsNull());

    // The fallback builds a new face, which no original edge has a polygon against.
    const Handle(Poly_Triangulation) mesh = Part::Tools::triangulationOfFace(TopoDS::Face(face));
    const TopoDS_Edge degenerate = findEdgeWithoutCurve(face);
    ASSERT_FALSE(degenerate.IsNull());
    EXPECT_TRUE(BRep_Tool::PolygonOnTriangulation(degenerate, mesh, loc).IsNull());
}

TEST_F(PartToolsTest, testAnEdgeWithoutACurveYieldsNoPolygon)
{
    // A pole is the cheapest degenerate edge, and stays one whatever the mesher does.
    const TopoDS_Shape sphere = BRepPrimAPI_MakeSphere(10.0).Shape();
    const TopoDS_Edge pole = findEdgeWithoutCurve(sphere);
    ASSERT_FALSE(pole.IsNull());

    TopLoc_Location loc;
    const Handle(Poly_Polygon3D) poly = Part::Tools::polygonOfEdge(pole, loc);

    EXPECT_TRUE(poly.IsNull());
}

