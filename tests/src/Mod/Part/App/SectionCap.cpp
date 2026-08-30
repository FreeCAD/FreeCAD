// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <cmath>

#include <Mod/Part/App/SectionCap.h>

using namespace Part::SectionCap;

// NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
namespace
{

/// A closed box as a triangle soup, spanning 0..size in every axis.
TriangleSoup box(double size)
{
    TriangleSoup soup;
    const double s = size;
    using V = Base::Vector3d;
    soup.points = {
        V(0, 0, 0),
        V(s, 0, 0),
        V(s, s, 0),
        V(0, s, 0),  // bottom
        V(0, 0, s),
        V(s, 0, s),
        V(s, s, s),
        V(0, s, s),  // top
    };
    // 12 triangles, wound outwards
    soup.indices = {
        0, 2, 1, 0, 3, 2,  // bottom
        4, 5, 6, 4, 6, 7,  // top
        0, 1, 5, 0, 5, 4,  // front
        1, 2, 6, 1, 6, 5,  // right
        2, 3, 7, 2, 7, 6,  // back
        3, 0, 4, 3, 4, 7,  // left
    };
    return soup;
}

const Base::Vector3d Z(0, 0, 1);
const Base::Vector3d U(1, 0, 0);
const Base::Vector3d V(0, 1, 0);

/// The box the view provider measures once at harvest time and then rejects
/// planes against, without touching the triangles again.
Base::BoundBox3d boundsOf(const TriangleSoup& soup)
{
    return Base::BoundBox3d(soup.points.data(), soup.points.size());
}

/// Total area of a triangle soup, by the cross product of each triangle.
double soupArea(const TriangleSoup& soup)
{
    double total = 0.0;
    for (std::size_t i = 0; i + 2 < soup.indices.size(); i += 3) {
        const Base::Vector3d& a = soup.points[soup.indices[i]];
        const Base::Vector3d& b = soup.points[soup.indices[i + 1]];
        const Base::Vector3d& c = soup.points[soup.indices[i + 2]];
        total += 0.5 * ((b - a).Cross(c - a)).Length();
    }
    return total;
}

}  // namespace


TEST(SectionCapSlice, testPlaneThroughABoxCrossesEightTriangles)
{
    // Act - halfway up, so it cuts the four side walls
    const auto segments = sliceTriangles(box(10), Z, 5.0);

    // Assert - each of the four walls is two triangles, and both are crossed
    EXPECT_EQ(segments.size(), 8);
}

TEST(SectionCapSlice, testPlaneAboveTheBoxCrossesNothing)
{
    EXPECT_TRUE(sliceTriangles(box(10), Z, 50.0).empty());
}

TEST(SectionCapSlice, testPlaneBelowTheBoxCrossesNothing)
{
    EXPECT_TRUE(sliceTriangles(box(10), Z, -50.0).empty());
}

TEST(SectionCapSlice, testEverySegmentLiesOnThePlane)
{
    const auto segments = sliceTriangles(box(10), Z, 3.5);

    ASSERT_FALSE(segments.empty());
    for (const auto& s : segments) {
        EXPECT_NEAR(s.start.z, 3.5, 1e-9);
        EXPECT_NEAR(s.end.z, 3.5, 1e-9);
    }
}

TEST(SectionCapSlice, testAVertexExactlyOnThePlaneDoesNotDuplicateSegments)
{
    // a single triangle with one vertex sitting exactly on z = 0
    TriangleSoup soup;
    soup.points = {Base::Vector3d(0, 0, 0), Base::Vector3d(10, 0, -5), Base::Vector3d(10, 0, 5)};
    soup.indices = {0, 1, 2};

    const auto segments = sliceTriangles(soup, Z, 0.0);

    // the half open test must yield one crossing, not two or none
    EXPECT_EQ(segments.size(), 1);
}

TEST(SectionCapSlice, testATriangleTouchingThePlaneAtOneVertexYieldsNothing)
{
    // A triangle resting a single vertex on the plane does not cross it. Both
    // "crossings" collapse onto that vertex, so a naive sign test emits a
    // zero length segment that then pollutes the chaining.
    TriangleSoup soup;
    soup.points = {Base::Vector3d(0, 0, 0), Base::Vector3d(10, 0, 5), Base::Vector3d(0, 10, 5)};
    soup.indices = {0, 1, 2};

    const auto segments = sliceTriangles(soup, Z, 0.0);

    for (const auto& s : segments) {
        EXPECT_GT(Base::Distance(s.start, s.end), 1e-9) << "zero length segment emitted";
    }
}

