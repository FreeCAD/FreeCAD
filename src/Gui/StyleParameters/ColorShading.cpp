// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Kacper Donat <kacper@kadet.net>                     *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include "ColorShading.h"

#include <algorithm>
#include <cmath>
#include <utility>

// NOLINTBEGIN(readability-magic-numbers)

namespace Gui::StyleParameters::ColorShading
{

namespace
{

constexpr float anchorTolerance = 1e-3F;
constexpr float minExponent = 0.1F;
constexpr float maxExponent = 10.0F;
constexpr float minRangeDelta = 1e-6F;
constexpr float minPivot = 0.01F;
constexpr float maxPivot = 0.99F;

Parameters normalized(const Parameters& parameters)
{
    Parameters result = parameters;

    result.minLightness = std::clamp(parameters.minLightness, 0.0F, 1.0F);
    result.maxLightness = std::clamp(parameters.maxLightness, 0.0F, 1.0F);

    if (result.minLightness > result.maxLightness) {
        std::swap(result.minLightness, result.maxLightness);
    }

    result.range = std::max(parameters.range, 0.0F);
    result.pivot = std::clamp(parameters.pivot, minPivot, maxPivot);
    result.chromaExponent = std::clamp(parameters.chromaExponent, 0.0F, maxExponent);

    return result;
}

std::pair<float, float> computeLightnessRange(float anchorLightness, const Parameters& parameters)
{
    float halfRange = parameters.range / 2.0F;
    float high = std::min(parameters.maxLightness, anchorLightness + halfRange);
    float low = std::max(parameters.minLightness, anchorLightness - halfRange);
    return {high, low};
}

float computeExponent(float anchorLightness, float high, float low, float pivot)
{
    float totalRange = high - low;
    if (totalRange < minRangeDelta) {
        return 1.0F;
    }
    float ratio = (high - anchorLightness) / totalRange;
    ratio = std::clamp(ratio, 0.01F, 0.99F);
    float exponent = std::log(ratio) / std::log(pivot);
    return std::clamp(exponent, minExponent, maxExponent);
}

float lightnessForPosition(float position, float exponent, float high, float low)
{
    return high - (high - low) * std::pow(position, exponent);
}

}  // namespace

OkLch computeShade(float position, const OkLch& anchor, const Parameters& rawParameters)
{
    const Parameters parameters = normalized(rawParameters);
    const float curvePosition = std::clamp(position, 0.0F, 1.0F);

    if (std::abs(curvePosition - parameters.pivot) < anchorTolerance) {
        return anchor;
    }

    auto [high, low] = computeLightnessRange(anchor.lightness, parameters);
    float exponent = computeExponent(anchor.lightness, high, low, parameters.pivot);
    float targetLightness = lightnessForPosition(curvePosition, exponent, high, low);
    float darkScale = (anchor.lightness > 0.0F) ? targetLightness / anchor.lightness : 0.0F;
    float lightScale = (anchor.lightness < 1.0F)
        ? (1.0F - targetLightness) / (1.0F - anchor.lightness)
        : 0.0F;
    // curvePosition is clamped to [0,1] above, which makes targetLightness a true convex
    // combination of low and high, so neither scale can go negative and this guard is
    // currently unreachable. Kept as defence-in-depth in case an unbounded position ever
    // reaches this line again (e.g. the clamp above is removed or bypassed).
    float chromaScale
        = std::pow(std::max(std::min(darkScale, lightScale), 0.0F), parameters.chromaExponent);

    return OkLch {
        .lightness = std::clamp(targetLightness, 0.0F, 1.0F),
        .chroma = anchor.chroma * chromaScale,
        .hue = anchor.hue,
    };
}

}  // namespace Gui::StyleParameters::ColorShading

// NOLINTEND(readability-magic-numbers)
