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

import math

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


def getObject(ref):
    if len(ref) != 2:
        return None
    subs = ref[1]
    if len(subs) < 1:
        return None
    sub_name = subs[0]

    # sub_name is "LinkOrAssembly1.LinkOrPart1.LinkOrBox.Edge16"
    # or           "LinkOrAssembly1.LinkOrPart1.LinkOrBody.pad.Edge16"
    # or           "Assembly.LinkOrAssembly1.LinkOrPart1.LinkOrBody.Local_CS.X"
    # We want either LinkOrBody or LinkOrBox or Local_CS.
    names = sub_name.split(".")

    if len(names) < 2:
        return None

    doc = ref[0].Document

    for i, obj_name in enumerate(names):
        obj = doc.getObject(obj_name)

        if obj is None:
            return None

        # the last is the element name. So if we are at the last but one name, then it must be the selected
        if i == len(names) - 2:
            return obj

        if obj.TypeId in {"App::Part", "Assembly::AssemblyObject"}:
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

        elif obj.TypeId == "App::Link":
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
                doc = linked_obj.Document
                continue

    return None


def isBodySubObject(typeId):
    return (
        typeId == "Sketcher::SketchObject"
        or typeId == "PartDesign::Point"
        or typeId == "PartDesign::Line"
        or typeId == "PartDesign::Plane"
        or typeId == "PartDesign::CoordinateSystem"
    )


# To be deprecated. CommandCreateView needs to stop using it.
def getContainingPart(full_name, selected_object, activeAssemblyOrPart=None):
    # full_name is "Assembly.Assembly1.LinkOrPart1.LinkOrBox.Edge16" -> LinkOrPart1
    # or           "Assembly.Assembly1.LinkOrPart1.LinkOrBody.pad.Edge16" -> LinkOrPart1
    # or           "Assembly.Assembly1.LinkOrPart1.LinkOrBody.Sketch.Edge1" -> LinkOrPart1

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
                    # If the user put the assembly inside a Part, then we ignore it.
                    continue
                else:
                    return obj

        elif obj.TypeId == "App::Link":
            linked_obj = obj.getLinkedObject()
            if linked_obj.TypeId == "PartDesign::Body" and isBodySubObject(selected_object.TypeId):
                if selected_object in linked_obj.OutListRecursive:
                    return obj
            if linked_obj.TypeId in ["App::Part", "Assembly::AssemblyObject"]:
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


# To be deprecated. Kept for migrationScript.
def getObjectInPart(objName, part):
    if part is None:
        return None

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


# Used by migrationScript.
def getRootPath(obj, part):
    sels = obj.Parents
    for sel in sels:
        rootObj = sel[0]
        # The part and the rootObj should be in the same doc
        if rootObj.Document.Name != part.Document.Name:
            continue

        path = sel[1]
        # we need to check that the part name is in the list.
        names = path.split(".")
        if part.Name not in names:
            continue

        # for bodies we need to add the tip to the path.
        if obj.TypeId == "PartDesign::Body":
            path = path + obj.Tip.Name + "."

        return rootObj, path

    return None, ""


# get the placement of Obj relative to its moving Part
# Example : assembly.part1.part2.partn.body1 : placement of Obj relative to part1
def getObjPlcRelativeToPart(assembly, ref):
    # we need plc to be relative to the moving part
    moving_part = getMovingPart(assembly, ref)
    obj_global_plc = getGlobalPlacement(ref)
    part_global_plc = getGlobalPlacement(ref, moving_part)

    return part_global_plc.inverse() * obj_global_plc


# Example : assembly.part1.part2.partn.body1 : jcsPlc is relative to body1
# This function returns jcsPlc relative to part1
def getJcsPlcRelativeToPart(assembly, jcsPlc, ref):
    obj_relative_plc = getObjPlcRelativeToPart(assembly, ref)
    return obj_relative_plc * jcsPlc


# Return the jcs global placement
def getJcsGlobalPlc(jcsPlc, ref):
    obj_global_plc = getGlobalPlacement(ref)
    return obj_global_plc * jcsPlc


