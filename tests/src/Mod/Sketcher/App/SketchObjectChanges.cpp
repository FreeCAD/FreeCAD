// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <FCConfig.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/Expression.h>
#include <App/ObjectIdentifier.h>
#include <Mod/Sketcher/App/GeoEnum.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include "SketcherTestHelpers.h"

using namespace SketcherTestHelpers;

TEST_F(SketchObjectTest, testSplitLineSegment)
{
    // Arrange
    Base::Vector3d splitPoint(2.0, 3.1, 0.0);
    Part::GeomLineSegment lineSeg;
    setupLineSegment(lineSeg);
    int geoId = getObject()->addGeometry(&lineSeg);

    // Act
    int result = getObject()->split(geoId, splitPoint);

    // Assert
    EXPECT_EQ(result, 0);
    // One additional curve should be added
    EXPECT_EQ(getObject()->getHighestCurveIndex(), geoId + 1);
    // Expect the resultant curves are line segments and shape is conserved
    int numberOfCoincidentConstraints = countConstraintsOfType(getObject(), Sketcher::Coincident);
    EXPECT_EQ(numberOfCoincidentConstraints, 1);
}

TEST_F(SketchObjectTest, testSplitCircle)
{
    // Arrange
    Base::Vector3d splitPoint(2.0, 3.1, 0.0);
    Part::GeomCircle circle;
    setupCircle(circle);
    int geoId = getObject()->addGeometry(&circle);

    // Act
    int result = getObject()->split(geoId, splitPoint);

    // Assert
    EXPECT_EQ(result, 0);
    // The circle should be split into an arc now
    EXPECT_EQ(getObject()->getHighestCurveIndex(), geoId);
}

TEST_F(SketchObjectTest, testSplitEllipse)
{
    // Arrange
    Base::Vector3d splitPoint(2.0, 3.1, 0.0);
    Part::GeomEllipse ellipse;
    setupEllipse(ellipse);
    int geoId = getObject()->addGeometry(&ellipse);

    // Act
    int result = getObject()->split(geoId, splitPoint);

    // Assert
    EXPECT_EQ(result, 0);
    // The ellipse should be split into an arc of ellipse now
    // FIXME: Internal geometries may be added or removed which may cause some issues
    // EXPECT_EQ(getObject()->getHighestCurveIndex(), geoId);
}

TEST_F(SketchObjectTest, testSplitArcOfCircle)
{
    // Arrange
    Base::Vector3d splitPoint(-2.0, 3.1, 0.0);
    Part::GeomArcOfCircle arcOfCircle;
    setupArcOfCircle(arcOfCircle);
    int geoId = getObject()->addGeometry(&arcOfCircle);

    // Act
    int result = getObject()->split(geoId, splitPoint);

    // Assert
    EXPECT_EQ(result, 0);
    // The arcOfCircle should be split into an arc now
    EXPECT_EQ(getObject()->getHighestCurveIndex(), geoId + 1);
    // Expect the end points and centers of the resultant curve are coincident.
    int numberOfCoincidentConstraints = countConstraintsOfType(getObject(), Sketcher::Coincident);
    EXPECT_EQ(numberOfCoincidentConstraints, 2);
}

TEST_F(SketchObjectTest, testSplitArcOfConic)
{
    // Arrange
    // Define a parabola/hyperbola as reference
    Base::Vector3d splitPoint(1.0, -1.1, 0.0);
    Part::GeomArcOfParabola arcOfConic;
    setupArcOfParabola(arcOfConic);
    int geoId = getObject()->addGeometry(&arcOfConic);

    // Act
    // TODO: Sample random points from both sides of the split
    int result = getObject()->split(geoId, splitPoint);
    for (int iterGeoId = 0; iterGeoId < getObject()->getHighestCurveIndex(); ++iterGeoId) {
        getObject()->deleteUnusedInternalGeometryAndUpdateGeoId(iterGeoId);
    }

    // Assert
    EXPECT_EQ(result, 0);
    // The arcOfConic should be split into two arcs of the same conic now
    EXPECT_EQ(getObject()->getHighestCurveIndex(), 1);
    // Expect the end points of the resultant curve are coincident.
    int numberOfCoincidentConstraints = countConstraintsOfType(getObject(), Sketcher::Coincident);
    EXPECT_EQ(numberOfCoincidentConstraints, 1);
}

TEST_F(SketchObjectTest, testSplitNonPeriodicBSpline)
{
    // Arrange
    auto nonPeriodicBSpline = createTypicalNonPeriodicBSpline();
    Base::Vector3d splitPoint(-0.5, 1.1, 0.0);
    int geoId = getObject()->addGeometry(nonPeriodicBSpline.get());
    // TODO: Put a point on this

    // Act
    // TODO: sample before point(s) at a random parameter
    int result = getObject()->split(geoId, splitPoint);
    for (int iterGeoId = 0; iterGeoId < getObject()->getHighestCurveIndex(); ++iterGeoId) {
        getObject()->deleteUnusedInternalGeometryAndUpdateGeoId(iterGeoId);
    }

    // Assert
    EXPECT_EQ(result, 0);
    EXPECT_EQ(getObject()->getHighestCurveIndex(), 1);
    // TODO: confirm sampled point(s) is/are at the same place
    int numberOfCoincidentConstraints = countConstraintsOfType(getObject(), Sketcher::Coincident);
    EXPECT_EQ(numberOfCoincidentConstraints, 1);
}

TEST_F(SketchObjectTest, testSplitPeriodicBSpline)
{
    // Arrange
    auto PeriodicBSpline = createTypicalPeriodicBSpline();
    Base::Vector3d splitPoint(-0.5, 1.1, 0.0);
    int geoId = getObject()->addGeometry(PeriodicBSpline.get());
    // TODO: Put a point on this

    // Act
    // TODO: sample before point(s) at a random parameter
    int result = getObject()->split(geoId, splitPoint);
    for (int iterGeoId = 0; iterGeoId < getObject()->getHighestCurveIndex(); ++iterGeoId) {
        getObject()->deleteUnusedInternalGeometryAndUpdateGeoId(iterGeoId);
    }

    // Assert
    EXPECT_EQ(result, 0);
    EXPECT_EQ(getObject()->getHighestCurveIndex(), 0);
    // TODO: confirm sampled point(s) is/are at the same place
}

