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

#ifndef BASE_COLORSHADING_H
#define BASE_COLORSHADING_H

#include <FCGlobal.h>

#include "OkLch.h"

namespace Base::ColorShading
{

struct Parameters
{
    float range = 0.8F;
    float minLightness = 0.17F;
    float maxLightness = 0.97F;
};

/**
 * @brief Returns target OkLch color for a position on the shade curve.
 *
 * Position 0 = lightest, 0.5 = anchor (unchanged), 1 = darkest.
 * Uses a centered power curve so that the anchor lightness maps to
 * position 0.5, and the output lightness is clamped to [minLightness, maxLightness].
 * Chroma is scaled proportionally to lightness to avoid oversaturation in dark shades.
 */
BaseExport OkLch computeShade(float position, const OkLch& anchor, const Parameters& parameters);

}  // namespace Base::ColorShading

#endif  // BASE_COLORSHADING_H
