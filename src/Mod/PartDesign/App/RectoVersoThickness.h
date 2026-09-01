// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 FreeCAD contributors                              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 ***************************************************************************/

#pragma once

#include <vector>

#include <Mod/Part/App/TopoShape.h>
#include <Mod/PartDesign/PartDesignGlobal.h>


namespace PartDesign
{

/** Build a wall centered on the retained shell of a solid.
 *
 * The closing faces are removed by the ordinary skin-thickness operation.
 * Two exact one-sided walls are built at +offset and -offset and regular-fused
 * across their shared source shell.  Consequently, offset is the distance on
 * each side and the total wall thickness is twice its absolute value.
 */
PartDesignExport Part::TopoShape makeRectoVersoThickness(
    const Part::TopoShape& solid,
    const std::vector<Part::TopoShape>& closingFaces,
    double offset,
    double tolerance,
    bool intersection,
    Part::JoinType join
);

}  // namespace PartDesign
