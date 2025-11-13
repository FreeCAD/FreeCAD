// SPDX-License-Identifier: LGPL-2.1-or-later

#include <cmath>
#include <numbers>

#include <gtest/gtest.h>

#include "Mod/Sketcher/App/planegcs/GCS.h"
#include "Mod/Sketcher/App/planegcs/Geo.h"
#include "Mod/Sketcher/App/planegcs/Util.h"
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
    std::vector<GCS::DeriParam> weightsAsPtr;
    std::vector<double> knots(bSplineControlPoints.size() - 2);  // Hardcoded for cubic
    std::vector<GCS::DeriParam> knotsAsPtr;
    std::vector<int> mult(bSplineControlPoints.size() - 2, 1);  // Hardcoded for cubic
    mult.front() = 4;                                           // Hardcoded for cubic
    mult.back() = 4;                                            // Hardcoded for cubic
    for (size_t i = 0; i < bSplineControlPoints.size(); ++i) {
        weightsAsPtr.emplace_back(&weights[i]);
    }
    for (size_t i = 0; i < knots.size(); ++i) {
        knots[i] = static_cast<double>(i);
        knotsAsPtr.emplace_back(&knots[i]);
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
TEST_F(ConstraintsTest, simpleDrag)  // NOLINT
{
    double lp1x = 0.0;
    double lp1y = 0.0;
    double lp2x = 0.0;
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
