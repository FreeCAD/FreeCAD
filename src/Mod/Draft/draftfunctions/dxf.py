# -*- coding: utf8 -*-
# ***************************************************************************
# *   Copyright (c) 2009 Yorik van Havre <yorik@uncreated.net>              *
# *   Copyright (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""Provides functions to return the DXF representation of various shapes.
"""
## @package dxf
# \ingroup draftfunctions
# \brief Provides functions to return the DXF representation of shapes.

import lazy_loader.lazy_loader as lz

import FreeCAD as App
import DraftVecUtils
import WorkingPlane
import draftutils.utils as utils

from draftutils.messages import _wrn

# Delay import of module until first use because it is heavy
Part = lz.LazyLoader("Part", globals(), "Part")
DraftGeomUtils = lz.LazyLoader("DraftGeomUtils", globals(), "DraftGeomUtils")
TechDraw = lz.LazyLoader("TechDraw", globals(), "TechDraw")


## \addtogroup draftfunctions
# @{

def _get_proj(vec, plane=None):
    """Get a projection of the vector in the plane's u and v directions.

    TODO: check if the same function for SVG and DXF projection can be used
    so that this function is not just duplicated code.
    This function may also be present elsewhere, like `WorkingPlane`
    or `DraftGeomUtils`, so we should avoid code duplication.
    """
    if not plane:
        return vec

    nx = DraftVecUtils.project(vec, plane.u)
    ny = DraftVecUtils.project(vec, plane.v)
    return App.Vector(nx.Length, ny.Length, 0)


def get_dxf(obj, direction=None):
    """Return a DXF entity from the given object.

    If direction is given, the object is projected in 2D.
    """
    plane = None
    result = ""
    if obj.isDerivedFrom("TechDraw::DrawView"):
        if obj.Source.isDerivedFrom("App::DocumentObjectGroup"):
            for o in obj.Source.Group:
                result += get_dxf(o, obj.Direction)
        else:
            result += get_dxf(obj.Source, obj.Direction)
        return result

    if direction and isinstance(direction, App.Vector):
        if direction != App.Vector(0, 0, 0):
            plane = WorkingPlane.Plane()
            plane.alignToPointAndAxis(App.Vector(0, 0, 0), direction)

    if utils.get_type(obj) in ("Dimension", "LinearDimension"):
        p1 = _get_proj(obj.Start, plane=plane)
        p2 = _get_proj(obj.End, plane=plane)
        p3 = _get_proj(obj.Dimline, plane=plane)
        result += "0\nDIMENSION\n8\n0\n62\n0\n3\nStandard\n70\n1\n"
        result += "10\n"+str(p3.x)+"\n20\n"+str(p3.y)+"\n30\n"+str(p3.z)+"\n"
        result += "13\n"+str(p1.x)+"\n23\n"+str(p1.y)+"\n33\n"+str(p1.z)+"\n"
        result += "14\n"+str(p2.x)+"\n24\n"+str(p2.y)+"\n34\n"+str(p2.z)+"\n"

    elif utils.get_type(obj) == "Annotation":
        # Only for App::Annotation
        p = _get_proj(obj.Position, plane=plane)
        count = 0
        for t in obj.LabeLtext:
            result += "0\nTEXT\n8\n0\n62\n0\n"
            result += "10\n"
            result += str(p.x) + "\n20\n"
            result += str(p.y + count) + "\n30\n"
            result += str(p.z) + "\n"
            result += "40\n1\n"
            result += "1\n" + str(t) + "\n"
            result += "7\nSTANDARD\n"
            count += 1

    elif hasattr(obj, 'Shape'):
        # TODO do this the Draft way, for ex. using polylines and rectangles
        if not direction:
            direction = App.Vector(0, 0, -1)

        if DraftVecUtils.isNull(direction):
            direction = App.Vector(0, 0, -1)

        try:
            d = TechDraw.projectToDXF(obj.Shape, direction)
        except Exception:
            # TODO: trap only specific exception.
            # Impossible to generate DXF from Shape? Which exception is throw?
            _wrn("get_dxf: "
                 "unable to project '{}' to {}".format(obj.Label, direction))
        else:
            result += d
    else:
        _wrn("get_dxf: unsupported object, '{}'".format(obj.Label))

    return result


def getDXF(obj,
           direction=None):
    """Return DXF string of the object. DEPRECATED. Use 'get_dxf'."""
    utils.use_instead("get_dxf")
    return get_dxf(obj,
                   direction=direction)

## @}
