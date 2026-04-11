// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 The FreeCAD Project Association AISBL               *
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

#pragma once

#include <Gui/StyleParameters/ParameterManager.h>

namespace SketcherGui::StyleParameters
{
// rubberband selection colors
DEFINE_STYLE_PARAMETER(
    SketcherRubberbandTouchSelectionColor,
    Base::Color(0.0F, 1.0F, 0.0F, 1.0F)
);  // green for touch selection (right to left)
DEFINE_STYLE_PARAMETER(
    SketcherRubberbandWindowSelectionColor,
    Base::Color(0.0F, 0.4F, 1.0F, 1.0F)
);  // blue for window selection (left to right)
}  // namespace SketcherGui::StyleParameters
