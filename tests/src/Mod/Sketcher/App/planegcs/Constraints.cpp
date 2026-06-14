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

TEST_F(ConstraintsTest, l2lAngle3DMatchesTargetAngle)  // NOLINT
{
    double p1x = 0.0, p1y = 0.0, p1z = 0.0;
    double p2x = 1.0, p2y = 0.0, p2z = 0.0;
    double p3x = 0.0, p3y = 0.0, p3z = 0.0;
    double p4x = 0.0, p4y = 1.0, p4z = 0.0;
    double angle = std::numbers::pi / 2.0;

    GCS::Point3D p1(&p1x, &p1y, &p1z);
    GCS::Point3D p2(&p2x, &p2y, &p2z);
    GCS::Point3D p3(&p3x, &p3y, &p3z);
    GCS::Point3D p4(&p4x, &p4y, &p4z);
    GCS::ConstraintL2LAngle3D constraint(p1, p2, p3, p4, &angle);

    EXPECT_NEAR(constraint.error(), 0.0, 1e-12);
}

TEST_F(ConstraintsTest, l2lAngle3DRespectsLineDirection)  // NOLINT
{
    // The 3D angle constraint measures the directed angle in [0, pi]
    // between the two line direction vectors v1 = (p2 - p1) and v2 = (p4 - p3).
    // Two vectors that differ by a supplementary gap give pi - gap, not the
    // smaller acute equivalent. Any user-facing fold to [0, pi/2] for line-
    // line undirected UX must be performed at the GUI layer, mirroring the
    // 2D Sketcher's reverseAngleConstraintToSupplementary pattern.
    const double gap = std::numbers::pi / 9.0;  // 20 degrees
    double p1x = 0.0, p1y = 0.0, p1z = 0.0;
    double p2x = 1.0, p2y = 0.0, p2z = 0.0;
    double p3x = 0.0, p3y = 0.0, p3z = 0.0;
    double p4x = -std::cos(gap), p4y = std::sin(gap), p4z = 0.0;
    double angle = std::numbers::pi - gap;  // 160 degrees, the directed angle

    GCS::Point3D p1(&p1x, &p1y, &p1z);
    GCS::Point3D p2(&p2x, &p2y, &p2z);
    GCS::Point3D p3(&p3x, &p3y, &p3z);
    GCS::Point3D p4(&p4x, &p4y, &p4z);
    GCS::ConstraintL2LAngle3D constraint(p1, p2, p3, p4, &angle);

    EXPECT_NEAR(constraint.error(), 0.0, 1e-12);

    // The acute supplementary value must NOT satisfy the constraint: the
    // residual should equal +(pi - 2 * gap), confirming the directed contract.
    angle = gap;
    EXPECT_NEAR(constraint.error(), std::numbers::pi - 2.0 * gap, 1e-12);

    angle = 0.0;
    constraint.evaluate();

    EXPECT_NEAR(angle, std::numbers::pi - gap, 1e-12);
}

TEST_F(ConstraintsTest, l2lAngle3DIsScaleInvariant)  // NOLINT
{
    double p1x = 0.0, p1y = 0.0, p1z = 0.0;
    double p2x = 1.0, p2y = 1.0, p2z = 0.0;
    double p3x = 0.0, p3y = 0.0, p3z = 0.0;
    double p4x = 1.0, p4y = 0.0, p4z = 0.0;
    double angle = std::numbers::pi / 3.0;

    GCS::Point3D p1(&p1x, &p1y, &p1z);
    GCS::Point3D p2(&p2x, &p2y, &p2z);
    GCS::Point3D p3(&p3x, &p3y, &p3z);
    GCS::Point3D p4(&p4x, &p4y, &p4z);
    GCS::ConstraintL2LAngle3D shortLines(p1, p2, p3, p4, &angle);
    const double shortError = shortLines.error();

    p2x = 10.0;
    p2y = 10.0;
    p4x = 5.0;
    GCS::ConstraintL2LAngle3D longLines(p1, p2, p3, p4, &angle);

    EXPECT_NEAR(longLines.error(), shortError, 1e-12);
}

