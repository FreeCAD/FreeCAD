// SPDX-License-Identifier: LGPL-2.1-or-later

#include <src/App/InitApplication.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/Part.h>
#include <App/GeoFeatureGroupExtension.h>
#include <App/MeasureManager.h>
#include <Mod/Measure/App/MeasureDistance.h>
#include <Mod/Measure/App/MeasureSnap.h>
#include <Mod/Part/App/PartFeature.h>
#include <Base/Placement.h>
#include <Base/Reader.h>
#include <Base/Rotation.h>
#include <Base/Vector3D.h>
#include <gtest/gtest.h>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <gp_Ax2.hxx>
#include <gp_Circ.hxx>
#include <gp_Dir.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Wire.hxx>
#include <algorithm>
#include <cstdio>
#include <sstream>

class MeasureDistance: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        document = App::GetApplication().newDocument("Measure");
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(document->getName());
    }

    App::Document* getDocument() const
    {
        return document;
    }

    TopoDS_Edge makeCircle(const gp_Pnt& pnt) const
    {
        gp_Circ circle;
        circle.SetLocation(pnt);
        circle.SetRadius(1.0);
        BRepBuilderAPI_MakeEdge mkEdge(circle);
        return mkEdge.Edge();
    }

    TopoDS_Edge makeArc(const gp_Pnt& pnt, double u1, double u2) const
    {
        gp_Circ circle;
        circle.SetLocation(pnt);
        circle.SetRadius(1.0);
        BRepBuilderAPI_MakeEdge mkEdge(circle, u1, u2);
        return mkEdge.Edge();
    }

    TopoDS_Edge makeLine(const gp_Pnt& p1, const gp_Pnt& p2) const
    {
        BRepBuilderAPI_MakeEdge mkEdge(p1, p2);
        return mkEdge.Edge();
    }

    TopoDS_Vertex makeVertex(const gp_Pnt& pnt) const
    {
        return BRepBuilderAPI_MakeVertex(pnt).Vertex();
    }

    TopoDS_Wire makeCircularWire(const gp_Pnt& pnt) const
    {
        BRepBuilderAPI_MakeWire mkWire(makeCircle(pnt));
        return mkWire.Wire();
    }

private:
    App::Document* document {};
};

// NOLINTBEGIN
TEST_F(MeasureDistance, testCurvedFaceValidSelection)
{
    // Regression: https://github.com/FreeCAD/FreeCAD/issues/29235
    App::Document* doc = getDocument();

    auto sphere = doc->addObject<Part::Feature>("Sphere");
    sphere->Shape.setValue(BRepPrimAPI_MakeSphere(5.0).Solid());

    auto box = doc->addObject<Part::Feature>("Box");
    box->Shape.setValue(BRepPrimAPI_MakeBox(gp_Pnt(20.0, 0.0, 0.0), 10.0, 10.0, 10.0).Solid());

    doc->recompute();

    App::MeasureSelectionItem item1 {App::SubObjectT {sphere, "Face1"}, Base::Vector3d {}};
    App::MeasureSelectionItem item2 {App::SubObjectT {box, "Face1"}, Base::Vector3d {}};

    EXPECT_TRUE(Measure::MeasureDistance::isValidSelection({item1, item2}));
}

TEST_F(MeasureDistance, testCurvedFaceDistance)
{
    // Regression: https://github.com/FreeCAD/FreeCAD/issues/29235
    App::Document* doc = getDocument();

    auto sphere = doc->addObject<Part::Feature>("Sphere");
    sphere->Shape.setValue(BRepPrimAPI_MakeSphere(5.0).Solid());

    auto box = doc->addObject<Part::Feature>("Box");
    box->Shape.setValue(BRepPrimAPI_MakeBox(gp_Pnt(20.0, 0.0, 0.0), 10.0, 10.0, 10.0).Solid());

    auto md = doc->addObject<Measure::MeasureDistance>("Distance");
    md->Element1.setValue(sphere, {"Face1"});
    md->Element2.setValue(box, {"Face1"});

    doc->recompute();

    EXPECT_NEAR(md->Distance.getValue(), 15.0, 1e-6);
}

TEST_F(MeasureDistance, testCircleCircle)
{
    App::Document* doc = getDocument();
    auto p1 = doc->addObject<Part::Feature>("Shape1");
    p1->Shape.setValue(makeCircle(gp_Pnt(0.0, 0.0, 0.0)));
    auto p2 = doc->addObject<Part::Feature>("Shape2");
    p2->Shape.setValue(makeCircle(gp_Pnt(3.0, 4.0, 0.0)));

    auto md = doc->addObject<Measure::MeasureDistance>("Distance");
    md->Element1.setValue(p1, {"Edge1"});
    md->Element2.setValue(p2, {"Edge1"});

    doc->recompute();

    EXPECT_DOUBLE_EQ(md->Distance.getValue(), 5.0);
    EXPECT_DOUBLE_EQ(md->DistanceX.getValue(), 3.0);
    EXPECT_DOUBLE_EQ(md->DistanceY.getValue(), 4.0);
    EXPECT_DOUBLE_EQ(md->DistanceZ.getValue(), 0.0);
    EXPECT_EQ(md->Position1.getValue(), Base::Vector3d(0.0, 0.0, 0.0));
    EXPECT_EQ(md->Position2.getValue(), Base::Vector3d(3.0, 4.0, 0.0));
}

