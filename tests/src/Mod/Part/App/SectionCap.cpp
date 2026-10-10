// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <numbers>

#include <Mod/Part/App/SectionCap.h>

#include <algorithm>
#include <cmath>
#include <map>
#include <numbers>

#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <TopoDS_Shape.hxx>
#include <BRep_Builder.hxx>
#include <BRepTools.hxx>
#include <BRepMesh_IncrementalMesh.hxx>

#include "Mod/Part/App/BRepMesh.h"

#include <filesystem>
#include <string>

#include <App/Application.h>
#include <src/App/InitApplication.h>

#include "Mod/Part/App/FeatureSectionAnalysis.h"

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
    const auto loops = chainLoops(segments);

    // Assert - the outline of a box is a single closed rectangle
    ASSERT_EQ(loops.size(), 1);
    EXPECT_TRUE(isClosed(loops[0], 1e-7));
}

TEST(SectionCapChain, testTheLoopEnclosesTheCrossSectionArea)
{
    const auto loops = chainLoops(sliceTriangles(box(10), Z, 5.0));

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
    const auto loops = chainLoops(sliceTriangles(soup, Z, 5.0));

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
    const auto loops = chainLoops(sliceTriangles(soup, Z, 0.0));

    // Assert - a partial boundary is more use to draw than nothing at all
    ASSERT_EQ(loops.size(), 1);
    EXPECT_FALSE(isClosed(loops[0], 1e-7));
}

