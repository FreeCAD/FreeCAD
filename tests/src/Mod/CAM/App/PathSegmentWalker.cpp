// SPDX-License-Identifier: LGPL-2.1-or-later

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <limits>
#include <locale>
#include <numbers>
#include <optional>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <App/Application.h>
#include <Base/Parameter.h>
#include <Mod/CAM/App/PathSegmentWalker.h>
#include <src/App/InitApplication.h>

// NOLINTBEGIN(readability-magic-numbers)
namespace
{
constexpr double fullTurn = 2 * std::numbers::pi;
constexpr double sweepTolerance = 1e-8;

// Right-handed bases: XY/+Z, ZX/+Y, YZ/+X.
struct Plane
{
    const char* command;
    unsigned short u;
    unsigned short v;
    unsigned short normal;

    Base::Vector3d point(double first, double second, double height = 0) const
    {
        Base::Vector3d result;
        result[u] = first;
        result[v] = second;
        result[normal] = height;
        return result;
    }
};

constexpr std::array<Plane, 3> planes {{{"G17", 0, 1, 2}, {"G18", 2, 0, 1}, {"G19", 1, 2, 0}}};

struct Arc
{
    int id;
    Base::Vector3d center;
    std::vector<Base::Vector3d> points;
};

class CollectSegments: public Path::PathSegmentVisitor
{
public:
    void g23(
        int id,
        const Base::Vector3d& last,
        const Base::Vector3d& next,
        const std::deque<Base::Vector3d>& pts,
        const Base::Vector3d& center
    ) override
    {
        Arc arc {id, center, {last}};
        arc.points.insert(arc.points.end(), pts.begin(), pts.end());
        arc.points.push_back(next);
        arcs.push_back(std::move(arc));
    }

    void g0(int, const Base::Vector3d&, const Base::Vector3d&, const std::deque<Base::Vector3d>&) override
    {
        ++rapids;
    }

    void g1(
        int,
        const Base::Vector3d& last,
        const Base::Vector3d& next,
        const std::deque<Base::Vector3d>&
    ) override
    {
        ++lines;
        lineStart = last;
        lineEnd = next;
    }

