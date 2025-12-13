// SPDX-License-Identifier: LGPL-2.1-or-later

#include <cmath>
#include <numbers>

#include <gtest/gtest.h>

#include "Mod/Sketcher/App/planegcs/GCS.h"
#include "Mod/Sketcher/App/planegcs/Geo.h"
#include "Mod/Sketcher/App/planegcs/Constraints.h"

class SystemTest: public GCS::System
{
public:
    size_t getNumberOfConstraints(int tagID = -1)
    {
        return _getNumberOfConstraints(tagID);
    }
};

class ConstraintsTest: public ::testing::Test
{
protected:
    void SetUp() override
    {
        _system = std::make_unique<SystemTest>();
    }

    void TearDown() override
    {
        _system.reset();
    }

    SystemTest* System()
    {
        return _system.get();
    }

private:
    std::unique_ptr<SystemTest> _system;
};

TEST_F(ConstraintsTest, tangentBSplineAndArc)  // NOLINT
{
    // Arrange
    // TODO: Add arc, B-spline, and point
    double pointX = 3.5, arcStartX = 5.0, arcEndX = 0.0, arcCenterX = 0.0;
    double pointY = 3.5, arcStartY = 0.0, arcEndY = 5.0, arcCenterY = 0.0;
    GCS::Point point, arcStart, arcEnd, arcCenter;
    point.x = &pointX;
    point.y = &pointY;
    arcStart.x = &arcStartX;
    arcStart.y = &arcStartY;
    arcEnd.x = &arcEndX;
    arcEnd.y = &arcEndY;
    arcCenter.x = &arcCenterX;
    arcCenter.y = &arcCenterY;
    double arcRadius = 5.0, arcStartAngle = 0.0, arcEndAngle = std::numbers::pi / 2;
    double desiredAngle = std::numbers::pi;
    double bSplineStartX = 0.0, bSplineEndX = 16.0;
    double bSplineStartY = 10.0, bSplineEndY = -10.0;
    GCS::Point bSplineStart, bSplineEnd;
    bSplineStart.x = &bSplineStartX;
    bSplineStart.y = &bSplineStartY;
    bSplineEnd.x = &bSplineEndX;
    bSplineEnd.y = &bSplineEndY;
    std::vector<double> bSplineControlPointsX(5);
    std::vector<double> bSplineControlPointsY(5);
    bSplineControlPointsX[0] = 0.0;
    bSplineControlPointsY[0] = 10.0;
    bSplineControlPointsX[1] = 0.0;
    bSplineControlPointsY[1] = 6.0;
    bSplineControlPointsX[2] = 6.0;
    bSplineControlPointsY[2] = 0.5;
    bSplineControlPointsX[3] = 16.0;
    bSplineControlPointsY[3] = 0.5;
    bSplineControlPointsX[4] = 16.0;
    bSplineControlPointsY[4] = -10.0;
    std::vector<GCS::Point> bSplineControlPoints(5);
    for (size_t i = 0; i < bSplineControlPoints.size(); ++i) {
        bSplineControlPoints[i].x = &bSplineControlPointsX[i];
        bSplineControlPoints[i].y = &bSplineControlPointsY[i];
    }
    std::vector<double> weights(bSplineControlPoints.size(), 1.0);
    std::vector<double*> weightsAsPtr;
    std::vector<double> knots(bSplineControlPoints.size() - 2);  // Hardcoded for cubic
    std::vector<double*> knotsAsPtr;
    std::vector<int> mult(bSplineControlPoints.size() - 2, 1);  // Hardcoded for cubic
    mult.front() = 4;                                           // Hardcoded for cubic
    mult.back() = 4;                                            // Hardcoded for cubic
    for (size_t i = 0; i < bSplineControlPoints.size(); ++i) {
        weightsAsPtr.push_back(&weights[i]);
    }
    for (size_t i = 0; i < knots.size(); ++i) {
        knots[i] = static_cast<double>(i);
        knotsAsPtr.push_back(&knots[i]);
    }
    GCS::Arc arc;
    arc.start = arcStart;
    arc.end = arcEnd;
    arc.center = arcCenter;
    arc.rad = &arcRadius;
    arc.startAngle = &arcStartAngle;
    arc.endAngle = &arcEndAngle;
    GCS::BSpline bspline;
    bspline.start = bSplineStart;
    bspline.end = bSplineEnd;
    bspline.poles = bSplineControlPoints;
    bspline.weights = weightsAsPtr;
    bspline.knots = knotsAsPtr;
    bspline.mult = mult;
    bspline.degree = 3;
    bspline.periodic = false;
    double bsplineParam = 0.35;

    std::vector<double*> params = {
        point.x,
        point.y,
        arcStart.x,
        arcStart.y,
        arcEnd.x,
        arcEnd.y,
        arcCenter.x,
        arcCenter.y,
        &arcRadius,
        bSplineStart.x,
        bSplineStart.y,
        bSplineEnd.x,
        bSplineEnd.y,
        &bSplineControlPointsX[0],
        &bSplineControlPointsY[0],
        &bSplineControlPointsX[1],
        &bSplineControlPointsY[1],
        &bSplineControlPointsX[2],
        &bSplineControlPointsY[2],
        &bSplineControlPointsX[3],
        &bSplineControlPointsY[3],
        &bSplineControlPointsX[4],
        &bSplineControlPointsY[4],
        &desiredAngle,
        &bsplineParam
    };
    params.insert(params.end(), weightsAsPtr.begin(), weightsAsPtr.end());
    params.insert(params.end(), knotsAsPtr.begin(), knotsAsPtr.end());

    // Act
    // TODO: Apply constraint and solve
    System()->addConstraintArcRules(arc);
    System()->addConstraintPointOnArc(point, arc, 0, true);
    System()->addConstraintPointOnBSpline(point, bspline, &bsplineParam, 0, true);
    System()->addConstraintAngleViaPointAndParam(bspline, arc, point, &bsplineParam, &desiredAngle, 0, true);
    int solveResult = System()->solve(params);
    if (solveResult == GCS::Success) {
        System()->applySolution();
    }

    // Assert
    EXPECT_EQ(solveResult, GCS::Success);
    // is point on arc?
    EXPECT_DOUBLE_EQ(
        (arcRadius) * (arcRadius),
        (pointX - arcCenterX) * (pointX - arcCenterX) + (pointY - arcCenterY) * (pointY - arcCenterY)
    );
    // is point on B-spline?
    GCS::DeriVector2 pointAtBSplineParam = bspline.Value(bsplineParam, 1.0);
    EXPECT_DOUBLE_EQ(pointAtBSplineParam.x, pointX);
    EXPECT_DOUBLE_EQ(pointAtBSplineParam.y, pointY);
    // TODO: are tangents at relevant parameter equal?
    GCS::DeriVector2 centerToPoint((pointX - arcCenterX), (pointY - arcCenterY));
    GCS::DeriVector2 tangentBSplineAtPoint(pointAtBSplineParam.dx, pointAtBSplineParam.dy);
    double dprd;
    // FIXME: This error is probably too high. Fixing this may require improving the solver,
    // however.
    EXPECT_NEAR(
        std::fabs(centerToPoint.crossProdZ(tangentBSplineAtPoint, dprd))
            / (centerToPoint.length() * tangentBSplineAtPoint.length()),
        1.0,
        0.005
    );
}