TEST(SectionCapSlice, testDegenerateIndicesAreIgnored)
{
    TriangleSoup soup = box(10);
    soup.indices.push_back(99);  // out of range
    soup.indices.push_back(-1);
    soup.indices.push_back(0);

    EXPECT_NO_THROW(sliceTriangles(soup, Z, 5.0));
}

TEST(SectionCapSlice, testASingleTriangleCrossingYieldsItsSegment)
{
    // the per triangle entry point the Coin traversal uses directly
    using V = Base::Vector3d;

    auto segment = planeTriangleIntersection(V(0, 0, -5), V(10, 0, -5), V(5, 0, 5), Z, 0.0);
    ASSERT_TRUE(segment.has_value());
    EXPECT_NEAR(segment.value().start.z, 0.0, 1e-9);
    EXPECT_NEAR(segment.value().end.z, 0.0, 1e-9);
}

TEST(SectionCapSlice, testATriangleClearOfThePlaneYieldsNothing)
{
    using V = Base::Vector3d;

    auto segment = planeTriangleIntersection(V(0, 0, 5), V(10, 0, 5), V(5, 0, 9), Z, 0.0);
    ASSERT_FALSE(segment.has_value());

    auto segment2 = planeTriangleIntersection(V(0, 0, -5), V(10, 0, -5), V(5, 0, -9), Z, 0.0);
    ASSERT_FALSE(segment2.has_value());
}

TEST(SectionCapSlice, testThePerTriangleAndSoupPathsAgree)
{
    // The soup version is what the tests above exercise and what the viewer
    // bypasses, so the two must not be allowed to drift apart.
    const TriangleSoup soup = box(10);
    const auto viaSoup = sliceTriangles(soup, Z, 5.0);

    std::vector<Segment> viaTriangle;
    for (std::size_t i = 0; i + 2 < soup.indices.size(); i += 3) {
        auto segment = planeTriangleIntersection(
            soup.points[soup.indices[i]],
            soup.points[soup.indices[i + 1]],
            soup.points[soup.indices[i + 2]],
            Z,
            5.0
        );
        if (segment.has_value()) {
            viaTriangle.push_back(segment.value());
        }
    }

    ASSERT_EQ(viaSoup.size(), viaTriangle.size());
    for (std::size_t i = 0; i < viaSoup.size(); ++i) {
        EXPECT_NEAR(Base::Distance(viaSoup[i].start, viaTriangle[i].start), 0.0, 1e-12);
        EXPECT_NEAR(Base::Distance(viaSoup[i].end, viaTriangle[i].end), 0.0, 1e-12);
    }
}

TEST(SectionCapChain, testBoxSectionChainsIntoOneClosedLoop)
{
    // Arrange
    const auto segments = sliceTriangles(box(10), Z, 5.0);

    // Act
    const auto loops = chainLoops(segments, 1e-7);

    // Assert - the outline of a box is a single closed rectangle
    ASSERT_EQ(loops.size(), 1);
    EXPECT_TRUE(isClosed(loops[0], 1e-7));
}

TEST(SectionCapChain, testTheLoopEnclosesTheCrossSectionArea)
{
    const auto loops = chainLoops(sliceTriangles(box(10), Z, 5.0), 1e-7);

    ASSERT_EQ(loops.size(), 1);
    EXPECT_NEAR(soupArea(fillLoops(loops, U, V)), 100.0, 1e-3);
}

TEST(SectionCapChain, testTwoSeparateBodiesGiveTwoLoops)
{
    // Arrange - two boxes side by side, sliced together
    TriangleSoup soup = box(10);
    TriangleSoup other = box(10);
    const int base = static_cast<int>(soup.points.size());
    for (auto& p : other.points) {
        p.x += 100;
        soup.points.push_back(p);
    }
    for (int idx : other.indices) {
        soup.indices.push_back(idx + base);
    }

    // Act
    const auto loops = chainLoops(sliceTriangles(soup, Z, 5.0), 1e-7);

    // Assert
    ASSERT_EQ(loops.size(), 2);
    EXPECT_TRUE(isClosed(loops[0], 1e-7));
    EXPECT_TRUE(isClosed(loops[1], 1e-7));
}

