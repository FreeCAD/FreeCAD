// SPDX-License-Identifier: LGPL-2.1-or-later

#include <cmath>
#include <numbers>

#include <gtest/gtest.h>

#include "Mod/Sketcher/App/planegcs/Substitution.h"

class SubstitutionTest: public ::testing::Test
{
protected:
    void SetUp() override
    {}

    void TearDown() override
    {}

private:
};

TEST_F(SubstitutionTest, simpleDist)
{
    double zero = 0.0;

    double lx1 = 4.5;
    double ly1 = 3.0;
    double lx2 = 18.0;
    double ly2 = 2.0;
    GCS::Line line;
    line.p1 = GCS::Point(&lx1, &ly1);
    line.p2 = GCS::Point(&lx2, &ly2);

    double linelength = 10;

    GCS::ConstraintP2PDistance llength(line.p1, line.p2, &linelength);
    GCS::ConstraintEqual lhoriz(line.p1.y, line.p2.y);
    GCS::ConstraintEqual lxorig(line.p1.x, &zero);
    GCS::ConstraintEqual lyorig(line.p1.y, &zero);

    std::vector<double*> unknowns = {&lx1, &ly1, &lx2, &ly2};
    std::vector<GCS::Constraint*> constraints = {&llength, &lhoriz, &lxorig, &lyorig};
    GCS::Substitution subst(unknowns, constraints);

    subst.applyConst();
    subst.applySubst();
    subst.applyReduction();

    EXPECT_NEAR(lx1, 0.0, 1e-10);
    EXPECT_NEAR(ly1, 0.0, 1e-10);
    EXPECT_NEAR(lx2, 10.0, 1e-10);
    EXPECT_NEAR(ly2, 0.0, 1e-10);
}
TEST_F(SubstitutionTest, pointOnLine)
{
    double zero = 0.0;

    double x0 = 4;
    double y0 = 5;
    GCS::Point p0(&x0, &y0);

    double lx1 = 4.5;
    double ly1 = 3.0;
    double lx2 = 5.0;
    double ly2 = 2.0;
    GCS::Line line;
    line.p1 = GCS::Point(&lx1, &ly1);
    line.p2 = GCS::Point(&lx2, &ly2);

    double linelength = 10;

    GCS::ConstraintP2PDistance llength(line.p1, line.p2, &linelength);
    GCS::ConstraintEqual lhoriz(line.p1.y, line.p2.y);
    GCS::ConstraintEqual lxorig(line.p1.x, &zero);
    GCS::ConstraintEqual lyorig(line.p1.y, &zero);

    GCS::ConstraintPointOnLine pol(p0, line);

    std::vector<double*> unknowns = {&x0, &y0, &lx1, &ly1, &lx2, &ly2};
    std::vector<GCS::Constraint*> constraints = {&llength, &lhoriz, &lxorig, &lyorig, &pol};
    GCS::Substitution subst(unknowns, constraints);

    subst.applyConst();
    subst.applySubst();
    subst.applyReduction();

    EXPECT_NEAR(lx1, 0.0, 1e-10);
    EXPECT_NEAR(ly1, 0.0, 1e-10);
    EXPECT_NEAR(lx2, 10.0, 1e-10);
    EXPECT_NEAR(ly2, 0.0, 1e-10);

    EXPECT_NEAR(y0, 0.0, 1e-10);
}

