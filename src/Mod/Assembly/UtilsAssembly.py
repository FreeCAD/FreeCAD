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

import PySide.QtCore as QtCore
import PySide.QtGui as QtGui


# translate = App.Qt.translate

__title__ = "Assembly utilitary functions"
__author__ = "Ondsel"
__url__ = "https://www.freecad.org"


def activePartOrAssembly():
    doc = Gui.ActiveDocument

    if doc is None or doc.ActiveView is None:
        return None

    return doc.ActiveView.getActiveObject("part")


def activeAssembly():
    active_assembly = activePartOrAssembly()
    if active_assembly is not None and active_assembly.isDerivedFrom("Assembly::AssemblyObject"):
        return active_assembly

    return None


def activePart():
    active_part = activePartOrAssembly()

    if active_part is not None and not active_part.isDerivedFrom("Assembly::AssemblyObject"):
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
        assembly = activePart()
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
                    obj2 = None
                    for obji in linked_obj.OutList:
                        if obji.Name == names[i + 1]:
                            obj2 = obji
                            break
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
                obj2 = None
                for obji in obj.OutList:
                    if obji.Name == names[i + 1]:
                        obj2 = obji
                        break
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


def getContainingPart(full_name, selected_object, activeAssemblyOrPart=None):
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
            if selected_object in obj.OutListRecursive:
                return obj

        # Note here we may want to specify a specific behavior for Assembly::AssemblyObject.
        if obj.TypeId == "App::Part":
            if selected_object in obj.OutListRecursive:
                if not activeAssemblyOrPart:
                    return obj
                elif activeAssemblyOrPart in obj.OutListRecursive or obj == activeAssemblyOrPart:
                    continue
                else:
                    return obj

        elif obj.TypeId == "App::Link":
            linked_obj = obj.getLinkedObject()
            if linked_obj.TypeId == "PartDesign::Body" and isBodySubObject(selected_object.TypeId):
                if selected_object in linked_obj.OutListRecursive:
                    return obj
            if linked_obj.TypeId == "App::Part":
                # linked_obj_doc = linked_obj.Document
                # selected_obj_in_doc = doc.getObject(selected_object.Name)
                if selected_object in linked_obj.OutListRecursive:
                    if not activeAssemblyOrPart:
                        return obj
                    elif (linked_obj.Document == activeAssemblyOrPart.Document) and (
                        activeAssemblyOrPart in linked_obj.OutListRecursive
                        or linked_obj == activeAssemblyOrPart
                    ):
                        continue
                    else:
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
        for obji in part.OutListRecursive:
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
    if targetObj is None:
        return App.Placement()

    inContainerBranch = container is None
    for rootObj in App.activeDocument().RootObjects:
        foundPlacement = getTargetPlacementRelativeTo(
            targetObj, rootObj, container, inContainerBranch
        )
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
        if part == linked_obj or linked_obj is None:
            return None  # upon loading this can happen for external links.

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


# This function returns all the objects within the argument that can move:
# - Part::Features (outside of parts)
# - App::parts
# - App::Links to Part::Features or App::parts.
# It does not include Part::Features that are within App::Parts.
# It includes things inside Groups.
def getMovablePartsWithin(group, partsAsSolid=False):
    parts = []
    for obj in group.OutList:
        parts = parts + getSubMovingParts(obj, partsAsSolid)
    return parts


def getSubMovingParts(obj, partsAsSolid):
    if obj.isDerivedFrom("Part::Feature"):
        return [obj]

    elif obj.isDerivedFrom("App::Part"):
        objs = []
        if not partsAsSolid:
            objs = getMovablePartsWithin(obj)
        objs.append(obj)
        return objs

    elif obj.TypeId == "App::DocumentObjectGroup":
        return getMovablePartsWithin(obj)

    if obj.TypeId == "App::Link":
        linked_obj = obj.getLinkedObject()
        if linked_obj.TypeId == "App::Part" or linked_obj.isDerivedFrom("Part::Feature"):
            return [obj]

    return []


# Find the center of mass of a list of parts.
# Note it could be useful to move this to Measure mod.
def getCenterOfMass(parts):
    total_mass = 0
    total_com = App.Vector(0, 0, 0)

    for part in parts:
        mass, com = getObjMassAndCom(part)
        total_mass += mass
        total_com += com

    # After all parts are processed, calculate the overall center of mass
    if total_mass > 0:  # Avoid division by zero
        overall_com = total_com / total_mass
    else:
        overall_com = App.Vector(0, 0, 0)  # Default if no mass is found

    return overall_com


