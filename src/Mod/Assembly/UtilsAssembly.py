# SPDX-License-Identifier: LGPL-2.1-or-later
# /**************************************************************************
#                                                                           *
#    Copyright (c) 2023 Ondsel <development@ondsel.com>                     *
#                                                                           *
#    This file is part of FreeCAD.                                          *
#                                                                           *
#    FreeCAD is free software: you can redistribute it and/or modify it     *
#    under the terms of the GNU Lesser General Public License as            *
#    published by the Free Software Foundation, either version 2.1 of the   *
#    License, or (at your option) any later version.                        *
#                                                                           *
#    FreeCAD is distributed in the hope that it will be useful, but         *
#    WITHOUT ANY WARRANTY; without even the implied warranty of             *
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
#    Lesser General Public License for more details.                        *
#                                                                           *
#    You should have received a copy of the GNU Lesser General Public       *
#    License along with FreeCAD. If not, see                                *
#    <https://www.gnu.org/licenses/>.                                       *
#                                                                           *
# **************************************************************************/

import FreeCAD as App
import Part

if App.GuiUp:
    import FreeCADGui as Gui

# translate = App.Qt.translate

__title__ = "Assembly utilitary functions"
__author__ = "Ondsel"
__url__ = "https://www.freecad.org"


def activeAssembly():
    doc = Gui.ActiveDocument

    if doc is None or doc.ActiveView is None:
        return None

    active_assembly = doc.ActiveView.getActiveObject("part")

    if active_assembly is not None and active_assembly.Type == "Assembly":
        return active_assembly

    return None


def activePart():
    doc = Gui.ActiveDocument

    if doc is None or doc.ActiveView is None:
        return None

    active_part = doc.ActiveView.getActiveObject("part")

    if active_part is not None and active_part.Type != "Assembly":
        return active_part

    return None


def isAssemblyCommandActive():
    return activeAssembly() is not None and not Gui.Control.activeDialog()


def isDocTemporary(doc):
    # Guard against older versions of FreeCad which don't have the Temporary attribute
    try:
        temp = doc.Temporary
    except AttributeError:
        temp = False
    return temp


def assembly_has_at_least_n_parts(n):
    assembly = activeAssembly()
    i = 0
    if not assembly:
        return False
    for obj in assembly.OutList:
        # note : groundedJoints comes in the outlist so we filter those out.
        if hasattr(obj, "Placement") and not hasattr(obj, "ObjectToGround"):
            i = i + 1
            if i == n:
                return True
    return False


def getObject(full_name):
    # full_name is "Assembly.LinkOrAssembly1.LinkOrPart1.LinkOrBox.Edge16"
    # or           "Assembly.LinkOrAssembly1.LinkOrPart1.LinkOrBody.pad.Edge16"
    # or           "Assembly.LinkOrAssembly1.LinkOrPart1.LinkOrBody.Local_CS.X"
    # We want either LinkOrBody or LinkOrBox or Local_CS.
    names = full_name.split(".")
    doc = App.ActiveDocument

    if len(names) < 3:
        App.Console.PrintError(
            "getObject() in UtilsAssembly.py the object name is too short, at minimum it should be something like 'Assembly.Box.edge16'. It shouldn't be shorter"
        )
        return None

    prevObj = None

    for i, objName in enumerate(names):
        if i == 0:
            prevObj = doc.getObject(objName)
            if prevObj.TypeId == "App::Link":
                prevObj = prevObj.getLinkedObject()
            continue

        obj = None
        if prevObj.TypeId in {"App::Part", "Assembly::AssemblyObject", "App::DocumentObjectGroup"}:
            for obji in prevObj.OutList:
                if obji.Name == objName:
                    obj = obji
                    break

        if obj is None:
            return None

        # the last is the element name. So if we are at the last but one name, then it must be the selected
        if i == len(names) - 2:
            return obj

        if obj.TypeId == "App::Link":
            linked_obj = obj.getLinkedObject()
            if linked_obj.TypeId == "PartDesign::Body":
                if i + 1 < len(names):
                    obj2 = doc.getObject(names[i + 1])
                    if obj2 and isBodySubObject(obj2.TypeId):
                        return obj2
                return obj
            elif linked_obj.isDerivedFrom("Part::Feature"):
                return obj
            else:
                prevObj = linked_obj
                continue

        elif obj.TypeId in {"App::Part", "Assembly::AssemblyObject", "App::DocumentObjectGroup"}:
            prevObj = obj
            continue

        elif obj.TypeId == "PartDesign::Body":
            if i + 1 < len(names):
                obj2 = doc.getObject(names[i + 1])
                if obj2 and isBodySubObject(obj2.TypeId):
                    return obj2
            return obj

        elif obj.isDerivedFrom("Part::Feature"):
            # primitive, fastener, gear ...
            return obj

    return None


