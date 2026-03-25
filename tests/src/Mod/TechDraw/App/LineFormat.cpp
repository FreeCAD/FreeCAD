#include <gtest/gtest.h>

#include <QColor>
#include <QtCore/Qt>

#include "Mod/TechDraw/App/LineFormat.h"

namespace
{

TechDraw::LineFormat makeLineFormat()
{
    return {Qt::SolidLine, 0.5, Base::Color(0.0F, 0.0F, 0.0F, 1.0F), true};
}

}  // namespace

TEST(TestLineFormat, setQColorKeepsOpaqueColorsOpaque)
{
    auto format = makeLineFormat();

    format.setQColor(QColor(255, 0, 0, 255));

    const Base::Color stored = format.getColor();
    EXPECT_FLOAT_EQ(stored.r, 1.0F);
    EXPECT_FLOAT_EQ(stored.g, 0.0F);
    EXPECT_FLOAT_EQ(stored.b, 0.0F);
    EXPECT_FLOAT_EQ(stored.a, 1.0F);

    const QColor roundTripped = format.getQColor();
    EXPECT_EQ(roundTripped.red(), 255);
    EXPECT_EQ(roundTripped.green(), 0);
    EXPECT_EQ(roundTripped.blue(), 0);
    EXPECT_EQ(roundTripped.alpha(), 255);
}

TEST(TestLineFormat, setQColorPreservesAlphaValue)
{
    auto format = makeLineFormat();

    format.setQColor(QColor(12, 34, 56, 78));

    const QColor roundTripped = format.getQColor();
    EXPECT_EQ(roundTripped.red(), 12);
    EXPECT_EQ(roundTripped.green(), 34);
    EXPECT_EQ(roundTripped.blue(), 56);
    EXPECT_EQ(roundTripped.alpha(), 78);
}
