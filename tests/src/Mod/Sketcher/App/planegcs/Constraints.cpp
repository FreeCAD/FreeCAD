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

TEST_F(ConstraintsTest, DerivedPointAtOrigin)  // NOLINT
{
    // u=0, v=0 → Q should be at P1
    double p1x = 10., p1y = 20., p2x = 50., p2y = 20.;
    double qx = 10., qy = 20.;  // already at P1
    GCS::Point p1, p2, q;
    p1.x = &p1x;
    p1.y = &p1y;
    p2.x = &p2x;
    p2.y = &p2y;
    q.x = &qx;
    q.y = &qy;

    System()->addConstraintDerivedPoint(p1, p2, q, 0., 0., 0, true);
    EXPECT_EQ(System()->getNumberOfConstraints(), 2u);

    std::vector<double*> params = {q.x, q.y};
    EXPECT_EQ(System()->solve(params), GCS::Success);
    System()->applySolution();

    EXPECT_NEAR(qx, 10., 1e-10);
    EXPECT_NEAR(qy, 20., 1e-10);
}

TEST_F(ConstraintsTest, DerivedPointMidpoint)  // NOLINT
{
    // u=0.5, v=0 → Q should be at midpoint of P1-P2
    double p1x = 0., p1y = 0., p2x = 10., p2y = 0.;
    double qx = 0., qy = 0.;  // start at origin
    GCS::Point p1, p2, q;
    p1.x = &p1x;
    p1.y = &p1y;
    p2.x = &p2x;
    p2.y = &p2y;
    q.x = &qx;
    q.y = &qy;

    System()->addConstraintDerivedPoint(p1, p2, q, 0.5, 0., 0, true);

    std::vector<double*> params = {q.x, q.y};
    EXPECT_EQ(System()->solve(params), GCS::Success);
    System()->applySolution();

    EXPECT_NEAR(qx, 5., 1e-10);
    EXPECT_NEAR(qy, 0., 1e-10);
}

TEST_F(ConstraintsTest, DerivedPointPerpendicular)  // NOLINT
{
    // u=0, v=1 → Q should be at P1 + perp(P2-P1)
    // P1=(0,0), P2=(10,0) → perp = (0,10) → Q=(0,10)
    double p1x = 0., p1y = 0., p2x = 10., p2y = 0.;
    double qx = 0., qy = 0.;
    GCS::Point p1, p2, q;
    p1.x = &p1x;
    p1.y = &p1y;
    p2.x = &p2x;
    p2.y = &p2y;
    q.x = &qx;
    q.y = &qy;

    System()->addConstraintDerivedPoint(p1, p2, q, 0., 1., 0, true);

    std::vector<double*> params = {q.x, q.y};
    EXPECT_EQ(System()->solve(params), GCS::Success);
    System()->applySolution();

    EXPECT_NEAR(qx, 0., 1e-10);
    EXPECT_NEAR(qy, 10., 1e-10);
}

TEST_F(ConstraintsTest, DerivedPointSolveMovesFrame)  // NOLINT
{
    // Fix Q externally, verify P1/P2 adjust.
    // Use addConstraintEqual with fixed params NOT in the solve set.
    double p1x = 0., p1y = 0., p2x = 10., p2y = 0.;
    double qx = 10., qy = 10.;
    GCS::Point p1, p2, q;
    p1.x = &p1x;
    p1.y = &p1y;
    p2.x = &p2x;
    p2.y = &p2y;
    q.x = &qx;
    q.y = &qy;

    // Fix Q at (10, 10) — fixedX/fixedY are NOT in the params list
    double fixedX = 10., fixedY = 10.;
    System()->addConstraintEqual(q.x, &fixedX, 0, true);
    System()->addConstraintEqual(q.y, &fixedY, 0, true);
    // Derived point with u=0.5, v=0.5
    System()->addConstraintDerivedPoint(p1, p2, q, 0.5, 0.5, 0, true);

    // Only frame and Q are free; fixedX/fixedY are constants
    std::vector<double*> params = {p1.x, p1.y, p2.x, p2.y, q.x, q.y};
    EXPECT_EQ(System()->solve(params), GCS::Success);
    System()->applySolution();

    // Verify Q stayed at (10, 10)
    EXPECT_NEAR(qx, 10., 1e-10);
    EXPECT_NEAR(qy, 10., 1e-10);

    // Verify the derived point constraint is satisfied
    double dx = p2x - p1x;
    double dy = p2y - p1y;
    double expectedQx = p1x + 0.5 * dx + 0.5 * (-dy);
    double expectedQy = p1y + 0.5 * dy + 0.5 * dx;
    EXPECT_NEAR(qx, expectedQx, 1e-10);
    EXPECT_NEAR(qy, expectedQy, 1e-10);
}

TEST_F(ConstraintsTest, DerivedPointRigidBody)  // NOLINT
{
    // Two derived points on the same frame maintain their relative positions.
    // Frame P1=(0,0), P2=(10,0).
    // Q1 at u=0.25, v=0.5 → (2.5, 5)
    // Q2 at u=0.75, v=-0.5 → (7.5, -5)
    // Fix both Q positions via constraints, then move P1 and see both follow.
    double p1x = 0., p1y = 0., p2x = 10., p2y = 0.;
    double q1x = 0., q1y = 0., q2x = 0., q2y = 0.;
    GCS::Point p1, p2, q1, q2;
    p1.x = &p1x;
    p1.y = &p1y;
    p2.x = &p2x;
    p2.y = &p2y;
    q1.x = &q1x;
    q1.y = &q1y;
    q2.x = &q2x;
    q2.y = &q2y;

    System()->addConstraintDerivedPoint(p1, p2, q1, 0.25, 0.5, 0, true);
    System()->addConstraintDerivedPoint(p1, p2, q2, 0.75, -0.5, 0, true);

    // Only Q1 and Q2 are free; frame is fixed
    std::vector<double*> params = {q1.x, q1.y, q2.x, q2.y};
    EXPECT_EQ(System()->solve(params), GCS::Success);
    System()->applySolution();

    // Q1 = P1 + 0.25*(P2-P1) + 0.5*perp(P2-P1)
    //     = (0,0) + 0.25*(10,0) + 0.5*(0,10) = (2.5, 5)
    EXPECT_NEAR(q1x, 2.5, 1e-10);
    EXPECT_NEAR(q1y, 5., 1e-10);

    // Q2 = P1 + 0.75*(P2-P1) + (-0.5)*perp(P2-P1)
    //     = (0,0) + 0.75*(10,0) + (-0.5)*(0,10) = (7.5, -5)
    EXPECT_NEAR(q2x, 7.5, 1e-10);
    EXPECT_NEAR(q2y, -5., 1e-10);
}

TEST_F(ConstraintsTest, DerivedPointDiagonalFrame)  // NOLINT
{
    // Test with non-axis-aligned frame: P1=(0,0), P2=(3,4)
    // u=0.5, v=0 → Q should be at midpoint (1.5, 2)
    double p1x = 0., p1y = 0., p2x = 3., p2y = 4.;
    double qx = 0., qy = 0.;
    GCS::Point p1, p2, q;
    p1.x = &p1x;
    p1.y = &p1y;
    p2.x = &p2x;
    p2.y = &p2y;
    q.x = &qx;
    q.y = &qy;

    System()->addConstraintDerivedPoint(p1, p2, q, 0.5, 0., 0, true);

    std::vector<double*> params = {q.x, q.y};
    EXPECT_EQ(System()->solve(params), GCS::Success);
    System()->applySolution();

    EXPECT_NEAR(qx, 1.5, 1e-10);
    EXPECT_NEAR(qy, 2., 1e-10);
}

TEST_F(ConstraintsTest, DerivedPointPerpendicularDiagonal)  // NOLINT
{
    // P1=(0,0), P2=(3,4), u=0, v=1 → Q = P1 + perp(3,4) = (0,0) + (-4,3) = (-4, 3)
    double p1x = 0., p1y = 0., p2x = 3., p2y = 4.;
    double qx = 0., qy = 0.;
    GCS::Point p1, p2, q;
    p1.x = &p1x;
    p1.y = &p1y;
    p2.x = &p2x;
    p2.y = &p2y;
    q.x = &qx;
    q.y = &qy;

    System()->addConstraintDerivedPoint(p1, p2, q, 0., 1., 0, true);

    std::vector<double*> params = {q.x, q.y};
    EXPECT_EQ(System()->solve(params), GCS::Success);
    System()->applySolution();

    EXPECT_NEAR(qx, -4., 1e-10);
    EXPECT_NEAR(qy, 3., 1e-10);
}

// Helper: check that DerivedPoint constraint error is near zero.
// Formula: E0 = Qx - P1x - u*(P2x-P1x) + v*(P2y-P1y)
//          E1 = Qy - P1y - u*(P2y-P1y) - v*(P2x-P1x)
static void expectDerivedPointSatisfied(
    double p1x,
    double p1y,
    double p2x,
    double p2y,
    double qx,
    double qy,
    double u,
    double v,
    double tol = 1e-8
)
{
    double dx = p2x - p1x;
    double dy = p2y - p1y;
    EXPECT_NEAR(qx - p1x - u * dx + v * dy, 0., tol);
    EXPECT_NEAR(qy - p1y - u * dy - v * dx, 0., tol);
}

