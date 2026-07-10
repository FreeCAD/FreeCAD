// SPDX-License-Identifier: LGPL-2.1-or-later

#include <Mod/Measure/App/MeasureSnap.h>
#include <gtest/gtest.h>
#include <Base/Vector3D.h>
#include <Bnd_Box.hxx>
#include <BRep_Tool.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeRevol.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeTorus.hxx>
#include <Geom_BezierCurve.hxx>
#include <gp_Ax1.hxx>
#include <gp_Circ.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Wire.hxx>
#include <cmath>

// NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)

// MeasureSnap is pure shape math: no App::Document, no property machinery, so
// the fixture only builds OCCT shapes to feed the static helpers.
class MeasureSnap: public ::testing::Test
{
protected:
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

    TopoDS_Wire makeCircularWire(const gp_Pnt& pnt) const
    {
        BRepBuilderAPI_MakeWire mkWire(makeCircle(pnt));
        return mkWire.Wire();
    }

    TopoDS_Vertex makeVertex(const gp_Pnt& pnt) const
    {
        BRepBuilderAPI_MakeVertex mkVertex(pnt);
        return mkVertex.Vertex();
    }

    // The line [0,10] with non-uniform parameterization: arc-length middle (5,0,0)
    // diverges from the parameter middle (3.125,0,0).
    TopoDS_Edge makeNonUniformStraightEdge() const
    {
        TColgp_Array1OfPnt poles(1, 4);
        poles(1) = gp_Pnt(0.0, 0.0, 0.0);
        poles(2) = gp_Pnt(2.0, 0.0, 0.0);
        poles(3) = gp_Pnt(3.0, 0.0, 0.0);
        poles(4) = gp_Pnt(10.0, 0.0, 0.0);
        Handle(Geom_BezierCurve) curve = new Geom_BezierCurve(poles);
        BRepBuilderAPI_MakeEdge mkEdge(curve);
        return mkEdge.Edge();
    }

    // A cubic Bezier approximating a unit quarter circle: geometrically circular,
    // but its adaptor type is GeomAbs_BezierCurve, not GeomAbs_Circle.
    TopoDS_Edge makeCircularBezierEdge() const
    {
        const double k = 4.0 / 3.0 * (std::sqrt(2.0) - 1.0);
        TColgp_Array1OfPnt poles(1, 4);
        poles(1) = gp_Pnt(1.0, 0.0, 0.0);
        poles(2) = gp_Pnt(1.0, k, 0.0);
        poles(3) = gp_Pnt(k, 1.0, 0.0);
        poles(4) = gp_Pnt(0.0, 1.0, 0.0);
        Handle(Geom_BezierCurve) curve = new Geom_BezierCurve(poles);
        BRepBuilderAPI_MakeEdge mkEdge(curve);
        return mkEdge.Edge();
    }

    // A sphere's poles produce degenerate seam edges that carry no 3D curve.
    TopoDS_Edge makeDegenerateEdge() const
    {
        const TopoDS_Shape sphere = BRepPrimAPI_MakeSphere(1.0).Shape();
        for (TopExp_Explorer exp(sphere, TopAbs_EDGE); exp.More(); exp.Next()) {
            const TopoDS_Edge& edge = TopoDS::Edge(exp.Current());
            if (BRep_Tool::Degenerated(edge)) {
                return edge;
            }
        }
        return TopoDS_Edge();
    }

    // Lateral face of a Z-axis primitive built at the origin.
    TopoDS_Face makeCylinderFace(double radius, double height) const
    {
        return BRepPrimAPI_MakeCylinder(radius, height).Face();
    }

    // Lateral face swept through less than a full turn: trimmed in U but still an
    // analytic cylinder, as when a user picks one wall of a pocket.
    TopoDS_Face makePartialCylinderFace(double radius, double height, double angle) const
    {
        return BRepPrimAPI_MakeCylinder(radius, height, angle).Face();
    }

    TopoDS_Face makeConeFace(double radius1, double radius2, double height) const
    {
        return BRepPrimAPI_MakeCone(radius1, radius2, height).Face();
    }

    // A Bezier profile in the XZ plane revolved about Z: non-canonical, so OCCT
    // keeps it as a true surface of revolution rather than reducing it to a cone.
    TopoDS_Face makeRevolutionFace() const
    {
        TColgp_Array1OfPnt poles(1, 3);
        poles(1) = gp_Pnt(1.0, 0.0, 0.0);
        poles(2) = gp_Pnt(2.0, 0.0, 1.0);
        poles(3) = gp_Pnt(1.0, 0.0, 2.0);
        Handle(Geom_BezierCurve) profile = new Geom_BezierCurve(poles);
        BRepBuilderAPI_MakeEdge mkEdge(profile);
        const gp_Ax1 axis(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(0.0, 0.0, 1.0));
        BRepPrimAPI_MakeRevol mkRevol(mkEdge.Edge(), axis);
        for (TopExp_Explorer exp(mkRevol.Shape(), TopAbs_FACE); exp.More(); exp.Next()) {
            return TopoDS::Face(exp.Current());
        }
        return TopoDS_Face();
    }

