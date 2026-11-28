// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <list>
#include <memory>
#include <ostream>
#include <utility>
#include <vector>

#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <GeomConvert.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_Curve.hxx>
#include <Geom_Ellipse.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <TColStd_Array1OfInteger.hxx>
#include <TColStd_Array1OfReal.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <gp.hxx>
#include <gp_Ax2.hxx>
#include <gp_Pnt.hxx>

#include "Mod/Part/App/Geometry.h"
#include <src/App/InitApplication.h>

namespace
{

constexpr double tolerance = 0.01;

// Non-rational Bezier curve in the XY plane: a single-span B-spline over [0, 1] with the given poles
Handle(Geom_BSplineCurve) makeBezier(const std::vector<std::pair<double, double>>& xy)
{
    const int n = static_cast<int>(xy.size());
    TColgp_Array1OfPnt poles(1, n);
    for (int i = 0; i < n; ++i) {
        poles.SetValue(i + 1, gp_Pnt(xy[i].first, xy[i].second, 0));
    }
    TColStd_Array1OfReal knots(1, 2);
    knots.SetValue(1, 0.0);
    knots.SetValue(2, 1.0);
    TColStd_Array1OfInteger mults(1, 2);
    mults.SetValue(1, n);
    mults.SetValue(2, n);
    return new Geom_BSplineCurve(poles, knots, mults, n - 1);
}

// Distance from p to a bounded segment (trimmed arc or line): the nearer of its end points and of
// any perpendicular foot inside its parameter range
double distanceToSegment(const gp_Pnt& p, const Handle(Geom_Curve) & segment)
{
    double d = std::min(
        p.Distance(segment->Value(segment->FirstParameter())),
        p.Distance(segment->Value(segment->LastParameter()))
    );
    GeomAPI_ProjectPointOnCurve projection(p, segment);
    if (projection.NbPoints() > 0) {
        d = std::min(d, projection.LowerDistance());
    }
    return d;
}

struct BiArcsResult
{
    size_t segments = 0;
    size_t samples = 0;
    double worstDistance = 0;   // largest distance from a curve sample to the toBiArcs output
    double worstParameter = 0;  // curve parameter of that sample
    double maxGap = 0;          // largest distance between two consecutive curve samples
    double gapParameter = 0;    // curve parameter where that gap starts
};

std::ostream& operator<<(std::ostream& os, const BiArcsResult& r)
{
    return os << r.segments << " segments; worst sampled distance " << r.worstDistance
              << " at t = " << r.worstParameter << "; " << r.samples
              << " samples, largest gap between samples " << r.maxGap
              << " at t = " << r.gapParameter;
}

// Approximate `curve` with toBiArcs(tolerance) and measure how far the curve gets from the result,
// at `samples` points evenly spaced in the curve's parameter. Also records the largest distance
// between consecutive samples, so expectInSpec can check the sampling was fine enough.
BiArcsResult approximateWithBiArcs(const Handle(Geom_BSplineCurve) & curve, int samples)
{
    BiArcsResult result;

    std::vector<std::unique_ptr<Part::Geometry>> owned;
    std::vector<Handle(Geom_Curve)> segments;
    for (Part::Geometry* geometry : Part::GeomBSplineCurve(curve).toBiArcs(tolerance)) {
        owned.emplace_back(geometry);
        segments.push_back(Handle(Geom_Curve)::DownCast(geometry->handle()));
    }
    result.segments = segments.size();
    if (segments.empty()) {
        result.worstDistance = std::numeric_limits<double>::infinity();
        return result;
    }

    const double first = curve->FirstParameter();
    const double last = curve->LastParameter();
    const int intervals = std::max(1, samples - 1);
    result.samples = intervals + 1;

    gp_Pnt previous = curve->Value(first);
    for (int k = 0; k <= intervals; ++k) {
        const double t = first + (last - first) * k / intervals;
        const gp_Pnt p = curve->Value(t);
        if (k > 0 && p.Distance(previous) > result.maxGap) {
            result.maxGap = p.Distance(previous);
            result.gapParameter = first + (last - first) * (k - 1) / intervals;
        }
        previous = p;
        double d = std::numeric_limits<double>::infinity();
        for (const auto& segment : segments) {
            d = std::min(d, distanceToSegment(p, segment));
        }
        if (d > result.worstDistance) {
            result.worstDistance = d;
            result.worstParameter = t;
        }
    }
    return result;
}

// Check a result from approximateWithBiArcs. Two separate checks, with different meanings:
//  1. Sampling: consecutive samples must be at most tolerance / 2 apart. If not, the TEST is at
//     fault: it did not sample finely enough to judge the result. Increase its sample count.
//  2. Fit: every sample must be within tolerance of the toBiArcs output. If not, TOBIARCS is at
//     fault: its output is not within tolerance of the curve.
// (With 1 satisfied, a curve that does not loop away between neighbouring samples is within about
// 1.5 * tolerance of the output everywhere.)
void expectInSpec(const BiArcsResult& r)
{
    EXPECT_LE(r.maxGap, tolerance / 2)
        << "Test sampling too coarse (a problem with the test, not with toBiArcs): consecutive "
           "samples are up to "
        << r.maxGap << " apart, more than tolerance / 2; increase the sample count. " << r;
    EXPECT_LE(r.worstDistance, tolerance)
        << "toBiArcs output is out of tolerance: a curve sample is " << r.worstDistance
        << " from it, more than " << tolerance << ". " << r;
}

}  // namespace

class BSplineCurveBiArcsTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }
};

// Control: a curve toBiArcs handles correctly, needing more than one split (a single split gives 4
// arcs)

TEST_F(BSplineCurveBiArcsTest, basicArch)
{
    const BiArcsResult r = approximateWithBiArcs(makeBezier({{0, 0}, {2, 8}, {8, 8}, {10, 0}}), 8000);
    EXPECT_GT(r.segments, 4U) << r;
    expectInSpec(r);
}

// Control: a straight line, with unevenly spaced poles so the parameterization is not uniform
TEST_F(BSplineCurveBiArcsTest, straightLine)
{
    const BiArcsResult r = approximateWithBiArcs(makeBezier({{0, 0}, {1, 2}, {4, 8}, {5, 10}}), 4000);
    expectInSpec(r);
}

// The fit is only checked at t = 1/4 and 3/4 of each span. This curve has the same end points and
// end tangents as a semicircle (center (5,0), R 5) and is within tolerance of it at t = 1/4 and
// 3/4, but P2 pulls it up to (5, 6.125) at t = 1/2, 1.125 away from the semicircle.
TEST_F(BSplineCurveBiArcsTest, quarterPointOverlap)
{
    const BiArcsResult r
        = approximateWithBiArcs(makeBezier({{0, 0}, {0, 1}, {5, 15}, {10, 1}, {10, 0}}), 6000);
    expectInSpec(r);
}

// Like the curve in issue #34515: P1 and P3 sit slightly behind the end points, so the derivative
// at both ends points backwards and the curve makes a tiny U-turn before following the short arc
// over the top of the circle (center (0,0), R 5) from (-3,4) to (3,4). The biarc fitted to the
// backwards tangents is exactly that circle the long way round, and the checked points lie on it.
TEST_F(BSplineCurveBiArcsTest, endTurnAround)
{
    const BiArcsResult r = approximateWithBiArcs(
        makeBezier({{-3, 4}, {-3.04, 3.97}, {0, 6.66}, {3.04, 3.97}, {3, 4}}),
        3000
    );
    expectInSpec(r);
}

// Same as endTurnAround, with P1 and P3 ten times further behind the end points
TEST_F(BSplineCurveBiArcsTest, endBigTurnAround)
{
    const BiArcsResult r
        = approximateWithBiArcs(makeBezier({{-3, 4}, {-3.4, 3.7}, {0, 7}, {3.4, 3.7}, {3, 4}}), 3000);
    expectInSpec(r);
}

// The end tangents are parallel, so calculateBiArcPoints checks straightness with one more tangent
// at t_mid = 10% into the span, but only if fabs(t_mid) > 0.1. Here t_mid is exactly 0.1, so the
// check is skipped and the S-curve is replaced by a straight line, which is never validated.
TEST_F(BSplineCurveBiArcsTest, parallelEndTangentsSCurve)
{
    const BiArcsResult r = approximateWithBiArcs(makeBezier({{0, 0}, {4, 6}, {6, -6}, {10, 0}}), 6000);
    expectInSpec(r);
}

// Like parallelEndTangentsSCurve, but the tangent at t = 0.1 (where the parallel-tangent branch
// looks for a change of direction) is also exactly horizontal: x(t) = 10t and
// y(t) = 64 t^2 (1-t)^2 (t - 5/32), so y'(0) = y'(0.1) = y'(1) = 0. The extra tangent check
// therefore passes and the curve, which reaches |y| = 1.65, is replaced by a straight line.
TEST_F(BSplineCurveBiArcsTest, parallelTangentsAtTenPercent)
{
    const BiArcsResult r = approximateWithBiArcs(
        makeBezier({{0, 0}, {2, 0}, {4, -1}, {6, 5.4}, {8, 0}, {10, 0}}),
        4000
    );
    expectInSpec(r);
}


// A NURBS: an arc of an ellipse (semi-axes 10 and 3, through the tight end of the major axis at
// angle pi), converted exactly to a rational B-spline with several spans
TEST_F(BSplineCurveBiArcsTest, ellipseArc)
{
    Handle(Geom_Ellipse) ellipse = new Geom_Ellipse(gp_Ax2(gp_Pnt(0, 0, 0), gp::DZ()), 10, 3);
    Handle(Geom_TrimmedCurve) arc = new Geom_TrimmedCurve(ellipse, 0.3, 4.0);
    Handle(Geom_BSplineCurve) nurbs = GeomConvert::CurveToBSplineCurve(arc);
    ASSERT_TRUE(nurbs->IsRational());

    const BiArcsResult r = approximateWithBiArcs(nurbs, 12000);
    expectInSpec(r);
}