// A trimmed circular edge is still GeomAbs_Circle, so distanceCircleCircle
// measures center-to-center: centers (0,0,0) and (3,4,0) give 5.0.
TEST_F(MeasureDistance, testArcArcCenterToCenter)
{
    App::Document* doc = getDocument();
    auto p1 = doc->addObject<Part::Feature>("Shape1");
    p1->Shape.setValue(makeArc(gp_Pnt(0.0, 0.0, 0.0), 0.0, 1.0));
    auto p2 = doc->addObject<Part::Feature>("Shape2");
    p2->Shape.setValue(makeArc(gp_Pnt(3.0, 4.0, 0.0), 0.0, 1.0));

    auto md = doc->addObject<Measure::MeasureDistance>("Distance");
    md->Element1.setValue(p1, {"Edge1"});
    md->Element2.setValue(p2, {"Edge1"});

    doc->recompute();

    EXPECT_DOUBLE_EQ(md->Distance.getValue(), 5.0);
    EXPECT_DOUBLE_EQ(md->DistanceX.getValue(), 3.0);
    EXPECT_DOUBLE_EQ(md->DistanceY.getValue(), 4.0);
    EXPECT_DOUBLE_EQ(md->DistanceZ.getValue(), 0.0);
    EXPECT_EQ(md->Position1.getValue(), Base::Vector3d(0.0, 0.0, 0.0));
    EXPECT_EQ(md->Position2.getValue(), Base::Vector3d(3.0, 4.0, 0.0));
}

// Only one shape is a circle, so the center shortcut does not fire and the
// generic extrema path runs. Nearest rim point (1,0,0) to line x=5 gives 4.0,
// not the 5.0 center-to-center distance (#27404).
TEST_F(MeasureDistance, testCircleLineExtremaNotCenter)
{
    App::Document* doc = getDocument();
    auto pCircle = doc->addObject<Part::Feature>("Circle");
    pCircle->Shape.setValue(makeCircle(gp_Pnt(0.0, 0.0, 0.0)));
    auto pLine = doc->addObject<Part::Feature>("Line");
    pLine->Shape.setValue(makeLine(gp_Pnt(5.0, 0.0, 0.0), gp_Pnt(5.0, 5.0, 0.0)));

    auto md = doc->addObject<Measure::MeasureDistance>("Distance");
    md->Element1.setValue(pCircle, {"Edge1"});
    md->Element2.setValue(pLine, {"Edge1"});

    doc->recompute();

    EXPECT_NEAR(md->Distance.getValue(), 4.0, 1e-6);
    EXPECT_NEAR(md->DistanceX.getValue(), 4.0, 1e-6);
    EXPECT_NEAR(md->DistanceY.getValue(), 0.0, 1e-6);
    EXPECT_NEAR(md->DistanceZ.getValue(), 0.0, 1e-6);
}

// Two parallel segments offset by 3 in Z. The closest pair is x-ambiguous, so
// assert the distance components rather than the positions.
TEST_F(MeasureDistance, testLineLineExtrema)
{
    App::Document* doc = getDocument();
    auto p1 = doc->addObject<Part::Feature>("Line1");
    p1->Shape.setValue(makeLine(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(2.0, 0.0, 0.0)));
    auto p2 = doc->addObject<Part::Feature>("Line2");
    p2->Shape.setValue(makeLine(gp_Pnt(0.0, 0.0, 3.0), gp_Pnt(2.0, 0.0, 3.0)));

    auto md = doc->addObject<Measure::MeasureDistance>("Distance");
    md->Element1.setValue(p1, {"Edge1"});
    md->Element2.setValue(p2, {"Edge1"});

    doc->recompute();

    EXPECT_NEAR(md->Distance.getValue(), 3.0, 1e-6);
    EXPECT_NEAR(md->DistanceX.getValue(), 0.0, 1e-6);
    EXPECT_NEAR(md->DistanceY.getValue(), 0.0, 1e-6);
    EXPECT_NEAR(md->DistanceZ.getValue(), 3.0, 1e-6);
}

// Vertex at (10,0,0) against a sphere of radius 5 at the origin: 10 - 5 = 5.0.
TEST_F(MeasureDistance, testVertexFaceExtrema)
{
    App::Document* doc = getDocument();
    auto pVertex = doc->addObject<Part::Feature>("Vertex");
    pVertex->Shape.setValue(makeVertex(gp_Pnt(10.0, 0.0, 0.0)));
    auto pSphere = doc->addObject<Part::Feature>("Sphere");
    pSphere->Shape.setValue(BRepPrimAPI_MakeSphere(5.0).Solid());

    auto md = doc->addObject<Measure::MeasureDistance>("Distance");
    md->Element1.setValue(pVertex, {"Vertex1"});
    md->Element2.setValue(pSphere, {"Face1"});

    doc->recompute();

    EXPECT_NEAR(md->Distance.getValue(), 5.0, 1e-6);
    EXPECT_NEAR(md->DistanceX.getValue(), 5.0, 1e-6);
    EXPECT_NEAR(md->DistanceY.getValue(), 0.0, 1e-6);
    EXPECT_NEAR(md->DistanceZ.getValue(), 0.0, 1e-6);
}