    TopoDS_Face makePlaneFace() const
    {
        const gp_Pln plane(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(0.0, 0.0, 1.0));
        return BRepBuilderAPI_MakeFace(plane).Face();
    }

    TopoDS_Face makeSphereFace() const
    {
        const TopoDS_Shape sphere = BRepPrimAPI_MakeSphere(1.0).Shape();
        for (TopExp_Explorer exp(sphere, TopAbs_FACE); exp.More(); exp.Next()) {
            return TopoDS::Face(exp.Current());
        }
        return TopoDS_Face();
    }

    TopoDS_Face makeTorusFace() const
    {
        const TopoDS_Shape torus = BRepPrimAPI_MakeTorus(5.0, 1.0).Shape();
        for (TopExp_Explorer exp(torus, TopAbs_FACE); exp.More(); exp.Next()) {
            return TopoDS::Face(exp.Current());
        }
        return TopoDS_Face();
    }
};

// A null shape is rejected on both entry points.
TEST_F(MeasureSnap, testNullShapeReturnsFalse)
{
    const TopoDS_Shape nullShape;
    gp_Pnt out;
    EXPECT_FALSE(
        Measure::MeasureSnap::computeSnapPoint(nullShape, Measure::MeasureSnapMode::Center, nullptr, out)
    );
    EXPECT_FALSE(
        Measure::MeasureSnap::computeSnapPoint(nullShape, Measure::MeasureSnapMode::Vertex, nullptr, out)
    );
    EXPECT_EQ(Measure::MeasureSnap::getAvailableSnapTypes(nullShape), 0);
}

// Auto and None never resolve to a point; callers short-circuit them.
TEST_F(MeasureSnap, testAutoAndNoneReturnFalse)
{
    const TopoDS_Edge circle = makeCircle(gp_Pnt(0.0, 0.0, 0.0));
    gp_Pnt out;
    EXPECT_FALSE(
        Measure::MeasureSnap::computeSnapPoint(circle, Measure::MeasureSnapMode::Auto, nullptr, out)
    );
    EXPECT_FALSE(
        Measure::MeasureSnap::computeSnapPoint(circle, Measure::MeasureSnapMode::None, nullptr, out)
    );
}

TEST_F(MeasureSnap, testCenterOnCircleEdge)
{
    const TopoDS_Edge circle = makeCircle(gp_Pnt(3.0, 4.0, 0.0));
    gp_Pnt out;
    ASSERT_TRUE(
        Measure::MeasureSnap::computeSnapPoint(circle, Measure::MeasureSnapMode::Center, nullptr, out)
    );
    EXPECT_DOUBLE_EQ(out.X(), 3.0);
    EXPECT_DOUBLE_EQ(out.Y(), 4.0);
    EXPECT_DOUBLE_EQ(out.Z(), 0.0);
}

// An arc snaps to the full-circle center, not to a point on the arc.
TEST_F(MeasureSnap, testCenterOnArcEdge)
{
    const TopoDS_Edge arc = makeArc(gp_Pnt(2.0, 3.0, 0.0), 0.0, 1.0);
    gp_Pnt out;
    ASSERT_TRUE(
        Measure::MeasureSnap::computeSnapPoint(arc, Measure::MeasureSnapMode::Center, nullptr, out)
    );
    EXPECT_DOUBLE_EQ(out.X(), 2.0);
    EXPECT_DOUBLE_EQ(out.Y(), 3.0);
    EXPECT_DOUBLE_EQ(out.Z(), 0.0);
}

TEST_F(MeasureSnap, testCenterOnLineReturnsFalse)
{
    const TopoDS_Edge line = makeLine(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(1.0, 0.0, 0.0));
    gp_Pnt out;
    EXPECT_FALSE(
        Measure::MeasureSnap::computeSnapPoint(line, Measure::MeasureSnapMode::Center, nullptr, out)
    );
}

// A wire wrapping a single circular edge is not classified as a circle by
// BRepAdaptor_CompCurve, so Center does not apply.
TEST_F(MeasureSnap, testCenterOnCircularWireReturnsFalse)
{
    const TopoDS_Wire wire = makeCircularWire(gp_Pnt(3.0, 4.0, 0.0));
    gp_Pnt out;
    EXPECT_FALSE(
        Measure::MeasureSnap::computeSnapPoint(wire, Measure::MeasureSnapMode::Center, nullptr, out)
    );
}