TEST(SectionCapChain, testAnOpenOutlineIsStillReturned)
{
    // Arrange - a single wall, so the crossing cannot close
    TriangleSoup soup;
    soup.points = {
        Base::Vector3d(0, 0, -5),
        Base::Vector3d(10, 0, -5),
        Base::Vector3d(10, 0, 5),
        Base::Vector3d(0, 0, 5)
    };
    soup.indices = {0, 1, 2, 0, 2, 3};

    // Act
    const auto loops = chainLoops(sliceTriangles(soup, Z, 0.0), 1e-7);

    // Assert - a partial boundary is more use to draw than nothing at all
    ASSERT_EQ(loops.size(), 1);
    EXPECT_FALSE(isClosed(loops[0], 1e-7));
}

TEST(SectionCapChain, testEndpointsWithinToleranceAreJoined)
{
    // Arrange - a triangle whose corners miss each other by 1e-9
    using V = Base::Vector3d;
    std::vector<Segment> segments = {
        Segment {V(0, 0, 0), V(10, 0, 0)},
        Segment {V(10, 0, 1e-9), V(10, 10, 0)},
        Segment {V(10, 10, 0), V(0, 0, -1e-9)},
    };

    // Act
    const auto loops = chainLoops(segments, 1e-6);

    // Assert - tessellation seams must not break the chain
    ASSERT_EQ(loops.size(), 1);
    EXPECT_TRUE(isClosed(loops[0], 1e-6));
}

TEST(SectionCapChain, testNoSegmentsGivesNoLoops)
{
    EXPECT_TRUE(chainLoops({}, 1e-7).empty());
}

namespace
{


/// An axis aligned square loop on z = 0, wound counter clockwise.
std::vector<Base::Vector3d> square(double x0, double y0, double side)
{
    using Vec = Base::Vector3d;
    return {Vec(x0, y0, 0), Vec(x0 + side, y0, 0), Vec(x0 + side, y0 + side, 0), Vec(x0, y0 + side, 0)};
}

double totalLength(const std::vector<Segment>& segments)
{
    double sum = 0.0;
    for (const auto& s : segments) {
        sum += Base::Distance(s.start, s.end);
    }
    return sum;
}

}  // namespace

TEST(SectionCapFill, testASquareIsFilledWithItsOwnArea)
{
    // Arrange - a 10 x 10 square
    const std::vector<std::vector<Base::Vector3d>> loops = {square(0, 0, 10)};

    // Act
    const auto soup = fillLoops(loops, U, V);

    // Assert - the strips tile the square exactly, so the areas agree
    EXPECT_NEAR(soupArea(soup), 100.0, 1e-6);
}

TEST(SectionCapFill, testAHoleIsNotFilled)
{
    // Arrange - 10 x 10 square with a 4 x 4 hole
    const std::vector<std::vector<Base::Vector3d>> loops = {square(0, 0, 10), square(3, 3, 4)};

    // Act
    const auto soup = fillLoops(loops, U, V);

    // Assert - the hole's 16 mm2 is missing. Filling it would put a surface
    // across a bore, which is exactly what the section is meant to reveal.
    EXPECT_NEAR(soupArea(soup), 100.0 - 16.0, 0.5);
}

TEST(SectionCapFill, testTheFillStaysOnTheLoopsOwnPlane)
{
    auto loop = square(0, 0, 10);
    for (auto& p : loop) {
        p.z = 7.0;
    }

    const auto soup = fillLoops({loop}, U, V);

    ASSERT_FALSE(soup.points.empty());
    for (const auto& p : soup.points) {
        EXPECT_NEAR(p.z, 7.0, 1e-9);
    }
}

TEST(SectionCapFill, testEveryTriangleIndexIsInRange)
{
    const auto soup = fillLoops({square(0, 0, 10), square(3, 3, 4)}, U, V);

    ASSERT_FALSE(soup.indices.empty());
    EXPECT_EQ(soup.indices.size() % 3, 0);
    for (int index : soup.indices) {
        EXPECT_GE(index, 0);
        EXPECT_LT(static_cast<std::size_t>(index), soup.points.size());
    }
}

TEST(SectionCapFill, testFillRunsFromTheSlicedGeometry)
{
    // the whole chain, as the view provider drives it
    const auto loops = chainLoops(sliceTriangles(box(10), Z, 5.0), 1e-7);
    ASSERT_EQ(loops.size(), 1);

    const auto soup = fillLoops(loops, U, V);

    EXPECT_NEAR(soupArea(soup), 100.0, 1e-6);
}

