// SPDX-License-Identifier: LGPL-2.1-or-later

#include <algorithm>
#include <array>
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
    const auto solveResult = System()->solve(params);
    if (solveResult == GCS::SolveStatus::Success) {
        System()->applySolution();
    }

    // Assert
    EXPECT_EQ(solveResult, GCS::SolveStatus::Success);
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

namespace
{
struct TestLinePair
{
    double x1, y1, x2, y2, x3, y3, x4, y4;
    GCS::Line l1, l2;

    TestLinePair(double ax, double ay, double bx, double by, double cx, double cy, double dx, double dy)
        : x1(ax)
        , y1(ay)
        , x2(bx)
        , y2(by)
        , x3(cx)
        , y3(cy)
        , x4(dx)
        , y4(dy)
    {
        l1.p1.x = &x1;
        l1.p1.y = &y1;
        l1.p2.x = &x2;
        l1.p2.y = &y2;
        l2.p1.x = &x3;
        l2.p1.y = &y3;
        l2.p2.x = &x4;
        l2.p2.y = &y4;
    }

    std::array<double*, 8> params()
    {
        return {&x1, &y1, &x2, &y2, &x3, &y3, &x4, &y4};
    }

    double dx1() const
    {
        return x1 - x2;
    }
    double dy1() const
    {
        return y1 - y2;
    }
    double dx2() const
    {
        return x3 - x4;
    }
    double dy2() const
    {
        return y3 - y4;
    }
};

// Central-difference step relative to the length of the second line. It must stay well below
// that length so that the difference samples a smooth neighborhood; 1e-3 relative keeps the
// truncation error near 1e-6. For a zero-length line it falls back to a step below the
// regularization length.
double finiteDifferenceStep(double length2)
{
    return length2 > 0.0 ? length2 / 1000.0 : 1e-10;
}

// Central-difference check of the analytic gradient for every parameter.
void expectGradientMatchesFiniteDifference(GCS::Constraint& constraint, TestLinePair& lines, double step)
{
    auto params = lines.params();
    for (std::size_t i = 0; i < params.size(); ++i) {
        double* param = params[i];
        const double analytic = constraint.grad(param);
        const double value = *param;

        *param = value + step;
        const double errorPlus = constraint.error();
        *param = value - step;
        const double errorMinus = constraint.error();
        *param = value;

        const double numerical = (errorPlus - errorMinus) / (2 * step);
        // 1e-4 covers the truncation error of the central difference at the regularization
        // length while still rejecting the component-free quotient-rule variant, which is off
        // by several percent at normal geometry and by orders of magnitude on short lines.
        EXPECT_NEAR(analytic, numerical, 1e-4 * std::max(1.0, std::abs(analytic)))
            << "parameter " << i << ", step " << step;
    }
}

// The gradient must not contain a component along either line direction: changing a line's
// length may not reduce the residual. This is the property the old frozen scale violated.
void expectGradientHasNoLengthComponent(GCS::Constraint& constraint, TestLinePair& lines)
{
    const double dx1 = lines.dx1();
    const double dy1 = lines.dy1();
    const double dx2 = lines.dx2();
    const double dy2 = lines.dy2();

    const double alongFirst = constraint.grad(&lines.x1) * dx1 + constraint.grad(&lines.y1) * dy1
        - constraint.grad(&lines.x2) * dx1 - constraint.grad(&lines.y2) * dy1;
    const double alongSecond = constraint.grad(&lines.x3) * dx2 + constraint.grad(&lines.y3) * dy2
        - constraint.grad(&lines.x4) * dx2 - constraint.grad(&lines.y4) * dy2;

    EXPECT_NEAR(alongFirst, 0.0, 1e-9);
    EXPECT_NEAR(alongSecond, 0.0, 1e-9);
}
}  // namespace