// Detection is by adaptor type, not geometric shape, so a circular-looking
// BSpline/Bezier edge (e.g. a STEP import) does not center-snap.
TEST_F(MeasureSnap, testCenterOnCircularBezierEdgeReturnsFalse)
{
    const TopoDS_Edge edge = makeCircularBezierEdge();
    gp_Pnt out;
    EXPECT_FALSE(
        Measure::MeasureSnap::computeSnapPoint(edge, Measure::MeasureSnapMode::Center, nullptr, out)
    );
}

TEST_F(MeasureSnap, testMidpointOnLineEdge)
{
    const TopoDS_Edge line = makeLine(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(4.0, 0.0, 0.0));
    gp_Pnt out;
    ASSERT_TRUE(
        Measure::MeasureSnap::computeSnapPoint(line, Measure::MeasureSnapMode::Midpoint, nullptr, out)
    );
    EXPECT_NEAR(out.X(), 2.0, 1e-6);
    EXPECT_NEAR(out.Y(), 0.0, 1e-6);
    EXPECT_NEAR(out.Z(), 0.0, 1e-6);
}

// The midpoint of an arc lies on the curve, not on the chord: on a unit circle
// spanning 1 rad, arc length equals angle, so the half-length point is at 0.5 rad.
TEST_F(MeasureSnap, testMidpointOnArcIsOnCurve)
{
    const TopoDS_Edge arc = makeArc(gp_Pnt(0.0, 0.0, 0.0), 0.0, 1.0);
    gp_Pnt out;
    ASSERT_TRUE(
        Measure::MeasureSnap::computeSnapPoint(arc, Measure::MeasureSnapMode::Midpoint, nullptr, out)
    );
    EXPECT_NEAR(out.X(), 0.877582561890, 1e-6);
    EXPECT_NEAR(out.Y(), 0.479425538604, 1e-6);
    EXPECT_NEAR(out.Z(), 0.0, 1e-6);
}

// Arc-length middle (5,0,0), not the parameter middle (3.125,0,0): the result
// must reflect the visible center of the non-uniformly parameterized segment.
TEST_F(MeasureSnap, testMidpointIsArcLengthNotParameter)
{
    const TopoDS_Edge edge = makeNonUniformStraightEdge();
    gp_Pnt out;
    ASSERT_TRUE(
        Measure::MeasureSnap::computeSnapPoint(edge, Measure::MeasureSnapMode::Midpoint, nullptr, out)
    );
    EXPECT_NEAR(out.X(), 5.0, 1e-6);
    EXPECT_NEAR(out.Y(), 0.0, 1e-6);
    EXPECT_NEAR(out.Z(), 0.0, 1e-6);
}

TEST_F(MeasureSnap, testMidpointOnVertexReturnsFalse)
{
    const TopoDS_Vertex vertex = makeVertex(gp_Pnt(1.0, 2.0, 3.0));
    gp_Pnt out;
    EXPECT_FALSE(
        Measure::MeasureSnap::computeSnapPoint(vertex, Measure::MeasureSnapMode::Midpoint, nullptr, out)
    );
}

TEST_F(MeasureSnap, testVertexOnVertexShape)
{
    const TopoDS_Vertex vertex = makeVertex(gp_Pnt(1.0, 2.0, 3.0));
    gp_Pnt out;
    ASSERT_TRUE(
        Measure::MeasureSnap::computeSnapPoint(vertex, Measure::MeasureSnapMode::Vertex, nullptr, out)
    );
    EXPECT_DOUBLE_EQ(out.X(), 1.0);
    EXPECT_DOUBLE_EQ(out.Y(), 2.0);
    EXPECT_DOUBLE_EQ(out.Z(), 3.0);
}

// With no cursor the first edge endpoint is returned, and the choice is stable
// across calls so a recompute never shifts a saved measurement.
TEST_F(MeasureSnap, testVertexOnEdgeNullCursorIsFirstAndStable)
{
    const TopoDS_Edge line = makeLine(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(4.0, 0.0, 0.0));
    gp_Pnt first;
    ASSERT_TRUE(
        Measure::MeasureSnap::computeSnapPoint(line, Measure::MeasureSnapMode::Vertex, nullptr, first)
    );
    EXPECT_DOUBLE_EQ(first.X(), 0.0);
    EXPECT_DOUBLE_EQ(first.Y(), 0.0);
    EXPECT_DOUBLE_EQ(first.Z(), 0.0);
    gp_Pnt again;
    ASSERT_TRUE(
        Measure::MeasureSnap::computeSnapPoint(line, Measure::MeasureSnapMode::Vertex, nullptr, again)
    );
    EXPECT_DOUBLE_EQ(again.X(), first.X());
    EXPECT_DOUBLE_EQ(again.Y(), first.Y());
    EXPECT_DOUBLE_EQ(again.Z(), first.Z());
}