TEST(SectionCapFill, testNonsenseInputIsRefused)
{
    EXPECT_TRUE(fillLoops({}, U, V).indices.empty());
    EXPECT_TRUE(fillLoops({{}}, U, V).indices.empty());
}
// --- gaps found by a coverage run ----------------------------------------

TEST(SectionCapExtent, testTheExtentSpansTheBodyAlongTheNormal)
{
    // This is what lets a body the plane misses be skipped without visiting a
    // single triangle, so it had better report the right range.
    double lo = 0.0;
    double hi = 0.0;

    ASSERT_TRUE(extentAlong(boundsOf(box(10)), Z, lo, hi));
    EXPECT_NEAR(lo, 0.0, 1e-12);
    EXPECT_NEAR(hi, 10.0, 1e-12);
}

TEST(SectionCapExtent, testTheExtentFollowsTheNormalGiven)
{
    // Measured along the direction asked for, not along z by habit
    double lo = 0.0;
    double hi = 0.0;
    const Base::Vector3d diagonal = Base::Vector3d(1, 1, 0).Normalize();

    ASSERT_TRUE(extentAlong(boundsOf(box(10)), diagonal, lo, hi));
    EXPECT_NEAR(lo, 0.0, 1e-12);
    EXPECT_NEAR(hi, 10.0 * std::sqrt(2.0), 1e-9);
}

TEST(SectionCapExtent, testAnEmptyBodyHasNoExtent)
{
    // False, rather than a range nothing can be rejected against
    double lo = 1.0;
    double hi = 2.0;

    EXPECT_FALSE(extentAlong(boundsOf(TriangleSoup {}), Z, lo, hi));
}

TEST(SectionCapExtent, testTheExtentIsMeasuredInConstantTime)
{
    // Taking the box rather than the soup is the whole point: a body with a
    // hundred times the triangles must report the same range, because the range
    // never depended on the triangles.
    const TriangleSoup coarse = box(10);
    TriangleSoup dense = coarse;
    for (int i = 0; i < 100; ++i) {
        dense.points.insert(dense.points.end(), coarse.points.begin(), coarse.points.end());
    }

    double coarseLo = 0.0;
    double coarseHi = 0.0;
    double denseLo = 0.0;
    double denseHi = 0.0;
    ASSERT_TRUE(extentAlong(boundsOf(coarse), Z, coarseLo, coarseHi));
    ASSERT_TRUE(extentAlong(boundsOf(dense), Z, denseLo, denseHi));

    EXPECT_NEAR(coarseLo, denseLo, 1e-12);
    EXPECT_NEAR(coarseHi, denseHi, 1e-12);
}

TEST(SectionCapSlice, testAnEmptySoupSlicesToNothing)
{
    EXPECT_TRUE(sliceTriangles(TriangleSoup {}, Z, 0.0).empty());
}

TEST(SectionCapChain, testALoopTooShortToEncloseAnythingIsNotClosed)
{
    using Vec = Base::Vector3d;
    // two coincident points are not a loop, however close the ends are
    EXPECT_FALSE(isClosed({Vec(0, 0, 0), Vec(0, 0, 0)}, 1e-6));
    EXPECT_FALSE(isClosed({}, 1e-6));
}

TEST(SectionCapFill, testDegenerateLoopsFillNothing)
{
    // Loops of fewer than three points contribute no edges, so there is
    // nothing for the sweep to stand on.
    using Vec = Base::Vector3d;
    const std::vector<std::vector<Base::Vector3d>> degenerate = {
        {Vec(0, 0, 0), Vec(10, 0, 0)},
        {Vec(5, 5, 0)},
    };

    EXPECT_TRUE(fillLoops(degenerate, U, V).indices.empty());
}

TEST(SectionCapFill, testAFlatRegionFillsNothing)
{
    // Every point on one line: no edge crosses a level, so there is no band
    // to fill.
    using Vec = Base::Vector3d;
    const std::vector<std::vector<Base::Vector3d>> flat = {
        {Vec(0, 0, 0), Vec(10, 0, 0), Vec(20, 0, 0)},
    };

    EXPECT_TRUE(fillLoops(flat, U, V).indices.empty());
}