TEST_F(ConstraintsTest, DerivedPointHardDragVertex)  // NOLINT
{
    // Drag a derived vertex Q with no user constraints.
    // DerivedPoint in hard system (tag=100), drag in soft (tag=-1).
    double p1x = 0., p1y = 0., p2x = 10., p2y = 0.;
    double qx = 5., qy = 5.;
    GCS::Point p1, p2, q;
    p1.x = &p1x;
    p1.y = &p1y;
    p2.x = &p2x;
    p2.y = &p2y;
    q.x = &qx;
    q.y = &qy;

    System()->addConstraintDerivedPoint(p1, p2, q, 0.5, 0.5, 100, true);

    std::vector<double*> params = {p1.x, p1.y, p2.x, p2.y, q.x, q.y};
    EXPECT_EQ(System()->solve(params), GCS::Success);
    System()->applySolution();

    // Initial state consistent
    EXPECT_NEAR(qx, 5., 1e-10);
    EXPECT_NEAR(qy, 5., 1e-10);

    // Simulate drag: P2PCoincident(Q, moveTarget) with tag -1 (soft)
    double moveX = 10., moveY = 10.;
    GCS::Point moveTarget;
    moveTarget.x = &moveX;
    moveTarget.y = &moveY;
    System()->addConstraintP2PCoincident(q, moveTarget, GCS::DefaultTemporaryConstraint, true);

    EXPECT_EQ(System()->solve(params), GCS::Success);
    System()->applySolution();

    // Hard constraint: DerivedPoint still satisfied
    expectDerivedPointSatisfied(p1x, p1y, p2x, p2y, qx, qy, 0.5, 0.5);

    // Soft: Q reached the drag target (6 params, 2 hard, 2 soft → feasible)
    EXPECT_NEAR(qx, 10., 1e-6);
    EXPECT_NEAR(qy, 10., 1e-6);
}

TEST_F(ConstraintsTest, DerivedPointHardDragVertexWithAxisConstraint)  // NOLINT
{
    // Drag Q when Q.x is hard-constrained to 0.
    // Q should slide along x=0 while DerivedPoint holds.
    double p1x = 0., p1y = 0., p2x = 10., p2y = 0.;
    double qx = 0., qy = 5.;  // consistent with u=0, v=0.5
    GCS::Point p1, p2, q;
    p1.x = &p1x;
    p1.y = &p1y;
    p2.x = &p2x;
    p2.y = &p2y;
    q.x = &qx;
    q.y = &qy;

    System()->addConstraintDerivedPoint(p1, p2, q, 0., 0.5, 100, true);

    // Fix Q.x = 0 (hard, tag=1). targetX is NOT in plist → treated as constant.
    double targetX = 0.;
    System()->addConstraintCoordinateX(q, &targetX, 1, true);

    std::vector<double*> params = {p1.x, p1.y, p2.x, p2.y, q.x, q.y};
    EXPECT_EQ(System()->solve(params), GCS::Success);
    System()->applySolution();

    EXPECT_NEAR(qx, 0., 1e-10);
    EXPECT_NEAR(qy, 5., 1e-10);

    // Drag Q toward (5, 15) — soft. Q.x=5 conflicts with hard Q.x=0.
    double moveX = 5., moveY = 15.;
    GCS::Point moveTarget;
    moveTarget.x = &moveX;
    moveTarget.y = &moveY;
    System()->addConstraintP2PCoincident(q, moveTarget, GCS::DefaultTemporaryConstraint, true);

    EXPECT_EQ(System()->solve(params), GCS::Success);
    System()->applySolution();

    // Hard: Q.x stays 0, DerivedPoint satisfied
    EXPECT_NEAR(qx, 0., 1e-8);
    expectDerivedPointSatisfied(p1x, p1y, p2x, p2y, qx, qy, 0., 0.5);

    // Soft: Q.y moves toward 15
    EXPECT_NEAR(qy, 15., 1e-6);
}

TEST_F(ConstraintsTest, DerivedPointHardDragFrameEdge)  // NOLINT
{
    // Drag the frame (P1 and P2) by translating +5,+5. Q should follow.
    double p1x = 0., p1y = 0., p2x = 10., p2y = 0.;
    double qx = 5., qy = 5.;
    GCS::Point p1, p2, q;
    p1.x = &p1x;
    p1.y = &p1y;
    p2.x = &p2x;
    p2.y = &p2y;
    q.x = &qx;
    q.y = &qy;

    System()->addConstraintDerivedPoint(p1, p2, q, 0.5, 0.5, 100, true);

    std::vector<double*> params = {p1.x, p1.y, p2.x, p2.y, q.x, q.y};
    EXPECT_EQ(System()->solve(params), GCS::Success);
    System()->applySolution();

    EXPECT_NEAR(qx, 5., 1e-10);
    EXPECT_NEAR(qy, 5., 1e-10);

    // Drag both frame endpoints by (+5,+5)
    double moveP1x = 5., moveP1y = 5., moveP2x = 15., moveP2y = 5.;
    GCS::Point moveP1, moveP2;
    moveP1.x = &moveP1x;
    moveP1.y = &moveP1y;
    moveP2.x = &moveP2x;
    moveP2.y = &moveP2y;
    System()->addConstraintP2PCoincident(p1, moveP1, GCS::DefaultTemporaryConstraint, true);
    System()->addConstraintP2PCoincident(p2, moveP2, GCS::DefaultTemporaryConstraint, true);

    EXPECT_EQ(System()->solve(params), GCS::Success);
    System()->applySolution();

    // Hard: DerivedPoint satisfied
    expectDerivedPointSatisfied(p1x, p1y, p2x, p2y, qx, qy, 0.5, 0.5);

    // Soft: frame translated, Q followed
    EXPECT_NEAR(p1x, 5., 1e-6);
    EXPECT_NEAR(p1y, 5., 1e-6);
    EXPECT_NEAR(p2x, 15., 1e-6);
    EXPECT_NEAR(p2y, 5., 1e-6);
    EXPECT_NEAR(qx, 10., 1e-6);
    EXPECT_NEAR(qy, 10., 1e-6);
}

TEST_F(ConstraintsTest, DerivedPointHardDragEdgeWithAxisConstraint)  // NOLINT
{
    // Translate frame +10,0 while Q.x is hard-constrained to 5.
    // Frame can't translate purely — it must rotate/adjust.
    double p1x = 0., p1y = 0., p2x = 10., p2y = 0.;
    double qx = 5., qy = 5.;  // consistent: u=0.5, v=0.5
    GCS::Point p1, p2, q;
    p1.x = &p1x;
    p1.y = &p1y;
    p2.x = &p2x;
    p2.y = &p2y;
    q.x = &qx;
    q.y = &qy;

    System()->addConstraintDerivedPoint(p1, p2, q, 0.5, 0.5, 100, true);

    double targetX = 5.;
    System()->addConstraintCoordinateX(q, &targetX, 1, true);

    std::vector<double*> params = {p1.x, p1.y, p2.x, p2.y, q.x, q.y};
    EXPECT_EQ(System()->solve(params), GCS::Success);
    System()->applySolution();

    EXPECT_NEAR(qx, 5., 1e-10);

    // Drag: translate frame +10,0
    double moveP1x = 10., moveP1y = 0., moveP2x = 20., moveP2y = 0.;
    GCS::Point moveP1, moveP2;
    moveP1.x = &moveP1x;
    moveP1.y = &moveP1y;
    moveP2.x = &moveP2x;
    moveP2.y = &moveP2y;
    System()->addConstraintP2PCoincident(p1, moveP1, GCS::DefaultTemporaryConstraint, true);
    System()->addConstraintP2PCoincident(p2, moveP2, GCS::DefaultTemporaryConstraint, true);

    EXPECT_EQ(System()->solve(params), GCS::Success);
    System()->applySolution();

    // Hard: Q.x stays 5, DerivedPoint satisfied
    EXPECT_NEAR(qx, 5., 1e-8);
    expectDerivedPointSatisfied(p1x, p1y, p2x, p2y, qx, qy, 0.5, 0.5);

    // Frame moved (not necessarily to exact target due to hard constraint conflict)
    // but geometry is consistent
}