TEST_F(SketchObjectTest, testTrimWithoutIntersection)
{
    // Arrange
    Part::GeomLineSegment lineSeg;
    setupLineSegment(lineSeg);
    int geoId = getObject()->addGeometry(&lineSeg);
    Base::Vector3d trimPoint(2.0, 3.1, 0.0);

    // Act
    int result = getObject()->trim(geoId, trimPoint);

    // Assert
    EXPECT_EQ(result, 0);
    // Once this line segment is trimmed, nothing should remain
    EXPECT_EQ(getObject()->getHighestCurveIndex(), geoId - 1);
}

// TODO: There are other combinations of constraints we may want to test with trim.

TEST_F(SketchObjectTest, testTrimLineSegmentEnd)
{
    // Arrange
    Part::GeomLineSegment lineSeg;
    setupLineSegment(lineSeg);
    // create curves intersecting at the right spots
    Base::Vector3d trimPoint(getPointAtNormalizedParameter(lineSeg, 0.2));
    Base::Vector3d p1(getPointAtNormalizedParameter(lineSeg, 0.5));
    Base::Vector3d p2(p1.x + 0.1, p1.y + 0.1, p1.z);
    Part::GeomLineSegment lineSegCut1;
    lineSegCut1.setPoints(p1, p2);
    getObject()->addGeometry(&lineSegCut1);
    int geoId = getObject()->addGeometry(&lineSeg);

    // Act
    int result = getObject()->trim(geoId, trimPoint);

    // Assert
    EXPECT_EQ(result, 0);
    // TODO: Once this line segment is trimmed, the curve should be "smaller"
    EXPECT_EQ(getObject()->getHighestCurveIndex(), geoId);
    // TODO: There should be a "point-on-object" constraint on the intersecting curves
    int numberOfCoincidentConstraints = countConstraintsOfType(getObject(), Sketcher::Coincident);
    EXPECT_EQ(numberOfCoincidentConstraints, 1);
}

TEST_F(SketchObjectTest, testTrimLineSegmentMid)
{
    // Arrange
    Part::GeomLineSegment lineSeg;
    setupLineSegment(lineSeg);
    // TODO: create curves intersecting at the right spots
    Base::Vector3d trimPoint(getPointAtNormalizedParameter(lineSeg, 0.5));
    Base::Vector3d p1(getPointAtNormalizedParameter(lineSeg, 0.3));
    Base::Vector3d p2(p1.x + 0.1, p1.y + 0.1, p1.z);
    Part::GeomLineSegment lineSegCut1;
    lineSegCut1.setPoints(p1, p2);
    getObject()->addGeometry(&lineSegCut1);
    Base::Vector3d p3(getPointAtNormalizedParameter(lineSeg, 0.7));
    Base::Vector3d p4(p3.x + 0.1, p3.y - 0.1, p3.z);
    // to ensure that this line clearly intersects the curve, not just have a point on object
    // without explicit constraint
    p3.x -= 0.1;
    p3.y += 0.1;
    Part::GeomLineSegment lineSegCut2;
    lineSegCut2.setPoints(p3, p4);
    getObject()->addGeometry(&lineSegCut2);
    int geoId = getObject()->addGeometry(&lineSeg);

    // Act
    int result = getObject()->trim(geoId, trimPoint);

    // Assert
    EXPECT_EQ(result, 0);
    // TODO: Once this line segment is trimmed, there should be two "smaller" curves in its place
    EXPECT_EQ(getObject()->getHighestCurveIndex(), geoId + 1);
    // TODO: There should be a "point-on-object" constraint on the intersecting curves
    int numberOfPointOnObjectConstraints =
        countConstraintsOfType(getObject(), Sketcher::PointOnObject);
    EXPECT_EQ(numberOfPointOnObjectConstraints, 1);
    int numberOfCoincidentConstraints = countConstraintsOfType(getObject(), Sketcher::Coincident);
    EXPECT_EQ(numberOfCoincidentConstraints, 1);
    // TODO: Ensure shape is preserved
}

TEST_F(SketchObjectTest, testTrimCircleEnd)
{
    // Arrange
    Part::GeomCircle circle;
    setupCircle(circle);
    // create curves intersecting at the right spots
    Base::Vector3d trimPoint(getPointAtNormalizedParameter(circle, 0.2));
    Base::Vector3d p1(getPointAtNormalizedParameter(circle, 0.5));
    Base::Vector3d p2(p1.x + 0.1, p1.y + 0.1, p1.z);
    Part::GeomLineSegment lineSegCut1;
    lineSegCut1.setPoints(p1, p2);
    getObject()->addGeometry(&lineSegCut1);
    int geoId = getObject()->addGeometry(&circle);

    // Act
    int result = getObject()->trim(geoId, trimPoint);

    // Assert
    EXPECT_EQ(result, 0);
    // TODO: Once this circle is trimmed, the circle should be deleted.
    EXPECT_EQ(getObject()->getHighestCurveIndex(), geoId - 1);
}

TEST_F(SketchObjectTest, testTrimCircleMid)
{
    // Arrange
    Part::GeomCircle circle;
    setupCircle(circle);
    // TODO: create curves intersecting at the right spots
    Base::Vector3d trimPoint(getPointAtNormalizedParameter(circle, 0.5));
    Base::Vector3d p1(getPointAtNormalizedParameter(circle, 0.3));
    Base::Vector3d p2(p1.x + 0.1, p1.y + 0.1, p1.z);
    Part::GeomLineSegment lineSegCut1;
    lineSegCut1.setPoints(p1, p2);
    getObject()->addGeometry(&lineSegCut1);
    Base::Vector3d p3(getPointAtNormalizedParameter(circle, 0.7));
    Base::Vector3d p4(p3.x + 0.1, p3.y + 0.1, p3.z);
    // to ensure that this line clearly intersects the curve, not just have a point on object
    // without explicit constraint
    p3.x -= 0.1;
    p3.y -= 0.1;
    Part::GeomLineSegment lineSegCut2;
    lineSegCut2.setPoints(p3, p4);
    getObject()->addGeometry(&lineSegCut2);
    int geoId = getObject()->addGeometry(&circle);

    // Act
    int result = getObject()->trim(geoId, trimPoint);

    // Assert
    EXPECT_EQ(result, 0);
    // TODO: Once this circle is trimmed, there should be one arc.
    EXPECT_EQ(getObject()->getHighestCurveIndex(), geoId);
    // There should be one "coincident" and one "point-on-object" constraint on the intersecting
    // curves
    int numberOfPointOnObjectConstraints =
        countConstraintsOfType(getObject(), Sketcher::PointOnObject);
    EXPECT_EQ(numberOfPointOnObjectConstraints, 1);
    int numberOfCoincidentConstraints = countConstraintsOfType(getObject(), Sketcher::Coincident);
    EXPECT_EQ(numberOfCoincidentConstraints, 1);
    // TODO: Ensure shape is preserved
}