TEST(SectionCapFill, testNotANumberInALoopIsSkippedRatherThanPoisoningTheFill)
{
    // One bad vertex must not take the whole cap with it
    using Vec = Base::Vector3d;
    const double nan = std::nan("");
    std::vector<Base::Vector3d> loop = square(0, 0, 10);
    loop.push_back(Vec(nan, nan, 0));

    const auto soup = fillLoops({loop}, U, V);

    for (const auto& p : soup.points) {
        EXPECT_TRUE(std::isfinite(p.x));
        EXPECT_TRUE(std::isfinite(p.y));
        EXPECT_TRUE(std::isfinite(p.z));
    }
}

// --- hatching a triangulated cap -----------------------------------------
//
// Both result modes come through here: Geometry mode has triangles from OCCT,
// Display mode gets them from fillLoops.

namespace
{
/// A square as two triangles, so the tests below start from a cap rather than
/// from a boundary.
TriangleSoup squareSoup(double x0, double y0, double size)
{
    TriangleSoup soup;
    soup.points = {
        Base::Vector3d(x0, y0, 0),
        Base::Vector3d(x0 + size, y0, 0),
        Base::Vector3d(x0 + size, y0 + size, 0),
        Base::Vector3d(x0, y0 + size, 0)
    };
    soup.indices = {0, 1, 2, 0, 2, 3};
    return soup;
}
}  // namespace

TEST(SectionCapHatchTriangles, testASquareCapIsFilledWithEvenlySpacedLines)
{
    // Arrange - the same 10 x 10 square the loop based tests use, but already
    // triangulated, hatched horizontally every 1 mm
    const auto cap = squareSoup(0, 0, 10);

    // Act - lines march along Y, so a line is a set of points with constant y
    const auto hatch = hatchTriangles(cap, V, 1.0);

    // Assert - each level crosses both triangles, so it arrives in two pieces
    // rather than one. Total length is what matters, not the count.
    EXPECT_NEAR(totalLength(hatch), 10 * 10.0, 1e-9);
}

TEST(SectionCapHatchTriangles, testAHoleIsLeftUnhatchedWithoutBeingIdentified)
{
    // Arrange - a square cap with the middle left untriangulated. There is no
    // hole loop anywhere; the hole is simply an absence of triangles, which is
    // the whole point of this path.
    TriangleSoup cap = squareSoup(0, 0, 10);
    const auto missing = squareSoup(3, 3, 4);
    // subtract by rebuilding the ring around the missing middle as four bands
    cap.points.clear();
    cap.indices.clear();
    auto addQuad = [&cap](double x0, double y0, double w, double h) {
        const auto base = static_cast<int>(cap.points.size());
        cap.points.push_back(Base::Vector3d(x0, y0, 0));
        cap.points.push_back(Base::Vector3d(x0 + w, y0, 0));
        cap.points.push_back(Base::Vector3d(x0 + w, y0 + h, 0));
        cap.points.push_back(Base::Vector3d(x0, y0 + h, 0));
        for (int i : {0, 1, 2, 0, 2, 3}) {
            cap.indices.push_back(base + i);
        }
    };
    addQuad(0, 0, 10, 3);  // below the hole
    addQuad(0, 7, 10, 3);  // above it
    addQuad(0, 3, 3, 4);   // left of it
    addQuad(7, 3, 3, 4);   // right of it

    // Act
    const auto hatch = hatchTriangles(cap, V, 1.0);

    // Assert - the same 100 - 16 the loop based version gives for a hole
    EXPECT_NEAR(totalLength(hatch), 10 * 10.0 - 4 * 4.0, 1e-9);

    // and nothing crosses where the triangles are missing
    for (const auto& s : hatch) {
        const bool spansHole = std::min(s.start.x, s.end.x) < 3.0
            && std::max(s.start.x, s.end.x) > 7.0 && s.start.y >= 3.0 && s.start.y < 7.0;
        EXPECT_FALSE(spansHole) << "hatch crossed the gap at y = " << s.start.y;
    }
}