TEST_F(ConstraintsTest, linelength)  // NOLINT
{
    // Arrange
    double lp1x = 0.0;
    double lp1y = -1.0;
    double lp2x = 4.0;
    double lp2y = 20.0;
    double llength = 10.2;
    GCS::Line line(GCS::Point(&lp1x, &lp1y), GCS::Point(&lp2x, &lp2y));

    std::vector<double*> unknowns;
    line.PushOwnParams(unknowns);

    // Act
    // TODO: Apply constraint and solve
    System()->addConstraintP2PDistance(line.p1, line.p2, &llength);

    int solveResult = System()->solve(unknowns);
    if (solveResult == GCS::Success) {
        System()->applySolution();
    }

    // Assert
    EXPECT_EQ(solveResult, GCS::Success);

    EXPECT_NEAR(std::sqrt(std::pow(lp1x - lp2x, 2.0) + std::pow(lp1y - lp2y, 2.0)), llength, 1e-12);
}
TEST_F(ConstraintsTest, rectangleWithOneOffsetDim)  // NOLINT
{
    double left = -1;
    double bottom = -5;
    double right = 6;
    double top = 8;

    double l1p1x = left;
    double l1p1y = bottom;
    double l1p2x = left;
    double l1p2y = top;
    double l1length = 10;
    GCS::Line l1(GCS::Point(&l1p1x, &l1p1y), GCS::Point(&l1p2x, &l1p2y));

    double l2p1x = left;
    double l2p1y = top;
    double l2p2x = right;
    double l2p2y = top;
    GCS::Line l2(GCS::Point(&l2p1x, &l2p1y), GCS::Point(&l2p2x, &l2p2y));

    double l3p1x = right;
    double l3p1y = top;
    double l3p2x = right;
    double l3p2y = bottom;
    GCS::Line l3(GCS::Point(&l3p1x, &l3p1y), GCS::Point(&l3p2x, &l3p2y));

    double l4p1x = right;
    double l4p1y = bottom;
    double l4p2x = left;
    double l4p2y = bottom;
    GCS::Line l4(GCS::Point(&l4p1x, &l4p1y), GCS::Point(&l4p2x, &l4p2y));


    std::vector<double*> unknowns;
    l1.PushOwnParams(unknowns);
    l2.PushOwnParams(unknowns);
    l3.PushOwnParams(unknowns);
    l4.PushOwnParams(unknowns);

    System()->addConstraintP2PCoincident(l1.p2, l2.p1);
    System()->addConstraintP2PCoincident(l2.p2, l3.p1);
    System()->addConstraintP2PCoincident(l3.p2, l4.p1);
    System()->addConstraintP2PCoincident(l4.p2, l1.p1);

    System()->addConstraintVertical(l1);
    System()->addConstraintHorizontal(l2);
    System()->addConstraintVertical(l3);
    System()->addConstraintHorizontal(l4);

    System()->addConstraintDifference(l1.p1.y, l1.p2.y, &l1length);

    int solveResult = System()->solve(unknowns);
    if (solveResult == GCS::Success) {
        System()->applySolution();
    }

    // Assert
    EXPECT_EQ(solveResult, GCS::Success);

    EXPECT_NEAR(*l1.p2.y, *l1.p1.y + l1length, 1e-12);
}
TEST_F(ConstraintsTest, drag2DOF)  // NOLINT
{
    // Drag a line which has a 2 degrees of freedom (the dragged point should go to the dragpos)
    double lp1x = -1.0;
    double lp1y = -2.0;
    double lp2x = -3.0;
    double lp2y = 20.0;
    double llength = 20;
    GCS::Line line(GCS::Point(&lp1x, &lp1y), GCS::Point(&lp2x, &lp2y));

    double dragPointx = 5.0;
    double dragPointy = 2.0;
    GCS::Point dragPoint(&dragPointx, &dragPointy);

    std::vector<double*> unknowns;
    line.PushOwnParams(unknowns);

    System()->addConstraintVertical(line);


    System()->addConstraintDifference(line.p1.y, line.p2.y, &llength);
    System()->addConstraintP2PCoincident(line.p1, dragPoint, GCS::DefaultTemporaryConstraint);

    int solveResult = System()->solve(unknowns);
    if (solveResult == GCS::Success) {
        System()->applySolution();
    }

    // Assert
    EXPECT_EQ(solveResult, GCS::Success);

    EXPECT_NEAR(lp1x, dragPointx, 1e-12);
    EXPECT_NEAR(lp1y, dragPointy, 1e-12);
    EXPECT_NEAR(lp2y, lp1y + llength, 1e-12);
}
TEST_F(ConstraintsTest, drag1DOF)
{
    // Drag a line which has a 1 degree of freedom (1 dimension of the dragged point should go to
    // the dragpoint)
    double zero = 0;
    double lp1x = -1.0;
    double lp1y = -2.0;
    double lp2x = -3.0;
    double lp2y = 20.0;
    GCS::Line line(GCS::Point(&lp1x, &lp1y), GCS::Point(&lp2x, &lp2y));

    double dragPointx = 5.0;
    double dragPointy = 2.0;
    GCS::Point dragPoint(&dragPointx, &dragPointy);

    std::vector<double*> unknowns;
    line.PushOwnParams(unknowns);

    System()->addConstraintEqual(&lp1x, &zero);
    System()->addConstraintEqual(&lp1y, &zero);
    System()->addConstraintVertical(line);

    System()->addConstraintP2PCoincident(line.p2, dragPoint, GCS::DefaultTemporaryConstraint);

    int solveResult = System()->solve(unknowns);
    if (solveResult == GCS::Success) {
        System()->applySolution();
    }

    // Assert
    EXPECT_EQ(solveResult, GCS::Success);

    EXPECT_NEAR(lp1x, 0, 1e-12);
    EXPECT_NEAR(lp1y, 0, 1e-12);
    EXPECT_NEAR(lp2y, dragPointy, 1e-12);
    EXPECT_NEAR(lp1x, lp2x, 1e-12);
}
TEST_F(ConstraintsTest, drag0DOF)
{
    // Drag a line which has a 0 degree of freedom (dragged point should not move)
    double zero = 0;
    double lp1x = -1.0;
    double lp1y = -2.0;
    double lp2x = 0.0;
    double lp2y = 20.0;
    double llength = 20;
    GCS::Line line(GCS::Point(&lp1x, &lp1y), GCS::Point(&lp2x, &lp2y));

    double dragPointx = 5.0;
    double dragPointy = 2.0;
    GCS::Point dragPoint(&dragPointx, &dragPointy);

    std::vector<double*> unknowns;
    line.PushOwnParams(unknowns);

    System()->addConstraintEqual(&lp1x, &zero);
    System()->addConstraintEqual(&lp1y, &zero);
    System()->addConstraintVertical(line);
    System()->addConstraintP2PDistance(line.p1, line.p2, &llength);

    System()->addConstraintP2PCoincident(line.p2, dragPoint, GCS::DefaultTemporaryConstraint);

    int solveResult = System()->solve(unknowns);
    if (solveResult == GCS::Success) {
        System()->applySolution();
    }

    // Assert
    EXPECT_EQ(solveResult, GCS::Success);

    EXPECT_NEAR(lp1x, 0, 1e-12);
    EXPECT_NEAR(lp1y, 0, 1e-12);
    EXPECT_NEAR(lp2y, llength, 1e-12);
    EXPECT_NEAR(lp1x, lp2x, 1e-12);
}