TEST_F(SketchObjectTest, testTrimArcOfCircleEnd)
{
    // This should also cover as a representative of arc of conic

    // Arrange
    Part::GeomArcOfCircle arcOfCircle;
    setupArcOfCircle(arcOfCircle);
    // create curves intersecting at the right spots
    Base::Vector3d trimPoint(getPointAtNormalizedParameter(arcOfCircle, 0.2));
    Base::Vector3d p1(getPointAtNormalizedParameter(arcOfCircle, 0.5));
    Base::Vector3d p2(p1.x + 0.1, p1.y + 0.1, p1.z);
    Part::GeomLineSegment lineSegCut1;
    lineSegCut1.setPoints(p1, p2);
    getObject()->addGeometry(&lineSegCut1);
    int geoId = getObject()->addGeometry(&arcOfCircle);

    // Act
    int result = getObject()->trim(geoId, trimPoint);

    // Assert
    EXPECT_EQ(result, 0);
    EXPECT_EQ(getObject()->getHighestCurveIndex(), geoId);
    // There should be a "point-on-object" constraint on the intersecting curves
    int numberOfCoincidentConstraints = countConstraintsOfType(getObject(), Sketcher::Coincident);
    EXPECT_EQ(numberOfCoincidentConstraints, 1);
}

TEST_F(SketchObjectTest, testTrimArcOfCircleMid)
{
    // Arrange
    Part::GeomArcOfCircle arcOfCircle;
    setupArcOfCircle(arcOfCircle);
    // create curves intersecting at the right spots
    Base::Vector3d trimPoint(getPointAtNormalizedParameter(arcOfCircle, 0.5));
    Base::Vector3d p1(getPointAtNormalizedParameter(arcOfCircle, 0.3));
    Base::Vector3d p2(p1.x + 0.1, p1.y + 0.1, p1.z);
    Part::GeomLineSegment lineSegCut1;
    lineSegCut1.setPoints(p1, p2);
    getObject()->addGeometry(&lineSegCut1);
    Base::Vector3d p3(getPointAtNormalizedParameter(arcOfCircle, 0.7));
    Base::Vector3d p4(p3.x + 0.1, p3.y + 0.1, p3.z);
    // to ensure that this line clearly intersects the curve, not just have a point on object
    // without explicit constraint
    p3.x -= 0.1;
    p3.y -= 0.1;
    Part::GeomLineSegment lineSegCut2;
    lineSegCut2.setPoints(p3, p4);
    getObject()->addGeometry(&lineSegCut2);
    int geoId = getObject()->addGeometry(&arcOfCircle);

    // Act
    int result = getObject()->trim(geoId, trimPoint);

    // Assert
    EXPECT_EQ(result, 0);
    EXPECT_EQ(getObject()->getHighestCurveIndex(), geoId + 1);
    // There should be a "point-on-object" constraint on the intersecting curves
    int numberOfPointOnObjectConstraints =
        countConstraintsOfType(getObject(), Sketcher::PointOnObject);
    EXPECT_EQ(numberOfPointOnObjectConstraints, 1);
    // There should be 2 coincident constraints: one with lineSegCut1 and one between centers of the
    // new arcs
    int numberOfCoincidentConstraints = countConstraintsOfType(getObject(), Sketcher::Coincident);
    EXPECT_EQ(numberOfCoincidentConstraints, 2);
    // TODO: Ensure shape is preserved
}

TEST_F(SketchObjectTest, testTrimEllipseEnd)
{
    // Arrange
    Part::GeomEllipse ellipse;
    setupEllipse(ellipse);
    // create curves intersecting at the right spots
    Base::Vector3d trimPoint(getPointAtNormalizedParameter(ellipse, 0.2));
    Base::Vector3d p1(getPointAtNormalizedParameter(ellipse, 0.5));
    Base::Vector3d p2(p1.x + 0.1, p1.y + 0.1, p1.z);
    Part::GeomLineSegment lineSegCut1;
    lineSegCut1.setPoints(p1, p2);
    getObject()->addGeometry(&lineSegCut1);
    int geoId = getObject()->addGeometry(&ellipse);

    // Act
    int result = getObject()->trim(geoId, trimPoint);
    // remove all internal geometry
    for (int iterGeoId = 0; iterGeoId < getObject()->getHighestCurveIndex(); ++iterGeoId) {
        getObject()->deleteUnusedInternalGeometryAndUpdateGeoId(iterGeoId);
    }

    // Assert
    EXPECT_EQ(result, 0);
    // Once this ellipse is trimmed, the ellipse should be deleted.
    // Only remaining: line segment
    EXPECT_EQ(getObject()->getHighestCurveIndex(), 0);
}

TEST_F(SketchObjectTest, testTrimEllipseMid)
{
    // Arrange
    Part::GeomEllipse ellipse;
    setupEllipse(ellipse);
    // create curves intersecting at the right spots
    Base::Vector3d trimPoint(getPointAtNormalizedParameter(ellipse, 0.5));
    Base::Vector3d p1(getPointAtNormalizedParameter(ellipse, 0.3));
    Base::Vector3d p2(p1.x + 0.1, p1.y + 0.1, p1.z);
    Part::GeomLineSegment lineSegCut1;
    lineSegCut1.setPoints(p1, p2);
    getObject()->addGeometry(&lineSegCut1);
    Base::Vector3d p3(getPointAtNormalizedParameter(ellipse, 0.7));
    Base::Vector3d p4(p3.x + 0.1, p3.y + 0.1, p3.z);
    // to ensure that this line clearly intersects the curve, not just have a point on object
    // without explicit constraint
    p3.x -= 0.1;
    p3.y -= 0.1;
    Part::GeomLineSegment lineSegCut2;
    lineSegCut2.setPoints(p3, p4);
    getObject()->addGeometry(&lineSegCut2);
    int geoId = getObject()->addGeometry(&ellipse);
    // FIXME: Doing this to avoid trimming only until minor/major axes. Should not be needed.
    getObject()->deleteUnusedInternalGeometry(geoId);

    // Act
    int result = getObject()->trim(geoId, trimPoint);
    // remove all internal geometry
    for (int iterGeoId = 0; iterGeoId < getObject()->getHighestCurveIndex(); ++iterGeoId) {
        getObject()->deleteUnusedInternalGeometryAndUpdateGeoId(iterGeoId);
    }

    // Assert
    EXPECT_EQ(result, 0);
    // Once this ellipse is trimmed, there should be one arc and line segments.
    EXPECT_EQ(getObject()->getHighestCurveIndex(), 2);
    // There should be one "coincident" and one "point-on-object" constraint on the intersecting
    // curves
    int numberOfPointOnObjectConstraints =
        countConstraintsOfType(getObject(), Sketcher::PointOnObject);
    EXPECT_EQ(numberOfPointOnObjectConstraints, 1);
    int numberOfCoincidentConstraints = countConstraintsOfType(getObject(), Sketcher::Coincident);
    EXPECT_EQ(numberOfCoincidentConstraints, 1);
    // TODO: Ensure shape is preserved
}

