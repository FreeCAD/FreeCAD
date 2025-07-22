# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2020 FreeCAD Developers                                 *
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
"""Provides functions to scale shapes."""
## @package scale
# \ingroup draftfunctions
# \brief Provides functions to scale shapes.

## \addtogroup draftfunctions
# @{
import math

import FreeCAD as App
import DraftVecUtils
from draftfunctions import join
from draftmake import make_clone
from draftmake import make_copy
from draftmake import make_line
from draftmake import make_wire
from draftobjects import layer
from draftutils import gui_utils
from draftutils import params
from draftutils import utils


def scale(selection, scale, center=App.Vector(0, 0, 0),
          copy=False, clone=False, subelements=False):
    """scale(selection, scale, [center], [copy], [clone], [subelements])

    Scales or copies selected objects.

    Parameters
    ----------
    selection: single object / list of objects / selection set
        When dealing with nested objects, use `Gui.Selection.getSelectionEx("", 0)`
        to create the selection set.

    scale: App.Vector
        The X, Y and Z component of the vector define the scale factors.

    center: App.Vector, optional
        Defaults to `App.Vector(0, 0, 0)`.
        Center of the scale operation.

    copy: bool, optional
        Defaults to `False`.
        If `True` the selected objects are not scaled, but scaled copies are
        created instead. If clone is `True`, copy is internally set to `False`.

    clone: bool, optional
        Defaults to `False`.
        If `True` the selected objects are not scaled, but scaled clones are
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
    utils.type_check([(scale, App.Vector), (center, App.Vector),
                      (copy, bool), (clone, bool), (subelements, bool)], "scale")
    sx, sy, sz = scale
    if sx * sy * sz == 0:
        raise ValueError("Zero component in scale vector")
    if not isinstance(selection, list):
        selection = [selection]
    if not selection:
        return None
    if clone:
        copy = False

    if selection[0].isDerivedFrom("Gui::SelectionObject"):
        if subelements:
            return _scale_subelements(selection, scale, center, copy)
        else:
            objs, parent_places, sel_info = utils._modifiers_process_selection(selection, (copy or clone), scale=True)
    else:
        objs = utils._modifiers_filter_objects(utils._modifiers_get_group_contents(selection), (copy or clone), scale=True)
        parent_places = None
        sel_info = None

    if not objs:
        return None

    newobjs = []
    newgroups = {}

    if copy or clone:
        for obj in objs:
            if obj.isDerivedFrom("App::DocumentObjectGroup") and obj.Name not in newgroups:
                newgroups[obj.Name] = obj.Document.addObject(obj.TypeId, utils.get_real_name(obj.Name))

    for idx, obj in enumerate(objs):
        newobj = None
        create_non_parametric = False

        if parent_places is not None:
            parent_place = parent_places[idx]
        elif hasattr(obj, "getGlobalPlacement"):
            parent_place = obj.getGlobalPlacement() * obj.Placement.inverse()
        else:
            parent_place = App.Placement()

        if obj.isDerivedFrom("App::DocumentObjectGroup"):
            if copy or clone:
                newobj = newgroups[obj.Name]
            else:
                newobj = obj

        elif obj.isDerivedFrom("App::Annotation"):
            if parent_place.isIdentity():
                pos = obj.Position
            else:
                pos = parent_place.multVec(obj.Position)
            pos = scale_vector_from_center(pos, scale, center)
            if copy or clone:
                newobj = make_copy.make_copy(obj)
                newobj.Position = pos
            else:
                newobj = obj
                if parent_place.isIdentity():
                    newobj.Position = pos
                else:
                    newobj.Position = parent_place.inverse().multVec(pos)

        elif obj.isDerivedFrom("Image::ImagePlane"):
            if parent_place.isIdentity():
                pla = obj.Placement
            else:
                pla = parent_place * obj.Placement
            pla = App.Placement(_get_scaled_matrix(pla, scale, center))
            scale = pla.Rotation.inverted().multVec(scale)
            if copy:
                newobj = make_copy.make_copy(obj)
                newobj.Placement = pla
            else:
                newobj = obj
                if parent_place.isIdentity():
                    newobj.Placement = pla
                else:
                    newobj.Placement = parent_place.inverse() * pla
            newobj.XSize = newobj.XSize * abs(scale.x)
            newobj.YSize = newobj.YSize * abs(scale.y)

        elif clone:
            if not hasattr(obj, "Placement"):
                continue
            if not hasattr(obj, "Shape"):
                continue
            if sx == sy == sz:
                newobj = make_clone.make_clone(obj, forcedraft=True)
                newobj.Placement.Base = scale_vector_from_center(newobj.Placement.Base, scale, center)
                newobj.Scale = scale
            else:
                if parent_place.isIdentity():
                    source = obj
                else:
                    source = make_clone.make_clone(obj, forcedraft=True)
                    source.Placement = parent_place * obj.Placement
                    source.Visibility = False
                delta = scale_vector_from_center(App.Vector(), scale, center)
                newobj = make_clone.make_clone(source, delta=delta, forcedraft=True)
                newobj.ForceCompound = True
                newobj.Scale = scale

        elif utils.get_type(obj) in ("Circle", "Ellipse"):
            if abs(sx) == abs(sy) == abs(sz):
                if parent_place.isIdentity():
                    pla = obj.Placement
                else:
                    pla = parent_place * obj.Placement
                pla = App.Placement(_get_scaled_matrix(pla, scale, center))
                if copy:
                    newobj = make_copy.make_copy(obj)
                    newobj.Placement = pla
                else:
                    newobj = obj
                    if parent_place.isIdentity():
                        newobj.Placement = pla
                    else:
                        newobj.Placement = parent_place.inverse() * pla
                if utils.get_type(newobj) == "Circle":
                    newobj.Radius = abs(sx) * newobj.Radius
                else:
                    newobj.MinorRadius = abs(sx) * newobj.MinorRadius
                    newobj.MajorRadius = abs(sx) * newobj.MajorRadius
                if newobj.FirstAngle != newobj.LastAngle and sx * sy * sz < 0:
                    newobj.FirstAngle = (newobj.FirstAngle.Value + 180) % 360
                    newobj.LastAngle = (newobj.LastAngle.Value + 180) % 360
            else:
                create_non_parametric = True

        elif utils.get_type(obj) == "Rectangle":
            if parent_place.isIdentity():
                pla = obj.Placement
            else:
                pla = parent_place * obj.Placement
            pts = [
                App.Vector (0.0, 0.0, 0.0),
                App.Vector (obj.Length.Value, 0.0, 0.0),
                App.Vector (obj.Length.Value, obj.Height.Value, 0.0),
                App.Vector (0.0, obj.Height.Value, 0.0)
            ]
            pts = [pla.multVec(p) for p in pts]
            pts = [scale_vector_from_center(p, scale, center) for p in pts]
            pla = App.Placement(_get_scaled_matrix(pla, scale, center))
            x_vec = pts[1] - pts[0]
            y_vec = pts[3] - pts[0]
            ang = x_vec.getAngle(y_vec)
            if math.isclose(ang % math.pi/2, math.pi/4, abs_tol=1e-6):
                if copy:
                    newobj = make_copy.make_copy(obj)
                    newobj.Placement = pla
                else:
                    newobj = obj
                    if parent_place.isIdentity():
                        newobj.Placement = pla
                    else:
                        newobj.Placement = parent_place.inverse() * pla
                newobj.Length = x_vec.Length
                newobj.Height = y_vec.Length
            else:
                newobj = make_wire.make_wire(pts, closed=True, placement=pla, face=obj.MakeFace)
                gui_utils.format_object(newobj, obj)
                if not copy:
                    obj.Document.removeObject(obj.Name)

        elif utils.get_type(obj) in ("Wire", "BSpline"):
            if parent_place.isIdentity():
                pla = obj.Placement
            else:
                pla = parent_place * obj.Placement
            pts = [pla.multVec(p) for p in obj.Points]
            pts = [scale_vector_from_center(p, scale, center) for p in pts]
            pla = App.Placement(_get_scaled_matrix(pla, scale, center))
            if copy:
                newobj = make_copy.make_copy(obj)
                newobj.Placement = pla
            else:
                newobj = obj
                if parent_place.isIdentity():
                    newobj.Placement = pla
                else:
                    newobj.Placement = parent_place.inverse() * pla
            pla_inv = pla.inverse()
            newobj.Points = [pla_inv.multVec(p) for p in pts]

        elif hasattr(obj, "Placement") and hasattr(obj, "Shape"):
            create_non_parametric = True

        if create_non_parametric:
            import Part
            if parent_place.isIdentity():
                pla = obj.Placement
            else:
                pla = parent_place * obj.Placement
            pla = App.Placement(_get_scaled_matrix(pla, scale, center))
            mtx = _get_scaled_matrix(parent_place, scale, center)
            shp = obj.Shape.copy().transformShape(pla.Matrix.inverse() * mtx, False, True)
            newobj = obj.Document.addObject("Part::FeaturePython", utils.get_real_name(obj.Name))
            newobj.Shape = Part.Compound([shp])
            newobj.Placement = pla
            if App.GuiUp:
                if utils.get_type(obj) in ("Circle", "Ellipse"):
                    from draftviewproviders.view_base import ViewProviderDraft
                    ViewProviderDraft(newobj.ViewObject)
                else:
                    newobj.ViewObject.Proxy = 0
            gui_utils.format_object(newobj, obj)
            if not copy:
                obj.Document.removeObject(obj.Name)

        if newobj is not None:
            newobjs.append(newobj)
            if copy:
                lyr = layer.get_layer(obj)
                if lyr is not None:
                    lyr.Proxy.addObject(lyr, newobj)
                for parent in obj.InList:
                    if parent.isDerivedFrom("App::DocumentObjectGroup") and (parent in objs):
                        newgroups[parent.Name].addObject(newobj)

    if not (copy or clone) or params.get_param("selectBaseObjects"):
        if sel_info is not None:
            gui_utils.select(sel_info)
        else:
            gui_utils.select(objs)
    else:
        gui_utils.select(newobjs)

    if len(newobjs) == 1:
        return newobjs[0]
    return newobjs


def _scale_subelements(selection, scale, center, copy):
    data_list, sel_info = utils._modifiers_process_subselection(selection, copy)
    newobjs = []
    if copy:
        for obj, vert_idx, edge_idx, global_place in data_list:
            if edge_idx >= 0:
                newobjs.append(copy_scaled_edge(obj, edge_idx, scale, center, global_place))
        newobjs = join.join_wires(newobjs)
    else:
        for obj, vert_idx, edge_idx, global_place in data_list:
            if vert_idx >= 0:
                scale_vertex(obj, vert_idx, scale, center, global_place)
            elif edge_idx >= 0:
                scale_edge(obj, edge_idx, scale, center, global_place)

    if not copy or params.get_param("selectBaseObjects"):
        gui_utils.select(sel_info)
    else:
        gui_utils.select(newobjs)

    if len(newobjs) == 1:
        return newobjs[0]
    return newobjs


def _get_scaled_matrix(pla, scale, center):
    mtx = App.Matrix(pla.Matrix)
    mtx.move(-center)
    mtx.scale(scale)
    mtx.move(center)
    return mtx


def scale_vector_from_center(vector, scale, center):
    """
    Needed for SubObjects modifiers.
    Implemented by Dion Moult during 0.19 dev cycle (works only with Draft Wire).
    """
    return vector.sub(center).scale(*scale).add(center)


def scale_vertex(obj, vert_idx, scale, center, global_place=None):
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
        scale_vector_from_center(glp.multVec(points[vert_idx]), scale, center)
    )
    obj.Points = points


def scale_edge(obj, edge_idx, scale, center, global_place=None):
    """
    Needed for SubObjects modifiers.
    Implemented by Dion Moult during 0.19 dev cycle (works only with Draft Wire).
    """
    scale_vertex(obj, edge_idx, scale, center, global_place)
    if utils.is_closed_edge(edge_idx, obj):
        scale_vertex(obj, 0, scale, center, global_place)
    else:
        scale_vertex(obj, edge_idx+1, scale, center, global_place)


def copy_scaled_edge(obj, edge_idx, scale, center, global_place=None):
    """
    Needed for SubObjects modifiers.
    Implemented by Dion Moult during 0.19 dev cycle (works only with Draft Wire).
    """
    if global_place is None:
        glp = obj.getGlobalPlacement()
    else:
        glp = global_place
    vertex1 = scale_vector_from_center(glp.multVec(obj.Points[edge_idx]), scale, center)
    if utils.is_closed_edge(edge_idx, obj):
        vertex2 = scale_vector_from_center(glp.multVec(obj.Points[0]), scale, center)
    else:
        vertex2 = scale_vector_from_center(glp.multVec(obj.Points[edge_idx+1]), scale, center)
    newobj = make_line.make_line(vertex1, vertex2)
    gui_utils.format_object(newobj, obj)
    return newobj

## @}
