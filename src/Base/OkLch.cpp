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

// Conversion matrices and algorithm from Bjorn Ottosson's OKLab color space,
// published at https://bottosson.github.io/posts/oklab/ (public domain / MIT license).

#include "OkLch.h"

#include <algorithm>
#include <cmath>
#include <numbers>

// NOLINTBEGIN(readability-magic-numbers)

namespace Base
{

namespace
{

/// sRGB gamma to linear
float srgbToLinear(float channel)
{
    if (channel <= 0.04045f) {
        return channel / 12.92f;
    }
    return std::pow((channel + 0.055f) / 1.055f, 2.4f);
}

/// Linear to sRGB gamma
float linearToSrgb(float channel)
{
    if (channel <= 0.0031308f) {
        return channel * 12.92f;
    }
    return 1.055f * std::pow(channel, 1.0f / 2.4f) - 0.055f;
}

struct OkLab
{
    float lightness;
    float a;
    float b;
};

/// Linear sRGB to OKLab using Ottosson's combined matrices (no XYZ intermediate).
OkLab linearSrgbToOkLab(float red, float green, float blue)
{
    float lCone = 0.4122214708f * red + 0.5363325363f * green + 0.0514459929f * blue;
    float mCone = 0.2119034982f * red + 0.6806995451f * green + 0.1073969566f * blue;
    float sCone = 0.0883024619f * red + 0.2817188376f * green + 0.6299787005f * blue;

    float lRoot = std::cbrt(lCone);
    float mRoot = std::cbrt(mCone);
    float sRoot = std::cbrt(sCone);

    return {
        .lightness = 0.2104542553f * lRoot + 0.7936177850f * mRoot - 0.0040720468f * sRoot,
        .a = 1.9779984951f * lRoot - 2.4285922050f * mRoot + 0.4505937099f * sRoot,
        .b = 0.0259040371f * lRoot + 0.7827717662f * mRoot - 0.8086757660f * sRoot,
    };
}

struct LinearRgb
{
    float red;
    float green;
    float blue;
};

/// OKLab to linear sRGB using Ottosson's inverse matrices.
LinearRgb okLabToLinearSrgb(const OkLab& lab)
{
    float lRoot = lab.lightness + 0.3963377774f * lab.a + 0.2158037573f * lab.b;
    float mRoot = lab.lightness - 0.1055613458f * lab.a - 0.0638541728f * lab.b;
    float sRoot = lab.lightness - 0.0894841775f * lab.a - 1.2914855480f * lab.b;

    float lCone = lRoot * lRoot * lRoot;
    float mCone = mRoot * mRoot * mRoot;
    float sCone = sRoot * sRoot * sRoot;

    return {
        .red = +4.0767416621f * lCone - 3.3077115913f * mCone + 0.2309699292f * sCone,
        .green = -1.2684380046f * lCone + 2.6097574011f * mCone - 0.3413193965f * sCone,
        .blue = -0.0041960863f * lCone - 0.7034186147f * mCone + 1.7076147010f * sCone,
    };
}


constexpr float pi = std::numbers::pi_v<float>;

constexpr float degreesPerRadian = 180.0f / pi;
constexpr float radiansPerDegree = pi / 180.0f;
constexpr float fullCircleDegrees = 360.0f;

OkLch okLabToOkLch(const OkLab& lab)
{
    float chroma = std::sqrt(lab.a * lab.a + lab.b * lab.b);
    float hueDegrees = std::atan2(lab.b, lab.a) * degreesPerRadian;
    if (hueDegrees < 0.0f) {
        hueDegrees += fullCircleDegrees;
    }

    return {
        .lightness = lab.lightness,
        .chroma = chroma,
        .hue = hueDegrees,
    };
}

OkLab okLchToOkLab(const OkLch& lch)
{
    float hueRadians = lch.hue * radiansPerDegree;

    return {
        .lightness = lch.lightness,
        .a = lch.chroma * std::cos(hueRadians),
        .b = lch.chroma * std::sin(hueRadians),
    };
}

bool isInGamut(const LinearRgb& rgb)
{
    constexpr float epsilon = 1e-6f;
    return rgb.red >= -epsilon && rgb.red <= 1.0f + epsilon && rgb.green >= -epsilon
        && rgb.green <= 1.0f + epsilon && rgb.blue >= -epsilon && rgb.blue <= 1.0f + epsilon;
}

}  // namespace

OkLch toOkLch(const Color& color)
{
    float linearRed = srgbToLinear(color.r);
    float linearGreen = srgbToLinear(color.g);
    float linearBlue = srgbToLinear(color.b);

    OkLab lab = linearSrgbToOkLab(linearRed, linearGreen, linearBlue);
    return okLabToOkLch(lab);
}

Color fromOkLch(const OkLch& oklch, float alpha)
{
    // Binary search: reduce chroma until the result fits sRGB gamut.
    constexpr int maxIterations = 20;
    constexpr float chromaTolerance = 1e-5f;

    float lowChroma = 0.0f;
    float highChroma = oklch.chroma;

    OkLch candidate = oklch;

    // First check if the full chroma is already in gamut.
    OkLab lab = okLchToOkLab(candidate);
    LinearRgb linear = okLabToLinearSrgb(lab);

    if (isInGamut(linear)) {
        return Color(
            std::clamp(linearToSrgb(linear.red), 0.0f, 1.0f),
            std::clamp(linearToSrgb(linear.green), 0.0f, 1.0f),
            std::clamp(linearToSrgb(linear.blue), 0.0f, 1.0f),
            alpha
        );
    }

    // Binary search for maximum in-gamut chroma.
    for (int iteration = 0; iteration < maxIterations; ++iteration) {
        float midChroma = (lowChroma + highChroma) * 0.5f;
        candidate.chroma = midChroma;

        lab = okLchToOkLab(candidate);
        linear = okLabToLinearSrgb(lab);

        if (isInGamut(linear)) {
            lowChroma = midChroma;
        }
        else {
            highChroma = midChroma;
        }

        if (highChroma - lowChroma < chromaTolerance) {
            break;
        }
    }

    // Use lowChroma (last known in-gamut value).
    candidate.chroma = lowChroma;
    lab = okLchToOkLab(candidate);
    linear = okLabToLinearSrgb(lab);

    return Color(
        std::clamp(linearToSrgb(linear.red), 0.0f, 1.0f),
        std::clamp(linearToSrgb(linear.green), 0.0f, 1.0f),
        std::clamp(linearToSrgb(linear.blue), 0.0f, 1.0f),
        alpha
    );
}

}  // namespace Base

// NOLINTEND(readability-magic-numbers)
