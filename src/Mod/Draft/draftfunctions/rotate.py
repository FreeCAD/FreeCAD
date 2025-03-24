# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2020 FreeCAD Developers                                 *
# *   Copyright (c) 2024 The FreeCAD Project Association                    *
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
"""Provides functions to rotate shapes around a center and axis."""
## @package rotate
# \ingroup draftfunctions
# \brief Provides functions to rotate shapes around a center and axis.

## \addtogroup draftfunctions
# @{
import math

import FreeCAD as App
import DraftVecUtils
from draftfunctions import join
from draftmake import make_copy
from draftmake import make_line
from draftobjects import layer
from draftutils import groups
from draftutils import gui_utils
from draftutils import params
from draftutils import utils


def rotate(selection, angle, center=App.Vector(0, 0, 0),
           axis=App.Vector(0, 0, 1), copy=False, subelements=False):
    """rotate(selection, angle, [center], [axis], [copy], [subelements])

    Rotates or copies selected objects.

    Parameters
    ----------
    selection: single object / list of objects / selection set
        When dealing with nested objects, use `Gui.Selection.getSelectionEx("", 0)`
        to create the selection set.

    angle: float / integer
        Rotation angle (in degrees).

    center: App.Vector, optional
        Defaults to `App.Vector(0, 0, 0)`.
        Rotation center.

    axis: App.Vector, optional
        Defaults to `App.Vector(0, 0, 1)`.
        Rotation axis.

    copy: bool, optional
        Defaults to `False`.
        If `True` the selected objects are not rotated, but rotated copies are
        created instead.

    subelements: bool, optional
        Defaults to `False`.
        If `True` subelements instead of whole objects are processed.
        Only used if selection is a selection set.

    Returns
    -------
    single object / list with 2 or more objects / empty list
        The objects (or their copies).
    """
    utils.type_check([(angle, (float, int)), (center, App.Vector),
                      (axis, App.Vector), (copy, bool), (subelements, bool)], "rotate")
    if not isinstance(selection, list):
        selection = [selection]
    if not selection:
        return None

    if selection[0].isDerivedFrom("Gui::SelectionObject"):
        if subelements:
            return _rotate_subelements(selection, angle, center, axis, copy)
        else:
            objs, parent_places, sel_info = utils._modifiers_process_selection(selection, copy)
    else:
        objs = utils._modifiers_filter_objects(utils._modifiers_get_group_contents(selection), copy)
        parent_places = None
        sel_info = None

    if not objs:
        return None

    newobjs = []
    newgroups = {}

    if copy:
        for obj in objs:
            if obj.isDerivedFrom("App::DocumentObjectGroup") and obj.Name not in newgroups:
                newgroups[obj.Name] = obj.Document.addObject(obj.TypeId, utils.get_real_name(obj.Name))

    for idx, obj in enumerate(objs):
        newobj = None

        if parent_places is not None:
            parent_place = parent_places[idx]
        elif hasattr(obj, "getGlobalPlacement"):
            parent_place = obj.getGlobalPlacement() * obj.Placement.inverse()
        else:
            parent_place = App.Placement()

        if copy or parent_place.isIdentity():
            real_center = center
            real_axis = axis
        else:
            real_center = parent_place.inverse().multVec(center)
            real_axis = parent_place.inverse().Rotation.multVec(axis)

        if obj.isDerivedFrom("App::DocumentObjectGroup"):
            if copy:
                newobj = newgroups[obj.Name]
            else:
                newobj = obj

        elif hasattr(obj, "Placement"):
            if copy:
                newobj = make_copy.make_copy(obj)
                if not parent_place.isIdentity():
                    newobj.Placement = parent_place * newobj.Placement
            else:
                newobj = obj
            newobj.Placement.rotate(real_center, real_axis, angle, comp=True)

        elif obj.isDerivedFrom("App::Annotation"):
            if copy:
                newobj = make_copy.make_copy(obj)
                if not parent_place.isIdentity():
                    newobj.Position = parent_place.multVec(newobj.Position)
            else:
                newobj = obj
            newobj.Position = rotate_vector_from_center(newobj.Position, angle, axis, center)

        elif utils.get_type(obj) in ("Dimension", "LinearDimension", "AngularDimension"):
            # "Dimension" was the type for linear dimensions <= v0.18.
            if copy:
                newobj = make_copy.make_copy(obj)
                if not parent_place.isIdentity():
                    newobj.Proxy.transform(newobj, parent_place)
            else:
                newobj = obj
            pla = App.Placement()
            pla.rotate(real_center, real_axis, angle, comp=True)
            newobj.Proxy.transform(newobj, pla)

        if newobj is not None:
            newobjs.append(newobj)
            if copy:
                lyr = layer.get_layer(obj)
                if lyr is not None:
                    lyr.Proxy.addObject(lyr, newobj)
                for parent in obj.InList:
                    if parent.isDerivedFrom("App::DocumentObjectGroup") and (parent in objs):
                        newgroups[parent.Name].addObject(newobj)

    if not copy or params.get_param("selectBaseObjects"):
        if sel_info is not None:
            gui_utils.select(sel_info)
        else:
            gui_utils.select(objs)
    else:
        gui_utils.select(newobjs)

    if len(newobjs) == 1:
        return newobjs[0]
    return newobjs


