// SPDX-License-Identifier: LGPL-2.1-or-later

#include <cmath>
#include <numbers>
#include <iomanip>

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


/*
    Regression tests
*/

TEST_F(ConstraintsTest, p2pDistanceRegression)  // NOLINT
{
    /*
        This test is here to catch regressions, it was not validated by
        an external tool, if your patch makes this test fail, it *might*
        be valid, but do check!
    */

    {
        double p1x = -57;
        double p1y = 22;
        GCS::Point p1(&p1x, &p1y);

        double p2x = 45;
        double p2y = 100;
        GCS::Point p2(&p2x, &p2y);

        double length = 20;

        GCS::ConstraintP2PDistance constr(p1, p2, &length);

        EXPECT_NEAR(constr.error(), 108.405607354, 1e-8);

        EXPECT_NEAR(constr.grad(p1.x), -0.794357832977, 1e-8);
        EXPECT_NEAR(constr.grad(p1.y), -0.607450107571, 1e-8);
        EXPECT_NEAR(constr.grad(p2.x), 0.794357832977, 1e-8);
        EXPECT_NEAR(constr.grad(p2.y), 0.607450107571, 1e-8);
    }
    {
        double p1x = 10;
        double p1y = 0;
        GCS::Point p1(&p1x, &p1y);

        double p2x = 10;
        double p2y = 100;
        GCS::Point p2(&p2x, &p2y);

        double length = 30;

        GCS::ConstraintP2PDistance constr(p1, p2, &length);

        EXPECT_NEAR(constr.error(), 70, 1e-8);

        EXPECT_NEAR(constr.grad(p1.x), 0, 1e-8);
        EXPECT_NEAR(constr.grad(p1.y), -1, 1e-8);
        EXPECT_NEAR(constr.grad(p2.x), 0, 1e-8);
        EXPECT_NEAR(constr.grad(p2.y), 1, 1e-8);
    }
}
TEST_F(ConstraintsTest, p2pAngleRegression)  // NOLINT
{
    /*
        This test is here to catch regressions, it was not validated by
        an external tool, if your patch makes this test fail, it *might*
        be valid, but do check!
    */

    {
        double p1x = -57;
        double p1y = 22;
        GCS::Point p1(&p1x, &p1y);

        double p2x = 45;
        double p2y = 100;
        GCS::Point p2(&p2x, &p2y);

        double angle = 20;

        GCS::ConstraintP2PAngle constr(p1, p2, &angle);

        EXPECT_NEAR(constr.error(), -0.49759744736, 1e-8);

        EXPECT_NEAR(constr.grad(p1.x), 0.004730713246, 1e-8);
        EXPECT_NEAR(constr.grad(p1.y), -0.00618631732169, 1e-8);
        EXPECT_NEAR(constr.grad(p2.x), -0.004730713246, 1e-8);
        EXPECT_NEAR(constr.grad(p2.y), 0.00618631732169, 1e-8);
    }
    {
        double p1x = 10;
        double p1y = 0;
        GCS::Point p1(&p1x, &p1y);

        double p2x = 10;
        double p2y = 100;
        GCS::Point p2(&p2x, &p2y);

        double angle = 30;

        GCS::ConstraintP2PAngle constr(p1, p2, &angle);

        EXPECT_NEAR(constr.error(), 2.98672286269, 1e-8);

        EXPECT_NEAR(constr.grad(p1.x), 0.01, 1e-8);
        EXPECT_NEAR(constr.grad(p1.y), 2.16840434497e-19, 1e-8);
        EXPECT_NEAR(constr.grad(p2.x), -0.01, 1e-8);
        EXPECT_NEAR(constr.grad(p2.y), -2.16840434497e-19, 1e-8);
    }
}
TEST_F(ConstraintsTest, p2lDistanceeRegression)  // NOLINT
{
    /*
        This test is here to catch regressions, it was not validated by
        an external tool, if your patch makes this test fail, it *might*
        be valid, but do check!
    */

    {
        double p0x = 20;
        double p0y = -16;
        GCS::Point p0(&p0x, &p0y);

        double p1x = -57;
        double p1y = 22;
        double p2x = 45;
        double p2y = 100;
        GCS::Line line(GCS::Point(&p1x, &p1y), GCS::Point(&p2x, &p2y));

        double distance = 10;

        GCS::ConstraintP2LDistance constr(p0, line, &distance);

        EXPECT_NEAR(constr.error(), 66.9592559361, 1e-8);

        EXPECT_NEAR(constr.grad(p0.x), 0.607450107571, 1e-8);
        EXPECT_NEAR(constr.grad(p0.y), -0.794357832977, 1e-8);
        EXPECT_NEAR(constr.grad(line.p1.x), -0.427292961403, 1e-8);
        EXPECT_NEAR(constr.grad(line.p1.y), 0.558767718757, 1e-8);
        EXPECT_NEAR(constr.grad(line.p2.x), -0.180157146168, 1e-8);
        EXPECT_NEAR(constr.grad(line.p2.y), 0.23559011422, 1e-8);
    }
    {
        double p0x = 100;
        double p0y = 1006;
        GCS::Point p0(&p0x, &p0y);

        double p1x = 10;
        double p1y = 0;
        double p2x = 10;
        double p2y = 100;
        GCS::Line line(GCS::Point(&p1x, &p1y), GCS::Point(&p2x, &p2y));

        double distance = 20;

        GCS::ConstraintP2LDistance constr(p0, line, &distance);

        EXPECT_NEAR(constr.error(), 70, 1e-8);

        EXPECT_NEAR(constr.grad(p0.x), 1, 1e-8);
        EXPECT_NEAR(constr.grad(p0.y), 0, 1e-8);
        EXPECT_NEAR(constr.grad(line.p1.x), 9.06, 1e-8);
        EXPECT_NEAR(constr.grad(line.p1.y), 0, 1e-8);
        EXPECT_NEAR(constr.grad(line.p2.x), -10.06, 1e-8);
        EXPECT_NEAR(constr.grad(line.p2.y), 0, 1e-8);
    }
}
TEST_F(ConstraintsTest, pointOnLineRegression)  // NOLINT
{
    /*
        This test is here to catch regressions, it was not validated by
        an external tool, if your patch makes this test fail, it *might*
        be valid, but do check!
    */

    {
        double p0x = 20;
        double p0y = -16;
        GCS::Point p0(&p0x, &p0y);

        double p1x = -57;
        double p1y = 22;
        GCS::Point p1(&p1x, &p1y);

        double p2x = 45;
        double p2y = 100;
        GCS::Point p2(&p2x, &p2y);

        GCS::ConstraintPointOnLine constr(p0, p1, p2);

        EXPECT_NEAR(constr.error(), -76.9592559361, 1e-8);

        EXPECT_NEAR(constr.grad(p0.x), -0.607450107571, 1e-8);
        EXPECT_NEAR(constr.grad(p0.y), 0.794357832977, 1e-8);
        EXPECT_NEAR(constr.grad(p1.x), 0.427292961403, 1e-8);
        EXPECT_NEAR(constr.grad(p1.y), -0.558767718757, 1e-8);
        EXPECT_NEAR(constr.grad(p2.x), 0.180157146168, 1e-8);
        EXPECT_NEAR(constr.grad(p2.y), -0.23559011422, 1e-8);
    }
    {
        double p0x = 100;
        double p0y = 1006;
        GCS::Point p0(&p0x, &p0y);

        double p1x = 10;
        double p1y = 0;
        GCS::Point p1(&p1x, &p1y);

        double p2x = 10;
        double p2y = 100;
        GCS::Point p2(&p2x, &p2y);

        GCS::ConstraintPointOnLine constr(p0, p1, p2);

        EXPECT_NEAR(constr.error(), -90, 1e-8);

        EXPECT_NEAR(constr.grad(p0.x), -1, 1e-8);
        EXPECT_NEAR(constr.grad(p0.y), 0, 1e-8);
        EXPECT_NEAR(constr.grad(p1.x), -9.06, 1e-8);
        EXPECT_NEAR(constr.grad(p1.y), 0, 1e-8);
        EXPECT_NEAR(constr.grad(p2.x), 10.06, 1e-8);
        EXPECT_NEAR(constr.grad(p2.y), 0, 1e-8);
    }
}
TEST_F(ConstraintsTest, pointOnPerpBisectorRegression)  // NOLINT
{
    /*
        This test is here to catch regressions, it was not validated by
        an external tool, if your patch makes this test fail, it *might*
        be valid, but do check!
    */

    {
        double p0x = 20;
        double p0y = -16;
        GCS::Point p0(&p0x, &p0y);

        double p1x = -57;
        double p1y = 22;
        GCS::Point p1(&p1x, &p1y);

        double p2x = 45;
        double p2y = 100;
        GCS::Point p2(&p2x, &p2y);

        GCS::ConstraintPointOnPerpBisector constr(p0, p1, p2);

        EXPECT_NEAR(constr.error(), -52.2407092511, 1e-8);

        EXPECT_NEAR(constr.grad(p0.x), 1.58871566595, 1e-8);
        EXPECT_NEAR(constr.grad(p0.y), 1.21490021514, 1e-8);
        EXPECT_NEAR(constr.grad(p1.x), -1.5225021759, 1e-8);
        EXPECT_NEAR(constr.grad(p1.y), 0.344738648553, 1e-8);
        EXPECT_NEAR(constr.grad(p2.x), -0.0662134900594, 1e-8);
        EXPECT_NEAR(constr.grad(p2.y), -1.55963886369, 1e-8);
    }
    {
        double p0x = 100;
        double p0y = 1006;
        GCS::Point p0(&p0x, &p0y);

        double p1x = 10;
        double p1y = 0;
        GCS::Point p1(&p1x, &p1y);

        double p2x = 10;
        double p2y = 100;
        GCS::Point p2(&p2x, &p2y);

        GCS::ConstraintPointOnPerpBisector constr(p0, p1, p2);

        EXPECT_NEAR(constr.error(), 1912, 1e-8);

        EXPECT_NEAR(constr.grad(p0.x), 0, 1e-8);
        EXPECT_NEAR(constr.grad(p0.y), 2, 1e-8);
        EXPECT_NEAR(constr.grad(p1.x), -1.8, 1e-8);
        EXPECT_NEAR(constr.grad(p1.y), -1, 1e-8);
        EXPECT_NEAR(constr.grad(p2.x), 1.8, 1e-8);
        EXPECT_NEAR(constr.grad(p2.y), -1, 1e-8);
    }
}
TEST_F(ConstraintsTest, parallelRegression)  // NOLINT
{
    /*
        This test is here to catch regressions, it was not validated by
        an external tool, if your patch makes this test fail, it *might*
        be valid, but do check!
    */

    {
        double l1p1x = 20;
        double l1p1y = -16;
        double l1p2x = -57;
        double l1p2y = 22;
        GCS::Line l1(GCS::Point(&l1p1x, &l1p1y), GCS::Point(&l1p2x, &l1p2y));

        double l2p1x = 100;
        double l2p1y = 10;
        double l2p2x = 77;
        double l2p2y = -22;
        GCS::Line l2(GCS::Point(&l2p1x, &l2p1y), GCS::Point(&l2p2x, &l2p2y));

        GCS::ConstraintParallel constr(l1, l2);

        EXPECT_NEAR(constr.error(), 0.98645774632, 1e-8);

        EXPECT_NEAR(constr.grad(l1.p1.x), 0.0094567549078, 1e-8);
        EXPECT_NEAR(constr.grad(l1.p1.y), -0.00679704258998, 1e-8);
        EXPECT_NEAR(constr.grad(l1.p2.x), -0.0094567549078, 1e-8);
        EXPECT_NEAR(constr.grad(l1.p2.y), 0.00679704258998, 1e-8);
        EXPECT_NEAR(constr.grad(l2.p1.x), 0.011229896453, 1e-8);
        EXPECT_NEAR(constr.grad(l2.p1.y), 0.0227553164969, 1e-8);
        EXPECT_NEAR(constr.grad(l2.p2.x), -0.011229896453, 1e-8);
        EXPECT_NEAR(constr.grad(l2.p2.y), -0.0227553164969, 1e-8);
    }
    {
        double l1p1x = -40;
        double l1p1y = -25;
        double l1p2x = -10;
        double l1p2y = 45;
        GCS::Line l1(GCS::Point(&l1p1x, &l1p1y), GCS::Point(&l1p2x, &l1p2y));

        double l2p1x = -60;
        double l2p1y = -60;
        double l2p2x = 60;
        double l2p2y = 60;
        GCS::Line l2(GCS::Point(&l2p1x, &l2p1y), GCS::Point(&l2p2x, &l2p2y));

        GCS::ConstraintParallel constr(l1, l2);

        EXPECT_NEAR(constr.error(), -0.371390676354, 1e-8);

        EXPECT_NEAR(constr.grad(l1.p1.x), -0.00928476690885, 1e-8);
        EXPECT_NEAR(constr.grad(l1.p1.y), 0.00928476690885, 1e-8);
        EXPECT_NEAR(constr.grad(l1.p2.x), 0.00928476690885, 1e-8);
        EXPECT_NEAR(constr.grad(l1.p2.y), -0.00928476690885, 1e-8);
        EXPECT_NEAR(constr.grad(l2.p1.x), 0.00541611403016, 1e-8);
        EXPECT_NEAR(constr.grad(l2.p1.y), -0.00232119172721, 1e-8);
        EXPECT_NEAR(constr.grad(l2.p2.x), -0.00541611403016, 1e-8);
        EXPECT_NEAR(constr.grad(l2.p2.y), 0.00232119172721, 1e-8);
    }
}
TEST_F(ConstraintsTest, perpendicularRegression)  // NOLINT
{
    /*
        This test is here to catch regressions, it was not validated by
        an external tool, if your patch makes this test fail, it *might*
        be valid, but do check!
    */

    {
        double l1p1x = 20;
        double l1p1y = -16;
        double l1p2x = -57;
        double l1p2y = 22;
        GCS::Line l1(GCS::Point(&l1p1x, &l1p1y), GCS::Point(&l1p2x, &l1p2y));

        double l2p1x = 100;
        double l2p1y = 10;
        double l2p2x = 77;
        double l2p2y = -22;
        GCS::Line l2(GCS::Point(&l2p1x, &l2p1y), GCS::Point(&l2p2x, &l2p2y));

        GCS::ConstraintPerpendicular constr(l1, l2);

        EXPECT_NEAR(constr.error(), 0.164015592932, 1e-8);

        EXPECT_NEAR(constr.grad(l1.p1.x), 0.00679704258998, 1e-8);
        EXPECT_NEAR(constr.grad(l1.p1.y), 0.0094567549078, 1e-8);
        EXPECT_NEAR(constr.grad(l1.p2.x), -0.00679704258998, 1e-8);
        EXPECT_NEAR(constr.grad(l1.p2.y), -0.0094567549078, 1e-8);
        EXPECT_NEAR(constr.grad(l2.p1.x), 0.0227553164969, 1e-8);
        EXPECT_NEAR(constr.grad(l2.p1.y), -0.011229896453, 1e-8);
        EXPECT_NEAR(constr.grad(l2.p2.x), -0.0227553164969, 1e-8);
        EXPECT_NEAR(constr.grad(l2.p2.y), 0.011229896453, 1e-8);
    }
    {
        double l1p1x = -40;
        double l1p1y = -25;
        double l1p2x = -10;
        double l1p2y = 45;
        GCS::Line l1(GCS::Point(&l1p1x, &l1p1y), GCS::Point(&l1p2x, &l1p2y));

        double l2p1x = -60;
        double l2p1y = -60;
        double l2p2x = 60;
        double l2p2y = 60;
        GCS::Line l2(GCS::Point(&l2p1x, &l2p1y), GCS::Point(&l2p2x, &l2p2y));

        GCS::ConstraintPerpendicular constr(l1, l2);

        EXPECT_NEAR(constr.error(), 0.928476690885, 1e-8);

        EXPECT_NEAR(constr.grad(l1.p1.x), -0.00928476690885, 1e-8);
        EXPECT_NEAR(constr.grad(l1.p1.y), -0.00928476690885, 1e-8);
        EXPECT_NEAR(constr.grad(l1.p2.x), 0.00928476690885, 1e-8);
        EXPECT_NEAR(constr.grad(l1.p2.y), 0.00928476690885, 1e-8);
        EXPECT_NEAR(constr.grad(l2.p1.x), -0.00232119172721, 1e-8);
        EXPECT_NEAR(constr.grad(l2.p1.y), -0.00541611403016, 1e-8);
        EXPECT_NEAR(constr.grad(l2.p2.x), 0.00232119172721, 1e-8);
        EXPECT_NEAR(constr.grad(l2.p2.y), 0.00541611403016, 1e-8);
    }
}
TEST_F(ConstraintsTest, l2lAngleRegression)  // NOLINT
{
    /*
        This test is here to catch regressions, it was not validated by
        an external tool, if your patch makes this test fail, it *might*
        be valid, but do check!
    */

    {
        double l1p1x = 20;
        double l1p1y = -16;
        double l1p2x = -57;
        double l1p2y = 22;
        GCS::Line l1(GCS::Point(&l1p1x, &l1p1y), GCS::Point(&l1p2x, &l1p2y));

        double l2p1x = 100;
        double l2p1y = 10;
        double l2p2x = 77;
        double l2p2y = -22;
        GCS::Line l2(GCS::Point(&l2p1x, &l2p1y), GCS::Point(&l2p2x, &l2p2y));

        double angle = 0.8;

        GCS::ConstraintL2LAngle constr(l1, l2, &angle);

        EXPECT_NEAR(constr.error(), 0.606036319623, 1e-8);

        EXPECT_NEAR(constr.grad(l1.p1.x), -0.00515394005154, 1e-8);
        EXPECT_NEAR(constr.grad(l1.p1.y), -0.0104435101044, 1e-8);
        EXPECT_NEAR(constr.grad(l1.p2.x), 0.00515394005154, 1e-8);
        EXPECT_NEAR(constr.grad(l1.p2.y), 0.0104435101044, 1e-8);
        EXPECT_NEAR(constr.grad(l2.p1.x), -0.020605280103, 1e-8);
        EXPECT_NEAR(constr.grad(l2.p1.y), 0.0148100450741, 1e-8);
        EXPECT_NEAR(constr.grad(l2.p2.x), 0.020605280103, 1e-8);
        EXPECT_NEAR(constr.grad(l2.p2.y), -0.0148100450741, 1e-8);
    }
    {
        double l1p1x = -40;
        double l1p1y = -25;
        double l1p2x = -10;
        double l1p2y = 45;
        GCS::Line l1(GCS::Point(&l1p1x, &l1p1y), GCS::Point(&l1p2x, &l1p2y));

        double l2p1x = -60;
        double l2p1y = -60;
        double l2p2x = 60;
        double l2p2y = 60;
        GCS::Line l2(GCS::Point(&l2p1x, &l2p1y), GCS::Point(&l2p2x, &l2p2y));

        double angle = -0.8;

        GCS::ConstraintL2LAngle constr(l1, l2, &angle);

        EXPECT_NEAR(constr.error(), 0.419493622888, 1e-8);

        EXPECT_NEAR(constr.grad(l1.p1.x), -0.0120689655172, 1e-8);
        EXPECT_NEAR(constr.grad(l1.p1.y), 0.0051724137931, 1e-8);
        EXPECT_NEAR(constr.grad(l1.p2.x), 0.0120689655172, 1e-8);
        EXPECT_NEAR(constr.grad(l1.p2.y), -0.0051724137931, 1e-8);
        EXPECT_NEAR(constr.grad(l2.p1.x), 0.00416666666667, 1e-8);
        EXPECT_NEAR(constr.grad(l2.p1.y), -0.00416666666667, 1e-8);
        EXPECT_NEAR(constr.grad(l2.p2.x), -0.00416666666667, 1e-8);
        EXPECT_NEAR(constr.grad(l2.p2.y), 0.00416666666667, 1e-8);
    }
}
TEST_F(ConstraintsTest, midPointOnLineRegression)  // NOLINT
{
    /*
        This test is here to catch regressions, it was not validated by
        an external tool, if your patch makes this test fail, it *might*
        be valid, but do check!
    */

    {
        double l1p1x = 20;
        double l1p1y = -16;
        double l1p2x = -57;
        double l1p2y = 22;
        GCS::Line l1(GCS::Point(&l1p1x, &l1p1y), GCS::Point(&l1p2x, &l1p2y));

        double l2p1x = 100;
        double l2p1y = 10;
        double l2p2x = 77;
        double l2p2y = -22;
        GCS::Line l2(GCS::Point(&l2p1x, &l2p1y), GCS::Point(&l2p2x, &l2p2y));

        GCS::ConstraintMidpointOnLine constr(l1, l2);

        EXPECT_NEAR(constr.error(), -92.138369167, 1e-8);

        EXPECT_NEAR(constr.grad(l1.p1.x), 0.406007685671, 1e-8);
        EXPECT_NEAR(constr.grad(l1.p1.y), -0.291818024076, 1e-8);
        EXPECT_NEAR(constr.grad(l1.p2.x), 0.406007685671, 1e-8);
        EXPECT_NEAR(constr.grad(l1.p2.y), -0.291818024076, 1e-8);
        EXPECT_NEAR(constr.grad(l2.p1.x), 0.730186391552, 1e-8);
        EXPECT_NEAR(constr.grad(l2.p1.y), -0.524821468928, 1e-8);
        EXPECT_NEAR(constr.grad(l2.p2.x), -1.54220176289, 1e-8);
        EXPECT_NEAR(constr.grad(l2.p2.y), 1.10845751708, 1e-8);
    }
    {
        double l1p1x = -40;
        double l1p1y = -25;
        double l1p2x = -10;
        double l1p2y = 45;
        GCS::Line l1(GCS::Point(&l1p1x, &l1p1y), GCS::Point(&l1p2x, &l1p2y));

        double l2p1x = -60;
        double l2p1y = -60;
        double l2p2x = 60;
        double l2p2y = 60;
        GCS::Line l2(GCS::Point(&l2p1x, &l2p1y), GCS::Point(&l2p2x, &l2p2y));

        GCS::ConstraintMidpointOnLine constr(l1, l2);

        EXPECT_NEAR(constr.error(), 24.7487373415, 1e-8);

        EXPECT_NEAR(constr.grad(l1.p1.x), -0.353553390593, 1e-8);
        EXPECT_NEAR(constr.grad(l1.p1.y), 0.353553390593, 1e-8);
        EXPECT_NEAR(constr.grad(l1.p2.x), -0.353553390593, 1e-8);
        EXPECT_NEAR(constr.grad(l1.p2.y), 0.353553390593, 1e-8);
        EXPECT_NEAR(constr.grad(l2.p1.x), 0.397747564417, 1e-8);
        EXPECT_NEAR(constr.grad(l2.p1.y), -0.397747564417, 1e-8);
        EXPECT_NEAR(constr.grad(l2.p2.x), 0.309359216769, 1e-8);
        EXPECT_NEAR(constr.grad(l2.p2.y), -0.309359216769, 1e-8);
    }
}
TEST_F(ConstraintsTest, tangeantCircumfRegression)  // NOLINT
{
    /*
        This test is here to catch regressions, it was not validated by
        an external tool, if your patch makes this test fail, it *might*
        be valid, but do check!
    */

    {
        double c1x = 50;
        double c1y = -10;
        double r1 = 40;
        GCS::Circle c1(GCS::Point(&c1x, &c1y), &r1);

        double c2x = 20;
        double c2y = 20;
        double r2 = 10;
        GCS::Circle c2(GCS::Point(&c2x, &c2y), &r2);

        GCS::ConstraintTangentCircumf constr(c1.center, c2.center, c1.rad, c2.rad);

        EXPECT_NEAR(constr.error(), -700, 1e-8);

        EXPECT_NEAR(constr.grad(c1.center.x), 60, 1e-8);
        EXPECT_NEAR(constr.grad(c1.center.y), -60, 1e-8);
        EXPECT_NEAR(constr.grad(c1.rad), -100, 1e-8);
        EXPECT_NEAR(constr.grad(c2.center.x), -60, 1e-8);
        EXPECT_NEAR(constr.grad(c2.center.y), 60, 1e-8);
        EXPECT_NEAR(constr.grad(c2.rad), -100, 1e-8);
    }
    {
        double c1x = -20;
        double c1y = -10;
        double r1 = 30;
        GCS::Circle c1(GCS::Point(&c1x, &c1y), &r1);

        double c2x = 13;
        double c2y = 17;
        double r2 = 15;
        GCS::Circle c2(GCS::Point(&c2x, &c2y), &r2);

        GCS::ConstraintTangentCircumf constr(c1.center, c2.center, c1.rad, c2.rad);

        EXPECT_NEAR(constr.error(), -207, 1e-8);

        EXPECT_NEAR(constr.grad(c1.center.x), -66, 1e-8);
        EXPECT_NEAR(constr.grad(c1.center.y), -54, 1e-8);
        EXPECT_NEAR(constr.grad(c1.rad), -90, 1e-8);
        EXPECT_NEAR(constr.grad(c2.center.x), 66, 1e-8);
        EXPECT_NEAR(constr.grad(c2.center.y), 54, 1e-8);
        EXPECT_NEAR(constr.grad(c2.rad), -90, 1e-8);
    }
}
TEST_F(ConstraintsTest, pointOnEllipseRegression)  // NOLINT
{
    /*
        This test is here to catch regressions, it was not validated by
        an external tool, if your patch makes this test fail, it *might*
        be valid, but do check!
    */

    {
        double p0x = -50;
        double p0y = 20;
        GCS::Point p0(&p0x, &p0y);

        double focusx = 25.2;
        double focusy = 21.89;
        GCS::Point focus(&focusx, &focusy);

        double cx = 22.81;
        double cy = 4.7;
        GCS::Point center(&cx, &cy);

        double radmin = 21.41;

        GCS::Ellipse el(center, focus, &radmin);

        GCS::ConstraintPointOnEllipse constr(p0, el);

        EXPECT_NEAR(constr.error(), 97.6558983402, 1e-8);

        EXPECT_NEAR(constr.grad(p0.x), -1.90770030154, 1e-8);
        EXPECT_NEAR(constr.grad(p0.y), 0.393810475974, 1e-8);
        EXPECT_NEAR(constr.grad(el.focus1.x), -0.081766561895, 1e-8);
        EXPECT_NEAR(constr.grad(el.focus1.y), -0.803364458911, 1e-8);
        EXPECT_NEAR(constr.grad(el.center.x), 1.98946686343, 1e-8);
        EXPECT_NEAR(constr.grad(el.center.y), 0.409553982936, 1e-8);
        EXPECT_NEAR(constr.grad(el.radmin), -1.55365734542, 1e-8);
    }
    {
        double p0x = 10;
        double p0y = 10;
        GCS::Point p0(&p0x, &p0y);

        double focusx = 94.75;
        double focusy = -35.8;
        GCS::Point focus(&focusx, &focusy);

        double cx = 61;
        double cy = -42;
        GCS::Point center(&cx, &cy);

        double radmin = 60;

        GCS::Ellipse el(center, focus, &radmin);

        GCS::ConstraintPointOnEllipse constr(p0, el);

        EXPECT_NEAR(constr.error(), 18.7974219733, 1e-8);

        EXPECT_NEAR(constr.grad(p0.x), -1.16392591687, 1e-8);
        EXPECT_NEAR(constr.grad(p0.y), 1.43420336522, 1e-8);
        EXPECT_NEAR(constr.grad(el.focus1.x), -0.380988858027, 1e-8);
        EXPECT_NEAR(constr.grad(el.focus1.y), 0.303943497172, 1e-8);
        EXPECT_NEAR(constr.grad(el.center.x), 1.54491477489, 1e-8);
        EXPECT_NEAR(constr.grad(el.center.y), -1.73814686239, 1e-8);
        EXPECT_NEAR(constr.grad(el.radmin), -1.73612417504, 1e-8);
    }
}
TEST_F(ConstraintsTest, pointOnHyperbollaRegression)  // NOLINT
{
    /*
        This test is here to catch regressions, it was not validated by
        an external tool, if your patch makes this test fail, it *might*
        be valid, but do check!
    */

    {
        double p0x = 10;
        double p0y = -10;
        GCS::Point p0(&p0x, &p0y);

        double v1x = 30;
        double v1y = -24;

        double focusx = 23;
        double focusy = 30;

        GCS::Parabola par(GCS::Point(&v1x, &v1y), GCS::Point(&focusx, &focusy));

        GCS::ConstraintPointOnParabola constr(p0, par);

        EXPECT_NEAR(constr.error(), -28.8472475367, 1e-8);

        EXPECT_NEAR(constr.grad(p0.x), -0.180532043696, 1e-8);
        EXPECT_NEAR(constr.grad(p0.y), -1.94273657539, 1e-8);
        EXPECT_NEAR(constr.grad(par.vertex.x), -0.585557251732, 1e-8);
        EXPECT_NEAR(constr.grad(par.vertex.y), 1.9408282659, 1e-8);
        EXPECT_NEAR(constr.grad(par.focus1.x), 0.766089295428, 1e-8);
        EXPECT_NEAR(constr.grad(par.focus1.y), 0.00190830949738, 1e-8);
    }
    {
        double p0x = -10;
        double p0y = 10;
        GCS::Point p0(&p0x, &p0y);

        double v1x = 58;
        double v1y = 15;

        double focusx = 50;
        double focusy = 13;

        GCS::Parabola par(GCS::Point(&v1x, &v1y), GCS::Point(&focusx, &focusy));

        GCS::ConstraintPointOnParabola constr(p0, par);

        EXPECT_NEAR(constr.error(), -15.3536262028, 1e-8);

        EXPECT_NEAR(constr.grad(p0.x), -0.0286098387325, 1e-8);
        EXPECT_NEAR(constr.grad(p0.y), 0.192598008092, 1e-8);
        EXPECT_NEAR(constr.grad(par.vertex.x), -2.28268823564, 1e-8);
        EXPECT_NEAR(constr.grad(par.vertex.y), 0.884541691309, 1e-8);
        EXPECT_NEAR(constr.grad(par.focus1.x), 2.31129807437, 1e-8);
        EXPECT_NEAR(constr.grad(par.focus1.y), -1.0771396994, 1e-8);
    }
}
