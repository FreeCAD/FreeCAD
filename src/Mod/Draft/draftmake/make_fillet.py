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

from draftutils.messages import _err
from draftutils.translate import translate

if App.GuiUp:
    import draftviewproviders.view_fillet as view_fillet

# Delay import of module until first use because it is heavy
Part = lz.LazyLoader("Part", globals(), "Part")
DraftGeomUtils = lz.LazyLoader("DraftGeomUtils", globals(), "DraftGeomUtils")

## \addtogroup draftmake
# @{

def _extract_edge(obj):
    """Extract the 1st edge from an object or shape."""
    if hasattr(obj, "Shape"):
        obj = obj.Shape
    if hasattr(obj, "ShapeType") and obj.ShapeType in ("Wire", "Edge"):
        return obj.Edges[0]
    return None


def _preprocess(objs, radius, chamfer):
    """Check the inputs and return the edges for the fillet."""
    if len(objs) != 2:
        _err(translate("draft", "Two objects are needed."))
        return None

    edge1 = _extract_edge(objs[0])
    edge2 = _extract_edge(objs[1])

    if edge1 is None or edge2 is None:
        _err(translate("draft", "One object is not valid."))
        return None

    edges = DraftGeomUtils.fillet([edge1, edge2], radius, chamfer)
    if len(edges) < 3:
        _err(translate("draft", "Edges are not connected or radius is too large."))
        return None

    return edges


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

    edges = _preprocess(objs, radius, chamfer)
    if edges is None:
        return

    try:
        wire = Part.Wire(edges)
    except Part.OCCError:
        return None

    doc = App.activeDocument()
    obj = doc.addObject("Part::Part2DObjectPython", "Fillet")
    fillet.Fillet(obj)
    obj.Shape = wire
    obj.Length = wire.Length
    obj.Start = wire.Vertexes[0].Point
    obj.End = wire.Vertexes[-1].Point
    obj.FilletRadius = radius

    if delete:
        doc.removeObject(objs[0].Name)
        doc.removeObject(objs[1].Name)

    if App.GuiUp:
        view_fillet.ViewProviderFillet(obj.ViewObject)
        gui_utils.format_object(obj)
        gui_utils.select(obj)

    return obj

## @}
