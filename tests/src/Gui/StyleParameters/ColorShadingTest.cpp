#include <gtest/gtest.h>
#include <cmath>

#include <Base/OkLch.h>
#include <Gui/StyleParameters/ColorShading.h>

using namespace Gui::StyleParameters;

// NOLINTBEGIN

class ColorShadingTest: public ::testing::Test
{
protected:
    ColorShading::Parameters defaultParameters;

    Base::OkLch makeAnchor(float lightness, float chroma = 0.1F, float hue = 240.0F)
    {
        return Base::OkLch {
            .lightness = lightness,
            .chroma = chroma,
            .hue = hue,
        };
    }
};

TEST_F(ColorShadingTest, AnchorReturnsUnchanged)
{
    // Position 0.5 should return the anchor unchanged
    auto anchor = makeAnchor(0.55F);
    auto result = ColorShading::computeShade(0.5F, anchor, defaultParameters);
    EXPECT_FLOAT_EQ(result.lightness, anchor.lightness);
    EXPECT_FLOAT_EQ(result.chroma, anchor.chroma);
    EXPECT_FLOAT_EQ(result.hue, anchor.hue);
}

TEST_F(ColorShadingTest, MonotonicDecrease)
{
    // Lightness must strictly decrease as position increases from 0 to 1
    auto anchor = makeAnchor(0.55F);
    float previous = ColorShading::computeShade(0.0F, anchor, defaultParameters).lightness;

    for (int index = 1; index <= 10; ++index) {
        float position = static_cast<float>(index) / 10.0F;
        // Skip the anchor position itself
        if (std::abs(position - 0.5F) < 1e-3F) {
            continue;
        }
        float current = ColorShading::computeShade(position, anchor, defaultParameters).lightness;
        EXPECT_LT(current, previous) << "position=" << position;
        previous = current;
    }
}

TEST_F(ColorShadingTest, SymmetricRangeIsLinear)
{
    // When anchor is exactly centered in the range, the exponent should be ~1 (linear)
    // This means equal steps in position produce equal steps in lightness
    auto anchor = makeAnchor(0.57F);  // centered in default [0.17, 0.97] with range 0.8

    float light = ColorShading::computeShade(0.25F, anchor, defaultParameters).lightness;
    float dark = ColorShading::computeShade(0.75F, anchor, defaultParameters).lightness;

    // Symmetric positions should produce symmetric lightness offsets from anchor
    float lightDelta = light - anchor.lightness;
    float darkDelta = anchor.lightness - dark;
    EXPECT_NEAR(lightDelta, darkDelta, 0.01F);
}

TEST_F(ColorShadingTest, ClampedRangeRespectsMinMax)
{
    // Results should stay within [minLightness, maxLightness]
    ColorShading::Parameters parameters = {
        .range = 0.6F,
        .minLightness = 0.2F,
        .maxLightness = 0.8F,
    };

    auto anchor = makeAnchor(0.5F);
    for (int index = 0; index <= 10; ++index) {
        float position = static_cast<float>(index) / 10.0F;
        if (std::abs(position - 0.5F) < 1e-3F) {
            continue;
        }
        float result = ColorShading::computeShade(position, anchor, parameters).lightness;
        EXPECT_GE(result, parameters.minLightness - 1e-6F) << "position=" << position;
        EXPECT_LE(result, parameters.maxLightness + 1e-6F) << "position=" << position;
    }
}

TEST_F(ColorShadingTest, NarrowRangeProducesLessVariation)
{
    auto anchor = makeAnchor(0.55F);

    ColorShading::Parameters narrowParameters = {
        .range = 0.2F,
        .minLightness = 0.17F,
        .maxLightness = 0.97F,
    };

    float wideLight = ColorShading::computeShade(0.1F, anchor, defaultParameters).lightness;
    float wideDark = ColorShading::computeShade(0.9F, anchor, defaultParameters).lightness;
    float wideSpan = wideLight - wideDark;

    float narrowLight = ColorShading::computeShade(0.1F, anchor, narrowParameters).lightness;
    float narrowDark = ColorShading::computeShade(0.9F, anchor, narrowParameters).lightness;
    float narrowSpan = narrowLight - narrowDark;

    EXPECT_LT(narrowSpan, wideSpan);
}