def isBodySubObject(typeId):
    return (
        typeId == "Sketcher::SketchObject"
        or typeId == "PartDesign::Point"
        or typeId == "PartDesign::Line"
        or typeId == "PartDesign::Plane"
        or typeId == "PartDesign::CoordinateSystem"
    )


def getContainingPart(full_name, selected_object):
    # full_name is "Assembly.Assembly1.LinkOrPart1.LinkOrBox.Edge16" -> LinkOrPart1
    # or           "Assembly.Assembly1.LinkOrPart1.LinkOrBody.pad.Edge16" -> LinkOrPart1
    # or           "Assembly.Assembly1.LinkOrPart1.LinkOrBody.Sketch.Edge1" -> LinkOrBody
    if selected_object is None:
        App.Console.PrintError("getContainingPart() in UtilsAssembly.py selected_object is None")
        return None

    names = full_name.split(".")
    doc = App.ActiveDocument
    if len(names) < 3:
        App.Console.PrintError(
            "getContainingPart() in UtilsAssembly.py the object name is too short, at minimum it should be something like 'Assembly.Box.edge16'. It shouldn't be shorter"
        )
        return None

    for objName in names:
        obj = doc.getObject(objName)

        if not obj:
            continue

        if obj == selected_object:
            return selected_object

        if obj.TypeId == "PartDesign::Body" and isBodySubObject(selected_object.TypeId):
            if obj.hasObject(selected_object, True):
                return obj

        # Note here we may want to specify a specific behavior for Assembly::AssemblyObject.
        if obj.TypeId == "App::Part":
            if obj.hasObject(selected_object, True):
                return obj

        elif obj.TypeId == "App::Link":
            linked_obj = obj.getLinkedObject()
            if linked_obj.TypeId == "PartDesign::Body" and isBodySubObject(selected_object.TypeId):
                if linked_obj.hasObject(selected_object, True):
                    return obj
            if linked_obj.TypeId == "App::Part":
                # linked_obj_doc = linked_obj.Document
                # selected_obj_in_doc = doc.getObject(selected_object.Name)
                if linked_obj.hasObject(selected_object, True):
                    return obj

    # no container found so we return the object itself.
    return selected_object


def getObjectInPart(objName, part):
    if part.Name == objName:
        return part

    if part.TypeId == "App::Link":
        part = part.getLinkedObject()

    if part.TypeId in {
        "App::Part",
        "Assembly::AssemblyObject",
        "App::DocumentObjectGroup",
        "PartDesign::Body",
    }:
        for obji in part.OutList:
            if obji.Name == objName:
                return obji

    return None


# get the placement of Obj relative to its containing Part
# Example : assembly.part1.part2.partn.body1 : placement of Obj relative to part1
def getObjPlcRelativeToPart(objName, part):
    obj = getObjectInPart(objName, part)

    # we need plc to be relative to the containing part
    obj_global_plc = getGlobalPlacement(obj, part)
    part_global_plc = getGlobalPlacement(part)

    return part_global_plc.inverse() * obj_global_plc