    std::vector<Arc> arcs;
    int rapids = 0;
    int lines = 0;
    Base::Vector3d lineStart;
    Base::Vector3d lineEnd;
};

void append(Path::Toolpath& path, const std::string& text)
{
    Path::Command command;
    command.setFromGCode(text);
    path.addCommand(command);
}

std::string coordinates(const char* axes, const Base::Vector3d& point)
{
    std::ostringstream text;
    text.imbue(std::locale::classic());
    text << std::setprecision(std::numeric_limits<double>::max_digits10);
    for (unsigned short axis = 0; axis < 3; ++axis) {
        text << ' ' << axes[axis] << point[axis];
    }
    return text.str();
}

struct ArcSpec
{
    Plane plane;
    std::string command;
    Base::Vector3d start;
    Base::Vector3d end;
    Base::Vector3d center;
    bool absoluteCenter = false;
    bool relativeEnd = false;
    bool omitEnd = false;
};

Path::Toolpath makePath(const ArcSpec& spec)
{
    Path::Toolpath path;
    append(path, spec.plane.command);
    append(path, "G90");
    append(path, spec.absoluteCenter ? "G90.1" : "G91.1");
    append(path, "G0" + coordinates("XYZ", spec.start));
    if (spec.relativeEnd) {
        append(path, "G91");
    }
    auto command = spec.command;
    if (!spec.omitEnd) {
        command += coordinates("XYZ", spec.relativeEnd ? spec.end - spec.start : spec.end);
    }
    command += coordinates("IJK", spec.absoluteCenter ? spec.center : spec.center - spec.start);
    append(path, command);
    return path;
}

CollectSegments walk(const Path::Toolpath& path)
{
    CollectSegments visitor;
    Path::PathSegmentWalker(path).walk(visitor, Base::Vector3d(0, 0, 0));
    return visitor;
}

void expectPoint(const Base::Vector3d& actual, const Base::Vector3d& expected, double tolerance)
{
    EXPECT_NEAR(actual.x, expected.x, tolerance);
    EXPECT_NEAR(actual.y, expected.y, tolerance);
    EXPECT_NEAR(actual.z, expected.z, tolerance);
}

// Measure the actual polyline, independently of the walker GetAngle calculation
void expectArc(const Arc& arc, const ArcSpec& spec, double expectedSweep)
{
    const auto& plane = spec.plane;
    const double radius = std::hypot(
        spec.start[plane.u] - spec.center[plane.u],
        spec.start[plane.v] - spec.center[plane.v]
    );
    const double tolerance = 1e-10 * std::max(1.0, radius)
        + 64 * std::numeric_limits<double>::epsilon()
            * std::max({1.0, spec.start.Length(), spec.end.Length(), spec.center.Length()});
    ASSERT_GE(arc.points.size(), 3);
    expectPoint(arc.points.front(), spec.start, tolerance);
    expectPoint(arc.points.back(), spec.end, tolerance);
    expectPoint(arc.center, spec.center, tolerance);

    double sweep = 0;
    double coverage = 0;
    const double height = spec.end[plane.normal] - spec.start[plane.normal];
    for (std::size_t i = 0; i < arc.points.size(); ++i) {
        SCOPED_TRACE(i);
        const auto& point = arc.points[i];
        ASSERT_TRUE(std::isfinite(point.x));
        ASSERT_TRUE(std::isfinite(point.y));
        ASSERT_TRUE(std::isfinite(point.z));
        const double u = point[plane.u] - spec.center[plane.u];
        const double v = point[plane.v] - spec.center[plane.v];
        EXPECT_NEAR(std::hypot(u, v), radius, tolerance);
        EXPECT_NEAR(
            point[plane.normal],
            spec.start[plane.normal] + height * i / (arc.points.size() - 1),
            tolerance
        );
        coverage = std::max(
            coverage,
            std::hypot(point[plane.u] - spec.start[plane.u], point[plane.v] - spec.start[plane.v])
        );
        if (i != 0) {
            const auto& previous = arc.points[i - 1];
            const double previousU = previous[plane.u] - spec.center[plane.u];
            const double previousV = previous[plane.v] - spec.center[plane.v];
            const double step
                = std::atan2(previousU * v - previousV * u, previousU * u + previousV * v);
            EXPECT_LT(std::abs(step), std::numbers::pi);
            EXPECT_GE(step * std::copysign(1.0, expectedSweep), -sweepTolerance);
            // Also inspect intermediate angles: an incorrect full turn can have the same
            // total sweep as an almost-full arc once its exact endpoint is appended
            EXPECT_NEAR(step, expectedSweep / (arc.points.size() - 1), sweepTolerance);
            EXPECT_GE((point[plane.normal] - previous[plane.normal]) * height, -tolerance);
            sweep += step;
        }
    }
    EXPECT_NEAR(sweep, expectedSweep, sweepTolerance);
    if (std::abs(expectedSweep) == fullTurn) {
        EXPECT_GT(coverage, 1.9 * radius);
    }
    else if (std::abs(expectedSweep) < 1e-3) {
        EXPECT_GT(std::abs(sweep), 0);
        EXPECT_LT(coverage, 1e-3 * radius);
    }
}

void expectSingleArc(const ArcSpec& spec, double expectedSweep)
{
    const auto path = makePath(spec);
    const auto visitor = walk(path);
    ASSERT_EQ(visitor.arcs.size(), 1);
    EXPECT_EQ(visitor.arcs.front().id, static_cast<int>(path.getSize() - 1));
    expectArc(visitor.arcs.front(), spec, expectedSweep);
}

class PathSegmentWalkerTest: public ::testing::Test
{
public:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

protected:
    void SetUp() override
    {
        parameters = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Part"
        );
        for (const auto& [name, value] : parameters->GetFloatMap()) {
            if (name == "MeshDeviation") {
                oldDeviation = value;
            }
        }
        parameters->SetFloat("MeshDeviation", 0.2);
    }

    void TearDown() override
    {
        if (oldDeviation) {
            parameters->SetFloat("MeshDeviation", *oldDeviation);
        }
        else {
            parameters->RemoveFloat("MeshDeviation");
        }
    }

    ParameterGrp::handle parameters;
    std::optional<double> oldDeviation;
};

TEST_F(PathSegmentWalkerTest, OriginalIssue32741)
{
    Path::Toolpath path;
    for (const auto* command :
         {"G17",
          "G90",
          "G91.1",
          "G0 X0 Y0 Z0",
          "G2 I-3.690986636971033 J7.094630419146597e-06 K0 X0 Y0 Z0"}) {
        append(path, command);
    }
    const auto visitor = walk(path);
    ASSERT_EQ(visitor.arcs.size(), 1);
    EXPECT_EQ(visitor.arcs.front().id, 4);
    expectArc(
        visitor.arcs.front(),
        {planes[0],
         "G2",
         Base::Vector3d(),
         Base::Vector3d(),
         Base::Vector3d(-3.690986636971033, 7.094630419146597e-06, 0)},
        -fullTurn
    );
}

