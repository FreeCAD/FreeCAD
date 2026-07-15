// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <array>

#include <App/ColorModel.h>
#include <Gui/ColorScaleOverlay.h>


namespace
{

TEST(ColorScaleOverlayTest, buildsGradientSnapshot)
{
    Gui::ColorScaleOverlay overlay;
    App::ColorGradientProfile profile;
    profile.fMin = -0.5F;
    profile.fMax = 0.5F;
    profile.ctColors = 5;
    profile.tStyle = App::ColorBarStyle::ZERO_BASED;
    overlay.setGradientProfile(profile, 1);

    const auto snapshot = overlay.snapshot();

    EXPECT_EQ(snapshot.mode, Gui::ColorScaleMode::Gradient);
    EXPECT_FLOAT_EQ(snapshot.minimum, -0.5F);
    EXPECT_FLOAT_EQ(snapshot.maximum, 0.5F);
    EXPECT_EQ(snapshot.gradientStops.size(), 5U);
    ASSERT_EQ(snapshot.ticks.size(), 5U);
    constexpr std::array expectedValues {0.5F, 0.25F, 0.0F, -0.25F, -0.5F};
    for (std::size_t index = 0; index < expectedValues.size(); ++index) {
        EXPECT_FLOAT_EQ(snapshot.ticks[index].value, expectedValues[index]);
    }
}

TEST(ColorScaleOverlayTest, usesScientificNotationWhenFixedLabelsCollide)
{
    Gui::ColorScaleOverlay overlay;
    App::ColorGradientProfile profile;
    profile.fMin = 0.001F;
    profile.fMax = 0.005F;
    profile.ctColors = 5;
    overlay.setGradientProfile(profile, 1);

    const auto snapshot = overlay.snapshot();

    ASSERT_EQ(snapshot.ticks.size(), 5U);
    EXPECT_EQ(snapshot.ticks.front().text, "+5.0e-03");
    EXPECT_EQ(snapshot.ticks.back().text, "+1.0e-03");
}

TEST(ColorScaleOverlayTest, buildsLegendSnapshotAndUsesLegendColors)
{
    App::ColorLegend legend;
    legend.setValue(0, 1.0F);
    legend.setValue(1, 2.0F);
    legend.setValue(2, 4.0F);
    legend.setValue(3, 8.0F);
    legend.setText(0, "Low");
    legend.setText(1, "Medium");
    legend.setText(2, "High");

    Gui::ColorScaleOverlay overlay;
    overlay.setLegend(legend, 2);
    overlay.setMode(Gui::ColorScaleMode::Legend);

    const auto snapshot = overlay.snapshot();

    EXPECT_EQ(snapshot.mode, Gui::ColorScaleMode::Legend);
    EXPECT_FLOAT_EQ(snapshot.minimum, 1.0F);
    EXPECT_FLOAT_EQ(snapshot.maximum, 8.0F);
    EXPECT_TRUE(snapshot.gradientStops.empty());
    ASSERT_EQ(snapshot.intervals.size(), 3U);
    EXPECT_EQ(snapshot.intervals.front().label, "High");
    EXPECT_FLOAT_EQ(snapshot.intervals.front().minimum, 4.0F);
    EXPECT_FLOAT_EQ(snapshot.intervals.front().maximum, 8.0F);
    EXPECT_EQ(snapshot.intervals.back().label, "Low");
    ASSERT_EQ(snapshot.ticks.size(), 4U);
    EXPECT_FLOAT_EQ(snapshot.ticks.front().value, 8.0F);
    EXPECT_EQ(snapshot.ticks.front().text, "+8.00");
    EXPECT_FLOAT_EQ(snapshot.ticks.back().value, 1.0F);
    EXPECT_EQ(snapshot.ticks.back().text, "+1.00");

    const auto color = overlay.getColor(3.0F);
    EXPECT_FLOAT_EQ(color.r, 0.0F);
    EXPECT_FLOAT_EQ(color.g, 1.0F);
    EXPECT_FLOAT_EQ(color.b, 0.0F);
}

}  // namespace