TEST_F(MeasureSnap, testVertexOnEdgeCursorSelectsNearer)
{
    const TopoDS_Edge line = makeLine(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(4.0, 0.0, 0.0));
    gp_Pnt out;
    const Base::Vector3d nearEnd(3.5, 0.0, 0.0);
    ASSERT_TRUE(
        Measure::MeasureSnap::computeSnapPoint(line, Measure::MeasureSnapMode::Vertex, &nearEnd, out)
    );
    EXPECT_DOUBLE_EQ(out.X(), 4.0);
    const Base::Vector3d nearStart(0.5, 0.0, 0.0);
    ASSERT_TRUE(
        Measure::MeasureSnap::computeSnapPoint(line, Measure::MeasureSnapMode::Vertex, &nearStart, out)
    );
    EXPECT_DOUBLE_EQ(out.X(), 0.0);
}

// A cursor exactly between the endpoints falls back to the first vertex.
TEST_F(MeasureSnap, testVertexEquidistantCursorPicksFirst)
{
    const TopoDS_Edge line = makeLine(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(4.0, 0.0, 0.0));
    gp_Pnt out;
    const Base::Vector3d middle(2.0, 0.0, 0.0);
    ASSERT_TRUE(
        Measure::MeasureSnap::computeSnapPoint(line, Measure::MeasureSnapMode::Vertex, &middle, out)
    );
    EXPECT_DOUBLE_EQ(out.X(), 0.0);
}

TEST_F(MeasureSnap, testVertexOnWireReturnsFalse)
{
    const TopoDS_Wire wire = makeCircularWire(gp_Pnt(0.0, 0.0, 0.0));
    gp_Pnt out;
    EXPECT_FALSE(
        Measure::MeasureSnap::computeSnapPoint(wire, Measure::MeasureSnapMode::Vertex, nullptr, out)
    );
}

TEST_F(MeasureSnap, testAvailableSnapTypesOnCircleEdge)
{
    const int expected = static_cast<int>(Measure::MeasureSnapFlag::FlagVertex)
        | static_cast<int>(Measure::MeasureSnapFlag::FlagMidpoint)
        | static_cast<int>(Measure::MeasureSnapFlag::FlagCenter);
    const TopoDS_Edge circle = makeCircle(gp_Pnt(0.0, 0.0, 0.0));
    EXPECT_EQ(Measure::MeasureSnap::getAvailableSnapTypes(circle), expected);
}

TEST_F(MeasureSnap, testAvailableSnapTypesOnLineEdge)
{
    const int expected = static_cast<int>(Measure::MeasureSnapFlag::FlagVertex)
        | static_cast<int>(Measure::MeasureSnapFlag::FlagMidpoint);
    const TopoDS_Edge line = makeLine(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(1.0, 0.0, 0.0));
    EXPECT_EQ(Measure::MeasureSnap::getAvailableSnapTypes(line), expected);
}

TEST_F(MeasureSnap, testAvailableSnapTypesOnVertex)
{
    const int expected = static_cast<int>(Measure::MeasureSnapFlag::FlagVertex);
    const TopoDS_Vertex vertex = makeVertex(gp_Pnt(1.0, 2.0, 3.0));
    EXPECT_EQ(Measure::MeasureSnap::getAvailableSnapTypes(vertex), expected);
}

TEST_F(MeasureSnap, testAvailableSnapTypesOnCircularWire)
{
    const TopoDS_Wire wire = makeCircularWire(gp_Pnt(0.0, 0.0, 0.0));
    EXPECT_EQ(Measure::MeasureSnap::getAvailableSnapTypes(wire), 0);
}

// A degenerate edge offers no flags and never throws, on either entry point.
TEST_F(MeasureSnap, testDegenerateEdgeRejected)
{
    const TopoDS_Edge pole = makeDegenerateEdge();
    ASSERT_TRUE(BRep_Tool::Degenerated(pole));
    EXPECT_EQ(Measure::MeasureSnap::getAvailableSnapTypes(pole), 0);
    gp_Pnt out;
    EXPECT_FALSE(
        Measure::MeasureSnap::computeSnapPoint(pole, Measure::MeasureSnapMode::Center, nullptr, out)
    );
    EXPECT_FALSE(
        Measure::MeasureSnap::computeSnapPoint(pole, Measure::MeasureSnapMode::Midpoint, nullptr, out)
    );
    EXPECT_FALSE(
        Measure::MeasureSnap::computeSnapPoint(pole, Measure::MeasureSnapMode::Vertex, nullptr, out)
    );
}

// A cylinder built on the origin Z axis yields that axis: direction along Z,
// origin at the base center.
TEST_F(MeasureSnap, testAxisOfCylinderFace)
{
    const TopoDS_Face face = makeCylinderFace(2.0, 5.0);
    gp_Ax1 axis;
    ASSERT_TRUE(Measure::MeasureSnap::axisOfFace(face, axis));
    EXPECT_TRUE(axis.Direction().IsParallel(gp_Dir(0.0, 0.0, 1.0), 1e-9));
    EXPECT_DOUBLE_EQ(axis.Location().X(), 0.0);
    EXPECT_DOUBLE_EQ(axis.Location().Y(), 0.0);
    EXPECT_DOUBLE_EQ(axis.Location().Z(), 0.0);
}

