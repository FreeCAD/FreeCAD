// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 David Kaufman
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

#include <TopTools_HSequenceOfShape.hxx>

#include <Mod/Part/PartGlobal.h>

namespace Part
{

// Wrapper of ShapeAnalysis_FreeBounds::ConnectEdgesToWires to work around
// OCCT bug 1330: filter out any edges with INTERNAL or EXTERNAL
// orientation. They do not contribute to the boundary, and until 1330 is
// fixed they are handled improperly.
PartExport void Fix_ShapeAnalysis_FreeBounds_ConnectEdgesToWires(
    Handle(TopTools_HSequenceOfShape) & edges,
    double toler,
    bool shared,
    Handle(TopTools_HSequenceOfShape) & wires
);

// Wrapper of ShapeAnalysis_FreeBounds::ConnectWiresToWires to work around
// OCCT bug 1330: filter out any edges with INTERNAL or EXTERNAL orientation
// from each input wire. They do not contribute to the boundary, and until
// 1330 is fixed they are handled improperly.
PartExport void Fix_ShapeAnalysis_FreeBounds_ConnectWiresToWires(
    Handle(TopTools_HSequenceOfShape) & iwires,
    double toler,
    bool shared,
    Handle(TopTools_HSequenceOfShape) & owires
);

}  // namespace Part