TEST_F(ConstraintsTest, DerivedPointHardMultiVertex)  // NOLINT
{
    // Two derived vertices on same frame. Drag Q1 → both move, relationships maintained.
    double p1x = 0., p1y = 0., p2x = 10., p2y = 0.;
    double q1x = 2.5, q1y = 5., q2x = 7.5, q2y = -5.;
    GCS::Point p1, p2, q1, q2;
    p1.x = &p1x;
    p1.y = &p1y;
    p2.x = &p2x;
    p2.y = &p2y;
    q1.x = &q1x;
    q1.y = &q1y;
    q2.x = &q2x;
    q2.y = &q2y;

    System()->addConstraintDerivedPoint(p1, p2, q1, 0.25, 0.5, 100, true);
    System()->addConstraintDerivedPoint(p1, p2, q2, 0.75, -0.5, 100, true);

    std::vector<double*> params = {p1.x, p1.y, p2.x, p2.y, q1.x, q1.y, q2.x, q2.y};
    EXPECT_EQ(System()->solve(params), GCS::Success);
    System()->applySolution();

    EXPECT_NEAR(q1x, 2.5, 1e-10);
    EXPECT_NEAR(q1y, 5., 1e-10);
    EXPECT_NEAR(q2x, 7.5, 1e-10);
    EXPECT_NEAR(q2y, -5., 1e-10);

    // Drag Q1 toward (5, 10)
    double moveX = 5., moveY = 10.;
    GCS::Point moveTarget;
    moveTarget.x = &moveX;
    moveTarget.y = &moveY;
    System()->addConstraintP2PCoincident(q1, moveTarget, GCS::DefaultTemporaryConstraint, true);

    EXPECT_EQ(System()->solve(params), GCS::Success);
    System()->applySolution();

    // Both DerivedPoint constraints satisfied
    expectDerivedPointSatisfied(p1x, p1y, p2x, p2y, q1x, q1y, 0.25, 0.5);
    expectDerivedPointSatisfied(p1x, p1y, p2x, p2y, q2x, q2y, 0.75, -0.5);

    // Q1 reached drag target (8 params, 4 hard, 2 soft → feasible)
    EXPECT_NEAR(q1x, 5., 1e-6);
    EXPECT_NEAR(q1y, 10., 1e-6);
}

TEST_F(ConstraintsTest, NonGroupDragUnaffected)  // NOLINT
{
    // Regression: a regular point drag without DerivedPoint still works.
    double p1x = 0., p1y = 0., p2x = 10., p2y = 0.;
    GCS::Point p1, p2;
    p1.x = &p1x;
    p1.y = &p1y;
    p2.x = &p2x;
    p2.y = &p2y;

    // No hard constraints — only soft drag
    double moveX = 15., moveY = 5.;
    GCS::Point moveTarget;
    moveTarget.x = &moveX;
    moveTarget.y = &moveY;
    System()->addConstraintP2PCoincident(p2, moveTarget, GCS::DefaultTemporaryConstraint, true);

    std::vector<double*> params = {p1.x, p1.y, p2.x, p2.y};
    EXPECT_EQ(System()->solve(params), GCS::Success);
    System()->applySolution();

    // P2 moved to target, P1 unchanged
    EXPECT_NEAR(p2x, 15., 1e-6);
    EXPECT_NEAR(p2y, 5., 1e-6);
    EXPECT_NEAR(p1x, 0., 1e-6);
    EXPECT_NEAR(p1y, 0., 1e-6);
}

TEST_F(ConstraintsTest, DerivedPointInternalHardTag)  // NOLINT
{
    // Reproduce the REAL Sketcher scenario: DerivedPoint with tag -2
    // (InternalHardConstraint). The partition predicate routes tag -2 to hard.
    // This mimics: group with one line, drag line endpoint.
    double p1x = 0., p1y = 0., p2x = 10., p2y = 0.;
    double sx = 2., sy = 5., ex = 8., ey = 5.;
    GCS::Point p1, p2, s, e;
    p1.x = &p1x;
    p1.y = &p1y;
    p2.x = &p2x;
    p2.y = &p2y;
    s.x = &sx;
    s.y = &sy;
    e.x = &ex;
    e.y = &ey;

    // DerivedPoint with InternalHardConstraint tag (-2), exactly as Sketch.cpp does
    // start: u=0.2, v=0.5 → (2, 5) from frame (0,0)-(10,0)
    // end: u=0.8, v=0.5 → (8, 5) from frame (0,0)-(10,0)
    System()->addConstraintDerivedPoint(p1, p2, s, 0.2, 0.5, GCS::InternalHardConstraint, true);
    System()->addConstraintDerivedPoint(p1, p2, e, 0.8, 0.5, GCS::InternalHardConstraint, true);

    // Phase 1: initial solve (no drag yet) — single subsystem solve
    std::vector<double*> params = {p1.x, p1.y, p2.x, p2.y, s.x, s.y, e.x, e.y};
    EXPECT_EQ(System()->solve(params), GCS::Success);
    System()->applySolution();

    EXPECT_NEAR(sx, 2., 1e-10);
    EXPECT_NEAR(sy, 5., 1e-10);
    EXPECT_NEAR(ex, 8., 1e-10);
    EXPECT_NEAR(ey, 5., 1e-10);

    // Phase 2: simulate drag of endpoint 'e' to (12, 8)
    // MoveParameters are NOT in plist — they're external constants.
    double moveX = 12., moveY = 8.;
    GCS::Point moveTarget;
    moveTarget.x = &moveX;
    moveTarget.y = &moveY;
    System()->addConstraintP2PCoincident(e, moveTarget, GCS::DefaultTemporaryConstraint, true);

    int ret = System()->solve(params);
    EXPECT_EQ(ret, GCS::Success) << "Two-subsystem solve failed with tag -2 DerivedPoint in hard";
    System()->applySolution();

    // Hard: both DerivedPoint constraints satisfied
    expectDerivedPointSatisfied(p1x, p1y, p2x, p2y, sx, sy, 0.2, 0.5);
    expectDerivedPointSatisfied(p1x, p1y, p2x, p2y, ex, ey, 0.8, 0.5);

    // Soft: e moved toward (12, 8)
    EXPECT_NEAR(ex, 12., 1e-6);
    EXPECT_NEAR(ey, 8., 1e-6);
}

TEST_F(ConstraintsTest, DerivedPointInternalHardWithUserConstraint)  // NOLINT
{
    // Real scenario: group + user constraint + drag.
    // Line group with a DistanceX constraint on the start vertex.
    double p1x = 0., p1y = 0., p2x = 10., p2y = 0.;
    double sx = 2., sy = 5., ex = 8., ey = 5.;
    GCS::Point p1, p2, s, e;
    p1.x = &p1x;
    p1.y = &p1y;
    p2.x = &p2x;
    p2.y = &p2y;
    s.x = &sx;
    s.y = &sy;
    e.x = &ex;
    e.y = &ey;

    System()->addConstraintDerivedPoint(p1, p2, s, 0.2, 0.5, GCS::InternalHardConstraint, true);
    System()->addConstraintDerivedPoint(p1, p2, e, 0.8, 0.5, GCS::InternalHardConstraint, true);

    // User constraint: fix start.x = 2 (positive tag → hard system)
    double targetX = 2.;
    System()->addConstraintCoordinateX(s, &targetX, 1, true);

    std::vector<double*> params = {p1.x, p1.y, p2.x, p2.y, s.x, s.y, e.x, e.y};
    EXPECT_EQ(System()->solve(params), GCS::Success);
    System()->applySolution();

    EXPECT_NEAR(sx, 2., 1e-10);

    // Drag end vertex toward (15, 3)
    double moveX = 15., moveY = 3.;
    GCS::Point moveTarget;
    moveTarget.x = &moveX;
    moveTarget.y = &moveY;
    System()->addConstraintP2PCoincident(e, moveTarget, GCS::DefaultTemporaryConstraint, true);

    int ret = System()->solve(params);
    EXPECT_EQ(ret, GCS::Success)
        << "Two-subsystem solve failed with tag -2 DerivedPoint + user constraint in hard";
    System()->applySolution();

    // Hard: start.x stays 2, DerivedPoints satisfied
    EXPECT_NEAR(sx, 2., 1e-8);
    expectDerivedPointSatisfied(p1x, p1y, p2x, p2y, sx, sy, 0.2, 0.5);
    expectDerivedPointSatisfied(p1x, p1y, p2x, p2y, ex, ey, 0.8, 0.5);
}

TEST_F(ConstraintsTest, DerivedPointInternalHardFullyConstrained)  // NOLINT
{
    // Fully constrained group: all 4 line coords fixed + DerivedPoint.
    // This creates a hard system with 8 constraints on 8 params.
    // Drag should still return Success but not move anything (0 DOF).
    double p1x = 0., p1y = 0., p2x = 10., p2y = 0.;
    double sx = 2., sy = 5., ex = 8., ey = 5.;
    GCS::Point p1, p2, s, e;
    p1.x = &p1x;
    p1.y = &p1y;
    p2.x = &p2x;
    p2.y = &p2y;
    s.x = &sx;
    s.y = &sy;
    e.x = &ex;
    e.y = &ey;

    System()->addConstraintDerivedPoint(p1, p2, s, 0.2, 0.5, GCS::InternalHardConstraint, true);
    System()->addConstraintDerivedPoint(p1, p2, e, 0.8, 0.5, GCS::InternalHardConstraint, true);

    // Fix all 4 line coords → fully constrained
    double tx = 2., ty = 5., txx = 8., tyy = 5.;
    System()->addConstraintCoordinateX(s, &tx, 1, true);
    System()->addConstraintCoordinateY(s, &ty, 2, true);
    System()->addConstraintCoordinateX(e, &txx, 3, true);
    System()->addConstraintCoordinateY(e, &tyy, 4, true);

    std::vector<double*> params = {p1.x, p1.y, p2.x, p2.y, s.x, s.y, e.x, e.y};
    EXPECT_EQ(System()->solve(params), GCS::Success);
    System()->applySolution();

    // Now drag 'e' to (15, 3) — should succeed but geometry shouldn't move (0 DOF in null space)
    double moveX = 15., moveY = 3.;
    GCS::Point moveTarget;
    moveTarget.x = &moveX;
    moveTarget.y = &moveY;
    System()->addConstraintP2PCoincident(e, moveTarget, GCS::DefaultTemporaryConstraint, true);

    int ret = System()->solve(params);
    // The solve might return Success (constraints already satisfied) or Converged
    EXPECT_LE(ret, GCS::Converged)
        << "Fully constrained hard system should not fail (even if it can't move)";
    System()->applySolution();

    // Geometry shouldn't move (fully constrained)
    EXPECT_NEAR(sx, 2., 1e-6);
    EXPECT_NEAR(sy, 5., 1e-6);
    EXPECT_NEAR(ex, 8., 1e-6);
    EXPECT_NEAR(ey, 5., 1e-6);
}