def getGlobalPlacement(ref, targetObj=None):
    if not isRefValid(ref, 1):
        return App.Placement()

    if targetObj is None:  # If no targetObj is given, we consider it's the getObject(ref)
        targetObj = getObject(ref)

    if targetObj is None:
        return App.Placement()

    rootObj = ref[0]
    names = ref[1][0].split(".")

    doc = rootObj.Document
    plc = rootObj.Placement

    for objName in names:
        obj = doc.getObject(objName)
        if not obj:
            continue

        plc = plc * obj.Placement

        if obj == targetObj:
            return plc

        if obj.TypeId == "App::Link":
            linked_obj = obj.getLinkedObject()
            doc = linked_obj.Document  # in case its an external link.

    # If targetObj has not been found there's a problem
    return App.Placement()


def isThereOneRootAssembly():
    for part in Gui.activeDocument().TreeRootObjects:
        if part.TypeId == "Assembly::AssemblyObject":
            return True
    return False


def getElementName(full_name):
    # full_name is "Assembly.Assembly1.Assembly2.Assembly3.Box.Edge16"
    # We want either Edge16.
    parts = full_name.split(".")

    if len(parts) < 2:
        # At minimum "Box.edge16". It shouldn't be shorter
        return ""

    # case of PartDesign datums : CoordinateSystem, point, line, plane
    if parts[-1] in {"X", "Y", "Z", "Point", "Line", "Plane"}:
        return ""

    # Case of origin objects
    if parts[-1] == "":
        if "X_Axis" in parts[-2]:
            return "X_Axis"
        if "Y_Axis" in parts[-2]:
            return "Y_Axis"
        if "Z_Axis" in parts[-2]:
            return "Z_Axis"
        if "XY_Plane" in parts[-2]:
            return "XY_Plane"
        if "XZ_Plane" in parts[-2]:
            return "XZ_Plane"
        if "YZ_Plane" in parts[-2]:
            return "YZ_Plane"

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


def findElementClosestVertex(assembly, ref, mousePos):
    element_name = getElementName(ref[1][0])
    if element_name == "":
        return ""

    moving_part = getMovingPart(assembly, ref)
    obj = getObject(ref)

    # We need mousePos to be relative to the part containing obj global placement
    if obj != moving_part:
        plc = App.Placement()
        plc.Base = mousePos
        global_plc = getGlobalPlacement(ref)
        plc = global_plc.inverse() * plc  # We make it relative to obj Origin
        plc = obj.Placement * plc  # Make plc in the same lcs as obj
        mousePos = plc.Base

    elt_type, elt_index = extract_type_and_number(element_name)

    if elt_type == "Vertex":
        return element_name

    elif elt_type == "Edge":
        edge = obj.Shape.Edges[elt_index - 1]
        curve = edge.Curve
        if curve.TypeId == "Part::GeomCircle":
            # For centers, as they are not shape vertexes, we return the element name.
            # For now we only allow selecting the center of arcs / circles.
            return element_name

        edge_points = getPointsFromVertexes(edge.Vertexes)

        if curve.TypeId == "Part::GeomLine":
            # For lines we allow users to select the middle of lines as well.
            line_middle = (edge_points[0] + edge_points[1]) * 0.5
            edge_points.append(line_middle)

        closest_vertex_index, _ = findClosestPointToMousePos(edge_points, mousePos)

        if curve.TypeId == "Part::GeomLine" and closest_vertex_index == 2:
            # If line center is closest then we have no vertex name to set so we put element name
            return element_name

        vertex_name = findVertexNameInObject(edge.Vertexes[closest_vertex_index], obj)

        return vertex_name

    elif elt_type == "Face":
        face = obj.Shape.Faces[elt_index - 1]
        surface = face.Surface
        _type = surface.TypeId
        if _type == "Part::GeomSphere" or _type == "Part::GeomTorus":
            return element_name

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
            return element_name

        if closest_vertex_index == len(face.Vertexes):
            # If center of gravity then we have no vertex name to set so we put element name
            return element_name

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


def getBomGroup(assembly):
    bom_group = None

    for obj in assembly.OutList:
        if obj.TypeId == "Assembly::BomGroup":
            bom_group = obj
            break

    if not bom_group:
        bom_group = assembly.newObject("Assembly::BomGroup", "Bills of Materials")

    return bom_group


