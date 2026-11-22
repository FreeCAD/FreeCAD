// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 FreeCAD contributors
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#pragma once

#include <vector>

#include <FCGlobal.h>

class SbVec2s;

namespace Gui
{
class View3DInventorViewer;

/**
 * @brief Apply box selection for the current 3D view polygon.
 *
 * A two-point polygon is expanded into a window/crossing rectangle, while
 * longer polygons are interpreted directly using the default center-based test.
 *
 * @param[in] viewer Active 3D view.
 * @param[in] picked Selection polygon in viewport coordinates.
 * @param[in] selectElement When true, select subelements instead of whole objects.
 * @param[in] additive When true, merge matches into the existing selection.
 */
GuiExport void applyBoxSelection(
    View3DInventorViewer* viewer,
    const std::vector<SbVec2s>& picked,
    bool selectElement = false,
    bool additive = false
);

}  // namespace Gui
