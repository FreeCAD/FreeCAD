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

if App.GuiUp:
    from PySide import QtCore, QtGui


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


# This part is higly experimental and used for moveSubElements function

def parse_shape(shape, selected_subelements, vector):
    """ Parse the given shape and rebuild it according to its
    original topological structure
    """
    import Part

    print('Parsing ' + shape.ShapeType + '\n')

    if shape.ShapeType in ("Compound", "CompSolid", "Solid", "Shell", "Wire"):
        # No geometry involved
        new_sub_shapes, touched_subshapes = parse_sub_shapes(shape, selected_subelements, vector)

        if shape.ShapeType == "Compound":
            new_shape = Part.Compound(new_sub_shapes)

        elif shape.ShapeType == "CompSolid":
            new_shape = Part.CompSolid(new_sub_shapes)

        elif shape.ShapeType == "Solid":
            if isinstance(new_sub_shapes, list) and len(new_sub_shapes) == 1:
                # check if shell object is given inside a list
                new_sub_shapes = new_sub_shapes[0]
            new_shape = Part.Solid(new_sub_shapes)

        elif shape.ShapeType == "Shell":
            new_shape = Part.Shell(new_sub_shapes)

        elif shape.ShapeType == "Wire":
            new_sub_shapes = Part.__sortEdges__(new_sub_shapes)
            new_shape = Part.Wire(new_sub_shapes)

        print(shape.ShapeType + " re-created.")
        touched = True

    elif shape.ShapeType == "Face":
        new_sub_shapes, touched_subshapes = parse_sub_shapes(shape, selected_subelements, vector)
        if touched_subshapes == 1 or touched_subshapes == 2:
            print("some subshapes touched " + shape.ShapeType + " recreated.")
            if shape.Surface.TypeId == 'Part::GeomPlane':
                new_sub_shapes = sort_wires(new_sub_shapes)
                new_shape = Part.Face(new_sub_shapes)
                touched = True
                # TODO: handle the usecase when the Face is not planar anymore after modification
            else:
                print("Face geometry not supported")
        elif touched_subshapes == 0:
            print("subshapes not touched " + shape.ShapeType + " not touched.")
            new_shape = shape 
            touched = False

    elif shape.ShapeType == "Edge":
        # TODO: Add geometry check
        new_sub_shapes, touched_subshapes = parse_sub_shapes(shape, selected_subelements, vector)
        if touched_subshapes == 2:
            print("all subshapes touched. " + shape.ShapeType + " translated.")
            # all subshapes touched
            new_shape = shape.translate(vector)
            touched = True
        elif touched_subshapes == 1:
            # some subshapes touched: recreate the edge as a straight vector: TODO Add geometry check
            print("some subshapes touched " + shape.ShapeType + " recreated.")
            new_shape = Part.makeLine(new_sub_shapes[0].Point, new_sub_shapes[1].Point)
            touched = True
        elif touched_subshapes == 0:
            # subshapes not touched
            print("subshapes not touched " + shape.ShapeType + " not touched.")
            new_shape = shape 
            touched = False

    elif shape.ShapeType == "Vertex":
        # TODO: Add geometry check
        touched = False
        for s in selected_subelements:
            if shape.isSame(s):
                touched = True
        if touched:
            print(shape.ShapeType + " translated.")
            new_shape = shape.translate(vector)
        else:
            print(shape.ShapeType + " not touched.")
            new_shape = shape

    return new_shape, touched


def sort_wires(wires):
    if not isinstance(wires, list):
        return wires
    if len(wires) == 1:
        return wires
    outer_wire = wires[0]

    for w in wires:
        if outer_wire.BoundBox.DiagonalLength < w.BoundBox.DiagonalLength:
            outer_wire = w
    
    new_wires = [outer_wire]
    for w in wires:
        if w != outer_wire:
            new_wires.append(w)
    
    return new_wires

    
def parse_sub_shapes(shape, selected_subelements, vector):
    """ Parse the subshapes of the given shape in order to
    find modified shapes and substitute them to the originals.
    """
    sub_shapes = []
    touched_subshapes = []
    if shape.SubShapes:
        for sub_shape in shape.SubShapes:
            new_sub_shape, touched_subshape = parse_shape(sub_shape, selected_subelements, vector)
            sub_shapes.append(new_sub_shape)

            if touched_subshape:
                touched_subshapes.append(2)
            else:
                touched_subshapes.append(0)

    if 0 in touched_subshapes and 2 in touched_subshapes:
        # only some subshapes touched
        touched = 1
    elif 2 in touched_subshapes:
        # all subshapes touched
        touched = 2
    elif 0 in touched_subshapes:
        # no subshapes touched
        touched = 0

    return sub_shapes, touched


def moveSubElements(obj, sub_objects_names, vector):
    """moveSubElements(obj, sub_objects_names, vector)
    
    Move the given object sub_objects according to a vector or crates an new one
    if the object is not a Part::Feature.

    Parameters
    ----------
    obj : the given object

    sub_objects_names : list of subelement names
        A list of subelement names to identify the subelements to move.

    vector : Base.Vector
        Delta Vector to move subobjects from the original position. 

    Return
    ----------
    shape : Part.Shape
        Return the new Shape or None.
    """

    import Part

    if not isinstance(sub_objects_names, list):
        sub_objects_names = [sub_objects_names]

    shape = obj.Shape
    new_shape = None
    if not shape.isValid():
        return

    selected_subelements = []
    for sub_objects_name in sub_objects_names:
        sub_object = obj.Shape.getElement(sub_objects_name)
        selected_subelements.append(sub_object)
        selected_subelements.extend(sub_object.Faces)
        selected_subelements.extend(sub_object.Edges)
        selected_subelements.extend(sub_object.Vertexes)

    new_shape, touched = parse_shape(shape, selected_subelements, vector)
    
    if not new_shape.isValid():
        should_fix = move_subelements_msgbox()
        if should_fix:
            new_shape.fix(0.001,0.001,0.001)

    if new_shape:
        if hasattr(obj, 'TypeId') and obj.TypeId == 'Part::Feature':
            obj.Shape = new_shape
        else:
            new_obj = App.ActiveDocument.addObject("Part::Feature", "Feature")
            new_obj.Shape = new_shape
        return new_shape

def move_subelements_msgbox():
    if not App.GuiUp:
        return False
    msgBox = QtGui.QMessageBox()
    msgBox.setText("Shape has become invalid after editing.")
    msgBox.setInformativeText("Do you want to try to fix it?\n")
    msgBox.setStandardButtons(QtGui.QMessageBox.Yes | QtGui.QMessageBox.No)
    msgBox.setDefaultButton(QtGui.QMessageBox.Yes)
    ret = msgBox.exec_()

    if ret == QtGui.QMessageBox.Yes:
        return True
    elif ret == QtGui.QMessageBox.No:
        return False