// A single circular-edge wire is not reduced to a circle by BRepAdaptor_CompCurve,
// so the center shortcut is skipped and generic extrema measures rim-to-rim: centers
// 5 apart, radii 1, gives 3.0 with rim points (0.6,0.8,0) and (2.4,3.2,0).
TEST_F(MeasureDistance, testWireCircleExtremaNotCenter)
{
    App::Document* doc = getDocument();
    auto pWire = doc->addObject<Part::Feature>("Wire");
    pWire->Shape.setValue(makeCircularWire(gp_Pnt(0.0, 0.0, 0.0)));
    auto pCircle = doc->addObject<Part::Feature>("Circle");
    pCircle->Shape.setValue(makeCircle(gp_Pnt(3.0, 4.0, 0.0)));

    auto md = doc->addObject<Measure::MeasureDistance>("Distance");
    md->Element1.setValue(pWire, {"Wire1"});
    md->Element2.setValue(pCircle, {"Edge1"});

    doc->recompute();

    EXPECT_NEAR(md->Distance.getValue(), 3.0, 1e-6);
    EXPECT_NEAR(md->DistanceX.getValue(), 1.8, 1e-6);
    EXPECT_NEAR(md->DistanceY.getValue(), 2.4, 1e-6);
    EXPECT_NEAR(md->DistanceZ.getValue(), 0.0, 1e-6);

    const Base::Vector3d position1 = md->Position1.getValue();
    EXPECT_NEAR(position1.x, 0.6, 1e-6);
    EXPECT_NEAR(position1.y, 0.8, 1e-6);
    EXPECT_NEAR(position1.z, 0.0, 1e-6);

    const Base::Vector3d position2 = md->Position2.getValue();
    EXPECT_NEAR(position2.x, 2.4, 1e-6);
    EXPECT_NEAR(position2.y, 3.2, 1e-6);
    EXPECT_NEAR(position2.z, 0.0, 1e-6);
}

// A measurement's results must survive save and reload unchanged.
TEST_F(MeasureDistance, testSaveReloadRoundTrip)
{
    const char* path = "MeasureRoundTrip.FCStd";

    App::Document* doc = App::GetApplication().newDocument("MeasureRoundTrip");
    auto p1 = doc->addObject<Part::Feature>("Shape1");
    p1->Shape.setValue(makeCircle(gp_Pnt(0.0, 0.0, 0.0)));
    auto p2 = doc->addObject<Part::Feature>("Shape2");
    p2->Shape.setValue(makeCircle(gp_Pnt(3.0, 4.0, 0.0)));

    auto md = doc->addObject<Measure::MeasureDistance>("Distance");
    md->Element1.setValue(p1, {"Edge1"});
    md->Element2.setValue(p2, {"Edge1"});

    doc->recompute();

    const double distance = md->Distance.getValue();
    const Base::Vector3d position1 = md->Position1.getValue();
    const Base::Vector3d position2 = md->Position2.getValue();

    doc->saveAs(path);
    App::GetApplication().closeDocument(doc->getName());

    App::Document* reloaded = App::GetApplication().openDocument(path);
    ASSERT_TRUE(reloaded);
    auto reloadedMd = dynamic_cast<Measure::MeasureDistance*>(reloaded->getObject("Distance"));
    ASSERT_TRUE(reloadedMd);

    EXPECT_DOUBLE_EQ(reloadedMd->Distance.getValue(), distance);
    EXPECT_EQ(reloadedMd->Position1.getValue(), position1);
    EXPECT_EQ(reloadedMd->Position2.getValue(), position2);

    App::GetApplication().closeDocument(reloaded->getName());
    std::remove(path);
}

// The committed fixture was authored before any Snap property existed, so it
// proves a pre-feature document still measures 5.0 center-to-center on load.
TEST_F(MeasureDistance, testLoadPreSnapFixture)
{
    const std::string path = App::Application::getHomePath()
        + "tests/TestModels/PreSnapTwoCircles.FCStd";
    App::Document* doc = App::GetApplication().openDocument(path.c_str());
    ASSERT_TRUE(doc);

    auto md = dynamic_cast<Measure::MeasureDistance*>(doc->getObject("Distance"));
    ASSERT_TRUE(md);
    EXPECT_DOUBLE_EQ(md->Distance.getValue(), 5.0);
    EXPECT_EQ(md->Position1.getValue(), Base::Vector3d(0.0, 0.0, 0.0));
    EXPECT_EQ(md->Position2.getValue(), Base::Vector3d(3.0, 4.0, 0.0));

    App::GetApplication().closeDocument(doc->getName());
}

// Both snap properties default to index 0 (Auto), preserving no-snap behavior.
TEST_F(MeasureDistance, testSnapPropertiesDefaultToAuto)
{
    App::Document* doc = getDocument();
    auto md = doc->addObject<Measure::MeasureDistance>("Distance");

    EXPECT_EQ(md->Snap1.getValue(), 0L);
    EXPECT_STREQ(md->Snap1.getValueAsString(), "Auto");
    EXPECT_EQ(md->Snap2.getValue(), 0L);
    EXPECT_STREQ(md->Snap2.getValueAsString(), "Auto");
}

