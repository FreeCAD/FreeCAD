// SPDX-License-Identifier: LGPL-2.1-or-later

#include <src/App/InitApplication.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/MeasureManager.h>
#include <Mod/Measure/App/MeasureAngle.h>
#include <Mod/Measure/App/MeasureBase.h>
#include <Mod/Part/App/MeasureClient.h>
#include <Mod/Part/App/MeasureInfo.h>
#include <Mod/Part/App/PartFeature.h>
#include <Base/Placement.h>
#include <Base/Rotation.h>
#include <Base/Vector3D.h>
#include <gtest/gtest.h>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <cmath>

// NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)

class MeasureAngle: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
        // The angle path pulls each element's direction from the Part measure-info
        // handlers; register them here (as module load does) so it resolves headless.
        Part::MeasureClient::initialize();
        for (auto& entry : Part::MeasureClient::reportAngleCB()) {
            Measure::MeasureBaseExtendable<Part::MeasureAngleInfo>::addGeometryHandler(
                entry.m_module,
                entry.m_callback
            );
        }
    }

    void SetUp() override
    {
        document = App::GetApplication().newDocument("MeasureAngleTest");
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(document->getName());
    }

    App::Document* getDocument() const
    {
        return document;
    }

    Part::Feature* addFeature(const char* name, const TopoDS_Shape& shape) const
    {
        auto feature = document->addObject<Part::Feature>(name);
        feature->Shape.setValue(shape);
        return feature;
    }

    TopoDS_Edge makeLine(const gp_Pnt& p1, const gp_Pnt& p2) const
    {
        return BRepBuilderAPI_MakeEdge(p1, p2).Edge();
    }

    // Bounded planar face at origin with the given normal.
    TopoDS_Face makePlaneFace(const gp_Dir& normal) const
    {
        const gp_Pln plane(gp_Pnt(0.0, 0.0, 0.0), normal);
        return BRepBuilderAPI_MakeFace(plane, -1.0, 1.0, -1.0, 1.0).Face();
    }

    // Lateral face of a Z-axis cylinder built at the origin.
    TopoDS_Face makeCylinderFace(double radius, double height) const
    {
        return BRepPrimAPI_MakeCylinder(radius, height).Face();
    }

    TopoDS_Face makeConeFace(double radius1, double radius2, double height) const
    {
        return BRepPrimAPI_MakeCone(radius1, radius2, height).Face();
    }

    TopoDS_Face makeSphereFace() const
    {
        const TopoDS_Shape sphere = BRepPrimAPI_MakeSphere(1.0).Shape();
        for (TopExp_Explorer exp(sphere, TopAbs_FACE); exp.More(); exp.Next()) {
            return TopoDS::Face(exp.Current());
        }
        return TopoDS_Face();
    }

private:
    App::Document* document {};
};

// Existing edge-edge path, unchanged: perpendicular edges read 90.
TEST_F(MeasureAngle, testEdgeEdgePerpendicular)
{
    App::Document* doc = getDocument();
    auto lineX = addFeature("LineX", makeLine(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(1.0, 0.0, 0.0)));
    auto lineY = addFeature("LineY", makeLine(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(0.0, 1.0, 0.0)));

    auto measure = doc->addObject<Measure::MeasureAngle>("Angle");
    measure->Element1.setValue(lineX, {"Edge1"});
    measure->Element2.setValue(lineY, {"Edge1"});
    doc->recompute();

    EXPECT_NEAR(measure->Angle.getValue(), 90.0, 1e-6);
}

// Existing face-face path, unchanged: perpendicular normals read 90.
TEST_F(MeasureAngle, testFaceFacePerpendicular)
{
    App::Document* doc = getDocument();
    auto planeXY = addFeature("PlaneXY", makePlaneFace(gp_Dir(0.0, 0.0, 1.0)));
    auto planeYZ = addFeature("PlaneYZ", makePlaneFace(gp_Dir(1.0, 0.0, 0.0)));

    auto measure = doc->addObject<Measure::MeasureAngle>("Angle");
    measure->Element1.setValue(planeXY, {"Face1"});
    measure->Element2.setValue(planeYZ, {"Face1"});
    doc->recompute();

    EXPECT_NEAR(measure->Angle.getValue(), 90.0, 1e-6);
}

// Existing face-edge (line-to-plane) path, unchanged: an edge 45 out of the plane reads 45.
TEST_F(MeasureAngle, testFaceEdgeLineToPlane)
{
    App::Document* doc = getDocument();
    auto planeXY = addFeature("PlaneXY", makePlaneFace(gp_Dir(0.0, 0.0, 1.0)));
    auto edge = addFeature("Edge", makeLine(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(1.0, 0.0, 1.0)));

    auto measure = doc->addObject<Measure::MeasureAngle>("Angle");
    measure->Element1.setValue(planeXY, {"Face1"});
    measure->Element2.setValue(edge, {"Edge1"});
    doc->recompute();

    EXPECT_NEAR(measure->Angle.getValue(), 45.0, 1e-6);
}