// TODO: Tests for other arcs of conics?

TEST_F(SketchObjectTest, testTrimPeriodicBSplineEnd)
{
    // Arrange
    auto periodicBSpline = createTypicalPeriodicBSpline();
    assert(periodicBSpline);
    // create curves intersecting at the right spots
    Base::Vector3d trimPoint(getPointAtNormalizedParameter(*periodicBSpline, 0.2));
    Base::Vector3d p1(getPointAtNormalizedParameter(*periodicBSpline, 0.5));
    Base::Vector3d p2(p1.x + 0.1, p1.y + 0.1, p1.z);
    Part::GeomLineSegment lineSegCut1;
    lineSegCut1.setPoints(p1, p2);
    getObject()->addGeometry(&lineSegCut1);
    int geoId = getObject()->addGeometry(periodicBSpline.get());

    // Act
    int result = getObject()->trim(geoId, trimPoint);

    // Assert
    EXPECT_EQ(result, 0);
    // FIXME: This will fail because of deleted internal geometry
    // Once this periodicBSpline is trimmed, the periodicBSpline should be deleted, leaving only the
    // line segment.
    EXPECT_EQ(getObject()->getHighestCurveIndex(), 0);
    // TODO: There should be a "point-on-object" constraint on the intersecting curves
}

TEST_F(SketchObjectTest, testTrimPeriodicBSplineMid)
{
    // Arrange
    auto periodicBSpline = createTypicalPeriodicBSpline();
    assert(periodicBSpline);
    // create curves intersecting at the right spots
    Base::Vector3d trimPoint(getPointAtNormalizedParameter(*periodicBSpline, 0.5));
    Base::Vector3d p1(getPointAtNormalizedParameter(*periodicBSpline, 0.3));
    Base::Vector3d p2(p1.x + 0.1, p1.y + 0.1, p1.z);
    Part::GeomLineSegment lineSegCut1;
    lineSegCut1.setPoints(p1, p2);
    getObject()->addGeometry(&lineSegCut1);
    Base::Vector3d p3(getPointAtNormalizedParameter(*periodicBSpline, 0.7));
    Base::Vector3d p4(p3.x + 0.1, p3.y + 0.1, p3.z);
    // to ensure that this line clearly intersects the curve, not just have a point on object
    // without explicit constraint
    p3.x -= 0.1;
    p3.y -= 0.1;
    Part::GeomLineSegment lineSegCut2;
    lineSegCut2.setPoints(p3, p4);
    getObject()->addGeometry(&lineSegCut2);
    int geoId = getObject()->addGeometry(periodicBSpline.get());

    // Act
    int result = getObject()->trim(geoId, trimPoint);
    // remove all internal geometry
    for (int iterGeoId = 0; iterGeoId < getObject()->getHighestCurveIndex(); ++iterGeoId) {
        getObject()->deleteUnusedInternalGeometryAndUpdateGeoId(iterGeoId);
    }

    // Assert
    EXPECT_EQ(result, 0);
    // Only remaining: Two line segments and the B-spline
    EXPECT_EQ(getObject()->getHighestCurveIndex(), 2);
    // There should be one "coincident" and one "point-on-object" constraint on the intersecting
    // curves
    int numberOfPointOnObjectConstraints =
        countConstraintsOfType(getObject(), Sketcher::PointOnObject);
    EXPECT_EQ(numberOfPointOnObjectConstraints, 1);
    int numberOfCoincidentConstraints = countConstraintsOfType(getObject(), Sketcher::Coincident);
    EXPECT_EQ(numberOfCoincidentConstraints, 1);
    // TODO: Ensure shape is preserved
}

TEST_F(SketchObjectTest, testTrimNonPeriodicBSplineEnd)
{
    // This should also cover as a representative of arc of conic

    // Arrange
    auto nonPeriodicBSpline = createTypicalNonPeriodicBSpline();
    assert(nonPeriodicBSpline);
    // create curves intersecting at the right spots
    Base::Vector3d trimPoint(getPointAtNormalizedParameter(*nonPeriodicBSpline, 0.2));
    Base::Vector3d p1(getPointAtNormalizedParameter(*nonPeriodicBSpline, 0.5));
    Base::Vector3d p2(p1.x + 0.1, p1.y + 0.1, p1.z);
    Part::GeomLineSegment lineSegCut1;
    lineSegCut1.setPoints(p1, p2);
    getObject()->addGeometry(&lineSegCut1);
    int geoId = getObject()->addGeometry(nonPeriodicBSpline.get());

    // Act
    int result = getObject()->trim(geoId, trimPoint);
    // remove all internal geometry
    for (int iterGeoId = 0; iterGeoId < getObject()->getHighestCurveIndex(); ++iterGeoId) {
        getObject()->deleteUnusedInternalGeometryAndUpdateGeoId(iterGeoId);
    }

    // Assert
    EXPECT_EQ(result, 0);
    // Only remaining: one line segment and the trimmed B-spline
    EXPECT_EQ(getObject()->getHighestCurveIndex(), 1);
    // FIXME: There should be a "point-on-object" constraint on the intersecting curves
    int numberOfCoincidentConstraints = countConstraintsOfType(getObject(), Sketcher::Coincident);
    EXPECT_EQ(numberOfCoincidentConstraints, 1);
}