TEST_F(ConstraintsTest, parallelGradientMatchesFiniteDifference)  // NOLINT
{
    for (double length2 : {1.0, 1e-3, 1e-6, 1e-9, 0.0}) {
        TestLinePair lines(3, 4, 0, 0, length2, 2 * length2, 0, 0);
        GCS::ConstraintParallel constraint(lines.l1, lines.l2);
        expectGradientMatchesFiniteDifference(constraint, lines, finiteDifferenceStep(length2));
    }
}

TEST_F(ConstraintsTest, perpendicularGradientMatchesFiniteDifference)  // NOLINT
{
    for (double length2 : {1.0, 1e-3, 1e-6, 1e-9, 0.0}) {
        TestLinePair lines(3, 4, 0, 0, length2, 2 * length2, 0, 0);
        GCS::ConstraintPerpendicular constraint(lines.l1, lines.l2);
        expectGradientMatchesFiniteDifference(constraint, lines, finiteDifferenceStep(length2));
    }
}

TEST_F(ConstraintsTest, residualFollowsGeometryChangesAfterConstruction)  // NOLINT
{
    // The old implementation normalized the residual once, in the constructor. Shrinking a
    // line afterwards therefore made the error shrink with that line, so a collapsed sketch
    // looked solved.
    // Test constructs at the initial length, then shrink the second line and expect
    // the residual to keep its angular value and the gradient to keep no length component.
    struct Geometry
    {
        double x1, y1, x2, y2;  // first line at construction
        double x3, y3, x4, y4;  // second line at construction
    };
    const Geometry geometries[] = {
        {16, 0, 0, 0, 0.6 * 16, 0.8 * 16, 0, 0},
        {3, 4, 0, 0, 1, 2, 0, 0},
    };

    for (const Geometry& geometry : geometries) {
        const double dx1 = geometry.x1 - geometry.x2;
        const double dy1 = geometry.y1 - geometry.y2;
        const double length1 = std::hypot(dx1, dy1);
        const double dx2 = geometry.x3 - geometry.x4;
        const double dy2 = geometry.y3 - geometry.y4;
        const double length2 = std::hypot(dx2, dy2);
        const double ux = dx2 / length2;
        const double uy = dy2 / length2;
        const double expectedErrorParallel = std::abs(dx1 * dy2 - dy1 * dx2) / (length1 * length2);
        const double expectedErrorPerpendicular = std::abs(dx1 * dx2 + dy1 * dy2)
            / (length1 * length2);

        for (double targetLength2 : {length2, 1.0, 1e-2, 1e-3, 1e-6}) {
            TestLinePair lines(
                geometry.x1,
                geometry.y1,
                geometry.x2,
                geometry.y2,
                geometry.x3,
                geometry.y3,
                geometry.x4,
                geometry.y4
            );
            GCS::ConstraintParallel parallel(lines.l1, lines.l2);
            GCS::ConstraintPerpendicular perpendicular(lines.l1, lines.l2);

            lines.x3 = geometry.x4 + targetLength2 * ux;
            lines.y3 = geometry.y4 + targetLength2 * uy;

            // The regularization adds an O(e^2/L^2) length dependence, so the shortest line
            // needs a wider tolerance.
            const double tolerance = targetLength2 < 1e-5 ? 1e-2 : 1e-6;
            EXPECT_NEAR(std::abs(parallel.error()), expectedErrorParallel, tolerance)
                << "length1 " << length1 << ", length2 " << targetLength2;
            EXPECT_NEAR(std::abs(perpendicular.error()), expectedErrorPerpendicular, tolerance)
                << "length1 " << length1 << ", length2 " << targetLength2;

            // The gradient may not reward shrinking a line. The regularized residual has an
            // O(e^2/L^2) length dependence, so only check this where it is negligible.
            if (targetLength2 >= 1e-2) {
                expectGradientHasNoLengthComponent(parallel, lines);
                expectGradientHasNoLengthComponent(perpendicular, lines);
            }
        }
    }
}