// Cylinder acts as its axis: an edge 30 off the axis reads 30 (line-to-line), not the 60 complement.
TEST_F(MeasureAngle, testCylinderAxisToEdge)
{
    App::Document* doc = getDocument();
    auto cylinder = addFeature("Cylinder", makeCylinderFace(1.0, 4.0));
    const double s = std::sqrt(3.0);
    auto edge = addFeature("Edge", makeLine(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(0.0, 1.0, s)));

    auto measure = doc->addObject<Measure::MeasureAngle>("Angle");
    measure->Element1.setValue(cylinder, {"Face1"});
    measure->Element2.setValue(edge, {"Edge1"});
    doc->recompute();

    EXPECT_NEAR(measure->Angle.getValue(), 30.0, 1e-4);
}

// Parallel axes read 0, not the 180 the face-face path gives.
TEST_F(MeasureAngle, testParallelCylinderAxes)
{
    App::Document* doc = getDocument();
    auto cyl1 = addFeature("Cyl1", makeCylinderFace(1.0, 4.0));
    auto cyl2 = addFeature("Cyl2", makeCylinderFace(1.0, 4.0));
    // Offset the second so the two axes are parallel but distinct.
    cyl2->Placement.setValue(Base::Placement(Base::Vector3d(5.0, 0.0, 0.0), Base::Rotation()));

    auto measure = doc->addObject<Measure::MeasureAngle>("Angle");
    measure->Element1.setValue(cyl1, {"Face1"});
    measure->Element2.setValue(cyl2, {"Face1"});
    doc->recompute();

    EXPECT_NEAR(measure->Angle.getValue(), 0.0, 1e-4);
}

// Cone acts as its axis, like the cylinder.
TEST_F(MeasureAngle, testConeAxisToEdge)
{
    App::Document* doc = getDocument();
    auto cone = addFeature("Cone", makeConeFace(2.0, 1.0, 4.0));
    const double s = std::sqrt(3.0);
    auto edge = addFeature("Edge", makeLine(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(0.0, 1.0, s)));

    auto measure = doc->addObject<Measure::MeasureAngle>("Angle");
    measure->Element1.setValue(cone, {"Face1"});
    measure->Element2.setValue(edge, {"Edge1"});
    doc->recompute();

    EXPECT_NEAR(measure->Angle.getValue(), 30.0, 1e-4);
}

// Axis to a plane whose normal is 45 off the axis is a 45 line-to-plane angle.
TEST_F(MeasureAngle, testCylinderAxisToPlane)
{
    App::Document* doc = getDocument();
    auto cylinder = addFeature("Cylinder", makeCylinderFace(1.0, 4.0));
    auto plane = addFeature("Plane", makePlaneFace(gp_Dir(0.0, 1.0, 1.0)));

    auto measure = doc->addObject<Measure::MeasureAngle>("Angle");
    measure->Element1.setValue(cylinder, {"Face1"});
    measure->Element2.setValue(plane, {"Face1"});
    doc->recompute();

    EXPECT_NEAR(measure->Angle.getValue(), 45.0, 1e-4);
}

// The gate accepts cylinder/cone with an edge, still rejects a sphere.
TEST_F(MeasureAngle, testValidSelectionAcceptsCylinderAndCone)
{
    auto cylinder = addFeature("Cylinder", makeCylinderFace(1.0, 4.0));
    auto cone = addFeature("Cone", makeConeFace(2.0, 1.0, 4.0));
    auto sphere = addFeature("Sphere", makeSphereFace());
    auto edge = addFeature("Edge", makeLine(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(1.0, 0.0, 0.0)));

    const App::MeasureSelection cylEdge {
        {App::SubObjectT(cylinder, "Face1"), Base::Vector3d()},
        {App::SubObjectT(edge, "Edge1"), Base::Vector3d()}
    };
    const App::MeasureSelection coneEdge {
        {App::SubObjectT(cone, "Face1"), Base::Vector3d()},
        {App::SubObjectT(edge, "Edge1"), Base::Vector3d()}
    };
    const App::MeasureSelection sphereEdge {
        {App::SubObjectT(sphere, "Face1"), Base::Vector3d()},
        {App::SubObjectT(edge, "Edge1"), Base::Vector3d()}
    };

    EXPECT_TRUE(Measure::MeasureAngle::isValidSelection(cylEdge));
    EXPECT_TRUE(Measure::MeasureAngle::isValidSelection(coneEdge));
    EXPECT_FALSE(Measure::MeasureAngle::isValidSelection(sphereEdge));
}

// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