TEST_F(ColorShadingTest, ExtremeLightnessIsHandled)
{
    // Anchor near 0 or 1 should not produce NaN or infinity
    for (float anchorLightness : {0.0F, 0.01F, 0.99F, 1.0F}) {
        auto anchor = makeAnchor(anchorLightness);
        for (int index = 0; index <= 10; ++index) {
            float position = static_cast<float>(index) / 10.0F;
            if (std::abs(position - 0.5F) < 1e-3F) {
                continue;
            }
            auto result = ColorShading::computeShade(position, anchor, defaultParameters);
            EXPECT_FALSE(std::isnan(result.lightness))
                << "anchor=" << anchorLightness << " position=" << position;
            EXPECT_FALSE(std::isinf(result.lightness))
                << "anchor=" << anchorLightness << " position=" << position;
            EXPECT_FALSE(std::isnan(result.chroma))
                << "anchor=" << anchorLightness << " position=" << position;
            EXPECT_FALSE(std::isinf(result.chroma))
                << "anchor=" << anchorLightness << " position=" << position;
        }
    }
}

TEST_F(ColorShadingTest, ChromaScalesWithGamutBoundary)
{
    // Chroma should scale by sqrt(min(targetL/anchorL, (1-targetL)/(1-anchorL)))
    // so it shrinks toward both black and white, with sqrt softening the effect
    auto anchor = makeAnchor(0.6F, 0.15F, 260.0F);

    for (float position : {0.1F, 0.3F, 0.7F, 0.9F}) {
        auto result = ColorShading::computeShade(position, anchor, defaultParameters);
        float darkScale = result.lightness / anchor.lightness;
        float lightScale = (1.0F - result.lightness) / (1.0F - anchor.lightness);
        float expectedChroma = anchor.chroma * std::pow(std::min(darkScale, lightScale), 0.5F);
        EXPECT_NEAR(result.chroma, expectedChroma, 1e-6F) << "position=" << position;
    }
}

TEST_F(ColorShadingTest, LighterShadesReduceChroma)
{
    // For lighter shades, chroma should decrease (not increase) to avoid tinted near-whites
    auto anchor = makeAnchor(0.74F, 0.014F, 250.0F);  // similar to #ADB5BD

    for (float position : {0.0F, 0.1F, 0.2F, 0.3F}) {
        auto result = ColorShading::computeShade(position, anchor, defaultParameters);
        EXPECT_GT(result.lightness, anchor.lightness) << "position=" << position;
        EXPECT_LT(result.chroma, anchor.chroma) << "position=" << position;
    }
}

TEST_F(ColorShadingTest, BlackAnchorProducesZeroChroma)
{
    // Anchor with lightness=0 should not cause division by zero; chroma should be 0
    auto anchor = Base::OkLch {
        .lightness = 0.0F,
        .chroma = 0.1F,
        .hue = 200.0F,
    };

    for (float position : {0.0F, 0.3F, 0.7F, 1.0F}) {
        auto result = ColorShading::computeShade(position, anchor, defaultParameters);
        EXPECT_FLOAT_EQ(result.chroma, 0.0F) << "position=" << position;
        EXPECT_FALSE(std::isnan(result.chroma)) << "position=" << position;
    }
}

TEST_F(ColorShadingTest, WhiteAnchorProducesZeroChroma)
{
    // Anchor with lightness=1 should not cause division by zero; chroma should be 0
    auto anchor = Base::OkLch {
        .lightness = 1.0F,
        .chroma = 0.1F,
        .hue = 200.0F,
    };

    for (float position : {0.0F, 0.3F, 0.7F, 1.0F}) {
        auto result = ColorShading::computeShade(position, anchor, defaultParameters);
        EXPECT_FLOAT_EQ(result.chroma, 0.0F) << "position=" << position;
        EXPECT_FALSE(std::isnan(result.chroma)) << "position=" << position;
    }
}