using PlaneAndCommand = std::tuple<Plane, const char*>;

class PathSegmentWalkerPlaneTest: public PathSegmentWalkerTest,
                                  public ::testing::WithParamInterface<PlaneAndCommand>
{
protected:
    ArcSpec circle() const
    {
        const auto& [plane, command] = GetParam();
        return {
            plane,
            command,
            Base::Vector3d(),
            Base::Vector3d(),
            plane.point(-3.690986636971033, 7.094630419146597e-06)
        };
    }

    double direction() const
    {
        const std::string command = std::get<1>(GetParam());
        return command == "G2" || command == "G02" ? -1 : 1;
    }
};

TEST_P(PathSegmentWalkerPlaneTest, FullCircle)
{
    // Original issue geometry, with aliases, directions and planes
    expectSingleArc(circle(), direction() * fullTurn);
}

TEST_P(PathSegmentWalkerPlaneTest, SimpleCenters)
{
    auto spec = circle();
    for (const auto& center : {spec.plane.point(-1, -1), spec.plane.point(-2, 0)}) {
        spec.center = center;
        expectSingleArc(spec, direction() * fullTurn);
    }
}

TEST_P(PathSegmentWalkerPlaneTest, SmallDistinctEndpoints)
{
    auto spec = circle();
    spec.start = spec.plane.point(1, 0);
    spec.center = Base::Vector3d();
    for (const double delta : {1e-4, 1e-7, std::numeric_limits<double>::epsilon() / 2}) {
        SCOPED_TRACE(delta);
        spec.end = spec.plane.point(std::cos(delta), direction() * std::sin(delta));
        const auto path = makePath(spec);
        const auto& command = path.getCommand(path.getSize() - 1);
        // Prove that text parsing preserved distinct endpoints (Vector3 == uses a tolerance)
        ASSERT_NE(command.getPlacement().getPosition()[spec.plane.v], spec.start[spec.plane.v]);
        if (delta < sweepTolerance) {
            ASSERT_TRUE(spec.start == spec.end);
            // At this scale only guard against a false full turn
            // sweepTolerance deliberately does not promise the angular accuracy of acos near
            // machine precision
        }
        const auto visitor = walk(path);
        ASSERT_EQ(visitor.arcs.size(), 1);
        expectArc(visitor.arcs.front(), spec, direction() * delta);
    }
}

TEST_P(PathSegmentWalkerPlaneTest, QuarterAndMajorArc)
{
    auto spec = circle();
    spec.start = spec.plane.point(1, 0);
    spec.end = spec.plane.point(0, 1);
    spec.center = Base::Vector3d();
    expectSingleArc(spec, direction() > 0 ? std::numbers::pi / 2 : -3 * std::numbers::pi / 2);
}

TEST_P(PathSegmentWalkerPlaneTest, Semicircle)
{
    auto spec = circle();
    spec.start = spec.plane.point(1, 0);
    spec.end = spec.plane.point(-1, 0);
    spec.center = Base::Vector3d();
    expectSingleArc(spec, direction() * std::numbers::pi);
}

TEST_P(PathSegmentWalkerPlaneTest, AlmostFullCircle)
{
    auto spec = circle();
    constexpr double delta = 1e-4;
    spec.start = spec.plane.point(1, 0);
    spec.end = spec.plane.point(std::cos(delta), -direction() * std::sin(delta));
    spec.center = Base::Vector3d();
    expectSingleArc(spec, direction() * (fullTurn - delta));
}

TEST_P(PathSegmentWalkerPlaneTest, Helix)
{
    auto spec = circle();
    for (double height : {2.0, -2.0}) {
        SCOPED_TRACE(height);
        spec.end = spec.plane.point(0, 0, height);
        expectSingleArc(spec, direction() * fullTurn);
    }
}

TEST_P(PathSegmentWalkerPlaneTest, CoordinateModes)
{
    auto spec = circle();
    const auto translation = spec.plane.point(10, -4, 7);
    spec.start += translation;
    spec.end += translation;
    spec.center += translation;
    // Resolved endpoints are equal for each coordinate mode
    for (int mode = 0; mode < 4; ++mode) {
        SCOPED_TRACE(mode);
        spec.absoluteCenter = mode == 1;
        spec.relativeEnd = mode == 2;
        spec.omitEnd = mode == 3;
        expectSingleArc(spec, direction() * fullTurn);
    }
}

TEST_P(PathSegmentWalkerPlaneTest, TranslationAndScale)
{
    for (double scale : {1e-3, 1.0, 1e3}) {
        SCOPED_TRACE(scale);
        auto spec = circle();
        spec.start = Base::Vector3d(100, -40, 7);
        spec.end = spec.start;
        spec.center = spec.start + spec.center * scale;
        expectSingleArc(spec, direction() * fullTurn);
    }
}

