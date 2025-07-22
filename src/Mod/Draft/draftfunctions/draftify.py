# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2020 FreeCAD Developers                                 *
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
"""Provides functions to transform sketches into Draft objects."""
## @package draftify
# \ingroup draftfunctions
# \brief Provides functions to transform sketches into Draft objects.

## \addtogroup draftfunctions
# @{

import lazy_loader.lazy_loader as lz

import FreeCAD as App
import draftutils.gui_utils as gui_utils
import draftmake.make_block as make_block
import draftmake.make_wire as make_wire
import draftmake.make_circle as make_circle
import draftmake.make_bspline as make_bspline
import draftmake.make_bezcurve as make_bezcurve
import draftmake.make_arc_3points as make_arc_3points

# Delay import of module until first use because it is heavy
Part = lz.LazyLoader("Part", globals(), "Part")
DraftGeomUtils = lz.LazyLoader("DraftGeomUtils", globals(), "DraftGeomUtils")

def draftify(objectslist, makeblock=False, delete=True):
    """draftify(objectslist,[makeblock],[delete])

    Turn each object of the given list (objectslist can also be a single
    object) into a Draft parametric wire.

    TODO: support more objects

    Parameters
    ----------
    objectslist :

    makeblock : bool
        If makeblock is True, multiple objects will be grouped in a block.

    delete : bool
        If delete = False, old objects are not deleted
    """

    if not isinstance(objectslist,list):
        objectslist = [objectslist]
    newobjlist = []
    for obj in objectslist:
        if hasattr(obj,'Shape'):
            for cluster in Part.sortEdges(obj.Shape.Edges):
                w = Part.Wire(cluster)
                nobj = draftify_shape(w)
                if nobj is None:
                    nobj = App.ActiveDocument.addObject("Part::Feature", obj.Name)
                    nobj.Shape = w
                newobjlist.append(nobj)
                gui_utils.format_object(nobj, obj)
                # sketches are always in wireframe mode. In Draft we don't like that!
                if App.GuiUp:
                    nobj.ViewObject.DisplayMode = "Flat Lines"
            if delete:
                App.ActiveDocument.removeObject(obj.Name)

    if makeblock:
        return make_block.make_block(newobjlist)
    else:
        if len(newobjlist) == 1:
            return newobjlist[0]
        return newobjlist

def draftify_shape(shape):

    nobj = None
    if DraftGeomUtils.hasCurves(shape):
        if (len(shape.Edges) == 1):
            edge = shape.Edges[0]
            edge_type = DraftGeomUtils.geomType(edge)
            if edge_type == "Circle":
                if edge.isClosed():
                    nobj = make_circle.make_circle(edge)
                else:
                    first_parameter = edge.FirstParameter
                    last_parameter = edge.LastParameter
                    points = [edge.Curve.value(first_parameter),
                              edge.Curve.value((first_parameter + last_parameter)/2),
                              edge.Curve.value(last_parameter)]
                    nobj = make_arc_3points.make_arc_3points(points)
           # TODO: take into consideration trimmed curves and capture the specific
           # type of BSpline and Bezier that can be converted to a draft object.
           # elif edge_type == "BSplineCurve":
           #     knots = [edge.Curve.value(p) for p in edge.Curve.getKnots()]
           #     nobj = make_bspline.make_bspline(knots, closed=edge.isClosed())
           # elif edge_type == "BezierCurve":
           #     nobj = make_bezcurve.make_bezcurve(edge.Curve.getPoles(),
           #                                        closed=edge.isClosed())
    else:
        nobj = make_wire.make_wire(shape)

    return nobj

## @}