def getObjMassAndCom(obj, containingPart=None):
    link_global_plc = None

    if obj.TypeId == "App::Link":
        link_global_plc = getGlobalPlacement(obj, containingPart)
        obj = obj.getLinkedObject()

    if obj.TypeId == "PartDesign::Body":
        part = part.Tip

    if obj.isDerivedFrom("Part::Feature"):
        mass = obj.Shape.Volume
        com = obj.Shape.CenterOfGravity
        # com takes into account obj placement, but not container placements.
        # So we make com relative to the obj :
        comPlc = App.Placement()
        comPlc.Base = com
        comPlc = obj.Placement.inverse() * comPlc
        # Then we make it relative to the origin of the doc
        global_plc = App.Placement()
        if link_global_plc is None:
            global_plc = getGlobalPlacement(obj, containingPart)
        else:
            global_plc = link_global_plc

        comPlc = global_plc * comPlc

        com = comPlc.Base * mass
        return mass, com

    elif obj.isDerivedFrom("App::Part") or obj.isDerivedFrom("App::DocumentObjectGroup"):
        if containingPart is None and obj.isDerivedFrom("App::Part"):
            containingPart = obj

        total_mass = 0
        total_com = App.Vector(0, 0, 0)

        for subObj in obj.OutList:
            mass, com = getObjMassAndCom(subObj, containingPart)
            total_mass += mass
            total_com += com
        return total_mass, total_com
    return 0, App.Vector(0, 0, 0)


def getCenterOfBoundingBox(objs, parts):
    i = 0
    center = App.Vector()
    for obj, part in zip(objs, parts):
        viewObject = obj.ViewObject
        if viewObject is None:
            continue
        boundingBox = viewObject.getBoundingBox()
        if boundingBox is None:
            continue
        bboxCenter = boundingBox.Center
        if part != obj:
            # bboxCenter does not take into account obj global placement
            plc = App.Placement(bboxCenter, App.Rotation())
            # change plc to be relative to the object placement.
            plc = obj.Placement.inverse() * plc
            # change plc to be relative to the origin of the document.
            global_plc = getGlobalPlacement(obj, part)
            plc = global_plc * plc
            bboxCenter = plc.Base

        center = center + bboxCenter
        i = i + 1

    if i != 0:
        center = center / i
    return center


def findCylindersIntersection(obj, surface, edge, elt_index):
    for j, facej in enumerate(obj.Shape.Faces):
        surfacej = facej.Surface
        if (elt_index - 1) == j or surfacej.TypeId != "Part::GeomCylinder":
            continue

        for edgej in facej.Edges:
            if (
                edgej.Curve.TypeId == "Part::GeomBSplineCurve"
                and edgej.CenterOfGravity == edge.CenterOfGravity
                and edgej.Length == edge.Length
            ):
                # we need intersection between the 2 cylinder axis.
                line1 = Part.Line(surface.Center, surface.Center + surface.Axis)
                line2 = Part.Line(surfacej.Center, surfacej.Center + surfacej.Axis)

                res = line1.intersect(line2, Part.Precision.confusion())

                if res:
                    return App.Vector(res[0].X, res[0].Y, res[0].Z)
    return surface.Center


def applyOffsetToPlacement(plc, offset):
    plc.Base = plc.Base + plc.Rotation.multVec(offset)
    return plc


def applyRotationToPlacement(plc, angle):
    return applyRotationToPlacementAlongAxis(plc, angle, App.Vector(0, 0, 1))


def applyRotationToPlacementAlongAxis(plc, angle, axis):
    rot = plc.Rotation
    zRotation = App.Rotation(axis, angle)
    plc.Rotation = rot * zRotation
    return plc


def flipPlacement(plc):
    return applyRotationToPlacementAlongAxis(plc, 180, App.Vector(1, 0, 0))


"""
So here we want to find a placement that corresponds to a local coordinate system that would be placed at the selected vertex.
- obj is usually a App::Link to a PartDesign::Body, or primitive, fasteners. But can also be directly the object.1
- elt can be a face, an edge or a vertex.
- If elt is a vertex, then vtx = elt And placement is vtx coordinates without rotation.
- if elt is an edge, then vtx = edge start/end vertex depending on which is closer. If elt is an arc or circle, vtx can also be the center. The rotation is the plane normal to the line positioned at vtx. Or for arcs/circle, the plane of the arc.
- if elt is a plane face, vtx is the face vertex (to the list of vertex we need to add arc/circle centers) the closer to the mouse. The placement is the plane rotation positioned at vtx
- if elt is a cylindrical face, vtx can also be the center of the arcs of the cylindrical face.
"""


