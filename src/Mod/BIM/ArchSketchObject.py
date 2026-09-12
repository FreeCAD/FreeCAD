# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2018-25 Paul Lee <paullee0@gmail.com>                   *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

from PySide.QtCore import QT_TRANSLATE_NOOP


def getSketchDefiningEdges(sketch, selected_edges=None, supported_geometry=None):
    import Part
    import Sketcher

    if supported_geometry is None:
        supported_geometry = (Part.LineSegment, Part.Circle, Part.ArcOfCircle, Part.Ellipse)

    selected_edges = {str(edge) for edge in selected_edges or []}
    edges = []

    for index, facade in enumerate(sketch.GeometryFacadeList):
        if selected_edges:
            if str(index) not in selected_edges:
                continue
        elif facade.Construction:
            continue

        if isinstance(facade.Geometry, supported_geometry):
            edges.append(facade.Geometry.toShape())

    external_geometry = list(getattr(sketch, "ExternalGeo", []))
    for index, geometry in enumerate(external_geometry[2:], start=2):
        facade = Sketcher.ExternalGeometryFacade(geometry)
        geo_id = -index - 1

        if not facade.Ref:
            continue

        if facade.testFlag("Missing"):
            continue

        if selected_edges:
            if str(geo_id) not in selected_edges:
                continue
        elif not facade.testFlag("Defining"):
            continue

        if isinstance(facade.Geometry, supported_geometry):
            edges.append(facade.Geometry.toShape())

    return edges


class ArchSketchObject:
    def __init__(self, obj):
        pass


class ArchSketch(ArchSketchObject):
    def __init__(self, obj):
        pass

    def setPropertiesLinkCommon(self, orgFp, linkFp=None, mode=None):
        pass


# from ArchSketchObjectExt import ArchSketch  # Doesn't work