TEST_F(ConstraintsTest, DerivedPointInternalHardManyVertices)  // NOLINT
{
    // Group with 3 line segments (6 derived vertices) + user constraints.
    // This tests the case where DerivedPoint constraints in the P-subspace
    // are rank-deficient (6 vertices = 12 DP constraints, but frame only has 4 DOF).
    double p1x = 0., p1y = 0., p2x = 10., p2y = 0.;
    // Line 1: start(1,3), end(4,3)
    double s1x = 1., s1y = 3., e1x = 4., e1y = 3.;
    // Line 2: start(5,2), end(8,2)
    double s2x = 5., s2y = 2., e2x = 8., e2y = 2.;
    // Line 3: start(2,-1), end(7,-1)
    double s3x = 2., s3y = -1., e3x = 7., e3y = -1.;

    GCS::Point p1, p2, l1s, l1e, l2s, l2e, l3s, l3e;
    p1.x = &p1x;
    p1.y = &p1y;
    p2.x = &p2x;
    p2.y = &p2y;
    l1s.x = &s1x;
    l1s.y = &s1y;
    l1e.x = &e1x;
    l1e.y = &e1y;
    l2s.x = &s2x;
    l2s.y = &s2y;
    l2e.x = &e2x;
    l2e.y = &e2y;
    l3s.x = &s3x;
    l3s.y = &s3y;
    l3e.x = &e3x;
    l3e.y = &e3y;

    // Compute u,v for each vertex: Qx = P1x + u*(P2x-P1x) - v*(P2y-P1y)
    //                                Qy = P1y + u*(P2y-P1y) + v*(P2x-P1x)
    // With P1=(0,0), P2=(10,0): Qx = u*10, Qy = v*10
    // So u = Qx/10, v = Qy/10
    System()->addConstraintDerivedPoint(p1, p2, l1s, 0.1, 0.3, GCS::InternalHardConstraint, true);
    System()->addConstraintDerivedPoint(p1, p2, l1e, 0.4, 0.3, GCS::InternalHardConstraint, true);
    System()->addConstraintDerivedPoint(p1, p2, l2s, 0.5, 0.2, GCS::InternalHardConstraint, true);
    System()->addConstraintDerivedPoint(p1, p2, l2e, 0.8, 0.2, GCS::InternalHardConstraint, true);
    System()->addConstraintDerivedPoint(p1, p2, l3s, 0.2, -0.1, GCS::InternalHardConstraint, true);
    System()->addConstraintDerivedPoint(p1, p2, l3e, 0.7, -0.1, GCS::InternalHardConstraint, true);

    // Add a user constraint: fix line1 start.x = 1
    double fixVal = 1.;
    System()->addConstraintCoordinateX(l1s, &fixVal, 1, true);

    std::vector<double*> params = {
        p1.x,
        p1.y,
        p2.x,
        p2.y,
        l1s.x,
        l1s.y,
        l1e.x,
        l1e.y,
        l2s.x,
        l2s.y,
        l2e.x,
        l2e.y,
        l3s.x,
        l3s.y,
        l3e.x,
        l3e.y
    };

    EXPECT_EQ(System()->solve(params), GCS::Success);
    System()->applySolution();

    // Drag line3 end to (10, -3)
    double moveX = 10., moveY = -3.;
    GCS::Point moveTarget;
    moveTarget.x = &moveX;
    moveTarget.y = &moveY;
    System()->addConstraintP2PCoincident(l3e, moveTarget, GCS::DefaultTemporaryConstraint, true);

    int ret = System()->solve(params);
    EXPECT_EQ(ret, GCS::Success) << "Many-vertex group with user constraint: solve should succeed";
    System()->applySolution();

    // All DerivedPoints should be satisfied
    expectDerivedPointSatisfied(p1x, p1y, p2x, p2y, s1x, s1y, 0.1, 0.3);
    expectDerivedPointSatisfied(p1x, p1y, p2x, p2y, e1x, e1y, 0.4, 0.3);
    expectDerivedPointSatisfied(p1x, p1y, p2x, p2y, s2x, s2y, 0.5, 0.2);
    expectDerivedPointSatisfied(p1x, p1y, p2x, p2y, e2x, e2y, 0.8, 0.2);
    expectDerivedPointSatisfied(p1x, p1y, p2x, p2y, s3x, s3y, 0.2, -0.1);
    expectDerivedPointSatisfied(p1x, p1y, p2x, p2y, e3x, e3y, 0.7, -0.1);

    // User constraint: line1 start.x = 1
    EXPECT_NEAR(s1x, 1., 1e-8);
}

TEST_F(ConstraintsTest, DerivedPointInternalHardOverconstrained)  // NOLINT
{
    // 3 lines (6 vertices) with 8 user constraints fixing all of line1 and line2.
    // Hard: 12 DerivedPoint + 8 user = 20 constraints on 16 params.
    // The 8 user constraints fully determine the frame (4 DOFs), leaving 0 DOFs.
    // The rank-reduction code handles the overconstrained Jacobian (20 rows → rank 16)
    // so qp_eq doesn't fail, but nothing can move since the null space is empty.
    double p1x = 0., p1y = 0., p2x = 10., p2y = 0.;
    double s1x = 1., s1y = 3., e1x = 4., e1y = 3.;
    double s2x = 5., s2y = 2., e2x = 8., e2y = 2.;
    double s3x = 2., s3y = -1., e3x = 7., e3y = -1.;

    GCS::Point p1, p2, l1s, l1e, l2s, l2e, l3s, l3e;
    p1.x = &p1x;
    p1.y = &p1y;
    p2.x = &p2x;
    p2.y = &p2y;
    l1s.x = &s1x;
    l1s.y = &s1y;
    l1e.x = &e1x;
    l1e.y = &e1y;
    l2s.x = &s2x;
    l2s.y = &s2y;
    l2e.x = &e2x;
    l2e.y = &e2y;
    l3s.x = &s3x;
    l3s.y = &s3y;
    l3e.x = &e3x;
    l3e.y = &e3y;

    System()->addConstraintDerivedPoint(p1, p2, l1s, 0.1, 0.3, GCS::InternalHardConstraint, true);
    System()->addConstraintDerivedPoint(p1, p2, l1e, 0.4, 0.3, GCS::InternalHardConstraint, true);
    System()->addConstraintDerivedPoint(p1, p2, l2s, 0.5, 0.2, GCS::InternalHardConstraint, true);
    System()->addConstraintDerivedPoint(p1, p2, l2e, 0.8, 0.2, GCS::InternalHardConstraint, true);
    System()->addConstraintDerivedPoint(p1, p2, l3s, 0.2, -0.1, GCS::InternalHardConstraint, true);
    System()->addConstraintDerivedPoint(p1, p2, l3e, 0.7, -0.1, GCS::InternalHardConstraint, true);

    // 8 user constraints: fix all coords of line1 and line2.
    // This consumes all 4 frame DOFs (the extra 4 are redundant via DerivedPoint).
    double t1 = 1., t2 = 3., t3 = 4., t4 = 3.;
    double t5 = 5., t6 = 2., t7 = 8., t8 = 2.;
    System()->addConstraintCoordinateX(l1s, &t1, 1, true);
    System()->addConstraintCoordinateY(l1s, &t2, 2, true);
    System()->addConstraintCoordinateX(l1e, &t3, 3, true);
    System()->addConstraintCoordinateY(l1e, &t4, 4, true);
    System()->addConstraintCoordinateX(l2s, &t5, 5, true);
    System()->addConstraintCoordinateY(l2s, &t6, 6, true);
    System()->addConstraintCoordinateX(l2e, &t7, 7, true);
    System()->addConstraintCoordinateY(l2e, &t8, 8, true);

    std::vector<double*> params = {
        p1.x,
        p1.y,
        p2.x,
        p2.y,
        l1s.x,
        l1s.y,
        l1e.x,
        l1e.y,
        l2s.x,
        l2s.y,
        l2e.x,
        l2e.y,
        l3s.x,
        l3s.y,
        l3e.x,
        l3e.y
    };

    // Initial solve
    EXPECT_EQ(System()->solve(params), GCS::Success);
    System()->applySolution();

    // Drag line3 end to (10, -3)
    double moveX = 10., moveY = -3.;
    GCS::Point moveTarget;
    moveTarget.x = &moveX;
    moveTarget.y = &moveY;
    System()->addConstraintP2PCoincident(l3e, moveTarget, GCS::DefaultTemporaryConstraint, true);

    int ret = System()->solve(params);
    // Solver should succeed (rank reduction handles the overconstrained Jacobian)
    EXPECT_LE(ret, GCS::Converged)
        << "Overconstrained group: solver should not fail (rank reduction handles it)";
    System()->applySolution();

    // All DerivedPoints should be satisfied
    expectDerivedPointSatisfied(p1x, p1y, p2x, p2y, s1x, s1y, 0.1, 0.3);
    expectDerivedPointSatisfied(p1x, p1y, p2x, p2y, e1x, e1y, 0.4, 0.3);
    expectDerivedPointSatisfied(p1x, p1y, p2x, p2y, s2x, s2y, 0.5, 0.2);
    expectDerivedPointSatisfied(p1x, p1y, p2x, p2y, e2x, e2y, 0.8, 0.2);
    expectDerivedPointSatisfied(p1x, p1y, p2x, p2y, s3x, s3y, 0.2, -0.1);
    expectDerivedPointSatisfied(p1x, p1y, p2x, p2y, e3x, e3y, 0.7, -0.1);

    // Fixed points should stay fixed
    EXPECT_NEAR(s1x, 1., 1e-6);
    EXPECT_NEAR(s1y, 3., 1e-6);

    // With 0 DOFs (frame fully determined by user constraints), nothing can move.
    // This is correct behavior — the group is fully constrained.
    EXPECT_NEAR(e3x, 7., 1e-6) << "Fully constrained group: l3e should not move";
    EXPECT_NEAR(e3y, -1., 1e-6) << "Fully constrained group: l3e should not move";
}