def findPlacement(obj, part, elt, vtx, ignoreVertex=False):
    if not obj or not part:
        return App.Placement()

    if not elt or not vtx:
        # case of whole parts such as PartDesign::Body or PartDesign::CordinateSystem/Point/Line/Plane.
        return App.Placement()

    plc = App.Placement()

    elt_type, elt_index = extract_type_and_number(elt)
    vtx_type, vtx_index = extract_type_and_number(vtx)

    isLine = False

    if elt_type == "Vertex":
        vertex = obj.Shape.Vertexes[elt_index - 1]
        plc.Base = (vertex.X, vertex.Y, vertex.Z)
    elif elt_type == "Edge":
        edge = obj.Shape.Edges[elt_index - 1]
        curve = edge.Curve

        # First we find the translation
        if vtx_type == "Edge" or ignoreVertex:
            # In this case the wanted vertex is the center.
            if curve.TypeId == "Part::GeomCircle":
                center_point = curve.Location
                plc.Base = (center_point.x, center_point.y, center_point.z)
            elif curve.TypeId == "Part::GeomLine":
                edge_points = getPointsFromVertexes(edge.Vertexes)
                line_middle = (edge_points[0] + edge_points[1]) * 0.5
                plc.Base = line_middle
        else:
            vertex = obj.Shape.Vertexes[vtx_index - 1]
            plc.Base = (vertex.X, vertex.Y, vertex.Z)

        # Then we find the Rotation
        if curve.TypeId == "Part::GeomCircle":
            plc.Rotation = App.Rotation(curve.Rotation)

        if curve.TypeId == "Part::GeomLine":
            isLine = True
            plane_normal = curve.Direction
            plane_origin = App.Vector(0, 0, 0)
            plane = Part.Plane(plane_origin, plane_normal)
            plc.Rotation = App.Rotation(plane.Rotation)
    elif elt_type == "Face":
        face = obj.Shape.Faces[elt_index - 1]
        surface = face.Surface

        # First we find the translation
        if vtx_type == "Face" or ignoreVertex:
            if surface.TypeId == "Part::GeomCylinder" or surface.TypeId == "Part::GeomCone":
                centerOfG = face.CenterOfGravity - surface.Center
                centerPoint = surface.Center + centerOfG
                centerPoint = centerPoint + App.Vector().projectToLine(centerOfG, surface.Axis)
                plc.Base = centerPoint
            elif surface.TypeId == "Part::GeomTorus" or surface.TypeId == "Part::GeomSphere":
                plc.Base = surface.Center
            else:
                plc.Base = face.CenterOfGravity
        elif vtx_type == "Edge":
            # In this case the edge is a circle/arc and the wanted vertex is its center.
            edge = face.Edges[vtx_index - 1]
            curve = edge.Curve
            if curve.TypeId == "Part::GeomCircle":
                center_point = curve.Location
                plc.Base = (center_point.x, center_point.y, center_point.z)

            elif (
                surface.TypeId == "Part::GeomCylinder" and curve.TypeId == "Part::GeomBSplineCurve"
            ):
                # handle special case of 2 cylinder intersecting.
                plc.Base = findCylindersIntersection(obj, surface, edge, elt_index)

        else:
            vertex = obj.Shape.Vertexes[vtx_index - 1]
            plc.Base = (vertex.X, vertex.Y, vertex.Z)

        # Then we find the Rotation
        if surface.TypeId == "Part::GeomPlane":
            plc.Rotation = App.Rotation(surface.Rotation)
        else:
            plc.Rotation = surface.Rotation

    # Now plc is the placement relative to the origin determined by the object placement.
    # But it does not take into account Part placements. So if the solid is in a part and
    # if the part has a placement then plc is wrong.

    # change plc to be relative to the object placement.
    plc = obj.Placement.inverse() * plc

    # post-process of plc for some special cases
    if elt_type == "Vertex":
        plc.Rotation = App.Rotation()
    elif isLine:
        plane_normal = plc.Rotation.multVec(App.Vector(0, 0, 1))
        plane_origin = App.Vector(0, 0, 0)
        plane = Part.Plane(plane_origin, plane_normal)
        plc.Rotation = App.Rotation(plane.Rotation)

    # change plc to be relative to the origin of the document.
    # global_plc = getGlobalPlacement(obj, part)
    # plc = global_plc * plc

    # change plc to be relative to the assembly.
    # plc = activeAssembly().Placement.inverse() * plc

    return plc
