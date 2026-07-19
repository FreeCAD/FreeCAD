// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <cmath>

#include "Mod/Sketcher/App/planegcs/GCS.h"

class SystemTest: public GCS::System
{
public:
    size_t getNumberOfConstraints(int tagID = -1)
    {
        return _getNumberOfConstraints(tagID);
    }
};

class GCSTest: public ::testing::Test
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

TEST_F(GCSTest, clearConstraints)  // NOLINT
{
    // Arrange
    const size_t numConstraints {100};
    for (size_t i = 0; i < numConstraints; ++i) {
        System()->addConstraint(new GCS::Constraint());
    }
    ASSERT_EQ(numConstraints, System()->getNumberOfConstraints());

    // Act
    System()->clear();

    // Assert
    EXPECT_EQ(0, System()->getNumberOfConstraints());
}

// Regression guard for the drag-Jacobian column bug. A permanent Equal
// constraint lives in the priority subsystem (subsysA), whose Jacobian the
// two-subsystem SQP drag solver assembles via calcJacobi(plistAB, ..) with a
// params array (plistAB) that differs in size and order from the subsystem's
// own plist. The previous Equal fast path cached its output column keyed to
// plist, so during dragging the Equal gradient landed in the wrong column and
// the constraint was not enforced. After the fix the two params stay equal
// while the drag pulls them to the target.
TEST(DragSolveTest, equalConstraintEnforcedDuringDrag)  // NOLINT
{
    GCS::System sys;
    double x0 = 1.0, y0 = 1.0;
    double x1 = 1.0, y1 = 0.0;

    GCS::Point p0(&x0, &y0), p1(&x1, &y1);

    // Permanent constraint (tag >= 0): x0 == x1. Routes to subSystems (subsysA).
    sys.addConstraintEqual(&x0, &x1);

    GCS::VEC_pD params = {&x0, &y0, &x1, &y1};
    sys.declareUnknowns(params);
    sys.initSolution();
    ASSERT_EQ(sys.solve(true, GCS::DogLeg), GCS::Success);
    sys.applySolution();

    // Drag point (MoveParameters, external to the declared unknowns) coincident
    // with p0, tag = -1 → subSystemsAux (subsysB). Its param set differs from
    // subsysA's, so plistAB is a reordered superset of subsysA's plist.
    double drag_x = x0, drag_y = y0;
    GCS::Point dragP(&drag_x, &drag_y);
    sys.addConstraintP2PCoincident(dragP, p0, GCS::DefaultTemporaryConstraint);
    sys.initSolution();

    // Move the drag target; the SQP solve must pull p0 toward it while keeping
    // the permanent Equal (x0 == x1) satisfied.
    drag_x = 7.0;
    drag_y = 3.0;
    ASSERT_EQ(sys.solve(true, GCS::DogLeg), GCS::Success);
    sys.applySolution();

    EXPECT_NEAR(x0, x1, 1e-6) << "Equal constraint (x0==x1) not enforced during drag "
                              << "(x0=" << x0 << ", x1=" << x1 << ")";
    EXPECT_NEAR(x0, 7.0, 1e-6) << "drag did not pull p0 to target x (x0=" << x0 << ")";
    EXPECT_NEAR(y0, 3.0, 1e-6) << "drag did not pull p0 to target y (y0=" << y0 << ")";
}

// Regression guard for the sparse DogLeg's rank-deficiency handling. Redundant
// (duplicate) constraints make the least-norm normal matrix J*J^T singular. Because
// SimplicialLDLT::info() only reports EXACT zero pivots, a near-singular factorization
// can slip through and produce a NaN/Inf step; the solver must detect the non-finite
// step and fall back to a dense solve, still converging on the (consistent) system.
TEST(SparseDogLegTest, redundantConstraintsDoNotProduceNaN)  // NOLINT
{
    GCS::System sys;
    double x0 = 0.0, y0 = 0.0;
    double x1 = 3.0, y1 = 0.0;
    double d = 5.0;

    GCS::Point p0(&x0, &y0), p1(&x1, &y1);

    // Two identical distance constraints on the same point pair -> J has duplicate
    // rows (rank 1), so with 4 free params (under-constrained) J*J^T (2x2) is singular.
    sys.addConstraintP2PDistance(p0, p1, &d);
    sys.addConstraintP2PDistance(p0, p1, &d);

    GCS::VEC_pD params = {&x0, &y0, &x1, &y1};
    sys.declareUnknowns(params);
    sys.initSolution();

    int res = sys.solve(true, GCS::DogLeg);
    sys.applySolution();

    // The solver must report success on this consistent (redundant) system, not
    // merely leave finite coordinates behind.
    EXPECT_EQ(res, GCS::Success) << "solver did not report Success on a consistent "
                                    "rank-deficient system (res=" << res << ")";
    // The result must be finite and satisfy the (consistent) distance constraint.
    EXPECT_TRUE(std::isfinite(x0) && std::isfinite(y0) && std::isfinite(x1) && std::isfinite(y1))
        << "solve produced a non-finite result on a rank-deficient system";
    double dist = std::hypot(x1 - x0, y1 - y0);
    EXPECT_NEAR(dist, 5.0, 1e-6) << "distance constraint not satisfied (dist=" << dist
                                 << ", res=" << res << ")";
}
