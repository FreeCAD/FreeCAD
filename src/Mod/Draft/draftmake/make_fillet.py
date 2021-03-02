# ***************************************************************************
# *   Copyright (c) 2019 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
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
"""Provides functions to create Fillet objects between two lines.

This creates a `Part::Part2DObjectPython`, and then assigns the Proxy class
`Fillet`, and the `ViewProviderFillet` for the view provider.
"""
## @package make_fillet
# \ingroup draftmake
# \brief Provides functions to create Fillet objects between two lines.

import lazy_loader.lazy_loader as lz

import FreeCAD as App
import draftutils.utils as utils
import draftutils.gui_utils as gui_utils
import draftobjects.fillet as fillet

from draftutils.messages import _msg, _err
from draftutils.translate import translate

if App.GuiUp:
    import draftviewproviders.view_fillet as view_fillet

# Delay import of module until first use because it is heavy
Part = lz.LazyLoader("Part", globals(), "Part")
DraftGeomUtils = lz.LazyLoader("DraftGeomUtils", globals(), "DraftGeomUtils")

## \addtogroup draftmake
# @{


def _print_obj_length(obj, edge, num=1):
    if hasattr(obj, "Label"):
        name = obj.Label
    else:
        name = num

    _msg("({0}): {1}; {2} {3}".format(num, name,
                                      translate("draft","length:"), edge.Length))


def _extract_edges(objs):
    """Extract the edges from the list of objects, Draft lines or Part.Edges.

    Parameters
    ----------
    objs: list of Draft Lines or Part.Edges
        The list of edges from which to create the fillet.
    """
    o1, o2 = objs
    if hasattr(o1, "PropertiesList"):
        if "Proxy" in o1.PropertiesList:
            if hasattr(o1.Proxy, "Type"):
                if o1.Proxy.Type in ("Wire", "Fillet"):
                    e1 = o1.Shape.Edges[0]
        elif "Shape" in o1.PropertiesList:
            if o1.Shape.ShapeType in ("Wire", "Edge"):
                e1 = o1.Shape
    elif hasattr(o1, "ShapeType"):
        if o1.ShapeType in "Edge":
            e1 = o1

    _print_obj_length(o1, e1, num=1)

    if hasattr(o2, "PropertiesList"):
        if "Proxy" in o2.PropertiesList:
            if hasattr(o2.Proxy, "Type"):
                if o2.Proxy.Type in ("Wire", "Fillet"):
                    e2 = o2.Shape.Edges[0]
        elif "Shape" in o2.PropertiesList:
            if o2.Shape.ShapeType in ("Wire", "Edge"):
                e2 = o2.Shape
    elif hasattr(o2, "ShapeType"):
        if o2.ShapeType in "Edge":
            e2 = o2

    _print_obj_length(o2, e2, num=2)

    return e1, e2


def make_fillet(objs, radius=100, chamfer=False, delete=False):
    """Create a fillet between two lines or Part.Edges.

    Parameters
    ----------
    objs: list
        List of two objects of type wire, or edges.

    radius: float, optional
        It defaults to 100. The curvature of the fillet.

    chamfer: bool, optional
        It defaults to `False`. If it is `True` it no longer produces
        a rounded fillet but a chamfer (straight edge)
        with the value of the `radius`.

    delete: bool, optional
        It defaults to `False`. If it is `True` it will delete
        the pair of objects that are used to create the fillet.
        Otherwise, the original objects will still be there.

    Returns
    -------
    Part::Part2DObjectPython
        The object of Proxy type `'Fillet'`.
        It returns `None` if it fails producing the object.
    """
    _name = "make_fillet"
    utils.print_header(_name, "Fillet")

    if len(objs) != 2:
        _err(translate("draft","Two elements are needed."))
        return None

    e1, e2 = _extract_edges(objs)

    edges = DraftGeomUtils.fillet([e1, e2], radius, chamfer)
    if len(edges) < 3:
        _err(translate("draft","Radius is too large") + ", r={}".format(radius))
        return None

    lengths = [edges[0].Length, edges[1].Length, edges[2].Length]
    _msg(translate("draft","Segment") + " 1, " + translate("draft","length:") + " {}".format(lengths[0]))
    _msg(translate("draft","Segment") + " 2, " + translate("draft","length:") + " {}".format(lengths[1]))
    _msg(translate("draft","Segment") + " 3, " + translate("draft","length:") + " {}".format(lengths[2]))

    try:
        wire = Part.Wire(edges)
    except Part.OCCError:
        return None

    _doc = App.activeDocument()
    obj = _doc.addObject("Part::Part2DObjectPython",
                         "Fillet")
    fillet.Fillet(obj)
    obj.Shape = wire
    obj.Length = wire.Length
    obj.Start = wire.Vertexes[0].Point
    obj.End = wire.Vertexes[-1].Point
    obj.FilletRadius = radius

    if delete:
        _doc.removeObject(objs[0].Name)
        _doc.removeObject(objs[1].Name)
        _msg(translate("draft","Removed original objects."))

    if App.GuiUp:
        view_fillet.ViewProviderFillet(obj.ViewObject)
        gui_utils.format_object(obj)
        gui_utils.select(obj)
        gui_utils.autogroup(obj)

    return obj

## @}