def getJointGroup(assembly):
    joint_group = None

    for obj in assembly.OutList:
        if obj.TypeId == "Assembly::JointGroup":
            joint_group = obj
            break

    if not joint_group:
        joint_group = assembly.newObject("Assembly::JointGroup", "Joints")

    return joint_group


def getViewGroup(assembly):
    view_group = None

    for obj in assembly.OutList:
        if obj.TypeId == "Assembly::ViewGroup":
            view_group = obj
            break

    if not view_group:
        view_group = assembly.newObject("Assembly::ViewGroup", "Exploded Views")

    return view_group


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


def getCenterOfBoundingBox(objs, refs):
    i = 0
    center = App.Vector()
    for obj, ref in zip(objs, refs):
        viewObject = obj.ViewObject
        if viewObject is None:
            continue
        boundingBox = viewObject.getBoundingBox()
        if boundingBox is None:
            continue
        bboxCenter = boundingBox.Center

        # bboxCenter does not take into account obj global placement
        plc = App.Placement(bboxCenter, App.Rotation())
        # change plc to be relative to the object placement.
        plc = obj.Placement.inverse() * plc
        # change plc to be relative to the origin of the document.
        global_plc = getGlobalPlacement(ref, obj)
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


def arePlacementSameDir(plc1, plc2):
    zAxis1 = plc1.Rotation.multVec(App.Vector(0, 0, 1))
    zAxis2 = plc2.Rotation.multVec(App.Vector(0, 0, 1))
    return zAxis1.dot(zAxis2) > 0


def arePlacementZParallel(plc1, plc2):
    zAxis1 = plc1.Rotation.multVec(App.Vector(0, 0, 1))
    zAxis2 = plc2.Rotation.multVec(App.Vector(0, 0, 1))
    return zAxis1.cross(zAxis2).Length < 1e-06


"""
So here we want to find a placement that corresponds to a local coordinate system that would be placed at the selected vertex.
- obj is usually a App::Link to a PartDesign::Body, or primitive, fasteners. But can also be directly the object.1
- elt can be a face, an edge or a vertex.
- If elt is a vertex, then vtx = elt And placement is vtx coordinates without rotation.
- if elt is an edge, then vtx = edge start/end vertex depending on which is closer. If elt is an arc or circle, vtx can also be the center. The rotation is the plane normal to the line positioned at vtx. Or for arcs/circle, the plane of the arc.
- if elt is a plane face, vtx is the face vertex (to the list of vertex we need to add arc/circle centers) the closer to the mouse. The placement is the plane rotation positioned at vtx
- if elt is a cylindrical face, vtx can also be the center of the arcs of the cylindrical face.
"""