# Example : assembly.part1.part2.partn.body1 : jcsPlc is relative to body1
# This function returns jcsPlc relative to part1
def getJcsPlcRelativeToPart(jcsPlc, objName, part):
    obj_relative_plc = getObjPlcRelativeToPart(objName, part)
    return obj_relative_plc * jcsPlc


# Return the jcs global placement
def getJcsGlobalPlc(jcsPlc, objName, part):
    obj = getObjectInPart(objName, part)

    obj_global_plc = getGlobalPlacement(obj, part)
    return obj_global_plc * jcsPlc


# The container is used to support cases where the same object appears at several places
# which happens when you have a link to a part.
def getGlobalPlacement(targetObj, container=None):
    inContainerBranch = container is None
    for part in App.activeDocument().RootObjects:
        foundPlacement = getTargetPlacementRelativeTo(targetObj, part, container, inContainerBranch)
        if foundPlacement is not None:
            return foundPlacement

    return App.Placement()


def isThereOneRootAssembly():
    for part in App.activeDocument().RootObjects:
        if part.TypeId == "Assembly::AssemblyObject":
            return True
    return False


def getTargetPlacementRelativeTo(
    targetObj, part, container, inContainerBranch, ignorePlacement=False
):
    inContainerBranch = inContainerBranch or (not ignorePlacement and part == container)

    if targetObj == part and inContainerBranch and not ignorePlacement:
        return targetObj.Placement

    if part.TypeId == "App::DocumentObjectGroup":
        for obj in part.OutList:
            foundPlacement = getTargetPlacementRelativeTo(
                targetObj, obj, container, inContainerBranch, ignorePlacement
            )
            if foundPlacement is not None:
                return foundPlacement

    elif part.TypeId in {"App::Part", "Assembly::AssemblyObject", "PartDesign::Body"}:
        for obj in part.OutList:
            foundPlacement = getTargetPlacementRelativeTo(
                targetObj, obj, container, inContainerBranch
            )
            if foundPlacement is None:
                continue

            # If we were called from a link then we need to ignore this placement as we use the link placement instead.
            if not ignorePlacement:
                foundPlacement = part.Placement * foundPlacement

            return foundPlacement

    elif part.TypeId == "App::Link":
        linked_obj = part.getLinkedObject()

        if linked_obj.TypeId in {"App::Part", "Assembly::AssemblyObject", "PartDesign::Body"}:
            for obj in linked_obj.OutList:
                foundPlacement = getTargetPlacementRelativeTo(
                    targetObj, obj, container, inContainerBranch
                )
                if foundPlacement is None:
                    continue

                foundPlacement = part.Placement * foundPlacement
                return foundPlacement

        foundPlacement = getTargetPlacementRelativeTo(
            targetObj, linked_obj, container, inContainerBranch, True
        )

        if foundPlacement is not None and not ignorePlacement:
            foundPlacement = part.Placement * foundPlacement

        return foundPlacement

    return None


def getElementName(full_name):
    # full_name is "Assembly.Assembly1.Assembly2.Assembly3.Box.Edge16"
    # We want either Edge16.
    parts = full_name.split(".")

    if len(parts) < 3:
        # At minimum "Assembly.Box.edge16". It shouldn't be shorter
        return ""

    # case of PartDesign datums : CoordinateSystem, point, line, plane
    if parts[-1] in {"X", "Y", "Z", "Point", "Line", "Plane"}:
        return ""

    return parts[-1]


def getObjsNamesAndElement(obj_name, sub_name):
    # if obj_name = "Assembly" and sub_name = "Assembly1.Assembly2.Assembly3.Box.Edge16"
    # this will return ["Assembly","Assembly1","Assembly2","Assembly3","Box"] and "Edge16"

    parts = sub_name.split(".")

    # The last part is always the element name even if empty
    element_name = parts[-1]

    # The remaining parts are object names
    obj_names = parts[:-1]
    obj_names.insert(0, obj_name)

    return obj_names, element_name