// getInputProps must list the snaps so the viewprovider tracks them (#13708).
TEST_F(MeasureDistance, testGetInputPropsIncludesSnaps)
{
    App::Document* doc = getDocument();
    auto md = doc->addObject<Measure::MeasureDistance>("Distance");

    const std::vector<std::string> props = md->getInputProps();
    EXPECT_NE(std::find(props.begin(), props.end(), "Snap1"), props.end());
    EXPECT_NE(std::find(props.begin(), props.end(), "Snap2"), props.end());
}

// A snap mode set by label or by index reads back through both accessors.
TEST_F(MeasureDistance, testSnapSetValueRoundTrip)
{
    App::Document* doc = getDocument();
    auto md = doc->addObject<Measure::MeasureDistance>("Distance");

    md->Snap1.setValue("Center");
    EXPECT_EQ(md->Snap1.getValue(), static_cast<long>(Measure::MeasureSnapMode::Center));
    EXPECT_STREQ(md->Snap1.getValueAsString(), "Center");

    md->Snap2.setValue(static_cast<long>(Measure::MeasureSnapMode::Midpoint));
    EXPECT_EQ(md->Snap2.getValue(), static_cast<long>(Measure::MeasureSnapMode::Midpoint));
    EXPECT_STREQ(md->Snap2.getValueAsString(), "Midpoint");
}

// Restore() accepts an out-of-range index (setValue() would reject it); it
// must still dispatch as Auto (#27404).
TEST_F(MeasureDistance, testRestoredOutOfRangeSnapIndexDispatchesAsAuto)
{
    App::Document* doc = getDocument();
    auto p1 = doc->addObject<Part::Feature>("Shape1");
    p1->Shape.setValue(makeCircle(gp_Pnt(0.0, 0.0, 0.0)));
    auto p2 = doc->addObject<Part::Feature>("Shape2");
    p2->Shape.setValue(makeCircle(gp_Pnt(3.0, 4.0, 0.0)));

    auto md = doc->addObject<Measure::MeasureDistance>("Distance");
    md->Element1.setValue(p1, {"Edge1"});
    md->Element2.setValue(p2, {"Edge1"});

    std::string str = "<?xml version='1.0' encoding='utf-8'?>\n";
    str += "<Property name='Snap1' type='App::PropertyEnumeration'>\n";
    str += "<Integer value=\"99\"/>\n";
    str += "</Property>\n";
    std::stringstream data(str);
    Base::XMLReader reader("Document.xml", data);
    md->Snap1.Restore(reader);

    doc->recompute();

    EXPECT_EQ(md->Snap1.getValue(), -1L);
    EXPECT_DOUBLE_EQ(md->Distance.getValue(), 5.0);
    EXPECT_EQ(md->Position1.getValue(), Base::Vector3d(0.0, 0.0, 0.0));
    EXPECT_EQ(md->Position2.getValue(), Base::Vector3d(3.0, 4.0, 0.0));
}

// An explicit snap choice survives save and reload, and the reloaded document
// recomputes to the same distance it was saved with (#13708).
TEST_F(MeasureDistance, testSnapValuePersistsAcrossReload)
{
    const char* path = "MeasureSnapPersist.FCStd";

    App::Document* doc = App::GetApplication().newDocument("MeasureSnapPersist");
    auto p1 = doc->addObject<Part::Feature>("Shape1");
    p1->Shape.setValue(makeCircle(gp_Pnt(0.0, 0.0, 0.0)));
    auto p2 = doc->addObject<Part::Feature>("Shape2");
    p2->Shape.setValue(makeCircle(gp_Pnt(3.0, 4.0, 0.0)));

    auto md = doc->addObject<Measure::MeasureDistance>("Distance");
    md->Element1.setValue(p1, {"Edge1"});
    md->Element2.setValue(p2, {"Edge1"});
    md->Snap1.setValue("None");
    md->Snap2.setValue("Center");

    doc->recompute();
    const double distance = md->Distance.getValue();

    doc->saveAs(path);
    App::GetApplication().closeDocument(doc->getName());

    App::Document* reloaded = App::GetApplication().openDocument(path);
    ASSERT_TRUE(reloaded);
    auto reloadedMd = dynamic_cast<Measure::MeasureDistance*>(reloaded->getObject("Distance"));
    ASSERT_TRUE(reloadedMd);

    EXPECT_STREQ(reloadedMd->Snap1.getValueAsString(), "None");
    EXPECT_STREQ(reloadedMd->Snap2.getValueAsString(), "Center");
    EXPECT_DOUBLE_EQ(reloadedMd->Distance.getValue(), distance);

    App::GetApplication().closeDocument(reloaded->getName());
    std::remove(path);
}