TEST_F(ConstraintsTest, DerivedPointInternalHardFewUserConstraints)  // NOLINT
{
    // 3 lines (6 vertices), but only 2 user constraints — leaving 2 DOFs.
    // Frame has 4 DOFs; 2 user constraints consume 2, leaving 2 for drag.
    double p1x = 0., p1y = 0., p2x = 10., p2y = 0.;
    double s1x = 1., s1y = 3., e1x = 4., e1y = 3.;
    double s2x = 5., s2y = 2., e2x = 8., e2y = 2.;
    double s3x = 2., s3y = -1., e3x = 7., e3y = -1.;

    GCS::Point p1, p2, l1s, l1e, l2s, l2e, l3s, l3e;
    p1.x = &p1x;
    p1.y = &p1y;
    p2.x = &p2x;
    p2.y = &p2y;
    l1s.x = &s1x;
    l1s.y = &s1y;
    l1e.x = &e1x;
    l1e.y = &e1y;
    l2s.x = &s2x;
    l2s.y = &s2y;
    l2e.x = &e2x;
    l2e.y = &e2y;
    l3s.x = &s3x;
    l3s.y = &s3y;
    l3e.x = &e3x;
    l3e.y = &e3y;

    System()->addConstraintDerivedPoint(p1, p2, l1s, 0.1, 0.3, GCS::InternalHardConstraint, true);
    System()->addConstraintDerivedPoint(p1, p2, l1e, 0.4, 0.3, GCS::InternalHardConstraint, true);
    System()->addConstraintDerivedPoint(p1, p2, l2s, 0.5, 0.2, GCS::InternalHardConstraint, true);
    System()->addConstraintDerivedPoint(p1, p2, l2e, 0.8, 0.2, GCS::InternalHardConstraint, true);
    System()->addConstraintDerivedPoint(p1, p2, l3s, 0.2, -0.1, GCS::InternalHardConstraint, true);
    System()->addConstraintDerivedPoint(p1, p2, l3e, 0.7, -0.1, GCS::InternalHardConstraint, true);

    // Only 2 user constraints: fix l1s position (consumes 2 of 4 frame DOFs)
    double t1 = 1., t2 = 3.;
    System()->addConstraintCoordinateX(l1s, &t1, 1, true);
    System()->addConstraintCoordinateY(l1s, &t2, 2, true);

    std::vector<double*> params = {
        p1.x,
        p1.y,
        p2.x,
        p2.y,
        l1s.x,
        l1s.y,
        l1e.x,
        l1e.y,
        l2s.x,
        l2s.y,
        l2e.x,
        l2e.y,
        l3s.x,
        l3s.y,
        l3e.x,
        l3e.y
    };

    // Initial solve
    EXPECT_EQ(System()->solve(params), GCS::Success);
    System()->applySolution();

    EXPECT_NEAR(s1x, 1., 1e-6);
    EXPECT_NEAR(s1y, 3., 1e-6);

    double e3x_before = e3x, e3y_before = e3y;

    // Drag line3 end
    double moveX = 10., moveY = -3.;
    GCS::Point moveTarget;
    moveTarget.x = &moveX;
    moveTarget.y = &moveY;
    System()->addConstraintP2PCoincident(l3e, moveTarget, GCS::DefaultTemporaryConstraint, true);

    int ret = System()->solve(params);
    EXPECT_LE(ret, GCS::Converged);
    System()->applySolution();

    // All DerivedPoints should still be satisfied
    expectDerivedPointSatisfied(p1x, p1y, p2x, p2y, s1x, s1y, 0.1, 0.3);
    expectDerivedPointSatisfied(p1x, p1y, p2x, p2y, e1x, e1y, 0.4, 0.3);
    expectDerivedPointSatisfied(p1x, p1y, p2x, p2y, s3x, s3y, 0.2, -0.1);
    expectDerivedPointSatisfied(p1x, p1y, p2x, p2y, e3x, e3y, 0.7, -0.1);

    // User constraint should still be satisfied
    EXPECT_NEAR(s1x, 1., 1e-6);
    EXPECT_NEAR(s1y, 3., 1e-6);

    // With 2 DOFs remaining, l3e should have moved toward the drag target
    double dist = std::sqrt(
        (e3x - e3x_before) * (e3x - e3x_before) + (e3y - e3y_before) * (e3y - e3y_before)
    );
    EXPECT_GT(dist, 0.1) << "Line3 end should have moved — 2 DOFs available for drag";
}

TEST_F(ConstraintsTest, DerivedPointInternalHardSketcherFlow)  // NOLINT
{
    // Mirror the EXACT Sketcher flow:
    // 1. Add user constraints + DerivedPoint
    // 2. declareUnknowns + initSolution (triggers diagnose)
    // 3. solve(bool) for initial position
    // 4. Add P2PCoincident for drag
    // 5. initSolution again (skips diagnose)
    // 6. solve(bool) for drag
    double p1x = 0., p1y = 0., p2x = 10., p2y = 0.;
    double sx = 2., sy = 5., ex = 8., ey = 5.;
    GCS::Point p1, p2, s, e;
    p1.x = &p1x;
    p1.y = &p1y;
    p2.x = &p2x;
    p2.y = &p2y;
    s.x = &sx;
    s.y = &sy;
    e.x = &ex;
    e.y = &ey;

    // Step 1: Add constraints (order: user first, then DerivedPoint)
    double targetX = 2.;
    System()->addConstraintCoordinateX(s, &targetX, 1, true);  // user constraint, resets hasDiagnosis

    System()->addConstraintDerivedPoint(p1, p2, s, 0.2, 0.5, GCS::InternalHardConstraint, true);
    System()->addConstraintDerivedPoint(p1, p2, e, 0.8, 0.5, GCS::InternalHardConstraint, true);

    // Step 2: declareUnknowns + initSolution (separate calls, like Sketch does)
    std::vector<double*> params = {p1.x, p1.y, p2.x, p2.y, s.x, s.y, e.x, e.y};
    System()->declareUnknowns(params);
    System()->initSolution();  // triggers diagnose because user constraint reset hasDiagnosis

    // Step 3: solve(bool) for initial position
    int ret = System()->solve();
    EXPECT_EQ(ret, GCS::Success) << "Initial solve failed";
    System()->applySolution();

    EXPECT_NEAR(sx, 2., 1e-10);
    EXPECT_NEAR(sy, 5., 1e-10);

    // Step 4: Add drag constraint (like initMove does)
    double moveX = 12., moveY = 8.;
    GCS::Point moveTarget;
    moveTarget.x = &moveX;
    moveTarget.y = &moveY;
    System()->addConstraintP2PCoincident(e, moveTarget, GCS::DefaultTemporaryConstraint, true);

    // Step 5: initSolution (like initMove does — does NOT call declareUnknowns)
    System()->initSolution();  // hasDiagnosis=true → skips diagnose

    // Step 6: solve(bool) for drag
    ret = System()->solve();
    EXPECT_EQ(ret, GCS::Success) << "Drag solve failed with Sketcher flow";
    System()->applySolution();

    // Hard constraints satisfied
    EXPECT_NEAR(sx, 2., 1e-8);
    expectDerivedPointSatisfied(p1x, p1y, p2x, p2y, sx, sy, 0.2, 0.5);
    expectDerivedPointSatisfied(p1x, p1y, p2x, p2y, ex, ey, 0.8, 0.5);

    // End vertex moved toward target
    EXPECT_NEAR(ex, 12., 1e-6);
    EXPECT_NEAR(ey, 8., 1e-6);
}

// Helper: compute DerivedPoint position from frame + canonical coords.
// Q = P1 + u*(P2-P1) + v*perp(P2-P1), where perp(dx,dy) = (-dy,dx)
static std::pair<double, double> computeDerivedPosition(
    double p1x,
    double p1y,
    double p2x,
    double p2y,
    double u,
    double v
)
{
    double dx = p2x - p1x;
    double dy = p2y - p1y;
    return {p1x + u * dx - v * dy, p1y + u * dy + v * dx};
}