def getFullObjName(obj_name, sub_name):
    # if obj_name = "Assembly" and sub_name = "Assembly1.Assembly2.Assembly3.Box.Edge16"
    # this will return "Assembly.Assembly1.Assembly2.Assembly3.Box"
    objs_names, element_name = getObjsNamesAndElement(obj_name, sub_name)
    return ".".join(objs_names)


def getFullElementName(obj_name, sub_name):
    # if obj_name = "Assembly" and sub_name = "Assembly1.Assembly2.Assembly3.Box.Edge16"
    # this will return "Assembly.Assembly1.Assembly2.Assembly3.Box.Edge16"
    return obj_name + "." + sub_name


def extract_type_and_number(element_name):
    element_type = ""
    element_number = ""

    for char in element_name:
        if char.isalpha():
            # If the character is a letter, it's part of the type
            element_type += char
        elif char.isdigit():
            # If the character is a digit, it's part of the number
            element_number += char
        else:
            break

    if element_type and element_number:
        element_number = int(element_number)
        return element_type, element_number
    else:
        return None, None


def findElementClosestVertex(selection_dict):
    obj = selection_dict["object"]

    mousePos = selection_dict["mouse_pos"]

    # We need mousePos to be relative to the part containing obj global placement
    if selection_dict["object"] != selection_dict["part"]:
        plc = App.Placement()
        plc.Base = mousePos
        global_plc = getGlobalPlacement(selection_dict["part"])
        plc = global_plc.inverse() * plc
        mousePos = plc.Base

    elt_type, elt_index = extract_type_and_number(selection_dict["element_name"])

    if elt_type == "Vertex":
        return selection_dict["element_name"]

    elif elt_type == "Edge":
        edge = obj.Shape.Edges[elt_index - 1]
        curve = edge.Curve
        if curve.TypeId == "Part::GeomCircle":
            # For centers, as they are not shape vertexes, we return the element name.
            # For now we only allow selecting the center of arcs / circles.
            return selection_dict["element_name"]

        edge_points = getPointsFromVertexes(edge.Vertexes)

        if curve.TypeId == "Part::GeomLine":
            # For lines we allow users to select the middle of lines as well.
            line_middle = (edge_points[0] + edge_points[1]) * 0.5
            edge_points.append(line_middle)

        closest_vertex_index, _ = findClosestPointToMousePos(edge_points, mousePos)

        if curve.TypeId == "Part::GeomLine" and closest_vertex_index == 2:
            # If line center is closest then we have no vertex name to set so we put element name
            return selection_dict["element_name"]

        vertex_name = findVertexNameInObject(edge.Vertexes[closest_vertex_index], obj)

        return vertex_name

    elif elt_type == "Face":
        face = obj.Shape.Faces[elt_index - 1]
        surface = face.Surface
        _type = surface.TypeId
        if _type == "Part::GeomSphere" or _type == "Part::GeomTorus":
            return selection_dict["element_name"]

        # Handle the circle/arc edges for their centers
        center_points = []
        center_points_edge_indexes = []
        edges = face.Edges

        for i, edge in enumerate(edges):
            curve = edge.Curve
            if curve.TypeId == "Part::GeomCircle" or curve.TypeId == "Part::GeomEllipse":
                center_points.append(curve.Location)
                center_points_edge_indexes.append(i)

            elif _type == "Part::GeomCylinder" and curve.TypeId == "Part::GeomBSplineCurve":
                # handle special case of 2 cylinder intersecting.
                for j, facej in enumerate(obj.Shape.Faces):
                    surfacej = facej.Surface
                    if (elt_index - 1) != j and surfacej.TypeId == "Part::GeomCylinder":
                        for edgej in facej.Edges:
                            if edgej.Curve.TypeId == "Part::GeomBSplineCurve":
                                if (
                                    edgej.CenterOfGravity == edge.CenterOfGravity
                                    and edgej.Length == edge.Length
                                ):
                                    center_points.append(edgej.CenterOfGravity)
                                    center_points_edge_indexes.append(i)

        if len(center_points) > 0:
            closest_center_index, closest_center_distance = findClosestPointToMousePos(
                center_points, mousePos
            )

        # Handle the face vertexes
        face_points = []

        if _type != "Part::GeomCylinder" and _type != "Part::GeomCone":
            face_points = getPointsFromVertexes(face.Vertexes)

        # We also allow users to select the center of gravity.
        if _type == "Part::GeomCylinder" or _type == "Part::GeomCone":
            centerOfG = face.CenterOfGravity - surface.Center
            centerPoint = surface.Center + centerOfG
            centerPoint = centerPoint + App.Vector().projectToLine(centerOfG, surface.Axis)
            face_points.append(centerPoint)
        else:
            face_points.append(face.CenterOfGravity)

        closest_vertex_index, closest_vertex_distance = findClosestPointToMousePos(
            face_points, mousePos
        )

        if len(center_points) > 0:
            if closest_center_distance < closest_vertex_distance:
                # Note the index here is the index within the face! Not the object.
                index = center_points_edge_indexes[closest_center_index] + 1
                return "Edge" + str(index)

        if _type == "Part::GeomCylinder" or _type == "Part::GeomCone":
            return selection_dict["element_name"]

        if closest_vertex_index == len(face.Vertexes):
            # If center of gravity then we have no vertex name to set so we put element name
            return selection_dict["element_name"]

        vertex_name = findVertexNameInObject(face.Vertexes[closest_vertex_index], obj)

        return vertex_name

    return ""