TEST_F(ColorShadingTest, GrayStaysNeutral)
{
    // Near-zero chroma should stay near-zero at all positions
    auto anchor = makeAnchor(0.5F, 0.001F, 0.0F);

    for (int index = 0; index <= 10; ++index) {
        float position = static_cast<float>(index) / 10.0F;
        if (std::abs(position - 0.5F) < 1e-3F) {
            continue;
        }
        auto result = ColorShading::computeShade(position, anchor, defaultParameters);
        EXPECT_LT(result.chroma, 0.01F) << "position=" << position;
    }
}

TEST_F(ColorShadingTest, HueIsPreserved)
{
    // Hue should remain unchanged across all positions
    auto anchor = makeAnchor(0.55F, 0.12F, 123.0F);

    for (float position : {0.0F, 0.2F, 0.8F, 1.0F}) {
        auto result = ColorShading::computeShade(position, anchor, defaultParameters);
        EXPECT_FLOAT_EQ(result.hue, anchor.hue) << "position=" << position;
    }
}

TEST_F(ColorShadingTest, PivotOfOneDoesNotProduceNaN)
{
    const Base::OkLch anchor {.lightness = 0.5F, .chroma = 0.1F, .hue = 30.0F};
    const ColorShading::Parameters parameters {.pivot = 1.0F};

    const Base::OkLch shade = ColorShading::computeShade(0.2F, anchor, parameters);

    EXPECT_TRUE(std::isfinite(shade.lightness));
    EXPECT_TRUE(std::isfinite(shade.chroma));
}

TEST_F(ColorShadingTest, PivotOfZeroDoesNotProduceNaN)
{
    const Base::OkLch anchor {.lightness = 0.5F, .chroma = 0.1F, .hue = 30.0F};
    const ColorShading::Parameters parameters {.pivot = 0.0F};

    const Base::OkLch shade = ColorShading::computeShade(0.2F, anchor, parameters);

    EXPECT_TRUE(std::isfinite(shade.lightness));
    EXPECT_TRUE(std::isfinite(shade.chroma));
}

TEST_F(ColorShadingTest, MaxLightnessAboveOneIsClamped)
{
    const Base::OkLch anchor {.lightness = 0.5F, .chroma = 0.1F, .hue = 30.0F};
    const ColorShading::Parameters parameters {.maxLightness = 2.0F};

    const Base::OkLch shade = ColorShading::computeShade(0.0F, anchor, parameters);

    EXPECT_TRUE(std::isfinite(shade.chroma));
    EXPECT_GE(shade.chroma, 0.0F);
    EXPECT_LE(shade.lightness, 1.0F);
}

TEST_F(ColorShadingTest, InvertedLightnessBoundsAreNormalized)
{
    const Base::OkLch anchor {.lightness = 0.5F, .chroma = 0.1F, .hue = 30.0F};
    const ColorShading::Parameters parameters {.minLightness = 0.9F, .maxLightness = 0.1F};

    const Base::OkLch shade = ColorShading::computeShade(0.7F, anchor, parameters);

    EXPECT_TRUE(std::isfinite(shade.lightness));
    EXPECT_GE(shade.lightness, 0.0F);
    EXPECT_LE(shade.lightness, 1.0F);
}

// The four cases above (PivotOfOneDoesNotProduceNaN, PivotOfZeroDoesNotProduceNaN,
// MaxLightnessAboveOneIsClamped, InvertedLightnessBoundsAreNormalized) are the brief's
// verbatim regression cases. They assert true, cheap-to-run properties, but mutation testing
// showed none of them actually reach the clamp their name suggests -- keep them for their
// end-to-end finiteness coverage, but do not read them as proof any specific clamp works.
// The clamps are actually covered as follows:
//   - pivot clamp:        NegativePivotDoesNotProduceNaN
//   - min/max swap:       InvertedBoundsPreserveLightToDarkDirection
//   - chroma std::max(0): not covered by any test. The position clamp added at
//                         ColorShading.cpp:93 (curvePosition = std::clamp(position, 0, 1))
//                         makes darkScale/lightScale a convex combination of two in-range
//                         endpoints, so neither can go negative any more -- the guard is
//                         unreachable defence-in-depth (see the comment at
//                         ColorShading.cpp:106-109).
// (See task-9 report for the mutation evidence behind each mapping.)