TEST_P(PathSegmentWalkerPlaneTest, DeviationAndCommandImmutability)
{
    const auto spec = circle();
    auto path = makePath(spec);
    path.getCommand(path.getSize() - 1).setAnnotation("test", "preserved");
    const auto before = path.getCommands();
    for (double deviation : {0.1, 0.2, 0.5}) {
        SCOPED_TRACE(deviation);
        parameters->SetFloat("MeshDeviation", deviation);
        const auto visitor = walk(path);
        ASSERT_EQ(visitor.arcs.size(), 1);
        expectArc(visitor.arcs.front(), spec, direction() * fullTurn);
        ASSERT_EQ(path.getSize(), before.size());
        for (std::size_t i = 0; i < before.size(); ++i) {
            EXPECT_EQ(path.getCommand(i).Name, before[i].Name);
            EXPECT_EQ(path.getCommand(i).Parameters, before[i].Parameters);
            EXPECT_EQ(path.getCommand(i).Annotations, before[i].Annotations);
        }
    }
}

INSTANTIATE_TEST_SUITE_P(
    PlanesAndDirections,
    PathSegmentWalkerPlaneTest,
    ::testing::Combine(::testing::ValuesIn(planes), ::testing::Values("G2", "G02", "G3", "G03")),
    [](const ::testing::TestParamInfo<PlaneAndCommand>& info) {
        return std::string(std::get<0>(info.param).command) + "_" + std::get<1>(info.param);
    }
);

TEST_F(PathSegmentWalkerTest, ConsecutiveCommandsAndRepeatedWalk)
{
    const ArcSpec
        circle {planes[0], "G3", Base::Vector3d(1, 0, 0), Base::Vector3d(1, 0, 0), Base::Vector3d()};
    auto path = makePath(circle);
    append(path, "G3 X0 Y1 I-1 J0");
    append(path, "G1 X2 Y3 Z4");
    Path::PathSegmentWalker walker(path);
    for (int repeat = 0; repeat < 2; ++repeat) {
        CollectSegments visitor;
        walker.walk(visitor, Base::Vector3d());
        ASSERT_EQ(visitor.arcs.size(), 2);
        EXPECT_EQ(visitor.rapids, 1);
        EXPECT_EQ(visitor.lines, 1);
        EXPECT_EQ(visitor.arcs[0].id, 4);
        EXPECT_EQ(visitor.arcs[1].id, 5);
        expectArc(visitor.arcs[0], circle, fullTurn);
        expectArc(
            visitor.arcs[1],
            {planes[0], "G3", Base::Vector3d(1, 0, 0), Base::Vector3d(0, 1, 0), Base::Vector3d()},
            std::numbers::pi / 2
        );
        expectPoint(visitor.lineStart, Base::Vector3d(0, 1, 0), 0);
        expectPoint(visitor.lineEnd, Base::Vector3d(2, 3, 4), 0);
    }
}

TEST_F(PathSegmentWalkerTest, EmptyPath)
{
    const auto visitor = walk(Path::Toolpath());
    EXPECT_TRUE(visitor.arcs.empty());
    EXPECT_EQ(visitor.rapids, 0);
    EXPECT_EQ(visitor.lines, 0);
}

TEST_F(PathSegmentWalkerTest, InvalidRadiusRemainsNonFinite)
{
    // The existing walker emits non-finite intermediate points for these inputs.
    // Full-circle recognition must not turn its invalid angle into a finite sweep.
    for (const double offset :
         {0.0,
          std::numeric_limits<double>::epsilon() / 2,
          std::numeric_limits<double>::epsilon(),
          std::numeric_limits<double>::quiet_NaN()}) {
        Path::Toolpath path;
        append(path, "G17");
        append(path, "G90");
        append(path, "G91.1");
        Path::Command command;
        command.setFromGCode("G2 X0 Y0 Z0 I0 J0 K0");
        command.Parameters["I"] = offset;
        path.addCommand(command);
        const auto visitor = walk(path);
        ASSERT_EQ(visitor.arcs.size(), 1);
        const auto& points = visitor.arcs.front().points;
        ASSERT_GT(points.size(), 2);
        EXPECT_TRUE(std::any_of(points.begin() + 1, points.end() - 1, [](const auto& point) {
            return !std::isfinite(point.x) || !std::isfinite(point.y) || !std::isfinite(point.z);
        }));
    }
}
}  // namespace
// NOLINTEND(readability-magic-numbers)
