// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include "src/App/InitApplication.h"
#include "Mod/Part/App/FaceMakerBullseye.h"
#include "Mod/Part/App/WireJoiner.h"

#include "PartTestHelpers.h"

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepGProp.hxx>
#include <GC_MakeCircle.hxx>
#include <GProp_GProps.hxx>
#include <gp_Pln.hxx>
#include <numbers>

// NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)

using namespace Part;
using namespace PartTestHelpers;

// Test accessor that exposes the protected FaceDriller and getWireDirection
class FaceMakerBullseyeAccessor: public FaceMakerBullseye
{
public:
    using FaceDriller = FaceMakerBullseye::FaceDriller;

    static int getWireDirection(const gp_Pln& plane, const TopoDS_Wire& wire)
    {
        return FaceDriller::getWireDirection(plane, wire);
    }
};

using TestDriller = FaceMakerBullseyeAccessor::FaceDriller;

class FaceMakerBullseyeTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        _docName = App::GetApplication().getUniqueDocumentName("test");
        App::GetApplication().newDocument(_docName.c_str(), "testUser");
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(_docName.c_str());
    }

    // Build a CCW rectangular wire on the XY plane
    static TopoDS_Wire makeRectWire(double x0, double y0, double x1, double y1, double z = 0.0)
    {
        gp_Pnt p0(x0, y0, z);
        gp_Pnt p1(x1, y0, z);
        gp_Pnt p2(x1, y1, z);
        gp_Pnt p3(x0, y1, z);
        BRepBuilderAPI_MakeWire mw;
        mw.Add(BRepBuilderAPI_MakeEdge(p0, p1).Edge());
        mw.Add(BRepBuilderAPI_MakeEdge(p1, p2).Edge());
        mw.Add(BRepBuilderAPI_MakeEdge(p2, p3).Edge());
        mw.Add(BRepBuilderAPI_MakeEdge(p3, p0).Edge());
        return mw.Wire();
    }

    // Build a CCW circular wire centered at (cx,cy,cz) on a given plane axis
    static TopoDS_Wire makeCircleWire(
        double cx,
        double cy,
        double cz,
        double radius,
        const gp_Dir& axis = gp::DZ()
    )
    {
        gp_Ax2 ax(gp_Pnt(cx, cy, cz), axis);
        Handle(Geom_Circle) circ = GC_MakeCircle(ax, radius).Value();
        BRepBuilderAPI_MakeWire mw;
        mw.Add(BRepBuilderAPI_MakeEdge(circ).Edge());
        return mw.Wire();
    }

    static double faceArea(const TopoDS_Shape& shape)
    {
        GProp_GProps props;
        BRepGProp::SurfaceProperties(shape, props);
        return props.Mass();
    }

private:
    std::string _docName;
};


TEST_F(FaceMakerBullseyeTest, getWireDirectionCCWRectangle)
{
    gp_Pln plane;  // Default-constructs to XY plane
    TopoDS_Wire wire = makeRectWire(0, 0, 3, 2);
    int dir = FaceMakerBullseyeAccessor::getWireDirection(plane, wire);
    EXPECT_EQ(dir, 1);
}

TEST_F(FaceMakerBullseyeTest, getWireDirectionCWRectangle)
{
    gp_Pln plane;
    TopoDS_Wire wire = makeRectWire(0, 0, 3, 2);
    wire.Reverse();
    int dir = FaceMakerBullseyeAccessor::getWireDirection(plane, wire);
    EXPECT_EQ(dir, -1);
}

TEST_F(FaceMakerBullseyeTest, getWireDirectionCCWCircle)
{
    gp_Pln plane;
    TopoDS_Wire wire = makeCircleWire(0, 0, 0, 5.0);
    int dir = FaceMakerBullseyeAccessor::getWireDirection(plane, wire);
    EXPECT_EQ(dir, 1);
}

TEST_F(FaceMakerBullseyeTest, getWireDirectionCWCircle)
{
    gp_Pln plane;
    TopoDS_Wire wire = makeCircleWire(0, 0, 0, 5.0);
    wire.Reverse();
    int dir = FaceMakerBullseyeAccessor::getWireDirection(plane, wire);
    EXPECT_EQ(dir, -1);
}

TEST_F(FaceMakerBullseyeTest, getWireDirectionFlippedPlane)
{
    // Plane with Z pointing downward: a CCW wire on the standard XY plane
    // should appear CW from the perspective of this flipped plane.
    gp_Pln plane(gp_Pnt(0, 0, 0), gp_Dir(0, 0, -1));
    TopoDS_Wire wire = makeRectWire(0, 0, 3, 2);
    int dir = FaceMakerBullseyeAccessor::getWireDirection(plane, wire);
    EXPECT_EQ(dir, -1);
}