// Regression: https://github.com/FreeCAD/FreeCAD/issues/27404
// None on both circles opts out of the center shortcut that Auto would take, so
// generic extrema measures rim-to-rim: centers 5 apart, radii 1, gives 3.0.
TEST_F(MeasureDistance, testNoneNoneCirclesRimToRim)
{
    App::Document* doc = getDocument();
    auto p1 = doc->addObject<Part::Feature>("Shape1");
    p1->Shape.setValue(makeCircle(gp_Pnt(0.0, 0.0, 0.0)));
    auto p2 = doc->addObject<Part::Feature>("Shape2");
    p2->Shape.setValue(makeCircle(gp_Pnt(3.0, 4.0, 0.0)));

    auto md = doc->addObject<Measure::MeasureDistance>("Distance");
    md->Element1.setValue(p1, {"Edge1"});
    md->Element2.setValue(p2, {"Edge1"});
    md->Snap1.setValue("None");
    md->Snap2.setValue("None");

    doc->recompute();

    EXPECT_NEAR(md->Distance.getValue(), 3.0, 1e-6);
    EXPECT_NEAR(md->DistanceX.getValue(), 1.8, 1e-6);
    EXPECT_NEAR(md->DistanceY.getValue(), 2.4, 1e-6);
    EXPECT_NEAR(md->DistanceZ.getValue(), 0.0, 1e-6);
}

// A single non-Auto side is enough to leave the shortcut: the same circles read
// 5.0 center-to-center under Auto/Auto but 3.0 rim-to-rim once either side is
// None, whichever side it is.
TEST_F(MeasureDistance, testMixedAutoNoneBypassesShortcutEitherOrder)
{
    App::Document* doc = getDocument();
    auto p1 = doc->addObject<Part::Feature>("Shape1");
    p1->Shape.setValue(makeCircle(gp_Pnt(0.0, 0.0, 0.0)));
    auto p2 = doc->addObject<Part::Feature>("Shape2");
    p2->Shape.setValue(makeCircle(gp_Pnt(3.0, 4.0, 0.0)));

    auto md = doc->addObject<Measure::MeasureDistance>("Distance");
    md->Element1.setValue(p1, {"Edge1"});
    md->Element2.setValue(p2, {"Edge1"});

    doc->recompute();
    EXPECT_DOUBLE_EQ(md->Distance.getValue(), 5.0);

    md->Snap2.setValue("None");
    doc->recompute();
    EXPECT_NEAR(md->Distance.getValue(), 3.0, 1e-6);

    md->Snap1.setValue("None");
    md->Snap2.setValue("Auto");
    doc->recompute();
    EXPECT_NEAR(md->Distance.getValue(), 3.0, 1e-6);
}

// Center and Midpoint both resolve to points, (0,0,0) and (4,3,0); setValues
// measures between them directly (3-4-5), so no extrema runs.
TEST_F(MeasureDistance, testCenterMidpointPointToPoint)
{
    App::Document* doc = getDocument();
    auto pCircle = doc->addObject<Part::Feature>("Circle");
    pCircle->Shape.setValue(makeCircle(gp_Pnt(0.0, 0.0, 0.0)));
    auto pLine = doc->addObject<Part::Feature>("Line");
    pLine->Shape.setValue(makeLine(gp_Pnt(4.0, 0.0, 0.0), gp_Pnt(4.0, 6.0, 0.0)));

    auto md = doc->addObject<Measure::MeasureDistance>("Distance");
    md->Element1.setValue(pCircle, {"Edge1"});
    md->Element2.setValue(pLine, {"Edge1"});
    md->Snap1.setValue("Center");
    md->Snap2.setValue("Midpoint");

    doc->recompute();

    EXPECT_NEAR(md->Distance.getValue(), 5.0, 1e-6);
    EXPECT_EQ(md->Position1.getValue(), Base::Vector3d(0.0, 0.0, 0.0));
    const Base::Vector3d position2 = md->Position2.getValue();
    EXPECT_NEAR(position2.x, 4.0, 1e-6);
    EXPECT_NEAR(position2.y, 3.0, 1e-6);
    EXPECT_NEAR(position2.z, 0.0, 1e-6);
}

// Center snaps the circle to (0,0,0); the line is Auto, so it stays a raw shape.
// The snapped side is pinned at the centre and only the line side comes from
// extrema: nearest point (10,0,0), distance 10.0.
TEST_F(MeasureDistance, testCenterAutoPinsSnappedPosition)
{
    App::Document* doc = getDocument();
    auto pCircle = doc->addObject<Part::Feature>("Circle");
    pCircle->Shape.setValue(makeCircle(gp_Pnt(0.0, 0.0, 0.0)));
    auto pLine = doc->addObject<Part::Feature>("Line");
    pLine->Shape.setValue(makeLine(gp_Pnt(10.0, 0.0, 0.0), gp_Pnt(10.0, 10.0, 0.0)));

    auto md = doc->addObject<Measure::MeasureDistance>("Distance");
    md->Element1.setValue(pCircle, {"Edge1"});
    md->Element2.setValue(pLine, {"Edge1"});
    md->Snap1.setValue("Center");
    md->Snap2.setValue("Auto");

    doc->recompute();

    EXPECT_NEAR(md->Distance.getValue(), 10.0, 1e-6);
    EXPECT_EQ(md->Position1.getValue(), Base::Vector3d(0.0, 0.0, 0.0));
    const Base::Vector3d position2 = md->Position2.getValue();
    EXPECT_NEAR(position2.x, 10.0, 1e-6);
    EXPECT_NEAR(position2.y, 0.0, 1e-6);
    EXPECT_NEAR(position2.z, 0.0, 1e-6);
}

