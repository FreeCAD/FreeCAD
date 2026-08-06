// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <numbers>

#include <FCConfig.h>

#include <Mod/Sketcher/App/GeoEnum.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include "SketcherTestHelpers.h"

using namespace SketcherTestHelpers;
using namespace Sketcher;

TEST_F(SketchObjectTest, testAddSymmetricCopyWithoutConstraints)
{
    // A simple mirror copy (addSymmetryConstraints=false) should duplicate geometry and copy
    // constraints, but not add any Symmetric constraints.
    Part::GeomLineSegment line;
    line.setPoints(Base::Vector3d(1, 0, 0), Base::Vector3d(3, 2, 0));
    int geo = getObject()->addGeometry(&line);

    int geoCount = getObject()->getHighestCurveIndex();
    int result = getObject()->addSymmetric({geo}, VAxis, PointPos::none, false);

    EXPECT_GE(result, 0);
    EXPECT_GT(getObject()->getHighestCurveIndex(), geoCount);
    EXPECT_EQ(countConstraintsOfType(getObject(), Symmetric), 0);
}

TEST_F(SketchObjectTest, testAddSymmetricWithConstraintsAddsSymmetric)
{
    // Mirror with addSymmetryConstraints=true should create Symmetric constraints linking
    // original and mirrored endpoints.
    Part::GeomLineSegment line;
    line.setPoints(Base::Vector3d(1, 0, 0), Base::Vector3d(3, 2, 0));
    int geo = getObject()->addGeometry(&line);

    getObject()->addSymmetric({geo}, VAxis, PointPos::none, true);

    EXPECT_GE(countConstraintsOfType(getObject(), Symmetric), 1);
}

TEST_F(SketchObjectTest, testAddSymmetricPreservesCoincidentTopology)
{
    // Two connected lines mirrored with constraints should preserve the Coincident connecting
    // the mirrored copies (topological constraint preservation).
    Part::GeomLineSegment line1, line2;
    line1.setPoints(Base::Vector3d(1, 0, 0), Base::Vector3d(2, 2, 0));
    line2.setPoints(Base::Vector3d(2, 2, 0), Base::Vector3d(3, 0, 0));
    int geo1 = getObject()->addGeometry(&line1);
    int geo2 = getObject()->addGeometry(&line2);

    auto* coincident = new Constraint();
    coincident->Type = Coincident;
    coincident->First = geo1;
    coincident->FirstPos = PointPos::end;
    coincident->Second = geo2;
    coincident->SecondPos = PointPos::start;
    getObject()->addConstraint(coincident);

    int coincidentBefore = countConstraintsOfType(getObject(), Coincident);

    getObject()->addSymmetric({geo1, geo2}, VAxis, PointPos::none, true);

    // The mirrored pair should also have a Coincident connecting them
    int coincidentAfter = countConstraintsOfType(getObject(), Coincident);
    EXPECT_GT(coincidentAfter, coincidentBefore);
}

TEST_F(SketchObjectTest, testAddSymmetricDowngradesTangentToCoincident)
{
    // Endpoint-to-endpoint Tangent between mirrored geometries would overconstrain (angle is
    // determined by the Symmetric constraints). These are downgraded to Coincident.
    Part::GeomLineSegment line1;
    line1.setPoints(Base::Vector3d(1, 0, 0), Base::Vector3d(2, 2, 0));
    int geo1 = getObject()->addGeometry(&line1);

    Part::GeomArcOfCircle arc;
    arc.setCenter(Base::Vector3d(3, 2, 0));
    arc.setRadius(1.0);
    arc.setRange(std::numbers::pi, std::numbers::pi * 1.5, true);
    int geo2 = getObject()->addGeometry(&arc);

    // Connect line end to arc start with tangent (endpoint-to-endpoint)
    auto* tang = new Constraint();
    tang->Type = Tangent;
    tang->First = geo1;
    tang->FirstPos = PointPos::end;
    tang->Second = geo2;
    tang->SecondPos = PointPos::start;
    getObject()->addConstraint(tang);

    getObject()->addSymmetric({geo1, geo2}, VAxis, PointPos::none, true);

    // The mirrored copy should have Coincident (downgraded from Tangent), not Tangent.
    // Original tangent is still there, plus new Coincident for the mirrored pair.
    int coincidentCount = countConstraintsOfType(getObject(), Coincident);
    EXPECT_GE(coincidentCount, 1);
}

