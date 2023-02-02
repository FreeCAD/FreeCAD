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
"""Provides functions to scale shapes."""
## @package scale
# \ingroup draftfunctions
# \brief Provides functions to scale shapes.

## \addtogroup draftfunctions
# @{
import FreeCAD as App
import DraftVecUtils
import draftutils.utils as utils
import draftutils.gui_utils as gui_utils
import draftfunctions.join as join
import draftmake.make_copy as make_copy
import draftmake.make_line as make_line


def scale(objectslist, scale=App.Vector(1,1,1),
          center=App.Vector(0,0,0), copy=False):
    """scale(objects, scale, [center], copy)

    Scales the objects contained in objects (that can be a list of objects or
    an object) of the given  around given center.

    Parameters
    ----------
    objectlist : list

    scale : Base.Vector
        Scale factors defined by a given vector (in X, Y, Z directions).

    objectlist : Base.Vector
        Center of the scale operation.

    copy : bool
        If copy is True, the actual objects are not scaled, but copies
        are created instead.

    Return
    ----------
    The objects (or their copies) are returned.
    """
    if not isinstance(objectslist, list):
        objectslist = [objectslist]
    newobjlist = []
    for obj in objectslist:
        if copy:
            newobj = make_copy.make_copy(obj)
        else:
            newobj = obj
        if hasattr(obj,'Shape'):
            scaled_shape = obj.Shape.copy()
            m = App.Matrix()
            m.move(center.negative())
            m.scale(scale.x,scale.y,scale.z)
            m.move(center)
            scaled_shape = scaled_shape.transformGeometry(m)
        if utils.get_type(obj) == "Rectangle":
            p = []
            for v in scaled_shape.Vertexes:
                p.append(v.Point)
            pl = obj.Placement.copy()
            pl.Base = p[0]
            diag = p[2].sub(p[0])
            bb = p[1].sub(p[0])
            bh = p[3].sub(p[0])
            nb = DraftVecUtils.project(diag,bb)
            nh = DraftVecUtils.project(diag,bh)
            if obj.Length < 0: l = -nb.Length
            else: l = nb.Length
            if obj.Height < 0: h = -nh.Length
            else: h = nh.Length
            newobj.Length = l
            newobj.Height = h
            tr = p[0].sub(obj.Shape.Vertexes[0].Point) # unused?
            newobj.Placement = pl
        elif utils.get_type(obj) == "Wire" or utils.get_type(obj) == "BSpline":
            for index, point in enumerate(newobj.Points):
                scale_vertex(newobj, index, scale, center)
        elif hasattr(obj,'Shape'):
            newobj.Shape = scaled_shape
        elif hasattr(obj,"Position"):
            d = obj.Position.sub(center)
            newobj.Position = center.add(App.Vector(d.x * scale.x,
                                                    d.y * scale.y,
                                                    d.z * scale.z))
        elif hasattr(obj,"Placement"):
            d = obj.Placement.Base.sub(center)
            newobj.Placement.Base = center.add(App.Vector(d.x * scale.x,
                                                    d.y * scale.y,
                                                    d.z * scale.z))
            if hasattr(obj,"Height"):
                obj.setExpression('Height', None)
                obj.Height = obj.Height * scale.y
            if hasattr(obj,"Width"):
                obj.setExpression('Width', None)
                obj.Width = obj.Width * scale.x
            if hasattr(obj,"XSize"):
                obj.setExpression('XSize', None)
                obj.XSize = obj.XSize * scale.x
            if hasattr(obj,"YSize"):
                obj.setExpression('YSize', None)
                obj.YSize = obj.YSize * scale.y
        if obj.ViewObject and hasattr(obj.ViewObject,"FontSize"):
            obj.ViewObject.FontSize = obj.ViewObject.FontSize * scale.y


        if copy:
            gui_utils.format_object(newobj,obj)
        newobjlist.append(newobj)
    if copy and utils.get_param("selectBaseObjects",False):
        gui_utils.select(objectslist)
    else:
        gui_utils.select(newobjlist)
    if len(newobjlist) == 1: return newobjlist[0]
    return newobjlist


#   Following functions are needed for SubObjects modifiers
#   implemented by Dion Moult during 0.19 dev cycle (works only with Draft Wire)


def scale_vertex(obj, vertex_index, scale, center):
    """
    Needed for SubObjects modifiers.
    Implemented by Dion Moult during 0.19 dev cycle (works only with Draft Wire).
    """
    points = obj.Points
    points[vertex_index] = obj.getGlobalPlacement().inverse().multVec(
        scale_vector_from_center(
            obj.getGlobalPlacement().multVec(points[vertex_index]),
            scale, center))
    obj.Points = points


scaleVertex = scale_vertex


def scale_vector_from_center(vector, scale, center):
    """
    Needed for SubObjects modifiers.
    Implemented by Dion Moult during 0.19 dev cycle (works only with Draft Wire).
    """
    return vector.sub(center).scale(scale.x, scale.y, scale.z).add(center)


scaleVectorFromCenter = scale_vector_from_center


def scale_edge(obj, edge_index, scale, center):
    """
    Needed for SubObjects modifiers.
    Implemented by Dion Moult during 0.19 dev cycle (works only with Draft Wire).
    """
    scale_vertex(obj, edge_index, scale, center)
    if utils.is_closed_edge(edge_index, obj):
        scale_vertex(obj, 0, scale, center)
    else:
        scale_vertex(obj, edge_index+1, scale, center)


scaleEdge = scale_edge


def copy_scaled_edge(obj, edge_index, scale, center):
    """
    Needed for SubObjects modifiers.
    Implemented by Dion Moult during 0.19 dev cycle (works only with Draft Wire).
    """
    vertex1 = scale_vector_from_center(
        obj.getGlobalPlacement().multVec(obj.Points[edge_index]),
        scale, center)
    if utils.is_closed_edge(edge_index, obj):
        vertex2 = scale_vector_from_center(
            obj.getGlobalPlacement().multVec(obj.Points[0]),
            scale, center)
    else:
        vertex2 = scale_vector_from_center(
            obj.getGlobalPlacement().multVec(obj.Points[edge_index+1]),
            scale, center)
    return make_line.make_line(vertex1, vertex2)


copyScaledEdge = copy_scaled_edge


def copy_scaled_edges(arguments):
    """
    Needed for SubObjects modifiers.
    Implemented by Dion Moult during 0.19 dev cycle (works only with Draft Wire).
    """
    copied_edges = []
    for argument in arguments:
        copied_edges.append(copy_scaled_edge(argument[0], argument[1],
            argument[2], argument[3]))
    join.join_wires(copied_edges)


copyScaledEdges = copy_scaled_edges

## @}