def getPointsFromVertexes(vertexes):
    points = []
    for vtx in vertexes:
        points.append(vtx.Point)
    return points


def findClosestPointToMousePos(candidates_points, mousePos):
    closest_point_index = None
    point_min_length = None

    for i, point in enumerate(candidates_points):
        length = (mousePos - point).Length
        if closest_point_index is None or length < point_min_length:
            closest_point_index = i
            point_min_length = length

    return closest_point_index, point_min_length


def findVertexNameInObject(vertex, obj):
    for i, vtx in enumerate(obj.Shape.Vertexes):
        if vtx.Point == vertex.Point:
            return "Vertex" + str(i + 1)
    return ""


def color_from_unsigned(c):
    return [
        float(int((c >> 24) & 0xFF) / 255),
        float(int((c >> 16) & 0xFF) / 255),
        float(int((c >> 8) & 0xFF) / 255),
    ]


def getJointGroup(assembly):
    joint_group = None

    for obj in assembly.OutList:
        if obj.TypeId == "Assembly::JointGroup":
            joint_group = obj
            break

    if not joint_group:
        joint_group = assembly.newObject("Assembly::JointGroup", "Joints")

    return joint_group


def isAssemblyGrounded():
    assembly = activeAssembly()
    if not assembly:
        return False

    jointGroup = getJointGroup(assembly)

    for joint in jointGroup.Group:
        if hasattr(joint, "ObjectToGround"):
            return True

    return False


def removeObjAndChilds(obj):
    removeObjsAndChilds([obj])


def removeObjsAndChilds(objs):
    def addsubobjs(obj, toremoveset):
        if obj.TypeId == "App::Origin":  # Origins are already handled
            return

        toremoveset.add(obj)
        if obj.TypeId != "App::Link":
            for subobj in obj.OutList:
                addsubobjs(subobj, toremoveset)

    toremove = set()
    for obj in objs:
        addsubobjs(obj, toremove)

    for obj in toremove:
        if obj:
            obj.Document.removeObject(obj.Name)