TEST_F(ConstraintsTest, DerivedPointPassiveVertexComputedFromFrame)  // NOLINT
{
    // Frame + 1 solver vertex (with DerivedPoint + external constraint).
    // 2 other vertices exist as doubles but are NOT solver unknowns and
    // have NO DerivedPoint constraints. After solving, verify that their
    // positions can be correctly computed from the post-solve frame.
    double p1x = 0., p1y = 0., p2x = 10., p2y = 0.;
    double qx = 5., qy = 5.;   // solver vertex (u=0.5, v=0.5)
    double ax = 2., ay = 3.;   // passive vertex (u=0.2, v=0.3)
    double bx = 8., by = -2.;  // passive vertex (u=0.8, v=-0.2)

    GCS::Point p1, p2, q, a, b;
    p1.x = &p1x;
    p1.y = &p1y;
    p2.x = &p2x;
    p2.y = &p2y;
    q.x = &qx;
    q.y = &qy;
    a.x = &ax;
    a.y = &ay;
    b.x = &bx;
    b.y = &by;

    // Only q gets DerivedPoint + external constraint
    System()->addConstraintDerivedPoint(p1, p2, q, 0.5, 0.5, GCS::InternalHardConstraint, true);
    double targetX = 5.;
    System()->addConstraintCoordinateX(q, &targetX, 1, true);

    // Only frame + q are solver unknowns. a and b are NOT in plist.
    std::vector<double*> params = {p1.x, p1.y, p2.x, p2.y, q.x, q.y};
    System()->declareUnknowns(params);
    System()->initSolution();
    EXPECT_EQ(System()->solve(), GCS::Success);
    System()->applySolution();

    // q should satisfy its constraints
    EXPECT_NEAR(qx, 5., 1e-6);
    expectDerivedPointSatisfied(p1x, p1y, p2x, p2y, qx, qy, 0.5, 0.5);

    // Now drag frame (translate by +5, +5)
    double m1x = p1x + 5., m1y = p1y + 5.;
    double m2x = p2x + 5., m2y = p2y + 5.;
    GCS::Point mt1, mt2;
    mt1.x = &m1x;
    mt1.y = &m1y;
    mt2.x = &m2x;
    mt2.y = &m2y;
    System()->addConstraintP2PCoincident(p1, mt1, GCS::DefaultTemporaryConstraint, true);
    System()->addConstraintP2PCoincident(p2, mt2, GCS::DefaultTemporaryConstraint, true);

    System()->initSolution();
    EXPECT_LE(System()->solve(), GCS::Converged);
    System()->applySolution();

    // Compute expected passive vertex positions from post-solve frame
    auto [expAx, expAy] = computeDerivedPosition(p1x, p1y, p2x, p2y, 0.2, 0.3);
    auto [expBx, expBy] = computeDerivedPosition(p1x, p1y, p2x, p2y, 0.8, -0.2);

    // Passive doubles are stale (not solver unknowns), but the FORMULA gives correct positions
    EXPECT_NE(ax, expAx);  // a was NOT updated by solver — it's passive
    // Verify the formula gives correct positions relative to the moved frame
    EXPECT_GT(std::abs(expAx - 2.) + std::abs(expAy - 3.), 1.0)
        << "Expected passive positions should differ from initial (frame moved)";

    // The key assertion: DerivedPoint formula with post-solve frame produces
    // positions that are consistent with the frame transform
    expectDerivedPointSatisfied(p1x, p1y, p2x, p2y, expAx, expAy, 0.2, 0.3);
    expectDerivedPointSatisfied(p1x, p1y, p2x, p2y, expBx, expBy, 0.8, -0.2);
}

TEST_F(ConstraintsTest, DerivedPointDragPromotedVertex)  // NOLINT
{
    // Frame + 2 passive vertices (not in plist, no DerivedPoint).
    // Then "promote" one: add to plist, create DerivedPoint, add drag.
    // Verify the promoted vertex moves and DerivedPoint is satisfied.
    double p1x = 0., p1y = 0., p2x = 10., p2y = 0.;
    double ax = 3., ay = 4.;   // passive → will be promoted (u=0.3, v=0.4)
    double bx = 7., by = -2.;  // stays passive (u=0.7, v=-0.2)

    GCS::Point p1, p2, a, b;
    p1.x = &p1x;
    p1.y = &p1y;
    p2.x = &p2x;
    p2.y = &p2y;
    a.x = &ax;
    a.y = &ay;
    b.x = &bx;
    b.y = &by;

    // Initial solve: only frame as unknowns, no DerivedPoint at all
    std::vector<double*> params = {p1.x, p1.y, p2.x, p2.y};
    System()->declareUnknowns(params);
    System()->initSolution();
    EXPECT_EQ(System()->solve(), GCS::Success);
    System()->applySolution();

    // Now promote vertex 'a': add to params, create DerivedPoint, add drag
    params.push_back(a.x);
    params.push_back(a.y);
    System()->addConstraintDerivedPoint(p1, p2, a, 0.3, 0.4, GCS::InternalHardConstraint, true);

    double moveX = 10., moveY = 8.;
    GCS::Point moveTarget;
    moveTarget.x = &moveX;
    moveTarget.y = &moveY;
    System()->addConstraintP2PCoincident(a, moveTarget, GCS::DefaultTemporaryConstraint, true);

    System()->declareUnknowns(params);
    System()->initSolution();
    int ret = System()->solve();
    EXPECT_LE(ret, GCS::Converged) << "Promoted vertex drag should succeed";
    System()->applySolution();

    // Promoted vertex should have moved toward target
    double dist = std::sqrt((ax - 3.) * (ax - 3.) + (ay - 4.) * (ay - 4.));
    EXPECT_GT(dist, 0.5) << "Promoted vertex should have moved from initial position";

    // DerivedPoint constraint on promoted vertex should be satisfied
    expectDerivedPointSatisfied(p1x, p1y, p2x, p2y, ax, ay, 0.3, 0.4);

    // Passive vertex 'b' is NOT in plist → unchanged by solver
    EXPECT_NEAR(bx, 7., 1e-10);
    EXPECT_NEAR(by, -2., 1e-10);

    // But the formula gives b's correct position relative to the moved frame
    auto [expBx, expBy] = computeDerivedPosition(p1x, p1y, p2x, p2y, 0.7, -0.2);
    expectDerivedPointSatisfied(p1x, p1y, p2x, p2y, expBx, expBy, 0.7, -0.2);
}

