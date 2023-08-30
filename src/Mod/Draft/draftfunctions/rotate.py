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
"""Provides functions to rotate shapes around a center and axis."""
## @package rotate
# \ingroup draftfunctions
# \brief Provides functions to rotate shapes around a center and axis.

## \addtogroup draftfunctions
# @{
import math

import FreeCAD as App
import DraftVecUtils
import draftutils.utils as utils
import draftutils.gui_utils as gui_utils
import draftutils.groups as groups
import draftfunctions.join as join
import draftmake.make_line as make_line
import draftmake.make_copy as make_copy


def rotate(objectslist, angle, center=App.Vector(0, 0, 0),
           axis=App.Vector(0, 0, 1), copy=False):
    """rotate(objects,angle,[center,axis,copy])

    Rotates the objects contained in objects (that can be a list of objects
    or an object) of the given angle (in degrees) around the center, using
    axis as a rotation axis.

    Parameters
    ----------
    objectslist : list

    angle : rotation angle (in degrees)

    center : Base.Vector

    axis : Base.Vector
        If axis is omitted, the rotation will be around the vertical Z axis.

    copy : bool
        If copy is True, the actual objects are not moved, but copies
        are created instead.

    Return
    ----------
    The objects (or their copies) are returned.
    """
    import Part
    utils.type_check([(copy,bool)], "rotate")
    if not isinstance(objectslist,list):
        objectslist = [objectslist]

    objectslist.extend(groups.get_movable_children(objectslist))
    newobjlist = []
    newgroups = {}
    objectslist = utils.filter_objects_for_modifiers(objectslist, copy)

    if copy:
        doc = App.ActiveDocument
        for obj in objectslist:
            if obj.isDerivedFrom("App::DocumentObjectGroup") \
                    and obj.Name not in newgroups:
                newgroups[obj.Name] = doc.addObject(obj.TypeId,
                                                    utils.get_real_name(obj.Name))

    for obj in objectslist:
        newobj = None

        # real_center and real_axis are introduced to take into account
        # the possibility that object is inside an App::Part
        if hasattr(obj, "getGlobalPlacement"):
            ci = obj.getGlobalPlacement().inverse().multVec(center)
            real_center = obj.Placement.multVec(ci)
            ai = obj.getGlobalPlacement().inverse().Rotation.multVec(axis)
            real_axis = obj.Placement.Rotation.multVec(ai)
        else:
            real_center = center
            real_axis = axis

        if obj.isDerivedFrom("App::Annotation"):
            # TODO: this is very different from how move handle annotations
            # maybe we can uniform the two methods
            if copy:
                newobj = make_copy.make_copy(obj)
            else:
                newobj = obj
            if axis.normalize() == App.Vector(1, 0, 0):
                newobj.ViewObject.RotationAxis = "X"
                newobj.ViewObject.Rotation = angle
            elif axis.normalize() == App.Vector(0, 1, 0):
                newobj.ViewObject.RotationAxis = "Y"
                newobj.ViewObject.Rotation = angle
            elif axis.normalize() == App.Vector(0, -1, 0):
                newobj.ViewObject.RotationAxis = "Y"
                newobj.ViewObject.Rotation = -angle
            elif axis.normalize() == App.Vector(0, 0, 1):
                newobj.ViewObject.RotationAxis = "Z"
                newobj.ViewObject.Rotation = angle
            elif axis.normalize() == App.Vector(0, 0, -1):
                newobj.ViewObject.RotationAxis = "Z"
                newobj.ViewObject.Rotation = -angle

        elif obj.isDerivedFrom("App::DocumentObjectGroup"):
            if copy:
                newobj = newgroups[obj.Name]
            else:
                newobj = obj

        elif hasattr(obj, "Placement"):
            # App.Console.PrintMessage("placement rotation\n")
            if copy:
                newobj = make_copy.make_copy(obj)
            else:
                newobj = obj
            newobj.Placement.rotate(real_center, real_axis, angle, comp=True)

        elif hasattr(obj, "Shape"):
            if copy:
                newobj = make_copy.make_copy(obj)
            else:
                newobj = obj
            shape = newobj.Shape.copy()
            shape.rotate(real_center, real_axis, angle)
            newobj.Shape = shape

        if newobj is not None:
            newobjlist.append(newobj)
            if copy:
                for parent in obj.InList:
                    if parent.isDerivedFrom("App::DocumentObjectGroup") \
                            and (parent in objectslist):
                        newgroups[parent.Name].addObject(newobj)
                    if utils.get_type(parent) == "Layer":
                        parent.Proxy.addObject(parent ,newobj)

    if copy and utils.get_param("selectBaseObjects", False):
        gui_utils.select(objectslist)
    else:
        gui_utils.select(newobjlist)

    if len(newobjlist) == 1:
        return newobjlist[0]
    return newobjlist


#   Following functions are needed for SubObjects modifiers
#   implemented by Dion Moult during 0.19 dev cycle (works only with Draft Wire)


def rotate_vertex(object, vertex_index, angle, center, axis):
    """
    Needed for SubObjects modifiers.
    Implemented by Dion Moult during 0.19 dev cycle (works only with Draft Wire).
    """
    points = object.Points
    points[vertex_index] = object.getGlobalPlacement().inverse().multVec(
        rotate_vector_from_center(
            object.getGlobalPlacement().multVec(points[vertex_index]),
            angle, axis, center))
    object.Points = points


rotateVertex = rotate_vertex


def rotate_vector_from_center(vector, angle, axis, center):
    """
    Needed for SubObjects modifiers.
    Implemented by Dion Moult during 0.19 dev cycle (works only with Draft Wire).
    """
    rv = vector.sub(center)
    rv = DraftVecUtils.rotate(rv, math.radians(angle), axis)
    return center.add(rv)


rotateVectorFromCenter = rotate_vector_from_center


def rotate_edge(object, edge_index, angle, center, axis):
    """
    Needed for SubObjects modifiers.
    Implemented by Dion Moult during 0.19 dev cycle (works only with Draft Wire).
    """
    rotate_vertex(object, edge_index, angle, center, axis)
    if utils.isClosedEdge(edge_index, object):
        rotate_vertex(object, 0, angle, center, axis)
    else:
        rotate_vertex(object, edge_index+1, angle, center, axis)


rotateEdge = rotate_edge


def copy_rotated_edges(arguments):
    """
    Needed for SubObjects modifiers.
    Implemented by Dion Moult during 0.19 dev cycle (works only with Draft Wire).
    """
    copied_edges = []
    for argument in arguments:
        copied_edges.append(copy_rotated_edge(argument[0], argument[1],
            argument[2], argument[3], argument[4]))
    join.join_wires(copied_edges)


copyRotatedEdges = copy_rotated_edges


def copy_rotated_edge(object, edge_index, angle, center, axis):
    """
    Needed for SubObjects modifiers.
    Implemented by Dion Moult during 0.19 dev cycle (works only with Draft Wire).
    """
    vertex1 = rotate_vector_from_center(
        object.getGlobalPlacement().multVec(object.Points[edge_index]),
        angle, axis, center)
    if utils.isClosedEdge(edge_index, object):
        vertex2 = rotate_vector_from_center(
            object.getGlobalPlacement().multVec(object.Points[0]),
            angle, axis, center)
    else:
        vertex2 = rotate_vector_from_center(
            object.getGlobalPlacement().multVec(object.Points[edge_index+1]),
            angle, axis, center)
    return make_line.make_line(vertex1, vertex2)

## @}