TEST_F(ColorShadingTest, NegativePivotDoesNotProduceNaN)
{
    // log(pivot) is NaN for a negative pivot, which propagates through computeExponent.
    // Unlike pivot=0 or pivot=1 (already finite via the pre-existing exponent clamp), this
    // is the case the pivot clamp actually guards against.
    const Base::OkLch anchor {.lightness = 0.5F, .chroma = 0.1F, .hue = 30.0F};
    const ColorShading::Parameters parameters {.pivot = -1.0F};

    const Base::OkLch shade = ColorShading::computeShade(0.2F, anchor, parameters);

    EXPECT_TRUE(std::isfinite(shade.lightness));
    EXPECT_TRUE(std::isfinite(shade.chroma));
}

TEST_F(ColorShadingTest, InvertedBoundsPreserveLightToDarkDirection)
{
    // Swapping inverted min/max doesn't change boundedness (both endpoints are already
    // clamped into [0,1], so any interpolation between them stays in [0,1] either way) but it
    // does change which end of the curve is lighter. Without the swap, position 0 would be
    // the darker shade and position 1 the lighter one, inverting the documented contract.
    const Base::OkLch anchor {.lightness = 0.5F, .chroma = 0.1F, .hue = 30.0F};
    const ColorShading::Parameters parameters {.minLightness = 0.9F, .maxLightness = 0.1F};

    const Base::OkLch lightEnd = ColorShading::computeShade(0.0F, anchor, parameters);
    const Base::OkLch darkEnd = ColorShading::computeShade(1.0F, anchor, parameters);

    EXPECT_GT(lightEnd.lightness, darkEnd.lightness);
}

TEST_F(ColorShadingTest, OutOfRangePositionDoesNotProduceNaN)
{
    // position IS clamped, to [0, 1], at ColorShading.cpp:93 -- so a theme author forgetting
    // the '%' (e.g. "shade(@color, 30)") has position=30 clamped down to curvePosition=1.0
    // before it reaches the exponent/lightness math. With default Parameters and
    // anchor.lightness=0.5, that clamped position drives targetLightness down to exactly
    // minLightness (0.17), not to some large negative number. This test is retained for its
    // end-to-end finiteness coverage, not as proof of the chroma std::max(0) guard below --
    // that guard is unreachable now that position is clamped (see the coverage map above).
    const Base::OkLch anchor {.lightness = 0.5F, .chroma = 0.1F, .hue = 30.0F};
    const ColorShading::Parameters parameters {};

    const Base::OkLch shade = ColorShading::computeShade(30.0F, anchor, parameters);

    EXPECT_TRUE(std::isfinite(shade.lightness));
    EXPECT_TRUE(std::isfinite(shade.chroma));
}

TEST_F(ColorShadingTest, NegativePositionDoesNotProduceNaN)
{
    const Base::OkLch anchor {.lightness = 0.5F, .chroma = 0.1F, .hue = 30.0F};

    const Base::OkLch shade = ColorShading::computeShade(-30.0F, anchor, {});

    EXPECT_TRUE(std::isfinite(shade.lightness));
    EXPECT_TRUE(std::isfinite(shade.chroma));
}

TEST_F(ColorShadingTest, OutOfRangePositionSaturatesAtTheCurveEnds)
{
    const Base::OkLch anchor {.lightness = 0.5F, .chroma = 0.1F, .hue = 30.0F};

    // Below 0 saturates at the lightest end, above 1 at the darkest.
    EXPECT_FLOAT_EQ(
        ColorShading::computeShade(-30.0F, anchor, {}).lightness,
        ColorShading::computeShade(0.0F, anchor, {}).lightness
    );
    EXPECT_FLOAT_EQ(
        ColorShading::computeShade(30.0F, anchor, {}).lightness,
        ColorShading::computeShade(1.0F, anchor, {}).lightness
    );
}

// NOLINTEND