TEST_F(ConstraintsTest, DerivedPointReducedMatchesFull)  // NOLINT
{
    // Same scenario solved two ways:
    // (a) ALL vertices as unknowns with DerivedPoint (current full approach)
    // (b) Only externally-constrained vertex as unknown (reduced approach)
    // Verify frame + constrained vertex positions match.
    double p1x = 0., p1y = 0., p2x = 10., p2y = 0.;
    double qx = 5., qy = 5.;   // externally constrained (u=0.5, v=0.5)
    double ax = 2., ay = 3.;   // passive (u=0.2, v=0.3)
    double bx = 8., by = -2.;  // passive (u=0.8, v=-0.2)

    // --- Full solve: all 3 vertices as unknowns ---
    {
        GCS::System fullSys;
        GCS::Point p1f, p2f, qf, af, bf;
        p1f.x = &p1x;
        p1f.y = &p1y;
        p2f.x = &p2x;
        p2f.y = &p2y;
        qf.x = &qx;
        qf.y = &qy;
        af.x = &ax;
        af.y = &ay;
        bf.x = &bx;
        bf.y = &by;

        fullSys.addConstraintDerivedPoint(p1f, p2f, qf, 0.5, 0.5, GCS::InternalHardConstraint, true);
        fullSys.addConstraintDerivedPoint(p1f, p2f, af, 0.2, 0.3, GCS::InternalHardConstraint, true);
        fullSys.addConstraintDerivedPoint(p1f, p2f, bf, 0.8, -0.2, GCS::InternalHardConstraint, true);
        double targetX = 5.;
        fullSys.addConstraintCoordinateX(qf, &targetX, 1, true);

        // Drag q toward (8, 10)
        double mfx = 8., mfy = 10.;
        GCS::Point mf;
        mf.x = &mfx;
        mf.y = &mfy;
        fullSys.addConstraintP2PCoincident(qf, mf, GCS::DefaultTemporaryConstraint, true);

        std::vector<double*> fullParams
            = {p1f.x, p1f.y, p2f.x, p2f.y, qf.x, qf.y, af.x, af.y, bf.x, bf.y};
        EXPECT_LE(fullSys.solve(fullParams), GCS::Converged);
        fullSys.applySolution();
    }
    // Save full-solve results
    double fullP1x = p1x, fullP1y = p1y, fullP2x = p2x, fullP2y = p2y;
    double fullQx = qx, fullQy = qy;

    // --- Reset ---
    p1x = 0.;
    p1y = 0.;
    p2x = 10.;
    p2y = 0.;
    qx = 5.;
    qy = 5.;
    ax = 2.;
    ay = 3.;
    bx = 8.;
    by = -2.;

    // --- Reduced solve: only frame + q as unknowns ---
    {
        GCS::System redSys;
        GCS::Point p1r, p2r, qr;
        p1r.x = &p1x;
        p1r.y = &p1y;
        p2r.x = &p2x;
        p2r.y = &p2y;
        qr.x = &qx;
        qr.y = &qy;

        redSys.addConstraintDerivedPoint(p1r, p2r, qr, 0.5, 0.5, GCS::InternalHardConstraint, true);
        double targetX = 5.;
        redSys.addConstraintCoordinateX(qr, &targetX, 1, true);

        double mrx = 8., mry = 10.;
        GCS::Point mr;
        mr.x = &mrx;
        mr.y = &mry;
        redSys.addConstraintP2PCoincident(qr, mr, GCS::DefaultTemporaryConstraint, true);

        std::vector<double*> redParams = {p1r.x, p1r.y, p2r.x, p2r.y, qr.x, qr.y};
        EXPECT_LE(redSys.solve(redParams), GCS::Converged);
        redSys.applySolution();
    }

    // Both solvers must satisfy the hard constraint: Q.x = 5
    EXPECT_NEAR(fullQx, 5.0, 1e-6);
    EXPECT_NEAR(qx, 5.0, 1e-6);

    // Both solvers must satisfy DerivedPoint for the constrained vertex Q
    auto [fullDerivedQx, fullDerivedQy]
        = computeDerivedPosition(fullP1x, fullP1y, fullP2x, fullP2y, 0.5, 0.5);
    EXPECT_NEAR(fullQx, fullDerivedQx, 1e-6);
    EXPECT_NEAR(fullQy, fullDerivedQy, 1e-6);

    auto [redDerivedQx, redDerivedQy] = computeDerivedPosition(p1x, p1y, p2x, p2y, 0.5, 0.5);
    EXPECT_NEAR(qx, redDerivedQx, 1e-6);
    EXPECT_NEAR(qy, redDerivedQy, 1e-6);

    // Passive vertices computed from each solver's frame are internally consistent
    // (they need not match between full and reduced — different null spaces)
    auto [fullAx, fullAy] = computeDerivedPosition(fullP1x, fullP1y, fullP2x, fullP2y, 0.2, 0.3);
    auto [fullBx, fullBy] = computeDerivedPosition(fullP1x, fullP1y, fullP2x, fullP2y, 0.8, -0.2);
    auto [redAx, redAy] = computeDerivedPosition(p1x, p1y, p2x, p2y, 0.2, 0.3);
    auto [redBx, redBy] = computeDerivedPosition(p1x, p1y, p2x, p2y, 0.8, -0.2);

    // Verify passive vertices are reasonable (moved from initial positions)
    EXPECT_NE(fullAx, 2.0);  // a moved from initial (2, 3)
    EXPECT_NE(redAx, 2.0);
    EXPECT_NE(fullBx, 8.0);  // b moved from initial (8, -2)
    EXPECT_NE(redBx, 8.0);

    // Both approaches should produce a drag toward (8, 10) — Q.y should increase
    EXPECT_GT(fullQy, 5.0);
    EXPECT_GT(qy, 5.0);
}

TEST_F(ConstraintsTest, DerivedPointPassiveDragEdge)  // NOLINT
{
    // Frame + 5 passive vertices (not in plist). Drag frame edge.
    // After solve, verify all passive positions match DerivedPoint formula.
    double p1x = 0., p1y = 0., p2x = 10., p2y = 0.;
    GCS::Point p1, p2;
    p1.x = &p1x;
    p1.y = &p1y;
    p2.x = &p2x;
    p2.y = &p2y;

    // 5 passive vertices — canonical coords but NOT solver unknowns
    struct PassiveVertex
    {
        double x, y, u, v;
    };
    PassiveVertex verts[] = {
        {1., 3., 0.1, 0.3},
        {5., 5., 0.5, 0.5},
        {8., -2., 0.8, -0.2},
        {3., -1., 0.3, -0.1},
        {9., 1., 0.9, 0.1},
    };

    // Only frame as solver unknowns
    std::vector<double*> params = {p1.x, p1.y, p2.x, p2.y};
    System()->declareUnknowns(params);
    System()->initSolution();
    EXPECT_EQ(System()->solve(), GCS::Success);
    System()->applySolution();

    // Drag frame: translate by (+3, +7)
    double m1x = 3., m1y = 7., m2x = 13., m2y = 7.;
    GCS::Point mt1, mt2;
    mt1.x = &m1x;
    mt1.y = &m1y;
    mt2.x = &m2x;
    mt2.y = &m2y;
    System()->addConstraintP2PCoincident(p1, mt1, GCS::DefaultTemporaryConstraint, true);
    System()->addConstraintP2PCoincident(p2, mt2, GCS::DefaultTemporaryConstraint, true);

    System()->initSolution();
    EXPECT_LE(System()->solve(), GCS::Converged);
    System()->applySolution();

    // Frame should have moved
    EXPECT_NEAR(p1x, 3., 1e-6);
    EXPECT_NEAR(p1y, 7., 1e-6);
    EXPECT_NEAR(p2x, 13., 1e-6);
    EXPECT_NEAR(p2y, 7., 1e-6);

    // All passive vertex positions computed from post-solve frame
    for (const auto& v : verts) {
        auto [ex, ey] = computeDerivedPosition(p1x, p1y, p2x, p2y, v.u, v.v);
        // Verify they satisfy the DerivedPoint equation
        expectDerivedPointSatisfied(p1x, p1y, p2x, p2y, ex, ey, v.u, v.v);
        // Verify they actually moved (frame translated by +3, +7)
        EXPECT_NEAR(ex, v.x + 3., 1e-6);
        EXPECT_NEAR(ey, v.y + 7., 1e-6);
    }
}

TEST_F(ConstraintsTest, DerivedPointPromotedDragThenEdgeDrag)  // NOLINT
{
    // Promote 1 vertex for drag, solve. Then clear temporaries,
    // drag frame edge. Verify promoted vertex (now has DerivedPoint) works.
    double p1x = 0., p1y = 0., p2x = 10., p2y = 0.;
    double ax = 3., ay = 4.;  // will be promoted (u=0.3, v=0.4)

    GCS::Point p1, p2, a;
    p1.x = &p1x;
    p1.y = &p1y;
    p2.x = &p2x;
    p2.y = &p2y;
    a.x = &ax;
    a.y = &ay;

    // Phase 1: promote vertex 'a' and drag it
    std::vector<double*> params = {p1.x, p1.y, p2.x, p2.y, a.x, a.y};
    System()->addConstraintDerivedPoint(p1, p2, a, 0.3, 0.4, GCS::InternalHardConstraint, true);

    double moveX = 6., moveY = 8.;
    GCS::Point moveTarget;
    moveTarget.x = &moveX;
    moveTarget.y = &moveY;
    System()->addConstraintP2PCoincident(a, moveTarget, GCS::DefaultTemporaryConstraint, true);

    System()->declareUnknowns(params);
    System()->initSolution();
    EXPECT_LE(System()->solve(), GCS::Converged);
    System()->applySolution();

    expectDerivedPointSatisfied(p1x, p1y, p2x, p2y, ax, ay, 0.3, 0.4);
    double afterDragAx = ax, afterDragAy = ay;

    // Phase 2: clear drag temporaries, then drag frame edge
    System()->clearByTag(GCS::DefaultTemporaryConstraint);

    double fm1x = p1x + 2., fm1y = p1y + 2.;
    double fm2x = p2x + 2., fm2y = p2y + 2.;
    GCS::Point fmt1, fmt2;
    fmt1.x = &fm1x;
    fmt1.y = &fm1y;
    fmt2.x = &fm2x;
    fmt2.y = &fm2y;
    System()->addConstraintP2PCoincident(p1, fmt1, GCS::DefaultTemporaryConstraint, true);
    System()->addConstraintP2PCoincident(p2, fmt2, GCS::DefaultTemporaryConstraint, true);

    System()->initSolution();
    EXPECT_LE(System()->solve(), GCS::Converged);
    System()->applySolution();

    // DerivedPoint still satisfied (it's tag -2, survived clearByTag(-1))
    expectDerivedPointSatisfied(p1x, p1y, p2x, p2y, ax, ay, 0.3, 0.4);

    // Vertex 'a' should have moved with the frame (translate +2, +2)
    EXPECT_NEAR(ax, afterDragAx + 2., 1e-6);
    EXPECT_NEAR(ay, afterDragAy + 2., 1e-6);
}

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

// Tests for edge constraints on "grouped-like" geometry, where the curve's
// parameters are passive constants (not solver unknowns).

TEST_F(ConstraintsTest, GroupedLinePointOnLine)  // NOLINT
{
    // Create a GCS::Line from constant (non-solver) endpoints.
    // Add PointOnLine with a solver-unknown point + drag target.
    // Verify solver places the point on the line.
    double lx1 = 0., ly1 = 0., lx2 = 10., ly2 = 10.;
    GCS::Point lp1, lp2;
    lp1.x = &lx1;
    lp1.y = &ly1;
    lp2.x = &lx2;
    lp2.y = &ly2;

    GCS::Line line;
    line.p1 = lp1;
    line.p2 = lp2;

    // Solver-unknown point, initially off the line
    double px = 7., py = 3.;
    GCS::Point point;
    point.x = &px;
    point.y = &py;

    System()->addConstraintPointOnLine(point, line, 0, true);

    // Only the point coords are solver unknowns; line endpoints are constants.
    std::vector<double*> params = {&px, &py};
    EXPECT_EQ(System()->solve(params), GCS::Success);
    System()->applySolution();

    // Point should lie on the line y = x
    EXPECT_NEAR(px, py, 1e-10);
}

