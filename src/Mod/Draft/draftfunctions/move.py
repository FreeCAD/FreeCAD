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
"""Provides functions to move objects from one position to another."""
## @package move
# \ingroup draftfuctions
# \brief Provides functions to move objects from one position to another.

## \addtogroup draftfuctions
# @{
import FreeCAD as App
import draftutils.utils as utils
import draftutils.gui_utils as gui_utils
import draftutils.groups as groups
import draftfunctions.join as join
import draftmake.make_copy as make_copy
import draftmake.make_line as make_line


def move(objectslist, vector, copy=False):
    """move(objects,vector,[copy])
    
    Move the objects contained in objects (that can be an object or a
    list of objects) in the direction and distance indicated by the given
    vector. 

    Parameters
    ----------
    objectslist : list

    vector : Base.Vector
        Delta Vector to move the clone from the original position. 

    copy : bool
        If copy is True, the actual objects are not moved, but copies
        are created instead. 

    Return
    ----------
    The objects (or their copies) are returned.
    """
    utils.type_check([(vector, App.Vector), (copy,bool)], "move")
    if not isinstance(objectslist, list):
        objectslist = [objectslist]

    objectslist.extend(groups.get_movable_children(objectslist))
    newobjlist = []
    newgroups = {}
    objectslist = utils.filter_objects_for_modifiers(objectslist, copy)

    for obj in objectslist:
        newobj = None
        # real_vector have been introduced to take into account
        # the possibility that object is inside an App::Part
        # TODO: Make Move work also with App::Link
        if hasattr(obj, "getGlobalPlacement"):
            v_minus_global = obj.getGlobalPlacement().inverse().Rotation.multVec(vector)
            real_vector = obj.Placement.Rotation.multVec(v_minus_global)
        else:
            real_vector = vector

        if utils.get_type(obj) == "Point":
            if copy:
                newobj = make_copy.make_copy(obj)
            else:
                newobj = obj
            newobj.X = obj.X.Value + real_vector.x
            newobj.Y = obj.Y.Value + real_vector.y
            newobj.Z = obj.Z.Value + real_vector.z

        elif obj.isDerivedFrom("App::DocumentObjectGroup"):
            pass

        elif hasattr(obj,'Shape'):
            if copy:
                newobj = make_copy.make_copy(obj)
            else:
                newobj = obj
            pla = newobj.Placement
            pla.move(real_vector)

        elif utils.get_type(obj) == "Annotation":
            if copy:
                newobj = make_copy.make_copy(obj)
            else:
                newobj = obj
            newobj.Position = obj.Position.add(real_vector)

        elif utils.get_type(obj) in ("Text", "DraftText"):
            if copy:
                newobj = make_copy.make_copy(obj)
            else:
                newobj = obj
            newobj.Placement.Base = obj.Placement.Base.add(real_vector)

        elif utils.get_type(obj) in ["Dimension", "LinearDimension"]:
            if copy:
                newobj = make_copy.make_copy(obj)
            else:
                newobj = obj
            newobj.Start = obj.Start.add(real_vector)
            newobj.End = obj.End.add(real_vector)
            newobj.Dimline = obj.Dimline.add(real_vector)

        elif utils.get_type(obj) in ["AngularDimension"]:
            if copy:
                newobj = make_copy.make_copy(obj)
            else:
                newobj = obj
            newobj.Center = obj.Start.add(real_vector)

        elif "Placement" in obj.PropertiesList:
            if copy:
                newobj = make_copy.make_copy(obj)
            else:
                newobj = obj
            pla = obj.Placement
            pla.move(real_vector)

        if newobj is not None:
            newobjlist.append(newobj)

        if copy:
            for p in obj.InList:
                if p.isDerivedFrom("App::DocumentObjectGroup") and (p in objectslist):
                    g = newgroups.setdefault(p.Name,App.ActiveDocument.addObject(p.TypeId,p.Name))
                    g.addObject(newobj)
                    break
                if utils.get_type(p) == "Layer":
                    p.Proxy.addObject(p,newobj)

    if copy and utils.get_param("selectBaseObjects",False):
        gui_utils.select(objectslist)
    else:
        gui_utils.select(newobjlist)
    if len(newobjlist) == 1:
        return newobjlist[0]
    return newobjlist


#   Following functions are needed for SubObjects modifiers
#   implemented by Dion Moult during 0.19 dev cycle (works only with Draft Wire)


def move_vertex(object, vertex_index, vector):
    """
    Needed for SubObjects modifiers.
    Implemented by Dion Moult during 0.19 dev cycle (works only with Draft Wire).
    """
    points = object.Points
    points[vertex_index] = points[vertex_index].add(vector)
    object.Points = points


moveVertex = move_vertex


def move_edge(object, edge_index, vector):
    """
    Needed for SubObjects modifiers.
    Implemented by Dion Moult during 0.19 dev cycle (works only with Draft Wire).
    """
    moveVertex(object, edge_index, vector)
    if utils.isClosedEdge(edge_index, object):
        moveVertex(object, 0, vector)
    else:
        moveVertex(object, edge_index+1, vector)


moveEdge = move_edge


def copy_moved_edges(arguments):
    """
    Needed for SubObjects modifiers.
    Implemented by Dion Moult during 0.19 dev cycle (works only with Draft Wire).
    """
    copied_edges = []
    for argument in arguments:
        copied_edges.append(copy_moved_edge(argument[0], argument[1], argument[2]))
    join.join_wires(copied_edges)


copyMovedEdges = copy_moved_edges


def copy_moved_edge(object, edge_index, vector):
    """
    Needed for SubObjects modifiers.
    Implemented by Dion Moult during 0.19 dev cycle (works only with Draft Wire).
    """
    vertex1 = object.Placement.multVec(object.Points[edge_index]).add(vector)
    if utils.isClosedEdge(edge_index, object):
        vertex2 = object.Placement.multVec(object.Points[0]).add(vector)
    else:
        vertex2 = object.Placement.multVec(object.Points[edge_index+1]).add(vector)
    return make_line.make_line(vertex1, vertex2)

## @}
