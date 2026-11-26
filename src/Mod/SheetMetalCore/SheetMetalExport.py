# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 sliptonic <shopinthewoods@gmail.com>
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

"""File export of the SheetMetalCore module (proof of concept)."""

import FreeCAD
import Part


def export_dxf(flat, path):
    """Write the outline and the bend lines of a flat pattern to a DXF file in the XY plane."""
    import importDXF

    # The flat pattern is in the plane of the root panel. Move the edges back to the XY plane.
    to_xy = flat.Shape.SubShapes[0].Placement.inverse()
    edges = Part.makeCompound(flat.Shape.Edges)
    edges.Placement = to_xy

    doc = flat.Document
    temp = doc.addObject("Part::Feature", "FlatPatternExport")
    temp.Shape = Part.makeCompound(edges.Edges)
    try:
        importDXF.export([temp], path)
    finally:
        doc.removeObject(temp.Name)