TEST_F(FaceMakerBullseyeTest, getWireDirectionTiltedPlane)
{
    // Plane tilted 45 degrees about the X axis
    gp_Dir normal(0, -1, 1);
    gp_Pln plane(gp_Pnt(0, 0, 0), normal);

    // Build a wire in that plane's coordinate system
    gp_Ax2 ax(gp_Pnt(0, 0, 0), normal);
    gp_Dir xDir = ax.XDirection();
    gp_Dir yDir = ax.YDirection();

    gp_Pnt p0(0, 0, 0);
    gp_Pnt p1 = p0.XYZ() + 3.0 * xDir.XYZ();
    gp_Pnt p2 = p1.XYZ() + 2.0 * yDir.XYZ();
    gp_Pnt p3 = p0.XYZ() + 2.0 * yDir.XYZ();

    BRepBuilderAPI_MakeWire mw;
    mw.Add(BRepBuilderAPI_MakeEdge(p0, p1).Edge());
    mw.Add(BRepBuilderAPI_MakeEdge(p1, p2).Edge());
    mw.Add(BRepBuilderAPI_MakeEdge(p2, p3).Edge());
    mw.Add(BRepBuilderAPI_MakeEdge(p3, p0).Edge());
    TopoDS_Wire wire = mw.Wire();

    int dir = FaceMakerBullseyeAccessor::getWireDirection(plane, wire);
    EXPECT_EQ(dir, 1);

    wire.Reverse();
    int dirRev = FaceMakerBullseyeAccessor::getWireDirection(plane, wire);
    EXPECT_EQ(dirRev, -1);
}


TEST_F(FaceMakerBullseyeTest, faceDrillerSimpleRectangle)
{
    gp_Pln plane;
    TopoDS_Wire outer = makeRectWire(0, 0, 4, 3);
    TestDriller driller(plane, outer);

    const TopoDS_Face& face = driller.Face();
    EXPECT_FALSE(face.IsNull());
    EXPECT_NEAR(faceArea(face), 12.0, 1e-6);
}

TEST_F(FaceMakerBullseyeTest, faceDrillerWithOneHole)
{
    gp_Pln plane;
    TopoDS_Wire outer = makeRectWire(0, 0, 4, 3);
    TestDriller driller(plane, outer);

    TopoDS_Wire hole = makeCircleWire(2, 1.5, 0, 0.5);
    driller.addHole(hole);

    double expected = 12.0 - (std::numbers::pi * 0.5 * 0.5);
    EXPECT_NEAR(faceArea(driller.Face()), expected, 1e-6);
}

TEST_F(FaceMakerBullseyeTest, faceDrillerReversedOuterWire)
{
    gp_Pln plane;
    TopoDS_Wire outer = makeRectWire(0, 0, 4, 3);
    outer.Reverse();  // CW -- FaceDriller should autocorrect
    TestDriller driller(plane, outer);

    const TopoDS_Face& face = driller.Face();
    EXPECT_FALSE(face.IsNull());
    EXPECT_NEAR(faceArea(face), 12.0, 1e-6);
}

TEST_F(FaceMakerBullseyeTest, faceDrillerReversedHoleWire)
{
    gp_Pln plane;
    TopoDS_Wire outer = makeRectWire(0, 0, 4, 3);
    TestDriller driller(plane, outer);

    // Add a hole with reversed (CCW) orientation -- should be autocorrected to CW
    TopoDS_Wire hole = makeCircleWire(2, 1.5, 0, 0.5);
    // The circle is naturally CCW; addHole should reverse it
    driller.addHole(hole);

    double expected = 12.0 - (std::numbers::pi * 0.5 * 0.5);
    EXPECT_NEAR(faceArea(driller.Face()), expected, 1e-6);
}

TEST_F(FaceMakerBullseyeTest, faceDrillerFlippedPlane)
{
    gp_Pln plane(gp_Pnt(0, 0, 0), gp_Dir(0, 0, -1));
    TopoDS_Wire outer = makeRectWire(0, 0, 4, 3);
    TestDriller driller(plane, outer);

    TopoDS_Wire hole = makeCircleWire(2, 1.5, 0, 0.5);
    driller.addHole(hole);

    double expected = 12.0 - (std::numbers::pi * 0.5 * 0.5);
    EXPECT_NEAR(faceArea(driller.Face()), expected, 1e-6);
}