// The cone axis is collinear with Z; the sign depends on the half-angle
// convention, so only collinearity and the on-axis origin are asserted.
TEST_F(MeasureSnap, testAxisOfConeFace)
{
    const TopoDS_Face face = makeConeFace(2.0, 1.0, 5.0);
    gp_Ax1 axis;
    ASSERT_TRUE(Measure::MeasureSnap::axisOfFace(face, axis));
    EXPECT_TRUE(axis.Direction().IsParallel(gp_Dir(0.0, 0.0, 1.0), 1e-9));
    EXPECT_DOUBLE_EQ(axis.Location().X(), 0.0);
    EXPECT_DOUBLE_EQ(axis.Location().Y(), 0.0);
}

TEST_F(MeasureSnap, testAxisOfRevolutionFace)
{
    const TopoDS_Face face = makeRevolutionFace();
    ASSERT_FALSE(face.IsNull());
    gp_Ax1 axis;
    ASSERT_TRUE(Measure::MeasureSnap::axisOfFace(face, axis));
    EXPECT_TRUE(axis.Direction().IsParallel(gp_Dir(0.0, 0.0, 1.0), 1e-9));
    EXPECT_DOUBLE_EQ(axis.Location().X(), 0.0);
    EXPECT_DOUBLE_EQ(axis.Location().Y(), 0.0);
}

TEST_F(MeasureSnap, testAxisOfPlaneFaceReturnsFalse)
{
    const TopoDS_Face face = makePlaneFace();
    gp_Ax1 axis;
    EXPECT_FALSE(Measure::MeasureSnap::axisOfFace(face, axis));
}

TEST_F(MeasureSnap, testAxisOfSphereFaceReturnsFalse)
{
    const TopoDS_Face face = makeSphereFace();
    gp_Ax1 axis;
    EXPECT_FALSE(Measure::MeasureSnap::axisOfFace(face, axis));
}

// A torus has an axis but is deferred in v1: the switch must not claim it, so neither
// the axis nor FlagAxis is offered.
TEST_F(MeasureSnap, testAxisOfTorusFaceReturnsFalse)
{
    const TopoDS_Face face = makeTorusFace();
    ASSERT_FALSE(face.IsNull());
    gp_Ax1 axis;
    EXPECT_FALSE(Measure::MeasureSnap::axisOfFace(face, axis));
    EXPECT_EQ(Measure::MeasureSnap::getAvailableSnapTypes(face), 0);
}

TEST_F(MeasureSnap, testAxisOfNullFaceReturnsFalse)
{
    const TopoDS_Face nullFace;
    gp_Ax1 axis;
    EXPECT_FALSE(Measure::MeasureSnap::axisOfFace(nullFace, axis));
}

// A Z axis shifted to (1,2,0): the foot keeps the axis line's X/Y and the point's
// height, independent of the axis origin's own Z.
TEST_F(MeasureSnap, testProjectOntoOffsetZAxis)
{
    const gp_Ax1 axis(gp_Pnt(1.0, 2.0, 0.0), gp_Dir(0.0, 0.0, 1.0));
    const gp_Pnt foot = Measure::MeasureSnap::projectOntoAxis(axis, gp_Pnt(5.0, 6.0, 10.0));
    EXPECT_DOUBLE_EQ(foot.X(), 1.0);
    EXPECT_DOUBLE_EQ(foot.Y(), 2.0);
    EXPECT_DOUBLE_EQ(foot.Z(), 10.0);
}

// The line is infinite: a foot below the axis origin has a negative parameter and
// still resolves, proving no clamping to the gp_Ax1 origin.
TEST_F(MeasureSnap, testProjectBehindAxisOriginStaysOnInfiniteLine)
{
    const gp_Ax1 axis(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(0.0, 0.0, 1.0));
    const gp_Pnt foot = Measure::MeasureSnap::projectOntoAxis(axis, gp_Pnt(2.0, 0.0, -3.0));
    EXPECT_DOUBLE_EQ(foot.X(), 0.0);
    EXPECT_DOUBLE_EQ(foot.Y(), 0.0);
    EXPECT_DOUBLE_EQ(foot.Z(), -3.0);
}

// Projecting (1,0,0) onto the (1,1,0) line through the origin gives (0.5,0.5,0);
// gp_Dir normalization makes this inexact.
TEST_F(MeasureSnap, testProjectOntoDiagonalAxis)
{
    const gp_Ax1 axis(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(1.0, 1.0, 0.0));
    const gp_Pnt foot = Measure::MeasureSnap::projectOntoAxis(axis, gp_Pnt(1.0, 0.0, 0.0));
    EXPECT_NEAR(foot.X(), 0.5, 1e-9);
    EXPECT_NEAR(foot.Y(), 0.5, 1e-9);
    EXPECT_NEAR(foot.Z(), 0.0, 1e-9);
}