TEST_F(SketchObjectTest, testTrimNonPeriodicBSplineMid)
{
    // Arrange
    auto nonPeriodicBSpline = createTypicalNonPeriodicBSpline();
    assert(nonPeriodicBSpline);
    // create curves intersecting at the right spots
    Base::Vector3d trimPoint(getPointAtNormalizedParameter(*nonPeriodicBSpline, 0.5));
    Base::Vector3d p1(getPointAtNormalizedParameter(*nonPeriodicBSpline, 0.3));
    Base::Vector3d p2(p1.x + 0.1, p1.y + 0.1, p1.z);
    Part::GeomLineSegment lineSegCut1;
    lineSegCut1.setPoints(p1, p2);
    getObject()->addGeometry(&lineSegCut1);
    Base::Vector3d p3(getPointAtNormalizedParameter(*nonPeriodicBSpline, 0.7));
    Base::Vector3d p4(p3.x + 0.1, p3.y + 0.1, p3.z);
    // to ensure that this line clearly intersects the curve, not just have a point on object
    // without explicit constraint
    p3.x -= 0.1;
    p3.y -= 0.1;
    Part::GeomLineSegment lineSegCut2;
    lineSegCut2.setPoints(p3, p4);
    getObject()->addGeometry(&lineSegCut2);
    int geoId = getObject()->addGeometry(nonPeriodicBSpline.get());

    // Act
    int result = getObject()->trim(geoId, trimPoint);
    // remove all internal geometry
    for (int i = 0; i < getObject()->getHighestCurveIndex(); ++i) {
        if (getObject()->getGeometry(i)->is<Part::GeomBSplineCurve>()) {
            getObject()->deleteUnusedInternalGeometry(i);
        }
    }

    // Assert
    EXPECT_EQ(result, 0);
    // Only remaining: one line segment and the trimmed B-spline
    EXPECT_EQ(getObject()->getHighestCurveIndex(), 3);
    // There should be a "point-on-object" constraint on the intersecting curves
    int numberOfPointOnObjectConstraints =
        countConstraintsOfType(getObject(), Sketcher::PointOnObject);
    EXPECT_EQ(numberOfPointOnObjectConstraints, 1);
    int numberOfCoincidentConstraints = countConstraintsOfType(getObject(), Sketcher::Coincident);
    EXPECT_EQ(numberOfCoincidentConstraints, 1);
    // TODO: Ensure shape is preserved
}

TEST_F(SketchObjectTest, testTrimEffectOnConstruction)
{
    // Ensure construction geometries remain construction
    // The issue #12715 was usually happening when the geometry changed type, which
    // currently only happens for circles and ellipses.

    // Arrange
    Part::GeomCircle circle;
    setupCircle(circle);
    // TODO: create curves intersecting at the right spots
    Base::Vector3d trimPoint(getPointAtNormalizedParameter(circle, 0.5));
    Base::Vector3d p1(getPointAtNormalizedParameter(circle, 0.3));
    Base::Vector3d p2(p1.x + 0.1, p1.y + 0.1, p1.z);
    Part::GeomLineSegment lineSegCut1;
    lineSegCut1.setPoints(p1, p2);
    getObject()->addGeometry(&lineSegCut1);
    Base::Vector3d p3(getPointAtNormalizedParameter(circle, 0.7));
    Base::Vector3d p4(p3.x + 0.1, p3.y + 0.1, p3.z);
    // to ensure that this line clearly intersects the curve, not just have a point on object
    // without explicit constraint
    p3.x -= 0.1;
    p3.y -= 0.1;
    Part::GeomLineSegment lineSegCut2;
    lineSegCut2.setPoints(p3, p4);
    getObject()->addGeometry(&lineSegCut2);
    int geoId = getObject()->addGeometry(&circle, true);

    // Act
    int result = getObject()->trim(geoId, trimPoint);

    // Assert
    EXPECT_EQ(result, 0);
    for (int i = 0; i < getObject()->getHighestCurveIndex(); ++i) {
        auto* geom = getObject()->getGeometry(i);
        if (geom->is<Part::GeomArcOfCircle>()) {
            EXPECT_TRUE(GeometryFacade::getConstruction(geom));
        }
    }
}

TEST_F(SketchObjectTest, testTrimEndEffectOnFullLengthConstraints)
{
    // Ensure constraints directly applying to full length disappear if one of the ends disappears.

    // Arrange
    Part::GeomLineSegment lineSeg;
    setupLineSegment(lineSeg);
    // TODO: create curves intersecting at the right spots
    Base::Vector3d trimPoint(getPointAtNormalizedParameter(lineSeg, 0.2));
    Base::Vector3d p1(getPointAtNormalizedParameter(lineSeg, 0.5));
    Base::Vector3d p2(p1.x + 0.1, p1.y + 0.1, p1.z);
    Part::GeomLineSegment lineSegCut1;
    lineSegCut1.setPoints(p1, p2);
    getObject()->addGeometry(&lineSegCut1);
    int geoId = getObject()->addGeometry(&lineSeg);
    auto constr = new Sketcher::Constraint();  // Ownership will be transferred to the sketch
    constr->Type = Sketcher::ConstraintType::Distance;
    constr->First = geoId;
    constr->FirstPos = Sketcher::PointPos::none;
    constr->setValue((getObject()->getPoint(geoId, Sketcher::PointPos::end)
                      - getObject()->getPoint(geoId, Sketcher::PointPos::start))
                         .Length());
    getObject()->addConstraint(constr);

    // Assert
    EXPECT_EQ(countConstraintsOfType(getObject(), Sketcher::ConstraintType::Distance), 1);

    // Act
    int result = getObject()->trim(geoId, trimPoint);

    // Assert
    EXPECT_EQ(result, 0);
    EXPECT_EQ(countConstraintsOfType(getObject(), Sketcher::ConstraintType::Distance), 0);
}

