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

    std::vector<double*> params = {point.x,
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
                                   &bsplineParam};
    params.insert(params.end(), weightsAsPtr.begin(), weightsAsPtr.end());
    params.insert(params.end(), knotsAsPtr.begin(), knotsAsPtr.end());

    // Act
    // TODO: Apply constraint and solve
    System()->addConstraintArcRules(arc);
    System()->addConstraintPointOnArc(point, arc, 0, true);
    System()->addConstraintPointOnBSpline(point, bspline, &bsplineParam, 0, true);
    System()->addConstraintAngleViaPointAndParam(bspline,
                                                 arc,
                                                 point,
                                                 &bsplineParam,
                                                 &desiredAngle,
                                                 0,
                                                 true);
    int solveResult = System()->solve(params);
    if (solveResult == GCS::Success) {
        System()->applySolution();
    }

    // Assert
    EXPECT_EQ(solveResult, GCS::Success);
    // is point on arc?
    EXPECT_DOUBLE_EQ((arcRadius) * (arcRadius),
                     (pointX - arcCenterX) * (pointX - arcCenterX)
                         + (pointY - arcCenterY) * (pointY - arcCenterY));
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
    EXPECT_NEAR(std::fabs(centerToPoint.crossProdNorm(tangentBSplineAtPoint, dprd))
                    / (centerToPoint.length() * tangentBSplineAtPoint.length()),
                1.0,
                0.005);
}