TEST_F(FaceMakerBullseyeTest, faceDrillerMultipleHoles)
{
    gp_Pln plane;
    TopoDS_Wire outer = makeRectWire(0, 0, 10, 6);
    TestDriller driller(plane, outer);

    driller.addHole(makeCircleWire(2, 3, 0, 0.5));
    driller.addHole(makeCircleWire(5, 3, 0, 1.0));
    driller.addHole(makeCircleWire(8, 3, 0, 0.75));

    double holeArea = std::numbers::pi * (0.5 * 0.5 + 1.0 * 1.0 + 0.75 * 0.75);
    double expected = 60.0 - holeArea;
    EXPECT_NEAR(faceArea(driller.Face()), expected, 1e-6);
}

TEST_F(FaceMakerBullseyeTest, buildEssenceOuterWithHole)
{
    FaceMakerBullseye fm;
    gp_Pln plane;
    fm.setPlane(plane);

    fm.addWire(makeRectWire(0, 0, 4, 3));
    fm.addWire(makeCircleWire(2, 1.5, 0, 0.5));

    fm.Build();
    ASSERT_TRUE(fm.IsDone());

    const TopoDS_Shape& result = fm.Shape();
    double expected = 12.0 - (std::numbers::pi * 0.5 * 0.5);
    EXPECT_NEAR(faceArea(result), expected, 1e-6);
}

TEST_F(FaceMakerBullseyeTest, buildEssenceNestedIslands)
{
    // Bullseye pattern: outer rect, inner circle hole, island circle inside the hole
    FaceMakerBullseye fm;
    gp_Pln plane;
    fm.setPlane(plane);

    fm.addWire(makeRectWire(0, 0, 10, 10));
    fm.addWire(makeCircleWire(5, 5, 0, 4.0));
    fm.addWire(makeCircleWire(5, 5, 0, 2.0));

    fm.Build();
    ASSERT_TRUE(fm.IsDone());

    // Expected: two faces
    // Face 1: outer rect minus big circle = 100 - pi*16
    // Face 2: small circle = pi*4
    double expected = (100.0 - std::numbers::pi * 16.0) + (std::numbers::pi * 4.0);
    EXPECT_NEAR(faceArea(fm.Shape()), expected, 1e-4);
}

TEST_F(FaceMakerBullseyeTest, buildEssenceTiltedPlane)
{
    gp_Dir normal(0, -1, 1);
    gp_Pln plane(gp_Pnt(0, 0, 0), normal);

    FaceMakerBullseye fm;
    fm.setPlane(plane);

    // Build a rectangular wire in the tilted plane
    gp_Ax2 ax(gp_Pnt(0, 0, 0), normal);
    gp_Dir xDir = ax.XDirection();
    gp_Dir yDir = ax.YDirection();

    gp_Pnt p0(0, 0, 0);
    gp_Pnt p1 = p0.XYZ() + 5.0 * xDir.XYZ();
    gp_Pnt p2 = p1.XYZ() + 3.0 * yDir.XYZ();
    gp_Pnt p3 = p0.XYZ() + 3.0 * yDir.XYZ();

    BRepBuilderAPI_MakeWire mw;
    mw.Add(BRepBuilderAPI_MakeEdge(p0, p1).Edge());
    mw.Add(BRepBuilderAPI_MakeEdge(p1, p2).Edge());
    mw.Add(BRepBuilderAPI_MakeEdge(p2, p3).Edge());
    mw.Add(BRepBuilderAPI_MakeEdge(p3, p0).Edge());
    fm.addWire(mw.Wire());

    fm.Build();
    ASSERT_TRUE(fm.IsDone());
    EXPECT_NEAR(faceArea(fm.Shape()), 15.0, 1e-6);
}

TEST_F(FaceMakerBullseyeTest, buildEssenceManyWires)
{
    FaceMakerBullseye fm;
    gp_Pln plane;
    fm.setPlane(plane);

    fm.addWire(makeRectWire(0, 0, 100, 100));

    // Add a grid of small circular holes
    double r = 1.0;
    int count = 0;
    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 5; ++col) {
            double cx = 10.0 + (col * 20.0);
            double cy = 10.0 + (row * 20.0);
            fm.addWire(makeCircleWire(cx, cy, 0, r));
            ++count;
        }
    }

    fm.Build();
    ASSERT_TRUE(fm.IsDone());

    double expected = 10000.0 - (count * std::numbers::pi * r * r);
    EXPECT_NEAR(faceArea(fm.Shape()), expected, 1e-3);
}

// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