TEST_F(SubstitutionTest, mixedAxisAligned)  // NOLINT
{
    // This creates 2 lines and the origin in an initial condition
    // and from that initial condition and substitution constraints,
    // we expect the origin to be at (0, 0)
    // we expect line1 to span (-10, 0) -> (-10, 25)
    // we expect line2 to span (-10, 5) -> (-7.5, 5)

    double x0 = 0;
    double y0 = 0;
    double zero = 0;
    GCS::Point origin(&x0, &y0);

    GCS::ConstraintEqual originX(&x0, &zero);
    GCS::ConstraintEqual originY(&y0, &zero);

    // Init a line generaly on the left side of the origin
    // with it's second point higher than the first point
    double l1x1 = -2.0;
    double l1y1 = -5.0;
    double l1x2 = -1.0;
    double l1y2 = -1.0;
    double l1l = 25;
    double l1h = 10;
    GCS::Line l1;
    l1.p1 = GCS::Point(&l1x1, &l1y1);
    l1.p2 = GCS::Point(&l1x2, &l1y2);

    GCS::ConstraintEqual l1p1onhoriz(&y0, &l1y1);
    GCS::ConstraintEqual l1vertical(&l1x1, &l1x2);
    GCS::ConstraintP2PDistance l1length(l1.p1, l1.p2, &l1l);
    GCS::ConstraintP2LDistance l1horizpos(origin, l1, &l1h);

    // Init a line with the second point generaly on the right of
    // the first point
    double l2x1 = 4.5;
    double l2y1 = 0.0;
    double l2x2 = 5.0;
    double l2y2 = 0.0;
    double l2l = 2.5;
    double l2v = 5;
    GCS::Line l2;
    l2.p1 = GCS::Point(&l2x1, &l2y1);
    l2.p2 = GCS::Point(&l2x2, &l2y2);

    GCS::ConstraintPerpendicular l1l2perp(l1, l2);
    GCS::ConstraintP2PDistance l2length(l2.p1, l2.p2, &l2l);
    GCS::ConstraintPointOnLine l2p1onl1(l2.p1, l1);
    GCS::ConstraintP2LDistance l2vertpos(l1.p1, l2, &l2v);


    std::vector<double*> unknowns =
        {&x0, &y0, &l1x1, &l1y1, &l1x2, &l1y2, &l2x1, &l2y1, &l2x2, &l2y2};
    std::vector<GCS::Constraint*> constraints = {&originX,
                                                 &originY,
                                                 &l1p1onhoriz,
                                                 &l1vertical,
                                                 &l1length,
                                                 &l1horizpos,
                                                 &l1l2perp,
                                                 &l2length,
                                                 &l2p1onl1,
                                                 &l2vertpos};
    GCS::Substitution subst(unknowns, constraints);

    subst.applyConst();
    subst.applySubst();
    subst.applyReduction();

    EXPECT_NEAR(x0, 0.0, 1e-10);
    EXPECT_NEAR(y0, 0.0, 1e-10);


    // we expect line1 to span (-10, 0) -> (-10, 25)
    // we expect line2 to span (-10, 5) -> (-7.5, 5)
    EXPECT_NEAR(l1x1, -10.0, 1e-10);
    EXPECT_NEAR(l1y1, 0.0, 1e-10);
    EXPECT_NEAR(l1x2, -10.0, 1e-10);
    EXPECT_NEAR(l1y2, 25.0, 1e-10);

    EXPECT_NEAR(l2x1, -10.0, 1e-10);
    EXPECT_NEAR(l2y1, 5.0, 1e-10);
    EXPECT_NEAR(l2x2, -7.5, 1e-10);
    EXPECT_NEAR(l2y2, 5.0, 1e-10);
}

