// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"

#include "Mod/Sketcher/App/planegcs/GCS.h"
#include "Mod/Sketcher/App/planegcs/Geo.h"

class GeoTest: public ::testing::Test
{
protected:
    void SetUp() override
    {}

    void TearDown() override
    {}
};

TEST_F(GeoTest, bezierCurveTest)  // NOLINT
{
    // Arrange
    // We can use the same pointers for B-spline and Bezier curves because
    // we are not solving anything here.
    // Additionally, this ensures that the same data is provided (as much as possible).
    std::vector<double> controlPointsX(4);
    std::vector<double> controlPointsY(4);
    controlPointsX[0] = 0.0;
    controlPointsY[0] = 10.0;
    controlPointsX[1] = 0.0;
    controlPointsY[1] = 6.0;
    controlPointsX[2] = 6.0;
    controlPointsY[2] = 0.5;
    controlPointsX[3] = 16.0;
    controlPointsY[3] = 0.5;
    std::vector<GCS::Point> controlPoints(4);
    for (size_t i = 0; i < controlPoints.size(); ++i) {
        controlPoints[i].x = &controlPointsX[i];
        controlPoints[i].y = &controlPointsY[i];
    }
    double bSplineStartX = 0.0, bSplineEndX = 16.0;
    double bSplineStartY = 10.0, bSplineEndY = 0.5;
    GCS::Point bSplineStart, bSplineEnd;
    bSplineStart.x = &bSplineStartX;
    bSplineStart.y = &bSplineStartY;
    bSplineEnd.x = &bSplineEndX;
    bSplineEnd.y = &bSplineEndY;
    std::vector<double> weights(controlPoints.size(), 1.0);
    std::vector<double*> weightsAsPtr;
    for (size_t i = 0; i < controlPoints.size(); ++i) {
        weightsAsPtr.push_back(&weights[i]);
    }
    std::vector<double> knots = {0.0, 1.0};
    std::vector<double*> knotsAsPtr;
    for (size_t i = 0; i < knots.size(); ++i) {
        knotsAsPtr.push_back(&knots[i]);
    }
    std::vector<int> mult(controlPoints.size() - 2, 1);  // Hardcoded for cubic
    mult.front() = 4;                                    // Hardcoded for cubic
    mult.back() = 4;                                     // Hardcoded for cubic
    GCS::BSpline bspline;
    bspline.start = bSplineStart;
    bspline.end = bSplineEnd;
    bspline.poles = controlPoints;
    bspline.weights = weightsAsPtr;
    bspline.knots = knotsAsPtr;
    bspline.mult = mult;
    bspline.degree = 3;
    bspline.periodic = false;
    bspline.setupFlattenedKnots();
    double bezierStartX = 0.0, bezierEndX = 16.0;
    double bezierStartY = 10.0, bezierEndY = 0.5;
    GCS::Point bezierStart, bezierEnd;
    bezierStart.x = &bezierStartX;
    bezierStart.y = &bezierStartY;
    bezierEnd.x = &bezierEndX;
    bezierEnd.y = &bezierEndY;
    GCS::BezierCurve bezier;
    bezier.start = bezierStart;
    bezier.end = bezierEnd;
    bezier.poles = controlPoints;
    bezier.weights = weightsAsPtr;
    bezier.degree = 3;
    double param = 0.35;

    // Act
    auto pointAtBSplineParam = bspline.Value(param, 1.0);
    auto pointAtBezierParam = bezier.Value(param, 1.0);
    EXPECT_DOUBLE_EQ(pointAtBSplineParam.x, pointAtBezierParam.x);
    EXPECT_DOUBLE_EQ(pointAtBSplineParam.y, pointAtBezierParam.y);


    // Assert
    // TODO: Check value at arbitrary value
}