// Center does not apply to a straight edge, so the snap degrades: it warns and
// falls back to nearest-point extrema between the raw edges, giving 3.0.
TEST_F(MeasureDistance, testCenterOnStraightEdgeDegradesToGeneric)
{
    App::Document* doc = getDocument();
    auto p1 = doc->addObject<Part::Feature>("Edge1");
    p1->Shape.setValue(makeLine(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(0.0, 2.0, 0.0)));
    auto p2 = doc->addObject<Part::Feature>("Edge2");
    p2->Shape.setValue(makeLine(gp_Pnt(3.0, 0.0, 0.0), gp_Pnt(3.0, 2.0, 0.0)));

    auto md = doc->addObject<Measure::MeasureDistance>("Distance");
    md->Element1.setValue(p1, {"Edge1"});
    md->Element2.setValue(p2, {"Edge1"});
    md->Snap1.setValue("Center");
    md->Snap2.setValue("Auto");

    doc->recompute();

    EXPECT_NEAR(md->Distance.getValue(), 3.0, 1e-6);
    EXPECT_NEAR(md->DistanceX.getValue(), 3.0, 1e-6);
    EXPECT_NEAR(md->DistanceY.getValue(), 0.0, 1e-6);
    EXPECT_NEAR(md->DistanceZ.getValue(), 0.0, 1e-6);
}

// Both cylinders snap to their axes: an X-axis through the origin and a Y-axis
// through (3,0,5) are skew, and their closest approach spans 5.0.
TEST_F(MeasureDistance, testAxisAxisSkewClosestApproach)
{
    App::Document* doc = getDocument();
    auto c1 = doc->addObject<Part::Feature>("Cyl1");
    c1->Shape.setValue(
        BRepPrimAPI_MakeCylinder(gp_Ax2(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(1.0, 0.0, 0.0)), 1.0, 4.0).Face()
    );
    auto c2 = doc->addObject<Part::Feature>("Cyl2");
    c2->Shape.setValue(
        BRepPrimAPI_MakeCylinder(gp_Ax2(gp_Pnt(3.0, 0.0, 5.0), gp_Dir(0.0, 1.0, 0.0)), 1.0, 4.0).Face()
    );

    auto md = doc->addObject<Measure::MeasureDistance>("Distance");
    md->Element1.setValue(c1, {"Face1"});
    md->Element2.setValue(c2, {"Face1"});
    md->Snap1.setValue("Axis");
    md->Snap2.setValue("Axis");

    doc->recompute();

    EXPECT_NEAR(md->Distance.getValue(), 5.0, 1e-6);
}

// The cylinder snaps to its Z axis at the origin; the box is None, so it stays a
// raw shape. A bounded stand-in edge for the infinite axis measures to the box's
// near face at x=10: a 10.0 gap along X.
TEST_F(MeasureDistance, testAxisNoneToBox)
{
    App::Document* doc = getDocument();
    auto cyl = doc->addObject<Part::Feature>("Cyl");
    cyl->Shape.setValue(BRepPrimAPI_MakeCylinder(1.0, 4.0).Face());
    auto box = doc->addObject<Part::Feature>("Box");
    box->Shape.setValue(BRepPrimAPI_MakeBox(gp_Pnt(10.0, 0.0, 0.0), 5.0, 5.0, 5.0).Solid());

    auto md = doc->addObject<Measure::MeasureDistance>("Distance");
    md->Element1.setValue(cyl, {"Face1"});
    md->Element2.setValue(box, {"Face1"});
    md->Snap1.setValue("Axis");
    md->Snap2.setValue("None");

    doc->recompute();

    EXPECT_NEAR(md->Distance.getValue(), 10.0, 1e-6);
    EXPECT_NEAR(md->DistanceX.getValue(), 10.0, 1e-6);
    EXPECT_NEAR(md->DistanceY.getValue(), 0.0, 1e-6);
    EXPECT_NEAR(md->DistanceZ.getValue(), 0.0, 1e-6);
}

// Center snaps the circle to (0,0,0); the cylinder snaps to its Z axis through
// (5,0,0). The point projects onto that axis at (5,0,0), giving 5.0, with the
// circle side pinned at the centre.
TEST_F(MeasureDistance, testCenterAxisPointToAxis)
{
    App::Document* doc = getDocument();
    auto pCircle = doc->addObject<Part::Feature>("Circle");
    pCircle->Shape.setValue(makeCircle(gp_Pnt(0.0, 0.0, 0.0)));
    auto cyl = doc->addObject<Part::Feature>("Cyl");
    cyl->Shape.setValue(
        BRepPrimAPI_MakeCylinder(gp_Ax2(gp_Pnt(5.0, 0.0, 0.0), gp_Dir(0.0, 0.0, 1.0)), 1.0, 4.0).Face()
    );

    auto md = doc->addObject<Measure::MeasureDistance>("Distance");
    md->Element1.setValue(pCircle, {"Edge1"});
    md->Element2.setValue(cyl, {"Face1"});
    md->Snap1.setValue("Center");
    md->Snap2.setValue("Axis");

    doc->recompute();

    EXPECT_NEAR(md->Distance.getValue(), 5.0, 1e-6);
    EXPECT_EQ(md->Position1.getValue(), Base::Vector3d(0.0, 0.0, 0.0));
    const Base::Vector3d position2 = md->Position2.getValue();
    EXPECT_NEAR(position2.x, 5.0, 1e-6);
    EXPECT_NEAR(position2.y, 0.0, 1e-6);
    EXPECT_NEAR(position2.z, 0.0, 1e-6);
}