TEST(SectionCapHatchTriangles, testTheDirectionSetsTheAngle)
{
    const auto cap = squareSoup(0, 0, 10);

    // Marching along X instead of Y turns the pattern a quarter turn. The
    // square is symmetric, so the same length arrives either way - what changes
    // is which coordinate the lines hold constant.
    const auto acrossY = hatchTriangles(cap, V, 1.0);
    const auto acrossX = hatchTriangles(cap, U, 1.0);

    EXPECT_NEAR(totalLength(acrossY), totalLength(acrossX), 1e-9);
    for (const auto& s : acrossY) {
        EXPECT_NEAR(s.start.y, s.end.y, 1e-9) << "lines marching along V must hold y";
    }
    for (const auto& s : acrossX) {
        EXPECT_NEAR(s.start.x, s.end.x, 1e-9) << "lines marching along U must hold x";
    }
}

TEST(SectionCapHatchTriangles, testTheGridIsAbsoluteNotPerBody)
{
    // Two caps at different places must put their lines on the same grid, or
    // neighbouring parts in an assembly would hatch out of step with each
    // other. Absolute multiples of the spacing are what guarantee it.
    const auto near = squareSoup(0, 0, 10);
    const auto far = squareSoup(100, 40, 10);

    const auto hatchNear = hatchTriangles(near, V, 2.0);
    const auto hatchFar = hatchTriangles(far, V, 2.0);

    ASSERT_FALSE(hatchNear.empty());
    ASSERT_FALSE(hatchFar.empty());
    for (const auto& s : hatchNear) {
        EXPECT_NEAR(std::fmod(s.start.y, 2.0), 0.0, 1e-9);
    }
    for (const auto& s : hatchFar) {
        EXPECT_NEAR(std::fmod(s.start.y, 2.0), 0.0, 1e-9);
    }
}

TEST(SectionCapHatchTriangles, testACapLyingOffTheOriginKeepsItsPlane)
{
    // The cap need not sit at z = 0; whatever plane the triangles are on, the
    // hatch has to come back on that same plane rather than at the origin.
    TriangleSoup cap = squareSoup(0, 0, 10);
    for (auto& p : cap.points) {
        p.z = 7.0;
    }

    const auto hatch = hatchTriangles(cap, V, 1.0);

    ASSERT_FALSE(hatch.empty());
    for (const auto& s : hatch) {
        EXPECT_NEAR(s.start.z, 7.0, 1e-9);
        EXPECT_NEAR(s.end.z, 7.0, 1e-9);
    }
}

TEST(SectionCapHatchTriangles, testATriangleTouchingALineAtOneVertexIsNotADash)
{
    // Two crossings collapse onto the vertex, which is a touch rather than a
    // crossing. A zero length segment would be drawn as nothing at best and
    // confuse a consumer at worst.
    TriangleSoup cap;
    cap.points = {Base::Vector3d(0, 0, 0), Base::Vector3d(4, 0, 0), Base::Vector3d(2, 3, 0)};
    cap.indices = {0, 1, 2};

    // spacing of 3 puts a line exactly on the apex
    const auto hatch = hatchTriangles(cap, V, 3.0);

    for (const auto& s : hatch) {
        EXPECT_GT(Base::Distance(s.start, s.end), 0.0) << "emitted a zero length hatch line";
    }
}

TEST(SectionCapHatchTriangles, testNonsenseInputIsRefused)
{
    const auto cap = squareSoup(0, 0, 10);

    EXPECT_TRUE(hatchTriangles({}, V, 1.0).empty());
    EXPECT_TRUE(hatchTriangles(cap, V, 0.0).empty());
    EXPECT_TRUE(hatchTriangles(cap, V, -1.0).empty());
    // a direction of no length gives no direction to march
    EXPECT_TRUE(hatchTriangles(cap, Base::Vector3d(0, 0, 0), 1.0).empty());
}

TEST(SectionCapHatchTriangles, testAnAbsurdlyFineSpacingIsBounded)
{
    // A spacing far below the geometry is a mistake, not a request. The bound
    // has to hold rather than the call trying to allocate its way through it.
    const auto cap = squareSoup(0, 0, 10);

    const auto hatch = hatchTriangles(cap, V, 1e-9, 500);

    EXPECT_LE(hatch.size(), 500u);
}

TEST(SectionCapHatchTriangles, testABadIndexIsSkippedNotDereferenced)
{
    TriangleSoup cap = squareSoup(0, 0, 10);
    cap.indices.push_back(0);
    cap.indices.push_back(1);
    cap.indices.push_back(99);  // past the end

    // the valid triangles still hatch, and the broken one is simply not visited
    const auto hatch = hatchTriangles(cap, V, 1.0);
    EXPECT_NEAR(totalLength(hatch), 10 * 10.0, 1e-9);
}

// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