TEST(SectionCapChain, testNoSegmentsGivesNoLoops)
{
    EXPECT_TRUE(chainLoops({}).empty());
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

namespace
{
/// Build a closed contour's segments the way slicing would: each crossing sits
/// on its own mesh edge, so chaining matches them by identity.
std::vector<Segment> keyedRing(const std::vector<Base::Vector3d>& points, int firstEdge)
{
    std::vector<Segment> out;
    const int n = static_cast<int>(points.size());
    for (int i = 0; i < n; ++i) {
        const int j = (i + 1) % n;
        out.push_back(
            Segment {
                points[i],
                points[j],
                MeshEdge {firstEdge + i, firstEdge + i},
                MeshEdge {firstEdge + j, firstEdge + j},
            }
        );
    }
    return out;
}
}  // namespace


TEST(SectionCapChain, testASliverAtTheStartDoesNotStealTheOutline)
{
    // Arrange - a cut grazing a chamfer leaves a pair of near coincident
    // segments in the soup. Chaining used to close on them because their ends
    // were within tolerance of each other, abandoning the outline they belong
    // to. Matching by mesh edge instead, nearness cannot mislead it.
    const std::vector<Base::Vector3d> contour = {
        Base::Vector3d(0, 0, 0),
        Base::Vector3d(0.005, 0, 0),  // the sliver, first in the list
        Base::Vector3d(0.0001, 0.0001, 0),
        Base::Vector3d(10, 0, 0),
        Base::Vector3d(10, 10, 0),
        Base::Vector3d(0, 10, 0),
    };
    const auto segments = keyedRing(contour, 0);

    // Act
    const auto loops = chainLoops(segments);

    // Assert - one outline that spans the square, not a sliver plus wreckage
    ASSERT_EQ(loops.size(), 1U);
    EXPECT_TRUE(isClosedExactly(loops[0]));
    double span = 0.0;
    for (const auto& point : loops[0]) {
        span = std::max(span, point.x);
    }
    EXPECT_GT(span, 9.0) << "the walk kept the outline, not the sliver";
}

namespace
{
/// How many edges of the soup are used by only one triangle. A closed manifold
/// has none, which is what makes every plane cut through it close.
int boundaryEdgeCount(const TriangleSoup& soup)
{
    std::map<std::pair<int, int>, int> uses;
    for (std::size_t i = 0; i + 2 < soup.indices.size(); i += 3) {
        const int v[3] = {soup.indices[i], soup.indices[i + 1], soup.indices[i + 2]};
        for (int e = 0; e < 3; ++e) {
            const int a = v[e];
            const int b = v[(e + 1) % 3];
            ++uses[{std::min(a, b), std::max(a, b)}];
        }
    }
    return static_cast<int>(std::count_if(uses.begin(), uses.end(), [](const auto& it) {
        return it.second == 1;
    }));
}
}  // namespace

class SectionCapSavedSolid: public ::testing::Test
{
protected:
    void SetUp() override
    {
        tests::initApplication();
    }
};

TEST_F(SectionCapSavedSolid, closesAtEveryAngle)
{
    // The guide from a real jig: a slender column with chamfers top and bottom.
    // Cuts that leave through its flat bottom came out hollow, because the
    // contour broke where it crossed from the side wall onto the end face.
    const std::string path = App::Application::getHomePath() + "/tests/brepfiles/sectioncolumn1.brep";
    if (!std::filesystem::exists(path)) {
        GTEST_SKIP() << "asset not installed: " << path;
    }
    BRep_Builder builder;
    TopoDS_Shape shape;
    ASSERT_TRUE(BRepTools::Read(shape, path.c_str(), builder));
    ASSERT_FALSE(shape.IsNull());

    // Meshed the way the view provider meshes it, deflection and all.
    const auto soup = Part::SectionCap::meshSolid(shape);
    ASSERT_GT(soup.indices.size(), 0U);
    EXPECT_EQ(boundaryEdgeCount(soup), 0) << "a solid must mesh to a closed manifold";

    int cuts = 0;
    int openContours = 0;
    for (int tilt = 0; tilt <= 25; ++tilt) {
        const double a = tilt * (std::numbers::pi / 180.0);
        Base::Vector3d normal(0.0, -std::cos(a), std::sin(a));
        normal.Normalize();

        double low = std::numeric_limits<double>::max();
        double high = std::numeric_limits<double>::lowest();
        for (const auto& point : soup.points) {
            const double d = point * normal;
            low = std::min(low, d);
            high = std::max(high, d);
        }

        for (double offset = low + 0.5; offset < high - 0.5; offset += 0.7) {
            const auto loops = chainLoops(sliceTriangles(soup, normal, offset));
            if (loops.empty()) {
                continue;
            }
            for (const auto& loop : loops) {
                if (!isClosedExactly(loop)) {
                    ++openContours;
                    if (openContours == 1) {
                        ADD_FAILURE()
                            << "open contour at tilt " << tilt << " offset " << offset << " ("
                            << loops.size() << " loops, " << loop.size() << " points, ends "
                            << Base::Distance(loop.front(), loop.back()) << " mm apart)";
                    }
                }
            }
            ++cuts;
        }
    }
    EXPECT_GT(cuts, 500);
    EXPECT_EQ(openContours, 0) << openContours << " contours of " << cuts << " cuts did not close";
}

TEST(SectionCapMesh, testASolidMeshesWatertight)
{
    // The invariant the whole cap rests on. A shape's own triangulation is per
    // face and does not meet along shared edges; this one has to.
    const TopoDS_Shape box = BRepPrimAPI_MakeBox(10.0, 20.0, 30.0).Shape();

    const auto soup = Part::SectionCap::meshSolid(box);

    EXPECT_GT(soup.indices.size(), 0U);
    EXPECT_EQ(boundaryEdgeCount(soup), 0) << "a solid must mesh to a closed manifold";
}

TEST(SectionCapMesh, testACurvedSolidMeshesWatertight)
{
    // A cylinder's seam is where per face triangulation comes apart.
    const TopoDS_Shape cylinder = BRepPrimAPI_MakeCylinder(7.5, 220.0).Shape();

    const auto soup = Part::SectionCap::meshSolid(cylinder);

    EXPECT_EQ(boundaryEdgeCount(soup), 0);
}

TEST(SectionCapMesh, testEveryCutOfASolidClosesAtEveryAngle)
{
    // The behaviour the user sees: sweep the plane and the cap never blinks.
    const TopoDS_Shape cylinder = BRepPrimAPI_MakeCylinder(7.5, 220.0).Shape();
    const auto soup = Part::SectionCap::meshSolid(cylinder);
    ASSERT_EQ(boundaryEdgeCount(soup), 0);

    int cuts = 0;
    for (int tilt = 0; tilt <= 40; ++tilt) {
        const double a = tilt * (std::numbers::pi / 180.0);
        const Base::Vector3d normal(0.0, std::sin(a), std::cos(a));

        // Only where the plane actually meets the solid; outside that there is
        // rightly nothing to cut.
        double low = std::numeric_limits<double>::max();
        double high = std::numeric_limits<double>::lowest();
        for (const auto& point : soup.points) {
            const double d = point * normal;
            low = std::min(low, d);
            high = std::max(high, d);
        }

        for (double offset = low + 1.0; offset < high - 1.0; offset += 3.7) {
            const auto loops = chainLoops(sliceTriangles(soup, normal, offset));
            ASSERT_FALSE(loops.empty()) << "no contour at tilt " << tilt << " offset " << offset;
            for (const auto& loop : loops) {
                EXPECT_TRUE(isClosedExactly(loop))
                    << "open contour at tilt " << tilt << " offset " << offset;
            }
            ++cuts;
        }
    }
    EXPECT_GT(cuts, 1000);
}

TEST(SectionCapChain, testTwoRegionsOfOneBodyAreNotJoined)
{
    // A fork or a U sections into separate regions. They are each closed, and
    // joining them would weld a bridge across the gap between the prongs.
    auto square = [](double x0) {
        return std::vector<Base::Vector3d> {
            Base::Vector3d(x0, 0, 0),
            Base::Vector3d(x0 + 4, 0, 0),
            Base::Vector3d(x0 + 4, 4, 0),
            Base::Vector3d(x0, 4, 0),
        };
    };
    std::vector<Segment> segments = keyedRing(square(0), 0);
    const auto second = keyedRing(square(20), 100);
    segments.insert(segments.end(), second.begin(), second.end());

    const auto loops = chainLoops(segments);

    ASSERT_EQ(loops.size(), 2U);
    for (const auto& loop : loops) {
        EXPECT_TRUE(isClosedExactly(loop));
    }
}

TEST(SectionCapChain, testTwoLoopsTouchingAtAPointStayTwoLoops)
{
    // Arrange - two squares meeting at the origin, as a cross section does
    // wherever a body pinches or a hole reaches its outer wall. Four segment
    // ends meet at that point, so the walk has to pick the one that stays on
    // the boundary it is already tracing.
    // The two squares meet in space but not in the mesh: the pinch point is two
    // distinct crossings, one on each boundary, so their keys differ.
    std::vector<Segment> segments = keyedRing(
        {Base::Vector3d(0, 0, 0),
         Base::Vector3d(1, 0, 0),
         Base::Vector3d(1, 1, 0),
         Base::Vector3d(0, 1, 0)},
        0
    );
    const auto below = keyedRing(
        {Base::Vector3d(0, 0, 0),
         Base::Vector3d(-1, 0, 0),
         Base::Vector3d(-1, -1, 0),
         Base::Vector3d(0, -1, 0)},
        100
    );
    segments.insert(segments.end(), below.begin(), below.end());

    // Act
    const auto loops = chainLoops(segments);

    // Assert - two squares, each closed, neither straying into the other
    ASSERT_EQ(loops.size(), 2U);
    for (const auto& loop : loops) {
        EXPECT_TRUE(isClosedExactly(loop));
        // Both squares meet at the origin, so the loop's first point says
        // nothing about which one it is. What matters is that it never holds
        // points from both.
        bool reachesPositive = false;
        bool reachesNegative = false;
        for (const auto& p : loop) {
            reachesPositive = reachesPositive || p.x > 0.5 || p.y > 0.5;
            reachesNegative = reachesNegative || p.x < -0.5 || p.y < -0.5;
        }
        EXPECT_NE(reachesPositive, reachesNegative)
            << "loop straddles both squares, so the walk crossed at the shared point";
    }
}

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

TEST(SectionCapFill, testAnOpenChainIsFilledAsIfItWereClosed)
{
    // Arrange - three sides of a square, the fourth never chained. This is what
    // a body with an open or non manifold mesh slices to.
    const std::vector<Base::Vector3d> open = {
        Base::Vector3d(0, 0, 0),
        Base::Vector3d(10, 0, 0),
        Base::Vector3d(10, 10, 0),
        Base::Vector3d(0, 10, 0),
    };

    // Act
    const auto soup = fillLoops({open}, U, V);

    // Assert - the sweep joins the last point back to the first and fills the
    // whole square, inventing the side that was missing. Callers must therefore
    // hand fillLoops closed loops only; ViewProviderSectionAnalysis filters on
    // isClosed before calling, or the invented edge floods empty space.
    EXPECT_NEAR(soupArea(soup), 100.0, 1e-3);
    EXPECT_FALSE(Part::SectionCap::isClosed(open, 1e-3));
}

TEST(SectionCapFill, testClosureIsSpeltByRepeatingTheFirstPoint)
{
    // The two functions read a loop differently, which is why only a caller that
    // knows where its loops came from can filter: fillLoops closes every loop
    // implicitly by wrapping the last point to the first, while isClosed asks for
    // the repeat to be there. chainLoops always writes it - it pushes the point
    // that met the start before stopping - so its output is safe to test.
    std::vector<Base::Vector3d> repeated = square(0, 0, 10);
    repeated.push_back(repeated.front());

    EXPECT_TRUE(Part::SectionCap::isClosed(repeated, 1e-3));
    EXPECT_FALSE(Part::SectionCap::isClosed(square(0, 0, 10), 1e-3));

    // Both still fill to the same square, the wrap making up the difference
    EXPECT_NEAR(soupArea(fillLoops({repeated}, U, V)), 100.0, 1e-3);
    EXPECT_NEAR(soupArea(fillLoops({square(0, 0, 10)}, U, V)), 100.0, 1e-3);
}


TEST(SectionCapFill, testAContourRestingOnAFlatFaceIsStillFilled)
{
    // The cut leaves the solid through its flat bottom, so a run of the contour
    // lies along that face - every one of those points at the very same sweep
    // level. Taken from a cut that came out hollow.
    // Data from manual tests which failed
    const std::vector<Base::Vector3d> contour = {Base::Vector3d(64.145546, -50.982294, 15.325052),
                                                 Base::Vector3d(63.804646, -51.032620, 15.000000),
                                                 Base::Vector3d(63.769105, -51.032620, 15.000000),
                                                 Base::Vector3d(63.079714, -51.032620, 15.000000),
                                                 Base::Vector3d(62.763278, -51.032620, 15.000000),
                                                 Base::Vector3d(56.968292, -51.032620, 15.000000),
                                                 Base::Vector3d(51.587550, -51.032620, 15.000000),
                                                 Base::Vector3d(51.349516, -51.032620, 15.000000),
                                                 Base::Vector3d(51.206736, -51.032620, 15.000000),
                                                 Base::Vector3d(50.699400, -50.956737, 15.490120),
                                                 Base::Vector3d(50.216004, -50.877793, 16.000000),
                                                 Base::Vector3d(50.525960, -49.757329, 23.236882),
                                                 Base::Vector3d(50.540825, -49.703594, 23.583945),
                                                 Base::Vector3d(51.277573, -48.336461, 32.414015),
                                                 Base::Vector3d(51.337563, -48.225143, 33.132998),
                                                 Base::Vector3d(51.427847, -48.121931, 33.799630),
                                                 Base::Vector3d(52.443314, -46.961052, 41.297542),
                                                 Base::Vector3d(53.610771, -46.113923, 46.768997),
                                                 Base::Vector3d(53.802628, -45.974709, 47.668160),
                                                 Base::Vector3d(55.097657, -45.422110, 51.237296),
                                                 Base::Vector3d(55.347343, -45.315567, 51.925438),
                                                 Base::Vector3d(55.622127, -45.265873, 52.246406),
                                                 Base::Vector3d(57.000000, -45.016685, 53.855862),
                                                 Base::Vector3d(57.290682, -45.029916, 53.770408),
                                                 Base::Vector3d(58.677731, -45.093048, 53.362649),
                                                 Base::Vector3d(60.037017, -45.469070, 50.933992),
                                                 Base::Vector3d(60.296406, -45.540825, 50.470538),
                                                 Base::Vector3d(61.571173, -46.227797, 46.033504),
                                                 Base::Vector3d(61.774857, -46.337563, 45.324550),
                                                 Base::Vector3d(62.905963, -47.326986, 38.934036),
                                                 Base::Vector3d(63.038948, -47.443314, 38.182699),
                                                 Base::Vector3d(63.962104, -48.715547, 29.965569),
                                                 Base::Vector3d(64.025291, -48.802628, 29.403130),
                                                 Base::Vector3d(64.673577, -50.321902, 19.590402),
                                                 Base::Vector3d(64.684433, -50.347343, 19.426082),
                                                 Base::Vector3d(64.780364, -50.877793, 16.000000),
                                                 Base::Vector3d(64.145546, -50.982294, 15.325052)};
    Base::Vector3d normal(0.0, -0.9882, 0.1530);
    normal.Normalize();
    Base::Vector3d u;
    Base::Vector3d v;
    Part::SectionAnalysis::planeFrame(normal, u, v);

    const auto fill = fillLoops({contour}, u, v);

    EXPECT_GT(fill.indices.size(), 0U) << "a closed contour must fill";
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
    const auto loops = chainLoops(sliceTriangles(box(10), Z, 5.0));
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