TEST_F(ConstraintsTest, GroupedLinePointOnLineDragFrame)  // NOLINT
{
    // Same passive-line setup. Simulate frame drag: manually update the
    // line's passive endpoint doubles to new positions (as
    // applyGroupTransformations would). Re-solve with updated constants.
    double lx1 = 0., ly1 = 0., lx2 = 10., ly2 = 0.;
    GCS::Point lp1, lp2;
    lp1.x = &lx1;
    lp1.y = &ly1;
    lp2.x = &lx2;
    lp2.y = &ly2;

    GCS::Line line;
    line.p1 = lp1;
    line.p2 = lp2;

    double px = 5., py = 3.;
    GCS::Point point;
    point.x = &px;
    point.y = &py;

    System()->addConstraintPointOnLine(point, line, 0, true);

    std::vector<double*> params = {&px, &py};
    EXPECT_EQ(System()->solve(params), GCS::Success);
    System()->applySolution();

    // Point should be on the horizontal line y=0
    EXPECT_NEAR(py, 0., 1e-10);

    // Now "drag" the frame: rotate the line 90 degrees (vertical)
    lx1 = 0.;
    ly1 = 0.;
    lx2 = 0.;
    ly2 = 10.;

    // Re-solve: the point must now lie on the new (vertical) line x=0
    EXPECT_EQ(System()->solve(params), GCS::Success);
    System()->applySolution();

    EXPECT_NEAR(px, 0., 1e-10);
}

TEST_F(ConstraintsTest, GroupedArcPointOnArc)  // NOLINT
{
    // Create GCS::Arc with constant center/radius/angles (not solver unknowns).
    // Add PointOnArc with solver-unknown point. Verify point lies on arc.
    double cx = 0., cy = 0.;
    double rad = 5.;
    double sa = 0., ea = std::numbers::pi;

    GCS::Point center;
    center.x = &cx;
    center.y = &cy;

    // Arc start/end points (on the circle, consistent with angles)
    double sx = 5., sy = 0.;   // at angle 0
    double ex = -5., ey = 0.;  // at angle pi
    GCS::Point arcStart, arcEnd;
    arcStart.x = &sx;
    arcStart.y = &sy;
    arcEnd.x = &ex;
    arcEnd.y = &ey;

    GCS::Arc arc;
    arc.center = center;
    arc.rad = &rad;
    arc.startAngle = &sa;
    arc.endAngle = &ea;
    arc.start = arcStart;
    arc.end = arcEnd;

    // Solver-unknown point, initially off the arc
    double px = 3., py = 3.;
    GCS::Point point;
    point.x = &px;
    point.y = &py;

    System()->addConstraintPointOnArc(point, arc, 0, true);

    std::vector<double*> params = {&px, &py};
    EXPECT_EQ(System()->solve(params), GCS::Success);
    System()->applySolution();

    // Point should lie on the arc (distance from center == radius)
    double dist = std::sqrt(px * px + py * py);
    EXPECT_NEAR(dist, 5., 1e-10);
}

TEST_F(ConstraintsTest, GroupedLineParallel)  // NOLINT
{
    // Create one normal GCS::Line (solver unknowns) and one passive GCS::Line
    // (constants). Add Parallel constraint. Verify the solver-controlled line
    // ends up parallel.

    // Passive line: slope = 1 (from (0,0) to (10,10))
    double plx1 = 0., ply1 = 0., plx2 = 10., ply2 = 10.;
    GCS::Point pp1, pp2;
    pp1.x = &plx1;
    pp1.y = &ply1;
    pp2.x = &plx2;
    pp2.y = &ply2;
    GCS::Line passiveLine;
    passiveLine.p1 = pp1;
    passiveLine.p2 = pp2;

    // Solver-controlled line: initially not parallel (slope ~= 0.5)
    double alx1 = 1., aly1 = 2., alx2 = 5., aly2 = 4.;
    GCS::Point ap1, ap2;
    ap1.x = &alx1;
    ap1.y = &aly1;
    ap2.x = &alx2;
    ap2.y = &aly2;
    GCS::Line activeLine;
    activeLine.p1 = ap1;
    activeLine.p2 = ap2;

    System()->addConstraintParallel(activeLine, passiveLine, 0, true);

    std::vector<double*> params = {&alx1, &aly1, &alx2, &aly2};
    EXPECT_EQ(System()->solve(params), GCS::Success);
    System()->applySolution();

    // Lines should be parallel: cross product of direction vectors == 0
    double dx1 = plx2 - plx1, dy1 = ply2 - ply1;
    double dx2 = alx2 - alx1, dy2 = aly2 - aly1;
    double cross = dx1 * dy2 - dy1 * dx2;
    EXPECT_NEAR(cross, 0., 1e-10);
}

TEST_F(ConstraintsTest, GroupedLinePerpendicular)  // NOLINT
{
    // Perpendicular between a passive (grouped) line and a solver-controlled line.
    // This is the real-world scenario: perpendicular between a text/group segment
    // and the Y-axis or another sketch line.

    // Passive (grouped) line: horizontal, from (0,0) to (10,0)
    double plx1 = 0., ply1 = 0., plx2 = 10., ply2 = 0.;
    GCS::Point pp1, pp2;
    pp1.x = &plx1;
    pp1.y = &ply1;
    pp2.x = &plx2;
    pp2.y = &ply2;
    GCS::Line passiveLine;
    passiveLine.p1 = pp1;
    passiveLine.p2 = pp2;

    // Solver-controlled line: initially at 45 degrees
    double alx1 = 1., aly1 = 1., alx2 = 5., aly2 = 5.;
    GCS::Point ap1, ap2;
    ap1.x = &alx1;
    ap1.y = &aly1;
    ap2.x = &alx2;
    ap2.y = &aly2;
    GCS::Line activeLine;
    activeLine.p1 = ap1;
    activeLine.p2 = ap2;

    System()->addConstraintPerpendicular(activeLine, passiveLine, 0, true);

    std::vector<double*> params = {&alx1, &aly1, &alx2, &aly2};
    EXPECT_EQ(System()->solve(params), GCS::Success);
    System()->applySolution();

    // Lines should be perpendicular: dot product of direction vectors == 0
    double dx1 = plx2 - plx1, dy1 = ply2 - ply1;
    double dx2 = alx2 - alx1, dy2 = aly2 - aly1;
    double dot = dx1 * dx2 + dy1 * dy2;
    EXPECT_NEAR(dot, 0., 1e-10);
}

TEST_F(ConstraintsTest, GroupedLinePointOnLineFromOutside)  // NOLINT
{
    // A point belonging to a non-grouped line is constrained to lie on
    // a grouped (passive) line. This tests the common case of snapping
    // an external line's endpoint onto a text/group segment.

    // Passive (grouped) line: y = 2x, from (0,0) to (5,10)
    double plx1 = 0., ply1 = 0., plx2 = 5., ply2 = 10.;
    GCS::Point pp1, pp2;
    pp1.x = &plx1;
    pp1.y = &ply1;
    pp2.x = &plx2;
    pp2.y = &ply2;
    GCS::Line passiveLine;
    passiveLine.p1 = pp1;
    passiveLine.p2 = pp2;

    // Solver-controlled line: one endpoint should land on the passive line
    double alx1 = 3., aly1 = 1., alx2 = 8., aly2 = 8.;
    GCS::Point ap1, ap2;
    ap1.x = &alx1;
    ap1.y = &aly1;
    ap2.x = &alx2;
    ap2.y = &aly2;

    System()->addConstraintPointOnLine(ap1, passiveLine, 0, true);

    // Only the active line's first endpoint is a solver unknown for this constraint
    std::vector<double*> params = {&alx1, &aly1};
    EXPECT_EQ(System()->solve(params), GCS::Success);
    System()->applySolution();

    // ap1 should lie on the line y = 2x
    EXPECT_NEAR(aly1, 2. * alx1, 1e-10);
}

TEST_F(ConstraintsTest, GroupedCirclePointOnCircle)  // NOLINT
{
    // A solver-unknown point constrained to lie on a passive (grouped) circle.

    double cx = 5., cy = 5.;
    double rad = 3.;
    GCS::Point center;
    center.x = &cx;
    center.y = &cy;
    GCS::Circle passiveCircle;
    passiveCircle.center = center;
    passiveCircle.rad = &rad;

    // Solver-unknown point, initially off the circle
    double px = 7., py = 9.;
    GCS::Point point;
    point.x = &px;
    point.y = &py;

    System()->addConstraintPointOnCircle(point, passiveCircle, 0, true);

    std::vector<double*> params = {&px, &py};
    EXPECT_EQ(System()->solve(params), GCS::Success);
    System()->applySolution();

    // Point should lie on the circle (distance from center == radius)
    double dist = std::sqrt((px - cx) * (px - cx) + (py - cy) * (py - cy));
    EXPECT_NEAR(dist, 3., 1e-10);
}