// A snap re-resolves when the linked geometry moves, on a pair whose snapped result
// differs from the Auto shortcut so the snapped path is actually exercised: circle
// Center to line Midpoint reads 4.0, then 8.0 after the circle centre moves from
// (0,3,0) to (-4,3,0). The circle side stays pinned to its centre.
TEST_F(MeasureDistance, testSnapReResolvesWhenGeometryMoves)
{
    App::Document* doc = getDocument();
    auto pCircle = doc->addObject<Part::Feature>("Circle");
    pCircle->Shape.setValue(makeCircle(gp_Pnt(0.0, 3.0, 0.0)));
    auto pLine = doc->addObject<Part::Feature>("Line");
    pLine->Shape.setValue(makeLine(gp_Pnt(4.0, 0.0, 0.0), gp_Pnt(4.0, 6.0, 0.0)));

    auto md = doc->addObject<Measure::MeasureDistance>("Distance");
    md->Element1.setValue(pCircle, {"Edge1"});
    md->Element2.setValue(pLine, {"Edge1"});
    md->Snap1.setValue("Center");
    md->Snap2.setValue("Midpoint");

    doc->recompute();
    EXPECT_NEAR(md->Distance.getValue(), 4.0, 1e-6);
    EXPECT_EQ(md->Position1.getValue(), Base::Vector3d(0.0, 3.0, 0.0));

    pCircle->Shape.setValue(makeCircle(gp_Pnt(-4.0, 3.0, 0.0)));
    doc->recompute();
    EXPECT_NEAR(md->Distance.getValue(), 8.0, 1e-6);
    EXPECT_EQ(md->Position1.getValue(), Base::Vector3d(-4.0, 3.0, 0.0));
}

// Reverse of the point/unresolved arm: the Center-snapped circle is Element2, so
// Position2 must pin to its centre and Position1 is the line foot. Distance 10.
TEST_F(MeasureDistance, testAutoCenterPinsSecondElement)
{
    App::Document* doc = getDocument();
    auto pLine = doc->addObject<Part::Feature>("Line");
    pLine->Shape.setValue(makeLine(gp_Pnt(10.0, 0.0, 0.0), gp_Pnt(10.0, 10.0, 0.0)));
    auto pCircle = doc->addObject<Part::Feature>("Circle");
    pCircle->Shape.setValue(makeCircle(gp_Pnt(0.0, 0.0, 0.0)));

    auto md = doc->addObject<Measure::MeasureDistance>("Distance");
    md->Element1.setValue(pLine, {"Edge1"});
    md->Element2.setValue(pCircle, {"Edge1"});
    md->Snap1.setValue("Auto");
    md->Snap2.setValue("Center");

    doc->recompute();

    EXPECT_NEAR(md->Distance.getValue(), 10.0, 1e-6);
    EXPECT_EQ(md->Position2.getValue(), Base::Vector3d(0.0, 0.0, 0.0));
    const Base::Vector3d position1 = md->Position1.getValue();
    EXPECT_NEAR(position1.x, 10.0, 1e-6);
    EXPECT_NEAR(position1.y, 0.0, 1e-6);
    EXPECT_NEAR(position1.z, 0.0, 1e-6);
}

// Reverse of the point/axis arm: the cylinder axis is Element1 and the Center-snapped
// circle is Element2, so Position2 pins to the circle centre and Position1 is the
// foot on the axis. Distance 5.
TEST_F(MeasureDistance, testAxisCenterPinsSecondElement)
{
    App::Document* doc = getDocument();
    auto cyl = doc->addObject<Part::Feature>("Cyl");
    cyl->Shape.setValue(
        BRepPrimAPI_MakeCylinder(gp_Ax2(gp_Pnt(5.0, 0.0, 0.0), gp_Dir(0.0, 0.0, 1.0)), 1.0, 4.0).Face()
    );
    auto pCircle = doc->addObject<Part::Feature>("Circle");
    pCircle->Shape.setValue(makeCircle(gp_Pnt(0.0, 0.0, 0.0)));

    auto md = doc->addObject<Measure::MeasureDistance>("Distance");
    md->Element1.setValue(cyl, {"Face1"});
    md->Element2.setValue(pCircle, {"Edge1"});
    md->Snap1.setValue("Axis");
    md->Snap2.setValue("Center");

    doc->recompute();

    EXPECT_NEAR(md->Distance.getValue(), 5.0, 1e-6);
    EXPECT_EQ(md->Position2.getValue(), Base::Vector3d(0.0, 0.0, 0.0));
    const Base::Vector3d position1 = md->Position1.getValue();
    EXPECT_NEAR(position1.x, 5.0, 1e-6);
    EXPECT_NEAR(position1.y, 0.0, 1e-6);
    EXPECT_NEAR(position1.z, 0.0, 1e-6);
}