TEST_F(SketchObjectTest, testTrimEndEffectOnSymmetricConstraints)
{
    // Ensure symmetric constraints go away

    // Arrange
    Part::GeomLineSegment lineSeg;
    setupLineSegment(lineSeg);
    // create curves intersecting at the right spots
    Base::Vector3d trimPoint(getPointAtNormalizedParameter(lineSeg, 0.2));
    Base::Vector3d p1(getPointAtNormalizedParameter(lineSeg, 0.5));
    Base::Vector3d p2(p1.x + 0.1, p1.y + 0.1, p1.z);
    Part::GeomLineSegment lineSegCut1;
    lineSegCut1.setPoints(p1, p2);
    int geoIdOfCutting = getObject()->addGeometry(&lineSegCut1);
    int geoId = getObject()->addGeometry(&lineSeg);
    auto constr = new Sketcher::Constraint();  // Ownership will be transferred to the sketch
    constr->Type = Sketcher::ConstraintType::Symmetric;
    constr->First = geoId;
    constr->FirstPos = Sketcher::PointPos::start;
    constr->Second = geoId;
    constr->SecondPos = Sketcher::PointPos::end;
    constr->Third = geoIdOfCutting;
    constr->ThirdPos = Sketcher::PointPos::start;
    getObject()->addConstraint(constr);

    // Assert
    EXPECT_EQ(countConstraintsOfType(getObject(), Sketcher::ConstraintType::Symmetric), 1);

    // Act
    int result = getObject()->trim(geoId, trimPoint);

    // Assert
    EXPECT_EQ(result, 0);
    EXPECT_EQ(countConstraintsOfType(getObject(), Sketcher::ConstraintType::Symmetric), 0);
}

TEST_F(SketchObjectTest, testTrimEndEffectOnUnrelatedTangent)
{
    // See https://github.com/AstoCAD/FreeCAD/issues/24

    // Arrange
    Part::GeomLineSegment lineSeg;
    lineSeg.setPoints(Base::Vector3d {1.0, -2.0, 0.0}, Base::Vector3d {1.0, 2.0, 0.0});
    Base::Vector3d trimPoint(getPointAtNormalizedParameter(lineSeg, 0.2));
    int geoId = getObject()->addGeometry(&lineSeg);
    Part::GeomCircle innerCircle;
    innerCircle.setCenter(Base::Vector3d {0.0, 0.0, 0.0});
    innerCircle.setRadius(1.0);
    int geoIdInnerCircle = getObject()->addGeometry(&innerCircle);
    Part::GeomCircle outerCircle;
    outerCircle.setCenter(Base::Vector3d {0.0, 0.0, 0.0});
    outerCircle.setRadius(1.5);
    getObject()->addGeometry(&outerCircle);  // no need to save
    // TODO: add tangent and confirm
    auto constraint = new Sketcher::Constraint();  // Ownership will be transferred to the sketch
    constraint->Type = Sketcher::ConstraintType::Tangent;
    constraint->First = geoId;
    constraint->FirstPos = Sketcher::PointPos::none;
    constraint->Second = geoIdInnerCircle;
    constraint->SecondPos = Sketcher::PointPos::none;
    getObject()->addConstraint(constraint);
    EXPECT_EQ(countConstraintsOfType(getObject(), Sketcher::ConstraintType::Tangent), 1);

    // Act
    int result = getObject()->trim(geoId, trimPoint);

    // Assert
    EXPECT_EQ(result, 0);
    // TODO: find tangent and confirm nature
    const auto& constraints = getObject()->Constraints.getValues();
    auto tangIt = std::ranges::find(constraints,
                                    Sketcher::ConstraintType::Tangent,
                                    &Sketcher::Constraint::Type);
    EXPECT_NE(tangIt, constraints.end());
    EXPECT_EQ((*tangIt)->FirstPos, Sketcher::PointPos::none);
    EXPECT_EQ((*tangIt)->SecondPos, Sketcher::PointPos::none);
}

// TODO: Ensure endpoint constraints go to the appropriate new geometry
// This will need a reliable way to get the resultant curves after the trim

TEST_F(SketchObjectTest, testModifyKnotMultInNonPeriodicBSplineToZero)
{
    // Arrange
    auto nonPeriodicBSpline = createTypicalNonPeriodicBSpline();
    assert(nonPeriodicBSpline);
    int geoId = getObject()->addGeometry(nonPeriodicBSpline.get());
    auto bsp1 = static_cast<const Part::GeomBSplineCurve*>(getObject()->getGeometry(geoId));
    int oldKnotCount = bsp1->countKnots();

    // Act
    // Try decreasing mult to zero.
    // NOTE: we still use OCCT notation of knot index starting with 1 (not 0).
    getObject()->modifyBSplineKnotMultiplicity(geoId, 2, -1);
    // Assert
    // Knot should disappear. We start with 3 (unique) knots, so expect 2.
    auto bsp2 = static_cast<const Part::GeomBSplineCurve*>(getObject()->getGeometry(geoId));
    EXPECT_EQ(bsp2->countKnots(), oldKnotCount - 1);
}

TEST_F(SketchObjectTest, testModifyKnotMultInNonPeriodicBSplineToDisallowed)
{
    // Arrange
    auto nonPeriodicBSpline = createTypicalNonPeriodicBSpline();
    assert(nonPeriodicBSpline);
    int geoId = getObject()->addGeometry(nonPeriodicBSpline.get());

    // Act and Assert
    // TODO: Try modifying such that resultant multiplicity > degree
    // TODO: This should immediately throw exception
    EXPECT_THROW(getObject()->modifyBSplineKnotMultiplicity(geoId, 2, 3), Base::ValueError);
    // TODO: Try modifying such that resultant multiplicity < 0
    // TODO: This should immediately throw exception
    EXPECT_THROW(getObject()->modifyBSplineKnotMultiplicity(geoId, 2, -2), Base::ValueError);
}

TEST_F(SketchObjectTest, testModifyKnotMultInNonPeriodicBSpline)
{
    // Arrange
    auto nonPeriodicBSpline = createTypicalNonPeriodicBSpline();
    assert(nonPeriodicBSpline);
    int geoId = getObject()->addGeometry(nonPeriodicBSpline.get());

    auto bsp = static_cast<const Part::GeomBSplineCurve*>(getObject()->getGeometry(geoId));
    int oldKnotsNum = bsp->countKnots();
    int oldMultiplicityOfTargetKnot = bsp->getMultiplicities()[1];

    // Act
    // TODO: Increase/decrease knot multiplicity normally
    getObject()->modifyBSplineKnotMultiplicity(geoId, 2, 1);
    // Assert
    // This should not alter the sizes of knot and multiplicity vectors.
    bsp = static_cast<const Part::GeomBSplineCurve*>(getObject()->getGeometry(geoId));
    EXPECT_EQ(bsp->countKnots(), oldKnotsNum);
    // This should increment the multiplicity.
    EXPECT_EQ(bsp->getMultiplicities()[1], oldMultiplicityOfTargetKnot + 1);
    // This should still be a non-periodic spline
    EXPECT_FALSE(bsp->isPeriodic());
    // TODO: Expect shape is preserved

    // Act
    // TODO: Increase/decrease knot multiplicity normally
    getObject()->modifyBSplineKnotMultiplicity(geoId, 2, -1);
    // Assert
    // This should not alter the sizes of knot and multiplicity vectors.
    bsp = static_cast<const Part::GeomBSplineCurve*>(getObject()->getGeometry(geoId));
    EXPECT_EQ(bsp->countKnots(), oldKnotsNum);
    // This should increment the multiplicity.
    EXPECT_EQ(bsp->getMultiplicities()[1], oldMultiplicityOfTargetKnot);
    // This should still be a non-periodic spline
    EXPECT_FALSE(bsp->isPeriodic());
}