// Skew X and Y axes; common perpendicular joins (3,0,0) to (3,0,5).
TEST_F(MeasureSnap, testClosestPointsSkew)
{
    const gp_Ax1 a(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(1.0, 0.0, 0.0));
    const gp_Ax1 b(gp_Pnt(3.0, 0.0, 5.0), gp_Dir(0.0, 1.0, 0.0));
    gp_Pnt onA;
    gp_Pnt onB;
    ASSERT_TRUE(Measure::MeasureSnap::closestPointsOnAxes(a, b, onA, onB));
    EXPECT_NEAR(onA.X(), 3.0, 1e-6);
    EXPECT_NEAR(onA.Y(), 0.0, 1e-6);
    EXPECT_NEAR(onA.Z(), 0.0, 1e-6);
    EXPECT_NEAR(onB.X(), 3.0, 1e-6);
    EXPECT_NEAR(onB.Y(), 0.0, 1e-6);
    EXPECT_NEAR(onB.Z(), 5.0, 1e-6);
    EXPECT_NEAR(onA.Distance(onB), 5.0, 1e-6);
}

// Parallel axes at different heights: the pair pins to A's height (z=10), not B's.
TEST_F(MeasureSnap, testClosestPointsParallelIndependentOfOriginHeight)
{
    const gp_Ax1 a(gp_Pnt(0.0, 0.0, 10.0), gp_Dir(0.0, 0.0, 1.0));
    const gp_Ax1 b(gp_Pnt(4.0, 0.0, -7.0), gp_Dir(0.0, 0.0, 1.0));
    gp_Pnt onA;
    gp_Pnt onB;
    ASSERT_TRUE(Measure::MeasureSnap::closestPointsOnAxes(a, b, onA, onB));
    EXPECT_DOUBLE_EQ(onA.X(), 0.0);
    EXPECT_DOUBLE_EQ(onA.Y(), 0.0);
    EXPECT_DOUBLE_EQ(onA.Z(), 10.0);
    EXPECT_DOUBLE_EQ(onB.X(), 4.0);
    EXPECT_DOUBLE_EQ(onB.Y(), 0.0);
    EXPECT_DOUBLE_EQ(onB.Z(), 10.0);
    EXPECT_DOUBLE_EQ(onA.Distance(onB), 4.0);
}

// Coincident axes are the degenerate parallel case: the rule returns A's origin on
// both sides, distance zero, without dividing by a zero-length perpendicular.
TEST_F(MeasureSnap, testClosestPointsCoincident)
{
    const gp_Ax1 a(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(0.0, 0.0, 1.0));
    const gp_Ax1 b(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(0.0, 0.0, 1.0));
    gp_Pnt onA;
    gp_Pnt onB;
    ASSERT_TRUE(Measure::MeasureSnap::closestPointsOnAxes(a, b, onA, onB));
    EXPECT_DOUBLE_EQ(onA.Distance(onB), 0.0);
}

// Near-parallel (0.001 rad) must take the skew branch, not the parallel rule:
// z=20 proves it (the parallel rule would anchor A at its origin, z=0).
TEST_F(MeasureSnap, testClosestPointsNearParallelStaysSkew)
{
    const gp_Ax1 a(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(0.0, 0.0, 1.0));
    const gp_Ax1 b(gp_Pnt(0.0, 5.0, 20.0), gp_Dir(0.001, 0.0, 1.0));
    gp_Pnt onA;
    gp_Pnt onB;
    ASSERT_TRUE(Measure::MeasureSnap::closestPointsOnAxes(a, b, onA, onB));
    EXPECT_NEAR(onA.X(), 0.0, 1e-4);
    EXPECT_NEAR(onA.Y(), 0.0, 1e-4);
    EXPECT_NEAR(onA.Z(), 20.0, 1e-4);
    EXPECT_NEAR(onB.X(), 0.0, 1e-4);
    EXPECT_NEAR(onB.Y(), 5.0, 1e-4);
    EXPECT_NEAR(onB.Z(), 20.0, 1e-4);
    EXPECT_NEAR(onA.Distance(onB), 5.0, 1e-4);
}

// A trimmed (wedge) cylindrical face still reports GeomAbs_Cylinder.
TEST_F(MeasureSnap, testAxisOfTrimmedCylinderFace)
{
    const TopoDS_Face face = makePartialCylinderFace(2.0, 5.0, 2.0);
    gp_Ax1 axis;
    ASSERT_TRUE(Measure::MeasureSnap::axisOfFace(face, axis));
    EXPECT_TRUE(axis.Direction().IsParallel(gp_Dir(0.0, 0.0, 1.0), 1e-9));
    EXPECT_DOUBLE_EQ(axis.Location().X(), 0.0);
    EXPECT_DOUBLE_EQ(axis.Location().Y(), 0.0);
    EXPECT_DOUBLE_EQ(axis.Location().Z(), 0.0);
}