// Reverse of the axis/unresolved arm: the box is Element1 (None) and the cylinder
// axis is Element2, so Position1 is the box point and Position2 sits on the axis.
TEST_F(MeasureDistance, testNoneAxisToBoxReversed)
{
    App::Document* doc = getDocument();
    auto box = doc->addObject<Part::Feature>("Box");
    box->Shape.setValue(BRepPrimAPI_MakeBox(gp_Pnt(10.0, 0.0, 0.0), 5.0, 5.0, 5.0).Solid());
    auto cyl = doc->addObject<Part::Feature>("Cyl");
    cyl->Shape.setValue(BRepPrimAPI_MakeCylinder(1.0, 4.0).Face());

    auto md = doc->addObject<Measure::MeasureDistance>("Distance");
    md->Element1.setValue(box, {"Face1"});
    md->Element2.setValue(cyl, {"Face1"});
    md->Snap1.setValue("None");
    md->Snap2.setValue("Axis");

    doc->recompute();

    EXPECT_NEAR(md->Distance.getValue(), 10.0, 1e-6);
    const Base::Vector3d position1 = md->Position1.getValue();
    const Base::Vector3d position2 = md->Position2.getValue();
    EXPECT_NEAR(position1.x, 10.0, 1e-6);
    EXPECT_NEAR(position2.x, 0.0, 1e-6);
}

TEST_F(MeasureDistance, testCircleCircleWithPlacement)
{
    // Baseline for issue #30365: distance and the reported positions must respect
    // each object's own top-level Placement. Worked *before* #30423.
    App::Document* doc = getDocument();
    auto p1 = doc->addObject<Part::Feature>("Shape1");
    p1->Shape.setValue(makeCircle(gp_Pnt(0.0, 0.0, 0.0)));
    p1->Placement.setValue(Base::Placement(Base::Vector3d(10.0, 0.0, 0.0), Base::Rotation()));
    auto p2 = doc->addObject<Part::Feature>("Shape2");
    p2->Shape.setValue(makeCircle(gp_Pnt(0.0, 0.0, 0.0)));
    p2->Placement.setValue(Base::Placement(Base::Vector3d(10.0, 0.0, 5.0), Base::Rotation()));

    auto md = doc->addObject<Measure::MeasureDistance>("Distance");
    md->Element1.setValue(p1, {"Edge1"});
    md->Element2.setValue(p2, {"Edge1"});

    doc->recompute();

    EXPECT_DOUBLE_EQ(md->Distance.getValue(), 5.0);
    EXPECT_DOUBLE_EQ(md->DistanceX.getValue(), 0.0);
    EXPECT_DOUBLE_EQ(md->DistanceY.getValue(), 0.0);
    EXPECT_DOUBLE_EQ(md->DistanceZ.getValue(), 5.0);
    EXPECT_EQ(md->Position1.getValue(), Base::Vector3d(10.0, 0.0, 0.0));
    EXPECT_EQ(md->Position2.getValue(), Base::Vector3d(10.0, 0.0, 5.0));
}

TEST_F(MeasureDistance, testTwoBoxesMovedByContainers)
{
    // Regression: https://github.com/FreeCAD/FreeCAD/issues/30365
    // Mirrors the issue report: two bodies built at the origin and then moved. Requires #30423 fix.
    App::Document* doc = getDocument();

    auto box1 = doc->addObject<Part::Feature>("Box1");
    box1->Shape.setValue(BRepPrimAPI_MakeBox(10.0, 10.0, 10.0).Solid());
    auto container1 = doc->addObject<App::Part>("Container1");
    container1->getExtensionByType<App::GeoFeatureGroupExtension>()->addObject(box1);

    auto box2 = doc->addObject<Part::Feature>("Box2");
    box2->Shape.setValue(BRepPrimAPI_MakeBox(10.0, 10.0, 10.0).Solid());
    auto container2 = doc->addObject<App::Part>("Container2");
    container2->Placement.setValue(Base::Placement(Base::Vector3d(50.0, 0.0, 0.0), Base::Rotation()));
    container2->getExtensionByType<App::GeoFeatureGroupExtension>()->addObject(box2);

    auto md = doc->addObject<Measure::MeasureDistance>("Distance");
    md->Element1.setValue(container1, {"Box1.Vertex1"});
    md->Element2.setValue(container2, {"Box2.Vertex1"});

    doc->recompute();

    EXPECT_DOUBLE_EQ(md->Distance.getValue(), 50.0);
    EXPECT_DOUBLE_EQ(md->DistanceX.getValue(), 50.0);
    EXPECT_DOUBLE_EQ(md->DistanceY.getValue(), 0.0);
    EXPECT_DOUBLE_EQ(md->DistanceZ.getValue(), 0.0);
}
// NOLINTEND