TEST_F(SketchObjectTest, testModifyKnotMultInPeriodicBSplineToZero)
{
    // Arrange
    auto PeriodicBSpline = createTypicalPeriodicBSpline();
    assert(PeriodicBSpline);
    int geoId = getObject()->addGeometry(PeriodicBSpline.get());
    auto bsp1 = static_cast<const Part::GeomBSplineCurve*>(getObject()->getGeometry(geoId));
    int oldKnotCount = bsp1->countKnots();

    // Act
    // Try decreasing mult to zero.
    // NOTE: we still use OCCT notation of knot index starting with 1 (not 0).
    getObject()->modifyBSplineKnotMultiplicity(geoId, 2, -1);
    // Assert
    // Knot should disappear.
    auto bsp2 = static_cast<const Part::GeomBSplineCurve*>(getObject()->getGeometry(geoId));
    EXPECT_EQ(bsp2->countKnots(), oldKnotCount - 1);
}

TEST_F(SketchObjectTest, testModifyKnotMultInPeriodicBSplineToDisallowed)
{
    // Arrange
    auto PeriodicBSpline = createTypicalPeriodicBSpline();
    assert(PeriodicBSpline);
    int geoId = getObject()->addGeometry(PeriodicBSpline.get());

    // Act and Assert
    // TODO: Try modifying such that resultant multiplicity > degree
    // TODO: This should immediately throw exception
    EXPECT_THROW(getObject()->modifyBSplineKnotMultiplicity(geoId, 2, 3), Base::ValueError);
    // TODO: Try modifying such that resultant multiplicity < 0
    // TODO: This should immediately throw exception
    EXPECT_THROW(getObject()->modifyBSplineKnotMultiplicity(geoId, 2, -2), Base::ValueError);
}

TEST_F(SketchObjectTest, testModifyKnotMultInPeriodicBSpline)
{
    // Arrange
    auto PeriodicBSpline = createTypicalPeriodicBSpline();
    assert(PeriodicBSpline);
    int geoId = getObject()->addGeometry(PeriodicBSpline.get());

    auto bsp = static_cast<const Part::GeomBSplineCurve*>(getObject()->getGeometry(geoId));
    int oldKnotsNum = bsp->countKnots();
    int oldMultiplicityOfTargetKnot = bsp->getMultiplicities()[1];

    // Act
    // TODO: Increase/decrease knot multiplicity normally
    getObject()->modifyBSplineKnotMultiplicity(geoId, 2, 1);
    // Assert
    // This should not alter the sizes of knot and multiplicity vectors.
    bsp = static_cast<const Part::GeomBSplineCurve*>(getObject()->getGeometry(geoId));
    EXPECT_EQ(bsp->countKnots(), oldKnotsNum);
    // This should increment the multiplicity.
    EXPECT_EQ(bsp->getMultiplicities()[1], oldMultiplicityOfTargetKnot + 1);
    // This should still be a periodic spline
    EXPECT_TRUE(bsp->isPeriodic());
    // TODO: Expect shape is preserved

    // Act
    // TODO: Increase/decrease knot multiplicity normally
    getObject()->modifyBSplineKnotMultiplicity(geoId, 2, -1);
    // Assert
    // This should not alter the sizes of knot and multiplicity vectors.
    bsp = static_cast<const Part::GeomBSplineCurve*>(getObject()->getGeometry(geoId));
    EXPECT_EQ(bsp->countKnots(), oldKnotsNum);
    // This should decrement the multiplicity.
    EXPECT_EQ(bsp->getMultiplicities()[1], oldMultiplicityOfTargetKnot);
    // This should still be a non-periodic spline
    EXPECT_TRUE(bsp->isPeriodic());
}

TEST_F(SketchObjectTest, testInsertKnotInNonPeriodicBSpline)
{
    // Arrange
    auto nonPeriodicBSpline = createTypicalNonPeriodicBSpline();
    assert(nonPeriodicBSpline);
    int geoId = getObject()->addGeometry(nonPeriodicBSpline.get());

    // Act and Assert
    // Try inserting knot with zero multiplicity
    // zero multiplicity knot should immediately throw exception
    EXPECT_THROW(getObject()->insertBSplineKnot(geoId, 0.5, 0), Base::ValueError);

    // Act and Assert
    // Try inserting knot with multiplicity > degree
    // This should immediately throw exception
    EXPECT_THROW(getObject()->insertBSplineKnot(geoId, 0.5, 4), Base::ValueError);

    // Act and Assert
    // TODO: Try inserting at an existing knot with resultant multiplicity > degree
    // TODO: This should immediately throw exception
    // FIXME: Not happening. May be ignoring existing values.
    // EXPECT_THROW(getObject()->insertBSplineKnot(geoId, 1.0, 3), Base::ValueError);

    auto bsp = static_cast<const Part::GeomBSplineCurve*>(getObject()->getGeometry(geoId));
    int oldKnotsNum = bsp->countKnots();
    int oldMultiplicityOfTargetKnot = bsp->getMultiplicities()[1];

    // Act
    // Add at a general position (where no knot exists)
    getObject()->insertBSplineKnot(geoId, 0.5, 1);
    // Assert
    // This should add to both the knot and multiplicity "vectors"
    bsp = static_cast<const Part::GeomBSplineCurve*>(getObject()->getGeometry(geoId));
    EXPECT_EQ(bsp->countKnots(), oldKnotsNum + 1);
    // This should still be a non-periodic spline
    EXPECT_FALSE(bsp->isPeriodic());

    // Act
    // Add a knot at an existing knot
    getObject()->insertBSplineKnot(geoId, 1.0, 1);
    // Assert
    // This should not alter the sizes of knot and multiplicity vectors.
    // (Since we previously added a knot, this means the total is still one more than original)
    bsp = static_cast<const Part::GeomBSplineCurve*>(getObject()->getGeometry(geoId));
    EXPECT_EQ(bsp->countKnots(), oldKnotsNum + 1);
    // This should increment the multiplicity.
    EXPECT_EQ(bsp->getMultiplicities()[2], oldMultiplicityOfTargetKnot + 1);
    // This should still be a non-periodic spline
    EXPECT_FALSE(bsp->isPeriodic());
}