TEST_F(SketchObjectTest, testAddSymmetricOnAxisPointGetsCoincident)
{
    // When a point lies exactly on the symmetry axis, addSymmetric should create a Coincident
    // (not Symmetric) to avoid solver singularity.
    Part::GeomLineSegment line;
    line.setPoints(Base::Vector3d(0, 1, 0), Base::Vector3d(2, 0, 0));
    int geo = getObject()->addGeometry(&line);

    getObject()->addSymmetric({geo}, VAxis, PointPos::none, true);

    // On-axis start point gets Coincident, off-axis end point gets Symmetric
    EXPECT_GE(countConstraintsOfType(getObject(), Coincident), 1);
    EXPECT_GE(countConstraintsOfType(getObject(), Symmetric), 1);
}

TEST_F(SketchObjectTest, testAddSymmetricOnAxisPointWithLineRef)
{
    // Mirror across a user-drawn line with a point sitting exactly on that line.
    Part::GeomLineSegment refLine;
    refLine.setPoints(Base::Vector3d(0, 0, 0), Base::Vector3d(0, 4, 0));
    int refGeo = getObject()->addGeometry(&refLine);

    Part::GeomLineSegment line;
    line.setPoints(Base::Vector3d(0, 2, 0), Base::Vector3d(3, 1, 0));
    int geo = getObject()->addGeometry(&line);

    getObject()->addSymmetric({geo}, refGeo, PointPos::none, true);

    // The on-axis start point should get Coincident
    EXPECT_GE(countConstraintsOfType(getObject(), Coincident), 1);
    EXPECT_GE(countConstraintsOfType(getObject(), Symmetric), 1);
}

TEST_F(SketchObjectTest, testAddSymmetricSharedVertexNoDuplicateConstraints)
{
    // Two lines sharing a vertex: the shared point should only get one Symmetric constraint,
    // not two (deduplication via coincidence groups).
    Part::GeomLineSegment line1, line2;
    line1.setPoints(Base::Vector3d(1, 0, 0), Base::Vector3d(2, 2, 0));
    line2.setPoints(Base::Vector3d(2, 2, 0), Base::Vector3d(3, 0, 0));
    int geo1 = getObject()->addGeometry(&line1);
    int geo2 = getObject()->addGeometry(&line2);

    auto* coincident = new Constraint();
    coincident->Type = Coincident;
    coincident->First = geo1;
    coincident->FirstPos = PointPos::end;
    coincident->Second = geo2;
    coincident->SecondPos = PointPos::start;
    getObject()->addConstraint(coincident);

    getObject()->addSymmetric({geo1, geo2}, VAxis, PointPos::none, true);

    // 2 lines * 2 endpoints = 4 points, but the shared vertex should be counted once. So we
    // expect 3 Symmetric constraints (or fewer if any point is on-axis), not 4.
    int symCount = countConstraintsOfType(getObject(), Symmetric);
    EXPECT_LE(symCount, 3);
    EXPECT_GE(symCount, 2);
}

TEST_F(SketchObjectTest, testAddSymmetricPointSymmetry)
{
    // Mirror across a point (not a line). Neither endpoint at the origin.
    Part::GeomLineSegment line;
    line.setPoints(Base::Vector3d(-1, 0, 0), Base::Vector3d(1, 0, 0));
    int geo = getObject()->addGeometry(&line);

    // Mirror about the origin (RootPoint)
    getObject()->addSymmetric({geo}, RtPnt, PointPos::start, true);

    // Neither endpoint is at the origin, so both should get Symmetric
    EXPECT_GE(countConstraintsOfType(getObject(), Symmetric), 2);
}

TEST_F(SketchObjectTest, testAddSymmetricPointSymmetryOnPoint)
{
    // Mirror across the origin when a point is at the origin.
    Part::GeomLineSegment line;
    line.setPoints(Base::Vector3d(0, 0, 0), Base::Vector3d(2, 1, 0));
    int geo = getObject()->addGeometry(&line);

    getObject()->addSymmetric({geo}, RtPnt, PointPos::start, true);

    // Start point is at the mirror point, should get Coincident not Symmetric
    EXPECT_GE(countConstraintsOfType(getObject(), Coincident), 1);
}

TEST_F(SketchObjectTest, testAddSymmetricCircleGetsCenterSymmetricAndEqual)
{
    // Mirroring a circle should create a Symmetric for the center and an Equal for the radius.
    Part::GeomCircle circle;
    circle.setCenter(Base::Vector3d(3, 0, 0));
    circle.setRadius(1.0);
    int geo = getObject()->addGeometry(&circle);

    getObject()->addSymmetric({geo}, VAxis, PointPos::none, true);

    EXPECT_GE(countConstraintsOfType(getObject(), Symmetric), 1);
    EXPECT_GE(countConstraintsOfType(getObject(), Equal), 1);
}
