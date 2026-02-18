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

#ifndef BASE_OKLCH_H
#define BASE_OKLCH_H

#include <FCGlobal.h>

#include "Color.h"

namespace Base
{

struct OkLch
{
    float lightness;  // [0, 1]
    float chroma;     // [0, ~0.37]
    float hue;        // [0, 360) degrees
};

/**
 * @brief Converts an sRGB color to OKLCH.
 *
 * Pipeline: sRGB -> linear RGB -> OKLab -> OKLCH.
 * Alpha is not included in the OKLCH representation.
 */
BaseExport OkLch toOkLch(const Color& color);

/**
 * @brief Converts an OKLCH color back to sRGB.
 *
 * Pipeline: OKLCH -> OKLab -> linear RGB -> sRGB.
 * Applies gamut mapping by reducing chroma until the result fits sRGB [0, 1].
 */
BaseExport Color fromOkLch(const OkLch& oklch, float alpha = 1.0f);

}  // namespace Base

#endif  // BASE_OKLCH_H