// One axis-bearing face suffices: getAvailableSnapTypes maps axisOfFace's result to
// FlagAxis, and the per-surface-type axis extraction is covered by the axisOfFace tests.
TEST_F(MeasureSnap, testAvailableSnapTypesOnCylinderFace)
{
    const int expected = static_cast<int>(Measure::MeasureSnapFlag::FlagAxis);
    const TopoDS_Face face = makeCylinderFace(2.0, 5.0);
    EXPECT_EQ(Measure::MeasureSnap::getAvailableSnapTypes(face), expected);
}

// No cursor: preview point is the bbox centre projected onto the axis, not the
// raw gp_Ax1 origin; a height-5 cylinder gives (0,0,2.5).
TEST_F(MeasureSnap, testAxisSnapNoCursorProjectsBboxCentre)
{
    const TopoDS_Face face = makeCylinderFace(2.0, 5.0);
    gp_Pnt out;
    gp_Dir dir;
    ASSERT_TRUE(
        Measure::MeasureSnap::computeSnapPoint(face, Measure::MeasureSnapMode::Axis, nullptr, out, &dir)
    );
    EXPECT_TRUE(dir.IsParallel(gp_Dir(0.0, 0.0, 1.0), 1e-9));
    EXPECT_DOUBLE_EQ(out.X(), 0.0);
    EXPECT_DOUBLE_EQ(out.Y(), 0.0);
    EXPECT_NEAR(out.Z(), 2.5, 1e-6);
}

// Cursor supplied: the preview point is the cursor projected onto the axis, so an
// off-axis cursor at height 3 resolves to (0,0,3) on the Z axis line.
TEST_F(MeasureSnap, testAxisSnapCursorProjectsCursor)
{
    const TopoDS_Face face = makeCylinderFace(2.0, 5.0);
    gp_Pnt out;
    gp_Dir dir;
    const Base::Vector3d cursor(10.0, 10.0, 3.0);
    ASSERT_TRUE(
        Measure::MeasureSnap::computeSnapPoint(face, Measure::MeasureSnapMode::Axis, &cursor, out, &dir)
    );
    EXPECT_TRUE(dir.IsParallel(gp_Dir(0.0, 0.0, 1.0), 1e-9));
    EXPECT_DOUBLE_EQ(out.X(), 0.0);
    EXPECT_DOUBLE_EQ(out.Y(), 0.0);
    EXPECT_DOUBLE_EQ(out.Z(), 3.0);
}

// Axis mode needs a face; an edge carries no surface axis and is rejected.
TEST_F(MeasureSnap, testAxisSnapOnNonFaceReturnsFalse)
{
    const TopoDS_Edge line = makeLine(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(1.0, 0.0, 0.0));
    gp_Pnt out;
    EXPECT_FALSE(
        Measure::MeasureSnap::computeSnapPoint(line, Measure::MeasureSnapMode::Axis, nullptr, out)
    );
}

// A planar face has no axis, so Axis mode returns false even on a face.
TEST_F(MeasureSnap, testAxisSnapOnPlaneFaceReturnsFalse)
{
    const TopoDS_Face face = makePlaneFace();
    gp_Pnt out;
    EXPECT_FALSE(
        Measure::MeasureSnap::computeSnapPoint(face, Measure::MeasureSnapMode::Axis, nullptr, out)
    );
}

// Spans twice the box diagonal: a diagonal-10 box at z=5 gives a length-20 edge.
TEST_F(MeasureSnap, testBoundedAxisEdgeSpansTwiceDiagonal)
{
    Bnd_Box bounds;
    bounds.Add(gp_Pnt(0.0, 0.0, 0.0));
    bounds.Add(gp_Pnt(0.0, 0.0, 10.0));
    const gp_Ax1 axis(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(0.0, 0.0, 1.0));
    const TopoDS_Edge edge = Measure::MeasureSnap::boundedAxisEdge(axis, bounds);
    ASSERT_FALSE(edge.IsNull());
    TopoDS_Vertex v1;
    TopoDS_Vertex v2;
    TopExp::Vertices(edge, v1, v2);
    const gp_Pnt p1 = BRep_Tool::Pnt(v1);
    const gp_Pnt p2 = BRep_Tool::Pnt(v2);
    EXPECT_NEAR(p1.Distance(p2), 20.0, 1e-6);
    EXPECT_NEAR((p1.Z() + p2.Z()) / 2.0, 5.0, 1e-6);
}

