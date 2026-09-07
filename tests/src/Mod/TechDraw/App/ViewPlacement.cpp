// SPDX-License-Identifier: LGPL-2.1-or-later
#include <gtest/gtest.h>

#include <Mod/TechDraw/Gui/ViewPlacement.h>

using TechDrawGui::findAlignedViewPlacement;

TEST(ViewPlacement, ClearsSourceAndKeepsAlignment)
{
    const QRectF bounds(90, 90, 20, 20);
    const QRectF paper(0, 0, 210, 297);
    const QRectF source(75, 75, 50, 50);
    for (const QPointF direction : {QPointF(1, 0), QPointF(0, 1),
                                   QPointF(0.6, 0.8), QPointF(-0.6, -0.8)}) {
        const auto placement = findAlignedViewPlacement(bounds, direction, paper, {source});
        ASSERT_TRUE(placement);
        const QPointF move = *placement;
        EXPECT_FALSE(bounds.translated(move).intersects(source));
        EXPECT_TRUE(paper.contains(bounds.translated(move)));
        EXPECT_NEAR(move.x() * direction.y() - move.y() * direction.x(), 0, 1e-8);
    }
}

TEST(ViewPlacement, UsesOppositeSideWhenBlocked)
{
    const QRectF bounds(90, 90, 20, 20);
    const QRectF source(75, 75, 50, 50);
    const QRectF blocker(125, 0, 85, 297);
    const auto placement = findAlignedViewPlacement(
        bounds, {1, 0}, {0, 0, 210, 297}, {source, blocker});
    ASSERT_TRUE(placement);
    const QPointF move = *placement;
    EXPECT_LT(move.x(), 0);
    EXPECT_DOUBLE_EQ(move.y(), 0);
    EXPECT_FALSE(bounds.translated(move).intersects(source));
    EXPECT_FALSE(bounds.translated(move).intersects(blocker));
}

TEST(ViewPlacement, FindsNarrowGapFurtherAlongGuide)
{
    const QRectF bounds(90, 90, 20, 20);
    const std::vector<QRectF> obstacles{{0, 0, 150, 297}, {170.01, 0, 40, 297}};
    const QRectF paper(0, 0, 210, 297);
    const auto placement = findAlignedViewPlacement(bounds, {1, 0}, paper, obstacles);
    ASSERT_TRUE(placement);
    const QPointF move = *placement;
    EXPECT_TRUE(paper.contains(bounds.translated(move)));
    for (const auto& obstacle : obstacles) {
        EXPECT_FALSE(bounds.translated(move).intersects(obstacle));
    }
}

TEST(ViewPlacement, FullPageLeavesPlacementToCaller)
{
    const QRectF bounds(90, 90, 20, 20);
    const QRectF paper(0, 0, 210, 297);
    EXPECT_FALSE(findAlignedViewPlacement(bounds, {0, 1}, paper, {paper}));
    EXPECT_FALSE(findAlignedViewPlacement(bounds, {1, 0}, paper, {paper}));
}

TEST(ViewPlacement, CanEnterPageFromOutside)
{
    const QRectF bounds(-30, 90, 20, 20);
    const QRectF paper(0, 0, 210, 297);
    const auto placement = findAlignedViewPlacement(bounds, {1, 0}, paper, {});
    ASSERT_TRUE(placement);
    const QPointF move = *placement;
    EXPECT_TRUE(paper.contains(bounds.translated(move)));
}

TEST(ViewPlacement, PreservesFreePositionAndExactFit)
{
    const QRectF paper(0, 0, 210, 297);
    EXPECT_EQ(findAlignedViewPlacement(paper, {1, 0}, paper, {}), QPointF());
    EXPECT_EQ(findAlignedViewPlacement({20, 20, 30, 40}, {0, 1}, paper, {}), QPointF());
}