TEST_F(SketchObjectTest, testInsertKnotInPeriodicBSpline)
{
    // This should also cover as a representative of arc of conic

    // Arrange
    auto PeriodicBSpline = createTypicalPeriodicBSpline();
    assert(PeriodicBSpline);
    int geoId = getObject()->addGeometry(PeriodicBSpline.get());

    // Act and Assert
    // Try inserting knot with zero multiplicity
    // zero multiplicity knot should immediately throw exception
    EXPECT_THROW(getObject()->insertBSplineKnot(geoId, 0.5, 0), Base::ValueError);

    // Act and Assert
    // Try inserting knot with multiplicity > degree
    // This should immediately throw exception
    EXPECT_THROW(getObject()->insertBSplineKnot(geoId, 0.5, 4), Base::ValueError);

    // Act and Assert
    // TODO: Try inserting at an existing knot with resultant multiplicity > degree
    // TODO: This should immediately throw exception

    auto bsp = static_cast<const Part::GeomBSplineCurve*>(getObject()->getGeometry(geoId));
    int oldKnotsNum = bsp->countKnots();
    int oldMultiplicityOfTargetKnot = bsp->getMultiplicities()[2];

    // Act
    // Add at a general position (where no knot exists)
    getObject()->insertBSplineKnot(geoId, 0.5, 1);
    // Assert
    // This should add to both the knot and multiplicity "vectors"
    bsp = static_cast<const Part::GeomBSplineCurve*>(getObject()->getGeometry(geoId));
    EXPECT_EQ(bsp->countKnots(), oldKnotsNum + 1);
    // This should still be a periodic spline
    EXPECT_TRUE(bsp->isPeriodic());

    // Act
    // Add a knot at an existing knot
    getObject()->insertBSplineKnot(geoId, 1.0, 1);
    // Assert
    // This should not alter the sizes of knot and multiplicity vectors.
    bsp = static_cast<const Part::GeomBSplineCurve*>(getObject()->getGeometry(geoId));
    EXPECT_EQ(bsp->countKnots(), oldKnotsNum + 1);
    // This should increment the multiplicity.
    EXPECT_EQ(bsp->getMultiplicities()[3], oldMultiplicityOfTargetKnot + 1);
    // This should still be a periodic spline
    EXPECT_TRUE(bsp->isPeriodic());
}

TEST_F(SketchObjectTest, testJoinCurves)
{
    // Arrange
    // Make two curves
    Base::Vector3d coordsCenter(0.0, 0.0, 0.0);
    double radius = 3.0, startParam = std::numbers::pi / 2, endParam = std::numbers::pi;
    Part::GeomArcOfCircle arcOfCircle;
    arcOfCircle.setCenter(coordsCenter);
    arcOfCircle.setRadius(radius);
    arcOfCircle.setRange(startParam, endParam, true);
    int geoId1 = getObject()->addGeometry(&arcOfCircle);

    Base::Vector3d coords1(0.1, 0.0, 0.0);
    Base::Vector3d coords2(3.0, 4.0, 0.0);
    Part::GeomLineSegment lineSeg;
    lineSeg.setPoints(coords1, coords2);
    int geoId2 = getObject()->addGeometry(&lineSeg);

    // Act
    // Join these curves
    getObject()->join(geoId1, Sketcher::PointPos::start, geoId2, Sketcher::PointPos::start);

    // Assert
    // Check they are replaced (here it means there is only one curve left after internal
    // geometries are removed)
    for (int iterGeoId = 0; iterGeoId < getObject()->getHighestCurveIndex(); ++iterGeoId) {
        getObject()->deleteUnusedInternalGeometryAndUpdateGeoId(iterGeoId);
    }
    EXPECT_EQ(getObject()->getHighestCurveIndex(), 0);
}

TEST_F(SketchObjectTest, testJoinCurvesWhenTangent)
{
    // Arrange
    // Make two curves
    Base::Vector3d coordsCenter(0.0, 0.0, 0.0);
    double radius = 3.0, startParam = std::numbers::pi / 2, endParam = std::numbers::pi;
    Part::GeomArcOfCircle arcOfCircle;
    arcOfCircle.setCenter(coordsCenter);
    arcOfCircle.setRadius(radius);
    arcOfCircle.setRange(startParam, endParam, true);
    int geoId1 = getObject()->addGeometry(&arcOfCircle);

    Base::Vector3d coords1(0.0, 0.0, 0.0);
    Base::Vector3d coords2(3.0, 0.0, 0.0);
    Part::GeomLineSegment lineSeg;
    lineSeg.setPoints(coords1, coords2);
    int geoId2 = getObject()->addGeometry(&lineSeg);

    // Add end-to-end tangent between these
    auto constraint = new Sketcher::Constraint();  // Ownership will be transferred to the sketch
    constraint->Type = Sketcher::ConstraintType::Tangent;
    constraint->First = geoId1;
    constraint->FirstPos = Sketcher::PointPos::start;
    constraint->Second = geoId2;
    constraint->SecondPos = Sketcher::PointPos::start;
    getObject()->addConstraint(constraint);

    // Act
    // Join these curves
    getObject()->join(geoId1, Sketcher::PointPos::start, geoId2, Sketcher::PointPos::start, 1);

    // Assert
    // Check they are replaced (here it means there is only one curve left after internal
    // geometries are removed)
    for (int iterGeoId = 0; iterGeoId < getObject()->getHighestCurveIndex(); ++iterGeoId) {
        getObject()->deleteUnusedInternalGeometryAndUpdateGeoId(iterGeoId);
    }
    EXPECT_EQ(getObject()->getHighestCurveIndex(), 0);
    // TODO: Check the shape is conserved (how?)
    // Check there is no C-0 knot (should be possible for the chosen example)
    auto mults = static_cast<const Part::GeomBSplineCurve*>(getObject()->getGeometry(0))
                     ->getMultiplicities();
    EXPECT_TRUE(std::all_of(mults.begin(), mults.end(), [](auto mult) {
        return mult >= 1;
    }));
}