def findPlacement(ref, ignoreVertex=False):
    if not isRefValid(ref, 2):
        return App.Placement()
    obj = getObject(ref)
    if not obj:
        return App.Placement()

    elt = getElementName(ref[1][0])
    vtx = getElementName(ref[1][1])

    # case of origin objects.
    if elt == "X_Axis" or elt == "YZ_Plane":
        return App.Placement(App.Vector(), App.Rotation(App.Vector(0, 1, 0), -90))
    if elt == "Y_Axis" or elt == "XZ_Plane":
        return App.Placement(App.Vector(), App.Rotation(App.Vector(1, 0, 0), 90))
    if elt == "Z_Axis" or elt == "XY_Plane":
        return App.Placement()

    if not elt or not vtx:
        # case of whole parts such as PartDesign::Body or PartDesign::CordinateSystem/Point/Line/Plane.
        return App.Placement()

    plc = App.Placement()

    elt_type, elt_index = extract_type_and_number(elt)
    vtx_type, vtx_index = extract_type_and_number(vtx)

    isLine = False

    if elt_type == "Vertex":
        vertex = get_element(obj.Shape.Vertexes, elt_index, elt)
        if vertex is None:
            return App.Placement()
        plc.Base = (vertex.X, vertex.Y, vertex.Z)
    elif elt_type == "Edge":
        edge = get_element(obj.Shape.Edges, elt_index, elt)
        if edge is None:
            return App.Placement()

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
            vertex = get_element(obj.Shape.Vertexes, vtx_index, vtx)
            if vertex is None:
                return App.Placement()

            plc.Base = (vertex.X, vertex.Y, vertex.Z)

        # Then we find the Rotation
        if curve.TypeId == "Part::GeomCircle":
            plc.Rotation = App.Rotation(curve.Rotation)

        if curve.TypeId == "Part::GeomLine":
            isLine = True
            plane_normal = round_vector(curve.Direction)
            plane_origin = App.Vector(0, 0, 0)
            plane = Part.Plane(plane_origin, plane_normal)
            plc.Rotation = App.Rotation(plane.Rotation)
    elif elt_type == "Face":
        face = get_element(obj.Shape.Faces, elt_index, elt)
        if face is None:
            return App.Placement()

        surface = face.Surface

        # First we find the translation
        if vtx_type == "Face" or ignoreVertex:
            if surface.TypeId == "Part::GeomCylinder":
                centerOfG = face.CenterOfGravity - surface.Center
                centerPoint = surface.Center + centerOfG
                centerPoint = centerPoint + App.Vector().projectToLine(centerOfG, surface.Axis)
                plc.Base = centerPoint
            elif surface.TypeId == "Part::GeomTorus" or surface.TypeId == "Part::GeomSphere":
                plc.Base = surface.Center
            elif surface.TypeId == "Part::GeomCone":
                plc.Base = surface.Apex
            else:
                plc.Base = face.CenterOfGravity
        elif vtx_type == "Edge":
            # In this case the edge is a circle/arc and the wanted vertex is its center.
            edge = get_element(face.Edges, vtx_index, vtx)
            if edge is None:
                return App.Placement()

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
            vertex = get_element(obj.Shape.Vertexes, vtx_index, vtx)
            if vertex is None:
                return App.Placement()

            plc.Base = (vertex.X, vertex.Y, vertex.Z)

        # Then we find the Rotation
        if hasattr(surface, "Rotation") and surface.Rotation is not None:
            plc.Rotation = App.Rotation(surface.Rotation)

    # Now plc is the placement relative to the origin determined by the object placement.
    # But it does not take into account Part placements. So if the solid is in a part and
    # if the part has a placement then plc is wrong.

    # change plc to be relative to the object placement.
    plc = obj.Placement.inverse() * plc

    # post-process of plc for some special cases
    if elt_type == "Vertex":
        plc.Rotation = App.Rotation()
    elif isLine:
        plane_normal = round_vector(plc.Rotation.multVec(App.Vector(0, 0, 1)))
        plane_origin = App.Vector(0, 0, 0)
        plane = Part.Plane(plane_origin, plane_normal)
        plc.Rotation = App.Rotation(plane.Rotation)

    # change plc to be relative to the origin of the document.
    # global_plc = getGlobalPlacement(obj, part)
    # plc = global_plc * plc

    # change plc to be relative to the assembly.
    # plc = activeAssembly().Placement.inverse() * plc

    return plc


def get_element(shape_elements, index, sub):
    if index - 1 < 0 or index - 1 >= len(shape_elements):
        print(f"Joint Corrupted: Index of {sub} out of bound.")
        return None
    return shape_elements[index - 1]


def isRefValid(ref, number_sub):
    if ref is None:
        return False
    if len(ref) != 2:
        return False
    if len(ref[1]) < number_sub:
        return False

    return True


def round_vector(v, decimals=10):
    """Round each component of the vector to a specified number of decimal places."""
    return App.Vector(round(v.x, decimals), round(v.y, decimals), round(v.z, decimals))


def saveAssemblyPartsPlacements(assembly):
    initialPlcs = {}
    assemblyParts = getMovablePartsWithin(assembly)
    for part in assemblyParts:
        initialPlcs[part.Name] = part.Placement

    return initialPlcs


def restoreAssemblyPartsPlacements(assembly, initialPlcs):
    assemblyParts = getMovablePartsWithin(assembly)
    for part in assemblyParts:
        if part.Name in initialPlcs:
            part.Placement = initialPlcs[part.Name]