TEST_F(ConstraintsTest, classicFlip)  // NOLINT
{
    // Create the classic flip L figure

    double l1dist = 20;
    double l4length = 30;
    double l6length = 10;

    double originx = 0.0;
    double originy = 0.0;
    double zero = 0;
    GCS::Point origin(&originx, &originy);

    double l1p1x = 20.336287;
    double l1p1y = 35.627689;
    double l1p2x = 20.642096;
    double l1p2y = -0.152126;
    GCS::Line l1(GCS::Point(&l1p1x, &l1p1y), GCS::Point(&l1p2x, &l1p2y));

    double l2p1x = 20.336287;
    double l2p1y = -0.152126;
    double l2p2x = 55.045757;
    double l2p2y = 0.0000000;
    GCS::Line l2(GCS::Point(&l2p1x, &l2p1y), GCS::Point(&l2p2x, &l2p2y));

    double l3p1x = 55.045757;
    double l3p1y = 0.0000000;
    double l3p2x = 54.892860;
    double l3p2y = 13.150624;
    GCS::Line l3(GCS::Point(&l3p1x, &l3p1y), GCS::Point(&l3p2x, &l3p2y));

    double l4p1x = 55.045757;
    double l4p1y = 13.150624;
    double l4p2x = 39.755249;
    double l4p2y = 12.997720;
    GCS::Line l4(GCS::Point(&l4p1x, &l4p1y), GCS::Point(&l4p2x, &l4p2y));

    double l5p1x = 39.755249;
    double l5p1y = 13.150624;
    double l5p2x = 39.908154;
    double l5p2y = 34.251545;
    GCS::Line l5(GCS::Point(&l5p1x, &l5p1y), GCS::Point(&l5p2x, &l5p2y));

    double l6p1x = 39.755249;
    double l6p1y = 34.251545;
    double l6p2x = 20.336287;
    double l6p2y = 35.627689;
    GCS::Line l6(GCS::Point(&l6p1x, &l6p1y), GCS::Point(&l6p2x, &l6p2y));

    std::vector<double*> unknowns;
    l1.PushOwnParams(unknowns);
    l2.PushOwnParams(unknowns);
    l3.PushOwnParams(unknowns);
    l4.PushOwnParams(unknowns);
    l5.PushOwnParams(unknowns);
    l6.PushOwnParams(unknowns);
    origin.PushOwnParams(unknowns);


    System()->addConstraintEqual(&originx, &zero);
    System()->addConstraintEqual(&originy, &zero);

    System()->addConstraintVertical(l1);
    System()->addConstraintHorizontal(l2);
    System()->addConstraintVertical(l3);
    System()->addConstraintHorizontal(l4);
    System()->addConstraintVertical(l5);
    System()->addConstraintHorizontal(l6);

    System()->addConstraintP2PCoincident(l1.p2, l2.p1);
    System()->addConstraintP2PCoincident(l2.p2, l3.p1);
    System()->addConstraintP2PCoincident(l3.p2, l4.p1);
    System()->addConstraintP2PCoincident(l4.p2, l5.p1);
    System()->addConstraintP2PCoincident(l5.p2, l6.p1);
    System()->addConstraintP2PCoincident(l6.p2, l1.p1);
    System()->addConstraintEqualLength(l3, l6);
    System()->addConstraintEqualLength(l4, l5);

    System()->addConstraintP2LDistance(origin, l1, &l1dist);
    // System()->addConstraintP2PDistance(l4.p1, l4.p2, &l4length);
    // System()->addConstraintP2PDistance(l6.p1, l6.p2, &l6length);
    System()->addConstraintDifference(l4.p2.x, l4.p1.x, &l4length);
    System()->addConstraintDifference(l6.p2.x, l6.p1.x, &l6length);
    System()->addConstraintEqual(&originy, l1.p2.y);

    int solveResult = System()->solve(unknowns);
    if (solveResult == GCS::Success) {
        System()->applySolution();
    }

    // Assert
    EXPECT_EQ(solveResult, GCS::Success);

    EXPECT_NEAR(l1p1x, l1dist, 1e-12);
    EXPECT_NEAR(l1p2x, l1dist, 1e-12);
    EXPECT_NEAR(l6p2x, l1dist, 1e-12);
    EXPECT_NEAR(l2p1x, l1dist, 1e-12);

    EXPECT_NEAR(l2p2x, l1dist + l4length + l6length, 1e-12);
    EXPECT_NEAR(l3p1x, l1dist + l4length + l6length, 1e-12);
    EXPECT_NEAR(l3p2x, l1dist + l4length + l6length, 1e-12);
    EXPECT_NEAR(l4p1x, l1dist + l4length + l6length, 1e-12);

    EXPECT_NEAR(l4p2x, l1dist + l6length, 1e-12);
    EXPECT_NEAR(l5p1x, l1dist + l6length, 1e-12);
    EXPECT_NEAR(l5p2x, l1dist + l6length, 1e-12);
    EXPECT_NEAR(l6p1x, l1dist + l6length, 1e-12);

    EXPECT_NEAR(l1p1y, l4length + l6length, 1e-12);
    EXPECT_NEAR(l6p1y, l4length + l6length, 1e-12);
    EXPECT_NEAR(l6p2y, l4length + l6length, 1e-12);
    EXPECT_NEAR(l5p2y, l4length + l6length, 1e-12);

    EXPECT_NEAR(l1p2y, 0, 1e-12);
    EXPECT_NEAR(l2p1y, 0, 1e-12);
    EXPECT_NEAR(l2p2y, 0, 1e-12);
    EXPECT_NEAR(l3p1y, 0, 1e-12);

    EXPECT_NEAR(l3p2y, l6length, 1e-12);
    EXPECT_NEAR(l4p1y, l6length, 1e-12);
    EXPECT_NEAR(l4p2y, l6length, 1e-12);
    EXPECT_NEAR(l5p1y, l6length, 1e-12);


    // Try to flip it
    l1dist = 100;
    solveResult = System()->solve(unknowns);
    if (solveResult == GCS::Success) {
        System()->applySolution();
    }

    // Assert
    EXPECT_EQ(solveResult, GCS::Success);

    EXPECT_NEAR(l1p1x, l1dist, 1e-12);
    EXPECT_NEAR(l1p2x, l1dist, 1e-12);
    EXPECT_NEAR(l6p2x, l1dist, 1e-12);
    EXPECT_NEAR(l2p1x, l1dist, 1e-12);

    EXPECT_NEAR(l2p2x, l1dist + l4length + l6length, 1e-12);
    EXPECT_NEAR(l3p1x, l1dist + l4length + l6length, 1e-12);
    EXPECT_NEAR(l3p2x, l1dist + l4length + l6length, 1e-12);
    EXPECT_NEAR(l4p1x, l1dist + l4length + l6length, 1e-12);

    EXPECT_NEAR(l4p2x, l1dist + l6length, 1e-12);
    EXPECT_NEAR(l5p1x, l1dist + l6length, 1e-12);
    EXPECT_NEAR(l5p2x, l1dist + l6length, 1e-12);
    EXPECT_NEAR(l6p1x, l1dist + l6length, 1e-12);

    EXPECT_NEAR(l1p1y, l4length + l6length, 1e-12);
    EXPECT_NEAR(l6p1y, l4length + l6length, 1e-12);
    EXPECT_NEAR(l6p2y, l4length + l6length, 1e-12);
    EXPECT_NEAR(l5p2y, l4length + l6length, 1e-12);

    EXPECT_NEAR(l1p2y, 0, 1e-12);
    EXPECT_NEAR(l2p1y, 0, 1e-12);
    EXPECT_NEAR(l2p2y, 0, 1e-12);
    EXPECT_NEAR(l3p1y, 0, 1e-12);

    EXPECT_NEAR(l3p2y, l6length, 1e-12);
    EXPECT_NEAR(l4p1y, l6length, 1e-12);
    EXPECT_NEAR(l4p2y, l6length, 1e-12);
    EXPECT_NEAR(l5p1y, l6length, 1e-12);
}
TEST_F(ConstraintsTest, addEquality)  // NOLINT
{
    double linelength = 40;
    double zero = 0.0;

    double lhx1 = 0.0;
    double lhy1 = 0.0;
    double lhx2 = 40.0;
    double lhy2 = 0.0;
    GCS::Line lh(GCS::Point(&lhx1, &lhy1), GCS::Point(&lhx2, &lhy2));

    double lvx1 = 20.0;
    double lvy1 = 0.0;
    double lvx2 = 20.0;
    double lvy2 = 10.0;
    GCS::Line lv(GCS::Point(&lvx1, &lvy1), GCS::Point(&lvx2, &lvy2));

    std::vector<double*> unknowns;
    lh.PushOwnParams(unknowns);
    lv.PushOwnParams(unknowns);

    System()->addConstraintEqual(lh.p1.x, &zero);
    System()->addConstraintEqual(lh.p1.y, &zero);
    System()->addConstraintHorizontal(lh);
    System()->addConstraintVertical(lv);

    System()->addConstraintP2PDistance(lh.p1, lh.p2, &linelength);
    System()->addConstraintPointOnLine(lv.p1, lh);

    int solveResult = System()->solve(unknowns);
    if (solveResult == GCS::Success) {
        System()->applySolution();
    }

    // Assert
    EXPECT_EQ(solveResult, GCS::Success);

    EXPECT_NEAR(lhx1, 0, 1e-12);
    EXPECT_NEAR(lhy1, 0, 1e-12);
    EXPECT_NEAR(lhx2, linelength, 1e-12);
    EXPECT_NEAR(lhy2, 0, 1e-12);

    EXPECT_NEAR(lvx1, 20, 1e-12);
    EXPECT_NEAR(lvy1, 0, 1e-12);
    EXPECT_NEAR(lvx2, 20, 1e-12);
    EXPECT_NEAR(lvy2, 10, 1e-12);

    System()->addConstraintEqualLength(lh, lv);
    solveResult = System()->solve(unknowns);
    if (solveResult == GCS::Success) {
        System()->applySolution();
    }

    EXPECT_EQ(solveResult, GCS::Success);

    EXPECT_NEAR(lhx1, 0, 1e-12);
    EXPECT_NEAR(lhy1, 0, 1e-12);
    EXPECT_NEAR(lhx2, linelength, 1e-12);
    EXPECT_NEAR(lhy2, 0, 1e-12);

    EXPECT_NEAR(lvx1, 20, 1e-12);
    EXPECT_NEAR(lvy1, 0, 1e-12);
    EXPECT_NEAR(lvx2, 20, 1e-12);
    EXPECT_NEAR(lvy2, linelength, 1e-12);
}
TEST_F(ConstraintsTest, substitutionWithTangeant)  // NOLINT
{
    double hlinelength = 50.0;
    double vlinelength = 10.0;
    double zero = 0.0;

    double lhx1 = 0.0;
    double lhy1 = 0.0;
    double lhx2 = hlinelength;
    double lhy2 = 0.0;
    GCS::Line lh(GCS::Point(&lhx1, &lhy1), GCS::Point(&lhx2, &lhy2));

    double lvx1 = hlinelength;
    double lvy1 = 0.0;
    double lvx2 = hlinelength;
    double lvy2 = vlinelength;
    GCS::Line lv(GCS::Point(&lvx1, &lvy1), GCS::Point(&lvx2, &lvy2));

    double cx = hlinelength / 2.0;
    double cy = vlinelength;
    double rad = hlinelength / 2.0;
    double sx = hlinelength;
    double sy = vlinelength;
    double ex = 0.0;
    double ey = vlinelength;
    double sa = 0.0;
    double ea = std::numbers::pi;

    GCS::Arc arc(GCS::Point(&cx, &cy), &rad, GCS::Point(&sx, &sy), GCS::Point(&ex, &ey), &sa, &ea);

    std::vector<double*> unknowns;
    lh.PushOwnParams(unknowns);
    lv.PushOwnParams(unknowns);
    arc.PushOwnParams(unknowns);

    System()->addConstraintHorizontal(lh);
    System()->addConstraintVertical(lv);
    System()->addConstraintArcRules(arc);
    System()->addConstraintAngleViaPoint(lv, arc, lv.p2, &zero);
    System()->addConstraintP2PCoincident(lh.p2, lv.p1);
    System()->addConstraintP2PCoincident(arc.start, lv.p2);

    int solveResult = System()->solve(unknowns);
    if (solveResult == GCS::Success) {
        System()->applySolution();
    }

    // Assert
    EXPECT_EQ(solveResult, GCS::Success);

    EXPECT_NEAR(lhx1, 0, 1e-12);
    EXPECT_NEAR(lhy1, 0, 1e-12);
    EXPECT_NEAR(lhx2, hlinelength, 1e-12);
    EXPECT_NEAR(lhy2, 0, 1e-12);

    EXPECT_NEAR(lvx1, hlinelength, 1e-12);
    EXPECT_NEAR(lvy1, 0, 1e-12);
    EXPECT_NEAR(lvx2, hlinelength, 1e-12);
    EXPECT_NEAR(lvy2, vlinelength, 1e-12);

    EXPECT_NEAR(sx, hlinelength, 1e-12);
    EXPECT_NEAR(sy, vlinelength, 1e-12);

    hlinelength = 100.0;
    System()->addConstraintP2PDistance(lh.p1, lh.p2, &hlinelength);
    solveResult = System()->solve(unknowns);
    if (solveResult == GCS::Success) {
        System()->applySolution();
    }

    EXPECT_EQ(solveResult, GCS::Success);

    EXPECT_NEAR(lhy1, 0, 1e-12);
    EXPECT_NEAR(lhx2 - lhx1, hlinelength, 1e-12);
    EXPECT_NEAR(lhy2, 0, 1e-12);

    EXPECT_NEAR(lvx1, lhx2, 1e-12);
    EXPECT_NEAR(lvy1, 0, 1e-12);
    EXPECT_NEAR(lvx2, lhx2, 1e-12);
    EXPECT_NEAR(lvy2, vlinelength, 1e-12);

    EXPECT_NEAR(sx, lhx2, 1e-12);
    EXPECT_NEAR(sy, vlinelength, 1e-12);
}
TEST_F(ConstraintsTest, substitutionCircleCircleDistance)  // NOLINT
{
    double c1x = 0;
    double c1y = 0;
    double c1r = 0;
    GCS::Circle c1(GCS::Point(&c1x, &c1y), &c1r);

    double c2x = 0;
    double c2y = 0;
    double c2r = 0;
    GCS::Circle c2(GCS::Point(&c2x, &c2y), &c2r);

    double zero = 0;

    std::vector<double*> unknowns;
    c1.PushOwnParams(unknowns);
    c2.PushOwnParams(unknowns);

    // Both circles with radius defined and aligned verticaly (substitution)
    // circles overlap
    {
        c1x = 4;
        c1y = -2;
        double rad1 = 5;

        c2x = 6;
        c2y = 3;
        double rad2 = 3;

        double dist = -2;

        System()->clear();

        System()->addConstraintCircleRadius(c1, &rad1);
        System()->addConstraintCircleRadius(c2, &rad2);
        System()->addConstraintVertical(c1.center, c2.center);
        System()->addConstraintC2CDistance(c1, c2, &dist);
        System()->addConstraintEqual(&c1x, &zero);
        System()->addConstraintEqual(&c1y, &zero);

        int solveResult = System()->solve(unknowns);
        if (solveResult == GCS::Success) {
            System()->applySolution();
        }

        EXPECT_EQ(solveResult, GCS::Success);

        EXPECT_NEAR(c1x, 0.0, 1e-12);
        EXPECT_NEAR(c1y, 0.0, 1e-12);
        EXPECT_NEAR(c2x, 0.0, 1e-12);
        EXPECT_NEAR(c2y, 6.0, 1e-12);
    }
    // Both circles with radius defined and aligned verticaly (substitution)
    // circles do not overlap
    {
        c1x = 4;
        c1y = -2;
        double rad1 = 5;

        c2x = 6;
        c2y = 3;
        double rad2 = 3;

        double dist = 2;

        System()->clear();

        System()->addConstraintCircleRadius(c1, &rad1);
        System()->addConstraintCircleRadius(c2, &rad2);
        System()->addConstraintVertical(c1.center, c2.center);
        System()->addConstraintC2CDistance(c1, c2, &dist);
        System()->addConstraintEqual(&c1x, &zero);
        System()->addConstraintEqual(&c1y, &zero);

        int solveResult = System()->solve(unknowns);
        if (solveResult == GCS::Success) {
            System()->applySolution();
        }

        EXPECT_EQ(solveResult, GCS::Success);

        EXPECT_NEAR(c1x, 0.0, 1e-12);
        EXPECT_NEAR(c1y, 0.0, 1e-12);
        EXPECT_NEAR(c2x, 0.0, 1e-12);
        EXPECT_NEAR(c2y, 10.0, 1e-12);
    }
    // Both circles with radius defined and aligned horizontaly (substitution)
    // circle overlap
    {
        c1x = 4;
        c1y = -2;
        double rad1 = 5;

        c2x = 6;
        c2y = 3;
        double rad2 = 3;

        double dist = -2;

        System()->clear();

        System()->addConstraintCircleRadius(c1, &rad1);
        System()->addConstraintCircleRadius(c2, &rad2);
        System()->addConstraintHorizontal(c1.center, c2.center);
        System()->addConstraintC2CDistance(c1, c2, &dist);
        System()->addConstraintEqual(&c1x, &zero);
        System()->addConstraintEqual(&c1y, &zero);

        int solveResult = System()->solve(unknowns);
        if (solveResult == GCS::Success) {
            System()->applySolution();
        }

        EXPECT_EQ(solveResult, GCS::Success);

        EXPECT_NEAR(c1x, 0.0, 1e-12);
        EXPECT_NEAR(c1y, 0.0, 1e-12);
        EXPECT_NEAR(c2x, 6.0, 1e-12);
        EXPECT_NEAR(c2y, 0.0, 1e-12);
    }
    // Both circles with radius defined and aligned horizontaly (substitution)
    // circle do not overlap
    {
        c1x = 4;
        c1y = -2;
        double rad1 = 5;

        c2x = 6;
        c2y = 3;
        double rad2 = 3;

        double dist = 2;

        System()->clear();

        System()->addConstraintCircleRadius(c1, &rad1);
        System()->addConstraintCircleRadius(c2, &rad2);
        System()->addConstraintHorizontal(c1.center, c2.center);
        System()->addConstraintC2CDistance(c1, c2, &dist);
        System()->addConstraintEqual(&c1x, &zero);
        System()->addConstraintEqual(&c1y, &zero);

        int solveResult = System()->solve(unknowns);
        if (solveResult == GCS::Success) {
            System()->applySolution();
        }

        EXPECT_EQ(solveResult, GCS::Success);

        EXPECT_NEAR(c1x, 0.0, 1e-12);
        EXPECT_NEAR(c1y, 0.0, 1e-12);
        EXPECT_NEAR(c2x, 10.0, 1e-12);
        EXPECT_NEAR(c2y, 0.0, 1e-12);
    }
}
TEST_F(ConstraintsTest, substitutionLineCircleDistance)  // NOLINT
{
    double cx = 0.0;
    double cy = 0.0;
    double cr = 5.0;
    GCS::Circle circle(GCS::Point(&cx, &cy), &cr);

    double lx1 = 0.0;
    double ly1 = 0.0;
    double lx2 = 0.0;
    double ly2 = 0.0;
    GCS::Line line(GCS::Point(&lx1, &ly1), GCS::Point(&lx2, &ly2));

    double zero = 0.0;

    std::vector<double*> unknowns;
    circle.PushOwnParams(unknowns);
    line.PushOwnParams(unknowns);

    // Circle radius defined, line vertical overlaps
    {
        double rad = 5.0;
        double dist = 4;

        lx1 = -0.8;
        ly1 = 4.0;
        lx2 = -2.3;
        ly2 = -3.0;

        System()->clear();

        System()->addConstraintCircleRadius(circle, &rad);
        System()->addConstraintVertical(line);
        System()->addConstraintC2LDistance(circle, line, &dist);
        System()->addConstraintEqual(&cx, &zero);
        System()->addConstraintEqual(&cy, &zero);

        int solveResult = System()->solve(unknowns);
        if (solveResult == GCS::Success) {
            System()->applySolution();
        }

        EXPECT_EQ(solveResult, GCS::Success);

        EXPECT_NEAR(cx, 0.0, 1e-12);
        EXPECT_NEAR(cy, 0.0, 1e-12);
        EXPECT_NEAR(lx1, lx2, 1e-12);
        EXPECT_NEAR(lx1, -1.0, 1e-12);
    }
    // Circle radius defined, line vertical does not overlaps
    {
        double rad = 5.0;
        double dist = 4;

        lx1 = -7.8;
        ly1 = 4.0;
        lx2 = -10.3;
        ly2 = -3.0;

        System()->clear();

        System()->addConstraintCircleRadius(circle, &rad);
        System()->addConstraintVertical(line);
        System()->addConstraintC2LDistance(circle, line, &dist);
        System()->addConstraintEqual(&cx, &zero);
        System()->addConstraintEqual(&cy, &zero);

        int solveResult = System()->solve(unknowns);
        if (solveResult == GCS::Success) {
            System()->applySolution();
        }

        EXPECT_EQ(solveResult, GCS::Success);

        EXPECT_NEAR(cx, 0.0, 1e-12);
        EXPECT_NEAR(cy, 0.0, 1e-12);
        EXPECT_NEAR(lx1, lx2, 1e-12);
        EXPECT_NEAR(lx1, -9.0, 1e-12);
    }
    // Circle radius defined, line horizontal overlaps
    {
        double rad = 5.0;
        double dist = 4;

        lx1 = 4.0;
        ly1 = 0.8;
        lx2 = -3.0;
        ly2 = 2.0;

        System()->clear();

        System()->addConstraintCircleRadius(circle, &rad);
        System()->addConstraintHorizontal(line);
        System()->addConstraintC2LDistance(circle, line, &dist);
        System()->addConstraintEqual(&cx, &zero);
        System()->addConstraintEqual(&cy, &zero);

        int solveResult = System()->solve(unknowns);
        if (solveResult == GCS::Success) {
            System()->applySolution();
        }

        EXPECT_EQ(solveResult, GCS::Success);

        EXPECT_NEAR(cx, 0.0, 1e-12);
        EXPECT_NEAR(cy, 0.0, 1e-12);
        EXPECT_NEAR(ly1, ly2, 1e-12);
        EXPECT_NEAR(ly1, 1.0, 1e-12);
    }
    // Circle radius defined, line horizontal does not overlaps
    {
        double rad = 5.0;
        double dist = 4;

        lx1 = 4.0;
        ly1 = 6.0;
        lx2 = -3.0;
        ly2 = 6.0;

        System()->clear();

        System()->addConstraintCircleRadius(circle, &rad);
        System()->addConstraintHorizontal(line);
        System()->addConstraintC2LDistance(circle, line, &dist);
        System()->addConstraintEqual(&cx, &zero);
        System()->addConstraintEqual(&cy, &zero);

        int solveResult = System()->solve(unknowns);
        if (solveResult == GCS::Success) {
            System()->applySolution();
        }

        EXPECT_EQ(solveResult, GCS::Success);

        EXPECT_NEAR(cx, 0.0, 1e-12);
        EXPECT_NEAR(cy, 0.0, 1e-12);
        EXPECT_NEAR(ly1, ly2, 1e-12);
        EXPECT_NEAR(ly1, 9.0, 1e-12);
    }
}
TEST_F(ConstraintsTest, substitutionPointCircleDistance)  // NOLINT
{
    double cx = 0.0;
    double cy = 0.0;
    double cr = 5.0;
    GCS::Circle circle(GCS::Point(&cx, &cy), &cr);

    double px = 0.0;
    double py = 0.0;
    GCS::Point point(&px, &py);

    double zero = 0.0;

    std::vector<double*> unknowns;
    circle.PushOwnParams(unknowns);
    point.PushOwnParams(unknowns);

    // Circle radius defined, point horizontaly aligned to center
    // inside circle
    {
        double rad = 5.0;
        double dist = 4;

        px = 4.0;
        py = 3.0;

        System()->clear();

        System()->addConstraintCircleRadius(circle, &rad);
        System()->addConstraintHorizontal(point, circle.center);
        System()->addConstraintP2CDistance(point, circle, &dist);
        System()->addConstraintEqual(&cx, &zero);
        System()->addConstraintEqual(&cy, &zero);

        int solveResult = System()->solve(unknowns);
        if (solveResult == GCS::Success) {
            System()->applySolution();
        }

        EXPECT_EQ(solveResult, GCS::Success);

        EXPECT_NEAR(cx, 0.0, 1e-12);
        EXPECT_NEAR(cy, 0.0, 1e-12);
        EXPECT_NEAR(px, 1.0, 1e-12);
        EXPECT_NEAR(py, 0.0, 1e-12);
    }
    // Circle radius defined, point horizontaly aligned to center
    // outside circle
    {
        double rad = 5.0;
        double dist = 4;

        px = 10.0;
        py = 10.0;

        System()->clear();

        System()->addConstraintCircleRadius(circle, &rad);
        System()->addConstraintHorizontal(point, circle.center);
        System()->addConstraintP2CDistance(point, circle, &dist);
        System()->addConstraintEqual(&cx, &zero);
        System()->addConstraintEqual(&cy, &zero);

        int solveResult = System()->solve(unknowns);
        if (solveResult == GCS::Success) {
            System()->applySolution();
        }

        EXPECT_EQ(solveResult, GCS::Success);

        EXPECT_NEAR(cx, 0.0, 1e-12);
        EXPECT_NEAR(cy, 0.0, 1e-12);
        EXPECT_NEAR(px, 9.0, 1e-12);
        EXPECT_NEAR(py, 0.0, 1e-12);
    }
    // Circle radius defined, point verticaly aligned to center
    // inside circle
    {
        double rad = 5.0;
        double dist = 4;

        px = 4.0;
        py = 3.0;

        System()->clear();

        System()->addConstraintCircleRadius(circle, &rad);
        System()->addConstraintVertical(point, circle.center);
        System()->addConstraintP2CDistance(point, circle, &dist);
        System()->addConstraintEqual(&cx, &zero);
        System()->addConstraintEqual(&cy, &zero);

        int solveResult = System()->solve(unknowns);
        if (solveResult == GCS::Success) {
            System()->applySolution();
        }

        EXPECT_EQ(solveResult, GCS::Success);

        EXPECT_NEAR(cx, 0.0, 1e-12);
        EXPECT_NEAR(cy, 0.0, 1e-12);
        EXPECT_NEAR(px, 0.0, 1e-12);
        EXPECT_NEAR(py, 1.0, 1e-12);
    }
    // Circle radius defined, point verticaly aligned to center
    // outside circle
    {
        double rad = 5.0;
        double dist = 4;

        px = 10.0;
        py = 10.0;

        System()->clear();

        System()->addConstraintCircleRadius(circle, &rad);
        System()->addConstraintVertical(point, circle.center);
        System()->addConstraintP2CDistance(point, circle, &dist);
        System()->addConstraintEqual(&cx, &zero);
        System()->addConstraintEqual(&cy, &zero);

        int solveResult = System()->solve(unknowns);
        if (solveResult == GCS::Success) {
            System()->applySolution();
        }

        EXPECT_EQ(solveResult, GCS::Success);

        EXPECT_NEAR(cx, 0.0, 1e-12);
        EXPECT_NEAR(cy, 0.0, 1e-12);
        EXPECT_NEAR(px, 0.0, 1e-12);
        EXPECT_NEAR(py, 9.0, 1e-12);
    }
}