// A probe near the top, offset 5 from the axis, must measure to the nearby
// perpendicular foot (5), not the far gp_Ax1 origin at the base (~95).
TEST_F(MeasureSnap, testBoundedAxisEdgeIgnoresArbitraryOrigin)
{
    const TopoDS_Face cylinder = makeCylinderFace(1.0, 100.0);
    gp_Ax1 axis;
    ASSERT_TRUE(Measure::MeasureSnap::axisOfFace(cylinder, axis));
    const TopoDS_Vertex probe = makeVertex(gp_Pnt(5.0, 0.0, 95.0));
    Bnd_Box bounds;
    BRepBndLib::Add(cylinder, bounds);
    BRepBndLib::Add(probe, bounds);
    const TopoDS_Edge edge = Measure::MeasureSnap::boundedAxisEdge(axis, bounds);
    ASSERT_FALSE(edge.IsNull());
    BRepExtrema_DistShapeShape dist(edge, probe);
    ASSERT_TRUE(dist.IsDone());
    EXPECT_NEAR(dist.Value(), 5.0, 1e-6);
}

// A void bounding box yields a null edge rather than throwing on CornerMin.
TEST_F(MeasureSnap, testBoundedAxisEdgeVoidBoxReturnsNull)
{
    const Bnd_Box voidBox;
    const gp_Ax1 axis(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(0.0, 0.0, 1.0));
    const TopoDS_Edge edge = Measure::MeasureSnap::boundedAxisEdge(axis, voidBox);
    EXPECT_TRUE(edge.IsNull());
}
// A stored index maps to its mode; the last valid index resolves, while an index
// past the last mode and the -1 of an out-of-range reload fall back to Auto.
TEST_F(MeasureSnap, testSnapModeFromIndexClampsOutOfRange)
{
    EXPECT_EQ(Measure::MeasureSnap::snapModeFromIndex(5), Measure::MeasureSnapMode::Axis);
    EXPECT_EQ(Measure::MeasureSnap::snapModeFromIndex(6), Measure::MeasureSnapMode::Auto);
    EXPECT_EQ(Measure::MeasureSnap::snapModeFromIndex(-1), Measure::MeasureSnapMode::Auto);
}

// Auto prefers Center, then Midpoint, then Vertex; an explicit mode needs its own flag;
// Axis is never auto-selected and only previews when explicitly requested.
TEST_F(MeasureSnap, testPickPreviewType)
{
    using Measure::MeasureSnap;
    using Measure::MeasureSnapFlag;
    using Measure::MeasureSnapMode;

    const int vertexOnly = static_cast<int>(MeasureSnapFlag::FlagVertex);
    const int lineFlags = static_cast<int>(MeasureSnapFlag::FlagVertex)
        | static_cast<int>(MeasureSnapFlag::FlagMidpoint);
    const int circleFlags = static_cast<int>(MeasureSnapFlag::FlagVertex)
        | static_cast<int>(MeasureSnapFlag::FlagMidpoint)
        | static_cast<int>(MeasureSnapFlag::FlagCenter);
    const int axisOnly = static_cast<int>(MeasureSnapFlag::FlagAxis);

    // Auto picks the highest-priority available point snap.
    EXPECT_EQ(MeasureSnap::pickPreviewType(circleFlags, MeasureSnapMode::Auto), MeasureSnapMode::Center);
    EXPECT_EQ(MeasureSnap::pickPreviewType(lineFlags, MeasureSnapMode::Auto), MeasureSnapMode::Midpoint);
    EXPECT_EQ(MeasureSnap::pickPreviewType(vertexOnly, MeasureSnapMode::Auto), MeasureSnapMode::Vertex);
    EXPECT_EQ(MeasureSnap::pickPreviewType(0, MeasureSnapMode::Auto), MeasureSnapMode::None);
    EXPECT_EQ(MeasureSnap::pickPreviewType(axisOnly, MeasureSnapMode::Auto), MeasureSnapMode::None);

    // An explicit mode previews only when its flag is present.
    EXPECT_EQ(MeasureSnap::pickPreviewType(circleFlags, MeasureSnapMode::Center), MeasureSnapMode::Center);
    EXPECT_EQ(MeasureSnap::pickPreviewType(lineFlags, MeasureSnapMode::Center), MeasureSnapMode::None);
    EXPECT_EQ(MeasureSnap::pickPreviewType(lineFlags, MeasureSnapMode::Midpoint), MeasureSnapMode::Midpoint);
    EXPECT_EQ(MeasureSnap::pickPreviewType(vertexOnly, MeasureSnapMode::Vertex), MeasureSnapMode::Vertex);

    // None never previews; Axis previews only when explicitly requested.
    EXPECT_EQ(MeasureSnap::pickPreviewType(circleFlags, MeasureSnapMode::None), MeasureSnapMode::None);
    EXPECT_EQ(MeasureSnap::pickPreviewType(axisOnly, MeasureSnapMode::Axis), MeasureSnapMode::Axis);
    EXPECT_EQ(MeasureSnap::pickPreviewType(vertexOnly, MeasureSnapMode::Axis), MeasureSnapMode::None);
}

// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