def getComAndSize(assembly):
    if assembly.ViewObject is None:
        # these vars use the bounding box which is only available in gui...
        # We could use the real center of mass, but it's too slow to compute it
        return App.Vector(), 100

    bbox = assembly.ViewObject.getBoundingBox()
    if not bbox.isValid():
        return App.Vector(), 100

    com = bbox.Center
    size = bbox.DiagonalLength
    return com, size


def getAssemblyShapes(assembly):
    shapes = []
    assemblyParts = getMovablePartsWithin(assembly)
    for part in assemblyParts:
        shapes.append(part.Shape)

    return shapes


def getJointDistance(joint):
    plc1 = getJcsGlobalPlc(joint.Placement1, joint.Reference1)
    plc2 = getJcsGlobalPlc(joint.Placement2, joint.Reference2)

    # Find the sign
    sign = 1
    plc3 = plc1.inverse() * plc2  # plc3 is plc2 relative to plc1
    if plc3.Base.z < 0:
        sign = -1

    return sign * (plc1.Base - plc2.Base).Length


def getJointXYAngle(joint):
    plc1 = getJcsGlobalPlc(joint.Placement1, joint.Reference1)
    plc2 = getJcsGlobalPlc(joint.Placement2, joint.Reference2)

    plc3 = plc1.inverse() * plc2  # plc3 is plc2 relative to plc1
    x_axis = plc3.Rotation.multVec(App.Vector(1, 0, 0))

    return math.atan2(x_axis.y, x_axis.x)


def getMovingPart(assembly, ref):
    # ref can be :
    # [assembly, ['box.edge1', 'box.vertex2']]
    # [Part, ['Assembly.box.edge1', 'Assembly.box.vertex2']]
    # [assembly, ['Body.Pad.edge1', 'Body.Pad.vertex2']]

    if assembly is None or ref is None or len(ref) != 2:
        return None

    obj = ref[0]
    subs = ref[1]

    if subs is None or len(subs) < 1:
        return None

    sub = ref[1][0]  # All subs should have the same object paths.
    names = [obj.Name] + sub.split(".")

    try:
        index = names.index(assembly.Name)
        # Get the sublist starting after the after the assembly (in case of Part1/Assembly/...)
        names = names[index + 1 :]
    except ValueError:
        return None

    doc = assembly.Document

    if len(names) < 2:
        App.Console.PrintError(
            "getMovingPart() in UtilsAssembly.py the object name is too short, at minimum it should be something like ['Box','edge16']. It shouldn't be shorter"
        )
        return None

    for objName in names:
        obj = doc.getObject(objName)

        if not obj:
            continue

        if obj.TypeId == "App::DocumentObjectGroup":
            continue  # we ignore groups.

        return obj

    return None


def truncateSubAtFirst(sub, target):
    # target=part1 & sub=asm.part1.link1.part1.obj -> asm.part1.
    names = sub.split(".")
    sub = ""
    for name in names:
        sub = sub + name + "."
        if name == target:
            break

    return sub


def truncateSubAtLast(sub, target):
    # target=part1 & sub=asm.part1.link1.part1.obj -> asm.part1.link1.part1.
    names = sub.split(".")
    sub = ""
    target_indices = [i for i, name in enumerate(names) if name == target]

    if target_indices:
        last_index = target_indices[-1]
        for i, name in enumerate(names):
            sub += name + "."
            if i == last_index:
                break

    return sub


def swapElNameInSubname(sub_name, new_elName):
    # turns assembly.box.edge1 into assembly.box.new_elName
    names = sub_name.split(".")

    # Replace the last element
    names[-1] = new_elName

    # Join the names back together
    modified_sub = ".".join(names)

    return modified_sub


def addVertexToReference(ref, vertex_name):
    # Turns [obj, ['box.face1']] and 'vertex1' into [obj, ['box.face1', 'box.vertex1']]
    if len(ref) == 2:
        subs = ref[1]
        if len(subs) > 0:
            sub_name = subs[0]
            vertex_full_sub = swapElNameInSubname(sub_name, vertex_name)
            if len(subs) == 2:  # Update the vertex sub
                subs[1] = vertex_full_sub
            else:
                subs.append(vertex_full_sub)

            ref = [ref[0], subs]

    return ref