TEST_F(SubstitutionTest, circleLineDist)  // NOLINT
{
    double radiusVal = 10.0;
    double zero = 0.0;
    double plus5 = 5.0;
    double minus5 = -5;


    double lvx1 = 0.0;
    double lvy1 = -5.0;
    double lvx2 = 0.0;
    double lvy2 = 5.0;
    GCS::Line lv;
    lv.p1 = GCS::Point(&lvx1, &lvy1);
    lv.p2 = GCS::Point(&lvx2, &lvy2);

    GCS::ConstraintEqual poslvx1(&lvx1, &zero);
    GCS::ConstraintEqual poslvy1(&lvy1, &minus5);
    GCS::ConstraintEqual poslvx2(&lvx2, &zero);
    GCS::ConstraintEqual poslvy2(&lvy2, &plus5);

    double lhx1 = -5.0;
    double lhy1 = 0.0;
    double lhx2 = 5.0;
    double lhy2 = 0.0;
    GCS::Line lh;
    lh.p1 = GCS::Point(&lhx1, &lhy1);
    lh.p2 = GCS::Point(&lhx2, &lhy2);

    GCS::ConstraintEqual poslhx1(&lhx1, &minus5);
    GCS::ConstraintEqual poslhy1(&lhy1, &zero);
    GCS::ConstraintEqual poslhx2(&lhx2, &plus5);
    GCS::ConstraintEqual poslhy2(&lhy2, &zero);


    // Line circle distance when the line is vertical and initialy
    // closer to the center then the radius from the right
    {
        double cx = 5.8;
        double cy = 0.0;
        double rad = 10;
        GCS::Circle circle;
        circle.center = GCS::Point(&cx, &cy);
        circle.rad = &rad;
        double dist = 4;

        GCS::ConstraintEqual radset(&rad, &radiusVal);
        GCS::ConstraintC2LDistance circdist(circle, lv, &dist);

        std::vector<double*> unknowns = {&lvx1, &lvy1, &lvx2, &lvy2, &cx, &cy, &rad};
        std::vector<GCS::Constraint*> constraints =
            {&poslvx1, &poslvy1, &poslvx2, &poslvy2, &radset, &circdist};
        GCS::Substitution subst(unknowns, constraints);

        subst.applyConst();
        subst.applySubst();
        subst.applyReduction();

        EXPECT_NEAR(cx, 6.0, 1e-10);
    }
    // Line circle distance when the line is vertical and initialy
    // closer to the center then the radius from the left
    {
        double cx = -5.8;
        double cy = 0.0;
        double rad = 10;
        GCS::Circle circle;
        circle.center = GCS::Point(&cx, &cy);
        circle.rad = &rad;
        double dist = 4;

        GCS::ConstraintEqual radset(&rad, &radiusVal);
        GCS::ConstraintC2LDistance circdist(circle, lv, &dist);

        std::vector<double*> unknowns = {&lvx1, &lvy1, &lvx2, &lvy2, &cx, &cy, &rad};
        std::vector<GCS::Constraint*> constraints =
            {&poslvx1, &poslvy1, &poslvx2, &poslvy2, &radset, &circdist};
        GCS::Substitution subst(unknowns, constraints);

        subst.applyConst();
        subst.applySubst();
        subst.applyReduction();

        EXPECT_NEAR(cx, -6.0, 1e-10);
    }

    // Line circle distance when the line is horizontal and initialy
    // farther to the center then the radius from the the top
    {
        double cx = 0.0;
        double cy = -18.0;
        double rad = 10;
        GCS::Circle circle;
        circle.center = GCS::Point(&cx, &cy);
        circle.rad = &rad;
        double dist = 4;

        GCS::ConstraintEqual radset(&rad, &radiusVal);
        GCS::ConstraintC2LDistance circdist(circle, lh, &dist);

        std::vector<double*> unknowns = {&lhx1, &lhy1, &lhx2, &lhy2, &cx, &cy, &rad};
        std::vector<GCS::Constraint*> constraints =
            {&poslhx1, &poslhy1, &poslhx2, &poslhy2, &radset, &circdist};
        GCS::Substitution subst(unknowns, constraints);

        subst.applyConst();
        subst.applySubst();
        subst.applyReduction();

        EXPECT_NEAR(cy, -14.0, 1e-10);
    }
    // Line circle distance when the line is horizontal and initialy
    // farther to the center then the radius from the the bottom
    {
        double cx = 0.0;
        double cy = 18.0;
        double rad = 10;
        GCS::Circle circle;
        circle.center = GCS::Point(&cx, &cy);
        circle.rad = &rad;
        double dist = 4;

        GCS::ConstraintEqual radset(&rad, &radiusVal);
        GCS::ConstraintC2LDistance circdist(circle, lh, &dist);

        std::vector<double*> unknowns = {&lhx1, &lhy1, &lhx2, &lhy2, &cx, &cy, &rad};
        std::vector<GCS::Constraint*> constraints =
            {&poslhx1, &poslhy1, &poslhx2, &poslhy2, &radset, &circdist};
        GCS::Substitution subst(unknowns, constraints);

        subst.applyConst();
        subst.applySubst();
        subst.applyReduction();

        EXPECT_NEAR(cy, 14.0, 1e-10);
    }
}
TEST_F(SubstitutionTest, equalLineLength)  // NOLINT
{
    double l1x1 = -2.0;
    double l1y1 = -5.0;
    double l1x2 = 1.0;
    double l1y2 = 1.0;
    double l1l = 7.0;
    GCS::Line l1;
    l1.p1 = GCS::Point(&l1x1, &l1y1);
    l1.p2 = GCS::Point(&l1x2, &l1y2);

    double l2x1 = -2.0;
    double l2y1 = -5.0;
    double l2x2 = 1.0;
    double l2y2 = 10.0;
    GCS::Line l2;
    l2.p1 = GCS::Point(&l2x1, &l2y1);
    l2.p2 = GCS::Point(&l2x2, &l2y2);

    GCS::ConstraintEqual l1horiz(l1.p1.y, l1.p2.y);
    GCS::ConstraintP2PDistance l1length(l1.p1, l1.p2, &l1l);
    GCS::ConstraintPerpendicular l2vert(l1, l2);

    GCS::ConstraintEqualLineLength l2length(l1, l2);

    std::vector<double*> unknowns = {&l1x1, &l1y1, &l1x2, &l1y2, &l2x1, &l2y1, &l2x2, &l2y2};
    std::vector<GCS::Constraint*> constraints = {&l1horiz, &l1length, &l2vert, &l2length};
    GCS::Substitution subst(unknowns, constraints);

    subst.applyConst();
    subst.applySubst();
    subst.applyReduction();

    EXPECT_NEAR(l2y2, l2y1 + l1l, 1e-12);
}