def _rotate_subelements(selection, angle, center, axis, copy):
    data_list, sel_info = utils._modifiers_process_subselection(selection, copy)
    newobjs = []
    if copy:
        for obj, vert_idx, edge_idx, global_place in data_list:
            if edge_idx >= 0:
                newobjs.append(copy_rotated_edge(obj, edge_idx, angle, center, axis, global_place))
        newobjs = join.join_wires(newobjs)
    else:
        for obj, vert_idx, edge_idx, global_place in data_list:
            if vert_idx >= 0:
                rotate_vertex(obj, vert_idx, angle, center, axis, global_place)
            elif edge_idx >= 0:
                rotate_edge(obj, edge_idx, angle, center, axis, global_place)

    if not copy or params.get_param("selectBaseObjects"):
        gui_utils.select(sel_info)
    else:
        gui_utils.select(newobjs)

    if len(newobjs) == 1:
        return newobjs[0]
    return newobjs


def rotate_vector_from_center(vector, angle, axis, center):
    """
    Needed for SubObjects modifiers.
    Implemented by Dion Moult during 0.19 dev cycle (works only with Draft Wire).
    """
    rv = vector.sub(center)
    rv = DraftVecUtils.rotate(rv, math.radians(angle), axis)
    return center.add(rv)


def rotate_vertex(obj, vert_idx, angle, center, axis, global_place=None):
    """
    Needed for SubObjects modifiers.
    Implemented by Dion Moult during 0.19 dev cycle (works only with Draft Wire).
    """
    if global_place is None:
        glp = obj.getGlobalPlacement()
    else:
        glp = global_place
    points = obj.Points
    points[vert_idx] = glp.inverse().multVec(
        rotate_vector_from_center(
            glp.multVec(points[vert_idx]),
            angle, axis, center))
    obj.Points = points


def rotate_edge(obj, edge_idx, angle, center, axis, global_place=None):
    """
    Needed for SubObjects modifiers.
    Implemented by Dion Moult during 0.19 dev cycle (works only with Draft Wire).
    """
    rotate_vertex(obj, edge_idx, angle, center, axis, global_place)
    if utils.is_closed_edge(edge_idx, obj):
        rotate_vertex(obj, 0, angle, center, axis, global_place)
    else:
        rotate_vertex(obj, edge_idx+1, angle, center, axis, global_place)


def copy_rotated_edge(obj, edge_idx, angle, center, axis, global_place=None):
    """
    Needed for SubObjects modifiers.
    Implemented by Dion Moult during 0.19 dev cycle (works only with Draft Wire).
    """
    if global_place is None:
        glp = obj.getGlobalPlacement()
    else:
        glp = global_place
    vertex1 = rotate_vector_from_center(
        glp.multVec(obj.Points[edge_idx]),
        angle, axis, center)
    if utils.is_closed_edge(edge_idx, obj):
        vertex2 = rotate_vector_from_center(
            glp.multVec(obj.Points[0]),
            angle, axis, center)
    else:
        vertex2 = rotate_vector_from_center(
            glp.multVec(obj.Points[edge_idx+1]),
            angle, axis, center)
    newobj = make_line.make_line(vertex1, vertex2)
    gui_utils.format_object(newobj, obj)
    return newobj

## @}
