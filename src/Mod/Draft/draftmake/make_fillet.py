# ***************************************************************************
# *   Copyright (c) 2019 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
# *   Copyright (c) 2024 FreeCAD Project Association                        *
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

def _preprocess(objs, radius, chamfer):
    """Check the inputs and return the edges for the fillet and the objects to be deleted."""
    edges = []
    del_objs = []
    if objs[0].isDerivedFrom("Gui::SelectionObject"):
        for sel in objs:
            for sub in sel.SubElementNames if sel.SubElementNames else [""]:
                shape = sel.Object.getSubObject(sub)
                if shape.ShapeType == "Edge":
                    edges.append(shape)
                    if sel.Object not in del_objs:
                        del_objs.append(sel.Object)
    else:
        for obj in objs:
            if hasattr(obj, "Shape"):
                shape = obj.Shape
                del_objs.append(obj)
            else:
                shape = obj
            if hasattr(shape, "ShapeType") and shape.ShapeType in ("Wire", "Edge"):
                edges.append(shape.Edges[0])

    if len(edges) != 2:
        _err(translate("draft", "Two edges are needed."))
        return None, None

    edges = DraftGeomUtils.fillet(edges, radius, chamfer)
    if len(edges) < 3:
        _err(translate("draft", "Edges are not connected or radius is too large."))
        return None, None

    return edges, del_objs


def make_fillet(objs, radius=100, chamfer=False, delete=False):
    """Create a fillet between two edges.

    Parameters
    ----------
    objs: list
        A list of two objects or shapes of type wire (1st edge is used) or edge,
        or a 2 edge selection set:`FreeCADGui.Selection.getSelectionEx("", 0)`.

    radius: float, optional
        It defaults to 100. The curvature of the fillet.

    chamfer: bool, optional
        It defaults to `False`. If it is `True` it no longer produces
        a rounded fillet but a chamfer (straight edge)
        with the value of the `radius`.

    delete: bool, optional
        It defaults to `False`. If `True` the source objects are deleted.
        Ignored for shapes.

    Returns
    -------
    Part::Part2DObjectPython
        The object of Proxy type `'Fillet'`.
        It returns `None` if it fails producing the object.
    """

    edges, del_objs = _preprocess(objs, radius, chamfer)
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
        for del_obj in del_objs:
            doc.removeObject(del_obj.Name)

    if App.GuiUp:
        view_fillet.ViewProviderFillet(obj.ViewObject)
        gui_utils.format_object(obj)
        gui_utils.select(obj)

    return obj

## @}