TEST_F(ConstraintsTest, l2lAngle3DGradIsFiniteForDegenerateLine)  // NOLINT
{
    double p1x = 0.0, p1y = 0.0, p1z = 0.0;
    double p2x = 0.0, p2y = 0.0, p2z = 0.0;
    double p3x = 0.0, p3y = 0.0, p3z = 0.0;
    double p4x = 1.0, p4y = 0.0, p4z = 0.0;
    double angle = std::numbers::pi / 4.0;

    GCS::Point3D p1(&p1x, &p1y, &p1z);
    GCS::Point3D p2(&p2x, &p2y, &p2z);
    GCS::Point3D p3(&p3x, &p3y, &p3z);
    GCS::Point3D p4(&p4x, &p4y, &p4z);
    GCS::ConstraintL2LAngle3D constraint(p1, p2, p3, p4, &angle);

    EXPECT_TRUE(std::isfinite(constraint.error()));
    EXPECT_TRUE(std::isfinite(constraint.grad(&p1x)));
    EXPECT_TRUE(std::isfinite(constraint.grad(&p1y)));
    EXPECT_TRUE(std::isfinite(constraint.grad(&p1z)));
    EXPECT_TRUE(std::isfinite(constraint.grad(&p2x)));
    EXPECT_TRUE(std::isfinite(constraint.grad(&p2y)));
    EXPECT_TRUE(std::isfinite(constraint.grad(&p2z)));
    EXPECT_TRUE(std::isfinite(constraint.grad(&angle)));
}

TEST_F(ConstraintsTest, l2lAngle3DGradMatchesFiniteDifference)  // NOLINT
{
    double p1x = 0.0, p1y = 0.0, p1z = 0.0;
    double p2x = 2.0, p2y = 1.0, p2z = 0.5;
    double p3x = 0.0, p3y = 0.0, p3z = 0.0;
    double p4x = -2.0, p4y = 0.3, p4z = 0.2;
    double angle = std::numbers::pi / 4.0;

    GCS::Point3D p1(&p1x, &p1y, &p1z);
    GCS::Point3D p2(&p2x, &p2y, &p2z);
    GCS::Point3D p3(&p3x, &p3y, &p3z);
    GCS::Point3D p4(&p4x, &p4y, &p4z);
    GCS::ConstraintL2LAngle3D constraint(p1, p2, p3, p4, &angle);

    constexpr double step = 1e-6;
    auto finiteDifference = [&](double& param) {
        param += step;
        const double positive = constraint.error();
        param -= 2.0 * step;
        const double negative = constraint.error();
        param += step;
        return (positive - negative) / (2.0 * step);
    };

    EXPECT_NEAR(constraint.grad(&p2y), finiteDifference(p2y), 1e-7);
    EXPECT_NEAR(constraint.grad(&p4z), finiteDifference(p4z), 1e-7);
    EXPECT_NEAR(constraint.grad(&angle), finiteDifference(angle), 1e-7);
}

TEST_F(ConstraintsTest, l2lAngle3DEvaluateUpdatesDrivenValue)  // NOLINT
{
    double p1x = 0.0, p1y = 0.0, p1z = 0.0;
    double p2x = 1.0, p2y = 0.0, p2z = 0.0;
    double p3x = 0.0, p3y = 0.0, p3z = 0.0;
    double p4x = 0.0, p4y = 1.0, p4z = 1.0;
    double angle = 0.0;

    GCS::Point3D p1(&p1x, &p1y, &p1z);
    GCS::Point3D p2(&p2x, &p2y, &p2z);
    GCS::Point3D p3(&p3x, &p3y, &p3z);
    GCS::Point3D p4(&p4x, &p4y, &p4z);
    GCS::ConstraintL2LAngle3D constraint(p1, p2, p3, p4, &angle);

    constraint.evaluate();

    EXPECT_NEAR(angle, std::numbers::pi / 2.0, 1e-12);
}
