# ######################################################################
#
#  SheetMetalCmd.py
#
#  Copyright 2015 Shai Seger <shaise at gmail dot com>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#  MA 02110-1301, USA.
#
#
# ######################################################################

import math

from PySide import QtCore, QtGui

import FreeCAD
import Part

import SheetMetalTools

# IMPORTANT: please remember to change the element map version in case
# of any changes in modeling logic.
smElementMapVersion = "sm1."

smEpsilon = SheetMetalTools.smEpsilon

# List of properties to be saved as defaults.
smAddWallDefaultVars = ["BendType", "LengthSpec", ("angle", "defaultAngle"),
                        ("radius", "defaultRadius"), "AutoMiter", ("kfactor", "defaultKFactor")]

translate = FreeCAD.Qt.translate


# Calculations for Angle and Offset face reference modes.
def GetSMComparisonFace(smObj, smSelItemName):
    """Find the sheet metal face reference to find the angle and offset
    with another face.
    Find the thickness edge coincident with sheet metal reference face.
    Find the thickness face.
    
    Args:
        smObj: Sheet metal part.
        smSelItemName: Selected item to create the wall/flange
            (can be face or edge).
    
    Returns:
        FaceReference, ThicknessEdgeCoincident, ThicknessFace.

    """
    smSelItem = smObj.Shape.getElement(smSelItemName)
    smObj = smObj.Shape
    thickness = sheet_thk(smObj, smSelItemName)[0]
    thickness = round(thickness, 4)

    def faces_with_edge(obj, edge):
        """Find all faces in the given object that contain the
        specified edge.
        
        Args:
            obj: The FreeCAD object to analyze.
            edge: The specific edge to search for.
        
        Returns:
            FacesWithEdge(list).

        """
        faces_with_edge = []
        for face in obj.Faces:
            for edgeFace in face.Edges:
                if edgeFace.isSame(edge):
                    faces_with_edge.append(face)
        return faces_with_edge

    def facesConnectedToFace(obj, smThkFace):
        """Find all faces in the given object that are connected to
        the given face.
        
        Args:
            obj: The FreeCAD object to analyze.
            smThkFace: The specific face to find connected faces to.
            
        Returns:
            FacesConnectedToface(list).

        """
        smFaces = obj.Faces
        faces_connected_to = []

        for edge1 in smThkFace.Edges:
            for face in smFaces:
                # Ensure the face is not the same as the selected face
                # by comparing geometry.
                if face.isSame(smThkFace):
                    continue

                # Check if the face shares the same edge.
                for edge2 in face.Edges:
                    if edge1.isSame(edge2):
                        if face not in faces_connected_to:
                            faces_connected_to.append(face)
                        # Stop checking other edges of this face since
                        # it's already added.
                        break

        return faces_connected_to

    # Get relevant faces on the object. Use a face as reference since
    # it was possible on past version.
    if type(smSelItem) == Part.Face:
        faces = facesConnectedToFace(smObj, smSelItem)
        thkFace = smSelItem
    else:
        faces = faces_with_edge(smObj, smSelItem)

    # Find pairs of parallel faces.
    smAllFaces = smObj.Faces
    parallel_faces = []
    for i, face1 in enumerate(smAllFaces):
        for j, face2 in enumerate(smAllFaces):
            if i >= j:
                continue
            if face1.normalAt(0, 0).isEqual(face2.normalAt(0, 0).multiply(-1), 1e-6):
                distance_info = face1.distToShape(face2)
                distance = distance_info[0]
                if abs(distance - thickness) <= 1e-5:
                    parallel_faces.extend([face1, face2])

    parallel_faces = Part.Shell(parallel_faces)

    # Get only the relevant parallel faces. Use a face as reference
    # since it was possible on past version.
    if type(smSelItem) == Part.Face:
        newParFaces = []
        for edge1 in smSelItem.Edges:
            for face in parallel_faces.Faces:
                for edge2 in face.Edges:
                    if edge1.isSame(edge2):
                        newParFaces.append(face)
    else:
        for face1 in faces:
            for face2 in parallel_faces.Faces:
                if face1.isEqual(face2):
                    oneParFace = face1

        normal_vector = oneParFace.normalAt(0, 0).normalize()

        for face in parallel_faces.Faces:
            # Test to find a face with opposite normal.
            if normal_vector.isEqual(face.normalAt(0, 0).normalize().multiply(-1), 1e-6):
                distance = oneParFace.distToShape(face)[0]
                if abs(distance - thickness) <= 1e-5:
                    # Is necessary 'try' and 'except','cause rounded
                    # surfaces offset can lead to errors if offset is
                    # bigger than its radius.
                    try:
                        checkFace = face.makeOffsetShape(-distance, 0)
                        checkCut = oneParFace.cut(checkFace)
                        # Test to ensure the opposite face is, in fact,
                        # the other side of the sheet metal part.
                        if checkCut.Area < 1e-6:
                            otherParFace = face
                            # Get sheet metal face for comparison with
                            # the reference face.
                            extFace = otherParFace
                    except:
                        FreeCAD.Console.PrintLog("Skip wrong offset")
                        continue

        newParFaces = [oneParFace, otherParFace]

        for face in faces:
            if face.isEqual(newParFaces[0]) or face1.isEqual(newParFaces[1]):
                pass
            else:
                thkFace = face  # Get the thickness face.

    # Get sheet metal face for comparison with the reference face.
    # Use a face as reference since it was possible on past version.
    if type(smSelItem) == Part.Face:
        refVector = FreeCAD.Vector(0, 0, 0)
        centerA = newParFaces[0].BoundBox.Center
        centerB = newParFaces[1].BoundBox.Center

        distA = centerA.distanceToPoint(refVector)
        distB = centerB.distanceToPoint(refVector)

        if distA > distB:
            extFace = newParFaces[0]

            refVectorXY = FreeCAD.Vector(extFace.normalAt(0, 0).x, extFace.normalAt(0, 0).y, 0)
            angleTest = math.degrees(extFace.normalAt(0,0).getAngle(refVectorXY))
            refVectorZ = FreeCAD.Vector(0,0,extFace.normalAt(0,0).z)
            angleTestZ = math.degrees(extFace.normalAt(0,0).getAngle(refVectorZ))

            extFaceNormal = False
            if extFace.normalAt(0, 0).x <= 0 and extFace.normalAt(0, 0).y <= 0:
                extFaceNormal = True
            elif extFace.normalAt(0, 0).x >= 0 and extFace.normalAt(0, 0).y >= 0:
                extFaceNormal = True

            if extFaceNormal and ((angleTest == 0.0 or angleTest == 180)
                                  or (angleTestZ == 0.0 or angleTestZ == 180)):
                extFace = newParFaces[1]
                if thkFace.Orientation == "Reversed":
                    extFace = newParFaces[0]
        elif distA < distB:
            extFace = newParFaces[1]

            refVectorXY = FreeCAD.Vector(extFace.normalAt(0, 0).x, extFace.normalAt(0, 0).y, 0)
            angleTest = math.degrees(extFace.normalAt(0, 0).getAngle(refVectorXY))
            refVectorZ = FreeCAD.Vector(0, 0, extFace.normalAt(0, 0).z)
            angleTestZ = math.degrees(extFace.normalAt(0, 0).getAngle(refVectorZ))

            extFaceNormal = False
            if extFace.normalAt(0, 0).x <= 0 and extFace.normalAt(0, 0).y <= 0:
                extFaceNormal = True
            elif extFace.normalAt(0, 0).x >= 0 and extFace.normalAt(0, 0).y >= 0:
                extFaceNormal = True

            if extFaceNormal and ((angleTest == 0.0 or angleTest == 180)
                                  or (angleTestZ == 0.0 or angleTestZ == 180)):
                extFace = newParFaces[0]
                if thkFace.Orientation == "Reversed":
                    extFace = newParFaces[0]

        if distA == distB:
            valueA = centerA.x + centerA.y + centerA.z
            valueB = centerB.x + centerB.y + centerB.z
            if valueA < valueB:
                extFace = newParFaces[0]

                refVectorXY = FreeCAD.Vector(extFace.normalAt(0, 0).x, extFace.normalAt(0, 0).y, 0)
                angleTest = math.degrees(extFace.normalAt(0, 0).getAngle(refVectorXY))
                refVectorZ = FreeCAD.Vector(0, 0, extFace.normalAt(0, 0).z)
                angleTestZ = math.degrees(extFace.normalAt(0, 0).getAngle(refVectorZ))

                extFaceNormal = False
                if extFace.normalAt(0, 0).x <= 0 and extFace.normalAt(0, 0).y <= 0:
                    extFaceNormal = True
                elif extFace.normalAt(0, 0).x >= 0 and extFace.normalAt(0, 0).y >= 0:
                    extFaceNormal = True

                if extFaceNormal and ((angleTest == 0.0 or angleTest == 180)
                                      or (angleTestZ == 0.0 or angleTestZ == 180)):
                    extFace = newParFaces[1]
                    if thkFace.Orientation == "Reversed":
                        extFace = newParFaces[0]
            else:
                extFace = newParFaces[1]

                refVectorXY = FreeCAD.Vector(extFace.normalAt(0, 0).x, extFace.normalAt(0, 0).y, 0)
                angleTest = math.degrees(extFace.normalAt(0, 0).getAngle(refVectorXY))
                refVectorZ = FreeCAD.Vector(0, 0, extFace.normalAt(0, 0).z)
                angleTestZ = math.degrees(extFace.normalAt(0, 0).getAngle(refVectorZ))

                extFaceNormal = False
                if extFace.normalAt(0, 0).x <= 0 and extFace.normalAt(0, 0).y <= 0:
                    extFaceNormal = True
                elif extFace.normalAt(0, 0).x >= 0 and extFace.normalAt(0, 0).y >= 0:
                    extFaceNormal = True

                if extFaceNormal and ((angleTest == 0.0 or angleTest == 180)
                                      or (angleTestZ == 0.0 or angleTestZ == 180)):
                    extFace = newParFaces[0]
                    if thkFace.Orientation == "Reversed":
                        extFace = newParFaces[0]

        thkFace = smSelItem  # Get thickness face.

    # Get the edge coincidence between the sheet metal face reference
    # and the thickness face.
    for edge1 in extFace.Edges:
        for edge2 in thkFace.Edges:
            if edge1.isSame(edge2):
                thkEdge = edge2

    return extFace, thkEdge, thkFace


# Calculations for Offset face reference mode.
def offsetFaceDistance(smFace, refFace, refEdge, thkFace):
    """Calculate the distance between the intersection of the planes of
    sheet metal object and the edge of face thickness.

    Args:
        smFace: Sheet metal surface that is opposite the wall.
        refFace: Any reference surface.
        refEdge: Edge that is coincident with thkFace and the smFace.
        thkFace: Face of the thickness where the wall going to be.

    Returns:
        OffsetDistance (can be positive or negative value).

    """

    def get_lowest_normal_distance_to_line(face, infinite_line):
        """Calculate the lowest normal distance from a face to
        an infinite line.
        
        Args:
            face (Part.Face): The face object.
            infinite_line (Part.Line): The infinite line object.
        
        Returns:
            float: The lowest normal distance.

        """
        # Find the closest point on the face to the infinite line.
        closest_point_face = face.distToShape(infinite_line.toShape())[1][0][0]

        # Get the normal vector at the closest point on the face.
        u, v = face.Surface.parameter(closest_point_face)
        normal = face.normalAt(u, v)

        # Create a line along the normal direction
        normal_line = Part.Line(closest_point_face, closest_point_face.add(normal))

        # Find the intersection of the normal line with the
        # infinite line.
        intersection = normal_line.toShape().distToShape(infinite_line.toShape())
        # Closest point on the infinite line.
        intersection_point = intersection[1][0][0]

        # Calculate the distance between the closest point and the
        # intersection point.
        distance = closest_point_face.distanceToPoint(intersection_point)

        return distance

    # Create planes from smFace and refFace.
    smPlane = Part.Plane(smFace.CenterOfMass, smFace.normalAt(0, 0))
    refPlane = Part.Plane(refFace.CenterOfMass, refFace.normalAt(0, 0))

    # Intersection line between the planes of sheet metal and
    # the reference.
    interLine = smPlane.intersect(refPlane)[0]
    # Get the distance between the intersection of the planes
    # and the edge.
    offsetDist = get_lowest_normal_distance_to_line(thkFace, interLine)

    # Test if is necessary negative offset.
    negOff = False
    refFaceNormal = refFace.normalAt(0, 0).normalize()
    thkFaceNormal = thkFace.normalAt(0, 0).normalize()
    denom = refFaceNormal.dot(thkFaceNormal)
    if abs(denom) < 1e-6:
        negOff = True
    if refFaceNormal.dot(refFace.BoundBox.Center - thkFace.BoundBox.Center) / denom < 0:
        negOff = True
    if negOff:
        offsetDist = -offsetDist
    offsetDist = round(offsetDist, 6)
    return offsetDist


# Calculations for Angle and Offset face reference modes.
def relatAngleCalc(thkFace, refEdge, refFace, smFace):
    """Find the relative angle to apply for a sheet metal wall
    (aka flange), considering the thickness where the wall going to be,
    the edge opposite to the wall bend side, the face that the wall
    going to be parallel to and the sheet metal face side in opposition
    to the wall bend side.

    Args:
        thkFace: Thickness attached to the created wall.
        refEdge: An edge common to `thkFace` and to a sheet metal
            face side.
        refFace: A face used as reference for bend angle.
        smFace: A sheet metal face side in opposition to the wall bend
            side which has an edge common with `thkFace`.

    Returns:
        angleParFace: Angle between the refFace and the smFace.

    """
    try:  # Get the face for 3D angle.
        # Get the edge perpendicular to sheet metal side face.
        normalEdges = []
        for edge1 in thkFace.Edges:
            edgeAvert1, edgeAvert2 = refEdge.Vertexes
            edgeBvert1, edgeBvert2 = edge1.Vertexes

            vecAng1 = edgeAvert1.Point - edgeAvert2.Point
            vecAng2 = edgeBvert1.Point - edgeBvert2.Point

            if math.degrees(vecAng1.getAngle(vecAng2)) == 90:
                normalEdges.append(edge1)

        # Create one face as reference to project a line that represents
        # the correct angle between the sheet metal and reference face.
        rotFace = thkFace.copy()
        rotEdge = normalEdges[0]
        rotAx3Dang = rotEdge.Vertexes[1].Point - rotEdge.Vertexes[0].Point

        rotFace.rotate(rotEdge.Vertexes[0].Point, rotAx3Dang, 90)

        projPlane = Part.Plane(rotFace.CenterOfMass, rotFace.normalAt(0, 0).normalize())
        refPlane = Part.Plane(refFace.CenterOfMass, refFace.normalAt(0, 0).normalize())
        # Edge that represents the correct angle.
        angEdge = refFace.common(projPlane.intersect(refPlane)[0], 1e-6)

        # Create a plane rotated at correct angle to be used to angle
        # calculation after.
        refAngPlane = projPlane.copy()
        rotAx3Dang = angEdge.Vertexes[1].Point - angEdge.Vertexes[0].Point
        rotPlac = FreeCAD.Placement(rotEdge.Vertexes[0].Point, rotAx3Dang, 90)
        refAngPlane.rotate(rotPlac)

        refFaceNormal = refAngPlane.normal(0, 0).normalize()
    except:
        refFaceNormal = refFace.normalAt(0, 0).normalize()

    smFaceNor = smFace.normalAt(0, 0).normalize()
    thkFaceNor = thkFace.normalAt(0, 0).normalize()

    if refFaceNormal.isEqual(smFaceNor, 1e-6) or refFaceNormal.isEqual(-smFaceNor, 1e-6):
        angleParFace = 180
    else:
        if thkFaceNor.getAngle(refFaceNormal) < thkFaceNor.getAngle(-refFaceNormal):
            refNormal = refFaceNormal
        else:
            refNormal = -refFaceNormal

        angleParFace = smFaceNor.getAngle(refNormal)
        angleParFace = round(math.degrees(angleParFace), 6)

    return angleParFace


def smStrEdge(e):
    return "[" + str(e.valueAt(e.FirstParameter)) + " , " + str(e.valueAt(e.LastParameter)) + "]"


def smMakeReliefFace(edge, dir, gap, reliefW, reliefD, reliefType, op=""):
    p1 = edge.valueAt(edge.FirstParameter + gap)
    p2 = edge.valueAt(edge.FirstParameter + gap + reliefW)
    if reliefType == "Round":  # and reliefD >= (reliefW / 2.0):
        e1 = Part.makeLine(p1, p2)
        p34 = (edge.valueAt(edge.FirstParameter + gap + reliefW / 2) + dir.normalize() * reliefD)

        if (reliefD - (reliefW / 2.0)) < smEpsilon:
            e3 = Part.Arc(p1, p34, p2).toShape()
            w = Part.Wire([e1, e3])
        else:
            p3 = edge.valueAt(edge.FirstParameter + gap + reliefW) + dir.normalize() * (
                    reliefD - reliefW / 2)
            p4 = edge.valueAt(edge.FirstParameter + gap) + dir.normalize() * (
                    reliefD - reliefW / 2)
            e2 = Part.makeLine(p2, p3)
            e3 = Part.Arc(p3, p34, p4).toShape()
            e4 = Part.makeLine(p4, p1)
            w = Part.Wire([e1, e2, e3, e4])
    else:
        p3 = (edge.valueAt(edge.FirstParameter + gap + reliefW) + dir.normalize()*reliefD)
        p4 = edge.valueAt(edge.FirstParameter + gap) + dir.normalize()*reliefD
        e1 = Part.makeLine(p1, p2)
        e2 = Part.makeLine(p2, p3)
        e3 = Part.makeLine(p3, p4)
        e4 = Part.makeLine(p4, p1)
        w = Part.Wire([e1, e2, e3, e4])

    face = Part.Face(w)
    if hasattr(face, "mapShapes"):
        face.mapShapes([(edge, face)], [], op)
    return face


def smMakePerforationFace(
        edge,
        dir,
        bendR,
        bendA,
        perforationAngle,
        flipped,
        extLen,
        gap1,
        gap2,
        lenIPerf1,
        lenIPerf2,
        lenPerf,
        lenNPerf,
        op="",
):
    L0 = (edge.LastParameter - gap2 - lenIPerf2) - (edge.FirstParameter + gap1 + lenIPerf1)
    Lp = lenPerf
    Ln = lenNPerf
    P0 = (L0-Ln) / (Lp+Ln)
    P = math.ceil(P0)
    N = P + 1
    F = L0 / (math.ceil(P0)*Lp + math.ceil(P0)*Ln + Ln)

    if perforationAngle == 0:
        perforationAngle = bendA
    extAngle = (perforationAngle-bendA) / 2
    S = 1 / math.cos(extAngle * (2*math.pi/360))

    pivotL = -bendR
    swingL = extLen + (S-1)*(bendR+extLen)
    if not flipped:
        pivotL = extLen - pivotL
        swingL = extLen - swingL

    faces = []

    # Initial perf, near.
    if lenIPerf1 > 0:
        p1 = edge.valueAt(edge.FirstParameter + gap1) + dir.normalize() * pivotL
        p2 = edge.valueAt(edge.FirstParameter + gap1 + lenIPerf1) + dir.normalize() * pivotL
        p3 = edge.valueAt(edge.FirstParameter + gap1 + lenIPerf1) + dir.normalize() * swingL
        p4 = edge.valueAt(edge.FirstParameter + gap1) + dir.normalize() * swingL
        w = Part.makePolygon([p1, p2, p3, p4, p1])
        faces.append(Part.Face(w))

    # Initial perf, far.
    if lenIPerf2 > 0:
        p1 = edge.valueAt(edge.LastParameter - gap2 - lenIPerf2) + dir.normalize() * pivotL
        p2 = edge.valueAt(edge.LastParameter - gap2) + dir.normalize() * pivotL
        p3 = edge.valueAt(edge.LastParameter - gap2) + dir.normalize() * swingL
        p4 = edge.valueAt(edge.LastParameter - gap2 - lenIPerf2) + dir.normalize() * swingL
        w = Part.makePolygon([p1, p2, p3, p4, p1])
        faces.append(Part.Face(w))

    # Perforations, inner.
    for i in range(P):
        x = (edge.FirstParameter + gap1 + lenIPerf1) + (Ln * F * (i + 1)) + (Lp * F * i)
        p1 = edge.valueAt(x) + dir.normalize() * pivotL
        p2 = edge.valueAt(x + Lp * F) + dir.normalize() * pivotL
        p3 = edge.valueAt(x + Lp * F) + dir.normalize() * swingL
        p4 = edge.valueAt(x) + dir.normalize() * swingL
        w = Part.makePolygon([p1, p2, p3, p4, p1])
        faces.append(Part.Face(w))

    totalFace = faces[0]
    for face in faces[1:]:
        totalFace = totalFace.fuse(face)

    if hasattr(totalFace, "mapShapes"):
        totalFace.mapShapes([(edge, totalFace)], None, op)
    return totalFace


def smMakeFace(edge, dir, extLen, gap1=0.0, gap2=0.0, angle1=0.0, angle2=0.0, op=""):
    len1 = extLen * math.tan(math.radians(angle1))
    len2 = extLen * math.tan(math.radians(angle2))

    p1 = edge.valueAt(edge.LastParameter - gap2)
    p2 = edge.valueAt(edge.FirstParameter + gap1)
    p3 = edge.valueAt(edge.FirstParameter + gap1 + len1) + dir.normalize() * extLen
    p4 = edge.valueAt(edge.LastParameter - gap2 - len2) + dir.normalize() * extLen

    e2 = Part.makeLine(p2, p3)
    e4 = Part.makeLine(p4, p1)
    section = e4.section(e2)

    if section.Vertexes:
        p5 = section.Vertexes[0].Point
        w = Part.makePolygon([p1, p2, p5, p1])
    else:
        w = Part.makePolygon([p1, p2, p3, p4, p1])
    face = Part.Face(w)
    if hasattr(face, "mapShapes"):
        face.mapShapes([(edge, face)], None, op)
    return face


def smRestrict(var, fromVal, toVal):
    if var < fromVal:
        return fromVal
    if var > toVal:
        return toVal
    return var


def smModifiedFace(Face, obj):
    """Find face Modified During loop."""
    for face in obj.Faces:
        face_common = face.common(Face)
        if face_common.Faces:
            if face.Area == face_common.Faces[0].Area:
                break
    return face


def LineAngle(edge1, edge2):
    # Find angle between two lines.
    v1a = edge1.Vertexes[0].Point
    v1b = edge1.Vertexes[1].Point
    v2a = edge2.Vertexes[0].Point
    v2b = edge2.Vertexes[1].Point

    # Find the right order of the wire verts to calculate the angle
    # the order of the verts v1a v1b v2a v2b should be such that v1b is
    # the closest vert to v2a.
    minlen = (v1a - v2a).Length
    order = [v1b, v1a, v2a, v2b]
    len_ = (v1a - v2b).Length
    if len_ < minlen:
        minlen = len_
        order = [v1b, v1a, v2b, v2a]
    len_ = (v1b - v2a).Length
    if len_ < minlen:
        minlen = len_
        order = [v1a, v1b, v2a, v2b]
    len_ = (v1b - v2b).Length
    if len_ < minlen:
        order = [v1a, v1b, v2b, v2a]

    lineDir = order[1] - order[0]
    edgeDir = order[3] - order[2]

    angleRad = edgeDir.getAngle(lineDir)
    angle = math.degrees(angleRad)
    # if angle > 90:
    #     angle = 180.0 - angle
    return angle


def smGetFace(Faces, obj):
    # Find face Name Modified obj.
    faceList = []
    for Face in Faces:
        for i, face in enumerate(obj.Faces):
            face_common = face.common(Face)
            if face_common.Faces:
                faceList.append("Face" + str(i + 1))
    # print(faceList)
    return faceList


def LineExtend(edge, distance1, distance2):
    """Extend a line by given distances."""
    result = edge.Curve.toShape(edge.FirstParameter - distance1, edge.LastParameter + distance2)
    if hasattr(result, "mapShapes"):
        result.mapShapes([(edge, result)], [])
    return result


def getParallel(edge1, edge2):
    # Get intersection between two lines.
    e1 = edge1.Curve.toShape()
    # Part.show(e1, "e1")
    e2 = edge2.Curve.toShape()
    # Part.show(e2, "e2")
    section = e1.section(e2)
    if section.Vertexes:
        # Part.show(section, "section")
        return False
    else:
        return True


def getCornerPoint(edge1, edge2):
    # Get intersection between two lines.
    # Part.show(edge1, "edge1")
    # Part.show(edge2, "edge21")
    e1 = edge1.Curve.toShape()
    # Part.show(e1, "e1")
    e2 = edge2.Curve.toShape()
    # Part.show(e2, "e2")
    section = e1.section(e2)
    cornerPoint = None
    if section.Vertexes:
        # Part.show(section,'section')
        cornerPoint = section.Vertexes[0].Point
    return cornerPoint


def getGap(line1, line2, maxExtendGap, mingap):
    # To find gap between two edges.
    gaps = 0.0
    extgap = 0.0
    section = line1.section(line2)
    if section.Vertexes:
        cornerPoint = section.Vertexes[0].Point
        size1 = abs((cornerPoint - line2.Vertexes[0].Point).Length)
        size2 = abs((cornerPoint - line2.Vertexes[1].Point).Length)
        if size1 < size2:
            gaps = size1
        else:
            gaps = size2
        gaps = gaps + mingap
        # print(gaps)
    else:
        cornerPoint = getCornerPoint(line1, line2)
        line3 = LineExtend(line1, maxExtendGap, maxExtendGap)
        # Part.show(line1, "line1")
        line4 = LineExtend(line2, maxExtendGap, maxExtendGap)
        # Part.show(line2, "line2")
        section = line3.section(line4)
        if section.Vertexes:
            # cornerPoint = section.Vertexes[0].Point
            # p1 = Part.Vertex(cornerPoint)
            section1 = line1.section(line4)
            size1 = abs((cornerPoint - line2.Vertexes[0].Point).Length)
            size2 = abs((cornerPoint - line2.Vertexes[1].Point).Length)
            # dist = cornerPoint.distanceToLine(line2.Curve.Location, line2.Curve.Direction)
            # print(["gap",size1, size2, dist])
            # if section1.Vertexes:
            #     extgap = 0.0
            if size1 < size2:
                extgap = size1
            else:
                extgap = size2
            # if dist < smEpsilon:
            #     gaps = extgap
            #     extgap = 0.0
            if extgap > mingap:
                extgap = extgap - mingap
            # print(extgap)
    return gaps, extgap, cornerPoint


def getSketchDetails(Sketch, sketchflip, sketchinvert, radius, thk):
    # Convert Sketch lines to length. Angles between line.
    LengthList, bendAList = ([], [])
    sketch_normal = Sketch.Placement.Rotation.multVec(FreeCAD.Vector(0, 0, 1))
    e0 = Sketch.Placement.Rotation.multVec(FreeCAD.Vector(1, 0, 0))
    WireList = Sketch.Shape.Wires[0]

    # Create filleted wire at centre of thickness.
    wire_extr = WireList.extrude(sketch_normal * -50)
    # Part.show(wire_extr, "wire_extr")
    wire_extr_mir = WireList.extrude(sketch_normal * 50)
    # Part.show(wire_extr_mir, "wire_extr_mir")
    wire_extr = wire_extr.makeOffsetShape(thk / 2.0, 0.0, fill=False, join=2)
    # Part.show(wire_extr, "wire_extr")
    wire_extr_mir = wire_extr_mir.makeOffsetShape(-thk / 2.0, 0.0, fill=False, join=2)
    # Part.show(wire_extr_mir, "wire_extr_mir")
    if len(WireList.Edges) > 1:
        filleted_extr = wire_extr.makeFillet((radius + thk / 2.0), wire_extr.Edges)
        # Part.show(filleted_extr, "filleted_extr")
        filleted_extr_mir = wire_extr_mir.makeFillet((radius + thk / 2.0), wire_extr_mir.Edges)
        # Part.show(filleted_extr_mir, "filleted_extr_mir")
    else:
        filleted_extr = wire_extr
        filleted_extr_mir = wire_extr_mir
    # Part.show(filleted_extr, "filleted_extr")
    sec_wirelist = filleted_extr_mir.section(filleted_extr)
    # Part.show(sec_wirelist, "sec_wirelist")

    for edge in sec_wirelist.Edges:
        if isinstance(edge.Curve, Part.Line):
            LengthList.append(edge.Length)

    for i in range(len(WireList.Vertexes) - 1):
        p1 = WireList.Vertexes[i].Point
        p2 = WireList.Vertexes[i + 1].Point
        e1 = p2 - p1
        # LengthList.append(e1.Length)
        normal = e0.cross(e1)
        coeff = sketch_normal.dot(normal)
        if coeff >= 0:
            sign = 1
        else:
            sign = -1
        angle_rad = e0.getAngle(e1)
        if sketchflip:
            angle = sign * math.degrees(angle_rad) * -1
        else:
            angle = sign * math.degrees(angle_rad)
        bendAList.append(angle)
        e0 = e1
    if sketchinvert:
        LengthList.reverse()
        bendAList.reverse()
    # print(LengthList, bendAList)
    return LengthList, bendAList


def check_parallel(edge1, edge2):
    v1 = edge1.Vertexes[0].Point - edge1.Vertexes[1].Point
    v2 = edge2.Vertexes[0].Point - edge2.Vertexes[1].Point
    if v1.isEqual(v2, 0.00001):
        return True, edge2.Vertexes[0].Point - edge1.Vertexes[0].Point
    if v1.isEqual(-v2, 0.00001):
        return True, edge2.Vertexes[0].Point - edge1.Vertexes[1].Point
    return False, None


def sheet_thk(MainObject, selFaceName):
    selItem = MainObject.getElement(SheetMetalTools.getElementFromTNP(selFaceName))
    selFace = SheetMetalTools.smGetFaceByEdge(selItem, MainObject)
    # Find the narrow edge.
    thk = 999999.0
    thkDir = None
    if type(selItem) == Part.Face:
        for edge in selFace.Edges:
            if abs(edge.Length) < thk:
                thk = abs(edge.Length)
    else:
        # If selected item is edge, try to find the closest parallel
        # edge - works better when object is refined and faces are not
        # rectangle.
        for edge in selFace.Edges:
            if edge.isSame(selItem):
                continue
            isParallel, distVect = check_parallel(selItem, edge)
            if isParallel:
                dist = distVect.Length
                if dist < thk:
                    thk = dist
                    thkDir = distVect
        thkDir.normalize()
    return thk, thkDir


def smEdge(selFaceName, MainObject):
    # Find Edge, if Face Selected.
    selItem = MainObject.getElement(SheetMetalTools.getElementFromTNP(selFaceName))
    thkDir = None
    if type(selItem) == Part.Face:
        # Find the narrow edge.
        thk = 999999.0
        for edge in selItem.Edges:
            if abs(edge.Length) < thk:
                thk = abs(edge.Length)
                thkEdge = edge

        # Find a length edge  =  revolve axis direction.
        p0 = thkEdge.valueAt(thkEdge.FirstParameter)
        for lenEdge in selItem.Edges:
            p1 = lenEdge.valueAt(lenEdge.FirstParameter)
            p2 = lenEdge.valueAt(lenEdge.LastParameter)
            if lenEdge.isSame(thkEdge):
                continue
            if (p1 - p0).Length < smEpsilon:
                revAxisV = p2 - p1
                break
            if (p2 - p0).Length < smEpsilon:
                revAxisV = p1 - p2
                break
        seledge = lenEdge
        selFace = selItem
    elif type(selItem) == Part.Edge:
        thk, thkDir = sheet_thk(MainObject, selFaceName)
        seledge = selItem
        selFace = SheetMetalTools.smGetFaceByEdge(selItem, MainObject)
        p1 = seledge.valueAt(seledge.FirstParameter)
        p2 = seledge.valueAt(seledge.LastParameter)
        revAxisV = p2 - p1
    # print(str(revAxisV))
    return seledge, selFace, thk, revAxisV, thkDir

def CheckEdgeFlip(checkEdge, refEdge):
    # Check if edge direction is same as reference edge, flip if not.
    p1 = refEdge.valueAt(refEdge.FirstParameter)
    p2 = refEdge.valueAt(refEdge.LastParameter)
    v2 = p2 - p1
    p1 = checkEdge.valueAt(checkEdge.FirstParameter)
    p2 = checkEdge.valueAt(checkEdge.LastParameter)
    v1 = p2 - p1
    angle = math.degrees(v1.getAngle(v2))
    if angle > 90.0:
        return Part.makeLine(p2, p1)
    return checkEdge

def getBendetail(selItemNames, MainObject, bendR, bendAngle, isflipped, offset, gap1, gap2):
    mainlist = []
    edgelist = []
    nogap_edgelist = []
    for selItemName in selItemNames:
        lenEdge, selFace, thk, revAxisV, thkDir = smEdge(selItemName, MainObject)

        # Find the large face connected with selected face.
        list2 = MainObject.ancestorsOfType(lenEdge, Part.Face)
        for Cface in list2:
            if not (Cface.isSame(selFace)):
                break
        # main Length Edge
        revAxisV.normalize()
        if thkDir is None:
            pThkDir1 = selFace.CenterOfMass
            pThkDir2 = lenEdge.Curve.value(lenEdge.Curve.parameter(pThkDir1))
            thkDir = pThkDir1.sub(pThkDir2).normalize()
            # print(str(thkDir))
        FaceDir = selFace.normalAt(0, 0)

        # Make sure the direction vector is correct in respect to
        # the normal.
        if (thkDir.cross(revAxisV).normalize() - FaceDir).Length < smEpsilon:
            revAxisV *= -1

        flipped = isflipped
        bendA = bendAngle
        # Restrict angle.
        if bendA < 0:
            bendA = -bendA
            flipped = not flipped

        if type(MainObject.getElement(
                SheetMetalTools.getElementFromTNP(selItemName))) == Part.Edge:
            flipped = not flipped

        if not flipped:
            revAxisP = lenEdge.valueAt(lenEdge.FirstParameter) + thkDir * (bendR+thk)
            revAxisV *= -1
        else:
            revAxisP = lenEdge.valueAt(lenEdge.FirstParameter) + thkDir * -bendR
        # Part.show(lenEdge, "lenEdge")
        mainlist.append(
            [Cface, selFace, thk, lenEdge, revAxisP, revAxisV, thkDir, FaceDir, bendA, flipped, ])
        if offset < 0.0:
            dist = lenEdge.valueAt(lenEdge.FirstParameter).distanceToPlane(
                FreeCAD.Vector(0, 0, 0), FaceDir
            )
            # print(dist)
            slice_wire = Cface.slice(FaceDir, dist + offset)
            # print(slice_wire)
            trimLenEdge = slice_wire[0].Edges[0]
            if gap1 != gap2:
                # Check if edge direction is same as reference edge, flip if not.
                trimLenEdge = CheckEdgeFlip(trimLenEdge, lenEdge)
        else:
            # Produce Offset Edge.
            trimLenEdge = lenEdge.copy()
            trimLenEdge.translate(selFace.normalAt(0, 0) * offset)
        # Part.show(trimLenEdge, "trimLenEdge1")
        nogap_edgelist.append(trimLenEdge)
        trimLenEdge = LineExtend(trimLenEdge, -gap1, -gap2)
        # Part.show(trimLenEdge, "trimLenEdge2")
        edgelist.append(trimLenEdge)
    # print(mainlist)
    trimedgelist = InsideEdge(edgelist)
    nogaptrimedgelist = InsideEdge(nogap_edgelist)
    return mainlist, trimedgelist, nogaptrimedgelist


def InsideEdge(edgelist):
    import BOPTools.JoinFeatures

    newedgelist = []
    for i, e in enumerate(edgelist):
        for j, ed in enumerate(edgelist):
            if i != j:
                section = e.section(ed)
                if section.Vertexes:   
                    edgeShape = BOPTools.JoinAPI.cutout_legacy(e, ed, tolerance=0.0)
                    e = edgeShape
        # Part.show(e,"newedge")
        newedgelist.append(e)
    return newedgelist


def smMiter(
    mainlist,
    trimedgelist,
    bendR=1.0,
    miterA1=0.0,
    miterA2=0.0,
    extLen=10.0,
    gap1=0.0,
    gap2=0.0,
    offset=0.0,
    reliefD=1.0,
    automiter=True,
    extend1=0.0,
    extend2=0.0,
    mingap=0.1,
    maxExtendGap=5.0,
):
    if not (automiter):
        miterA1List = [miterA1 for n in mainlist]
        miterA2List = [miterA2 for n in mainlist]
        gap1List = [gap1 for n in mainlist]
        gap2List = [gap2 for n in mainlist]
        extgap1List = [extend1 for n in mainlist]
        extgap2List = [extend2 for n in mainlist]
    else:
        miterA1List = [0.0 for n in mainlist]
        miterA2List = [0.0 for n in mainlist]
        gap1List = [gap1 for n in mainlist]
        gap2List = [gap2 for n in mainlist]
        extgap1List = [extend1 for n in mainlist]
        extgap2List = [extend2 for n in mainlist]

        facelist, tranfacelist = ([], [])
        extfacelist, exttranfacelist = ([], [])
        lenedgelist, tranedgelist = ([], [])
        for i, sublist in enumerate(mainlist):
            # Find the narrow edge.
            (Cface,
             selFace,
             thk,
             MlenEdge,
             revAxisP,
             revAxisV,
             thkDir,
             FaceDir,
             bendA,
             flipped) = sublist

            # Produce Offset Edge.
            lenEdge = trimedgelist[i].copy()
            # Part.show(lenEdge)
            revAxisP = revAxisP + FaceDir*offset

            # Narrow the wall, if we have gaps.
            BendFace = smMakeFace(lenEdge, FaceDir, extLen, gap1 - extend1, gap2 - extend2,
                                  op="SMB")
            if BendFace.normalAt(0, 0) != thkDir:
                BendFace.reverse()
            # Part.show(BendFace)
            transBendFace = BendFace.copy()
            BendFace.rotate(revAxisP, revAxisV, bendA)

            # Part.show(BendFace, "BendFace")
            facelist.append(BendFace)
            transBendFace.translate(thkDir * thk)
            transBendFace.rotate(revAxisP, revAxisV, bendA)
            tranfacelist.append(transBendFace)
            # Part.show(transBendFace,'transBendFace')

            # Narrow the wall, if we have gaps.
            BendFace = smMakeFace(lenEdge, FaceDir, extLen, gap1 - extend1 - maxExtendGap,
                                  gap2 - extend2 - maxExtendGap, op="SMB")
            if BendFace.normalAt(0, 0) != thkDir:
                BendFace.reverse()

            transBendFace = BendFace.copy()
            BendFace.rotate(revAxisP, revAxisV, bendA)
            # Part.show(BendFace, "BendFaceB")
            extfacelist.append(BendFace)
            transBendFace.translate(thkDir * thk)
            transBendFace.rotate(revAxisP, revAxisV, bendA)
            exttranfacelist.append(transBendFace)
            # Part.show(transBendFace, "transBendFaceB")

            # edge_len = lenEdge.copy()
            edge_len = LineExtend(lenEdge, (-gap1 + extend1), (-gap2 + extend2))
            edge_len.rotate(revAxisP, revAxisV, bendA)
            lenedgelist.append(edge_len)
            # Part.show(edge_len, "edge_len")

            # edge_len = lenEdge.copy()
            edge_len = LineExtend(lenEdge, (-gap1 + extend1), (-gap2 + extend2))
            edge_len.translate(thkDir * thk)
            edge_len.rotate(revAxisP, revAxisV, bendA)
            tranedgelist.append(edge_len)
            # Part.show(edge_len, "edge_len")

        # Check faces intersect each other.
        for i, face in enumerate(facelist):
            for j, lenedge in enumerate(lenedgelist):
                if i != j and face.isCoplanar(facelist[j]) and not getParallel(lenedgelist[i],
                                                                               lenedge):
                    # Part.show(lenedgelist[i], "edge_len1")
                    # Part.show(lenedge, "edge_len2")
                    gaps1, extgap1, cornerPoint1 = getGap(lenedgelist[i], lenedge, maxExtendGap,
                                                          mingap)
                    gaps2, extgap2, cornerPoint2 = getGap(tranedgelist[i], tranedgelist[j],
                                                          maxExtendGap, mingap)
                    # print([gaps1,gaps2, extgap1, extgap2])
                    gaps = max(gaps1, gaps2)
                    extgap = min(extgap1, extgap2)
                    p1 = lenedge.valueAt(lenedge.FirstParameter)
                    p2 = lenedge.valueAt(lenedge.LastParameter)
                    Angle = LineAngle(lenedgelist[i], lenedge)
                    # print(Angle)
                    if gaps > 0.0:
                        # walledge_common = lenedge.section(lenedgelist[i])
                        # vp1 = walledge_common.Vertexes[0].Point
                        dist1 = (p1 - cornerPoint1).Length
                        dist2 = (p2 - cornerPoint1).Length
                        if abs(dist1) < abs(dist2):
                            miterA1List[j] = Angle / 2.0
                            if gaps > 0.0:
                                gap1List[j] = gaps
                            else:
                                gap1List[j] = 0.0
                        elif abs(dist2) < abs(dist1):
                            miterA2List[j] = Angle / 2.0
                            if gaps > 0.0:
                                gap2List[j] = gaps
                            else:
                                gap2List[j] = 0.0
                    elif extgap != 0.0 and (extgap + mingap) < maxExtendGap:
                        wallface_common = facelist[j].common(face)
                        dist1 = (p1 - cornerPoint1).Length
                        dist2 = (p2 - cornerPoint1).Length
                        if abs(dist1) < abs(dist2):
                            if wallface_common.Faces:
                                miterA1List[j] = Angle / 2.0
                            else:
                                miterA1List[j] = -Angle / 2.0
                            if extgap > 0.0:
                                extgap1List[j] = extgap
                            else:
                                extgap1List[j] = 0.0
                        elif abs(dist2) < abs(dist1):
                            if wallface_common.Faces:
                                miterA2List[j] = Angle / 2.0
                            else:
                                miterA2List[j] = -Angle / 2.0
                            if extgap > 0.0:
                                extgap2List[j] = extgap
                            else:
                                extgap2List[j] = 0.0
                elif i != j and not (getParallel(lenedgelist[i], lenedge)):
                    # Part.show(lenedgelist[i], "edge_len1")
                    # Part.show(lenedge, "edge_len2")
                    # Part.show(tranedgelist[i], "edge_len1")
                    # Part.show(tranedgelist[j], "edge_len2")
                    gaps1, extgap1, cornerPoint1 = getGap(lenedgelist[i], lenedge, maxExtendGap,
                                                          mingap)
                    gaps2, extgap2, cornerPoint2 = getGap(tranedgelist[i], tranedgelist[j],
                                                          maxExtendGap, mingap)
                    # print([gaps1, gaps2, extgap1, extgap2])
                    gaps = max(gaps1, gaps2)
                    extgap = min(extgap1, extgap2)
                    p1 = lenedge.valueAt(lenedge.FirstParameter)
                    p2 = lenedge.valueAt(lenedge.LastParameter)
                    if gaps > 0.0:
                        wallface_common = facelist[j].section(face)
                        # Part.show(facelist[j], "facelist")
                        # Part.show(face, "facelist")
                        wallface_common1 = tranfacelist[j].section(tranfacelist[i])
                        # Part.show(tranfacelist[j], "tranfacelist")
                        # Part.show(tranfacelist[i], "tranfacelist")
                        # Part.show(wallface_common, "wallface_common")
                        vp1 = None
                        vp2 = None
                        if wallface_common.Edges:
                            vp1 = wallface_common.Vertexes[0].Point
                            vp2 = wallface_common.Vertexes[1].Point
                        elif wallface_common1.Edges:
                            vp1 = wallface_common1.Vertexes[0].Point
                            vp2 = wallface_common1.Vertexes[1].Point
                        dist1 = (p1 - vp1).Length
                        dist2 = (p2 - vp1).Length
                        if abs(dist1) < abs(dist2):
                            edgedir = (p1 - p2).normalize()
                            dist3 = (cornerPoint1 - vp1).Length
                            dist4 = (cornerPoint1 - vp2).Length
                            if dist4 < dist3:
                                lineDir = (vp2 - vp1).normalize()
                            else:
                                lineDir = (vp1 - vp2).normalize()
                            angle1 = edgedir.getAngle(lineDir)
                            Angle2 = math.degrees(angle1)
                            Angle = 90 - Angle2
                            # print([Angle, Angle2, "ext"])
                            miterA1List[j] = Angle
                            if gaps > 0.0:
                                gap1List[j] = gaps
                            else:
                                gap1List[j] = 0.0
                        elif abs(dist2) < abs(dist1):
                            edgedir = (p2 - p1).normalize()
                            dist3 = (cornerPoint1 - vp1).Length
                            dist4 = (cornerPoint1 - vp2).Length
                            if dist4 < dist3:
                                lineDir = (vp2 - vp1).normalize()
                            else:
                                lineDir = (vp1 - vp2).normalize()
                            angle1 = edgedir.getAngle(lineDir)
                            Angle2 = math.degrees(angle1)
                            Angle = 90 - Angle2
                            # print([Angle, Angle2, "ext"])
                            miterA2List[j] = Angle
                            if gaps > 0.0:
                                gap2List[j] = gaps
                            else:
                                gap2List[j] = 0.0
                    elif extgap != 0.0 and (extgap + mingap) < maxExtendGap:
                        wallface_common = extfacelist[j].section(extfacelist[i])
                        # Part.show(extfacelist[j], "extfacelist")
                        # Part.show(extfacelist[i],"extfacelist")
                        wallface_common1 = exttranfacelist[j].section(exttranfacelist[i])
                        # Part.show(exttranfacelist[j], "exttranfacelist")
                        # Part.show(exttranfacelist[i], "exttranfacelist")
                        # Part.show(wallface_common, "wallface_common")
                        if wallface_common.Edges:
                            vp1 = wallface_common.Vertexes[0].Point
                            vp2 = wallface_common.Vertexes[1].Point
                        elif wallface_common1.Edges:
                            vp1 = wallface_common1.Vertexes[0].Point
                            vp2 = wallface_common1.Vertexes[1].Point
                        dist1 = (p1 - vp1).Length
                        dist2 = (p2 - vp1).Length
                        if abs(dist1) < abs(dist2):
                            edgedir = (p1 - p2).normalize()
                            dist3 = (cornerPoint1 - vp1).Length
                            dist4 = (cornerPoint1 - vp2).Length
                            if dist4 < dist3:
                                lineDir = (vp2 - vp1).normalize()
                            else:
                                lineDir = (vp1 - vp2).normalize()
                            angle1 = edgedir.getAngle(lineDir)
                            Angle2 = math.degrees(angle1)
                            Angle = 90 - Angle2
                            # print([Angle, Angle2, 'ext'])
                            miterA1List[j] = Angle
                            if extgap > 0.0:
                                extgap1List[j] = extgap
                            else:
                                extgap1List[j] = 0.0
                        elif abs(dist2) < abs(dist1):
                            edgedir = (p2 - p1).normalize()
                            dist3 = (cornerPoint1 - vp1).Length
                            dist4 = (cornerPoint1 - vp2).Length
                            if dist4 < dist3:
                                lineDir = (vp2 - vp1).normalize()
                            else:
                                lineDir = (vp1 - vp2).normalize()
                            angle1 = edgedir.getAngle(lineDir)
                            Angle2 = math.degrees(angle1)
                            Angle = 90 - Angle2
                            # print([Angle, Angle2, 'ext'])
                            miterA2List[j] = Angle
                            if extgap > 0.0:
                                extgap2List[j] = extgap
                            else:
                                extgap2List[j] = 0.0

    # print(miterA1List, miterA2List, gap1List, gap2List, extgap1List, extgap2List)
    return miterA1List, miterA2List, gap1List, gap2List, extgap1List, extgap2List

def smCalcCutGap(trimedEdge, origEdge, edgePos, bendGap, extend, reliefW, reliefD):
    if extend < SheetMetalTools.smEpsilon:
        return -min(reliefW, bendGap)
    gap_1 = (origEdge.valueAt(edgePos)
                - trimedEdge.valueAt(trimedEdge.FirstParameter)).Length
    gap_2 = (origEdge.valueAt(edgePos)
                - trimedEdge.valueAt(trimedEdge.LastParameter)).Length
    gap = min(gap_1, gap_2)
    return -gap

def smBend(
    thk,
    bendR=1.0,
    bendA=90.0,
    miterA1=0.0,
    miterA2=0.0,
    BendType="Material Outside",
    flipped=False,
    unfold=False,
    offset=0.0,
    extLen=10.0,
    gap1=0.0,
    gap2=0.0,
    reliefType="Rectangle",
    reliefW=0.8,
    reliefD=1.0,
    minReliefgap=1.0,
    extend1=0.0,
    extend2=0.0,
    kfactor=0.45,
    ReliefFactor=0.7,
    UseReliefFactor=False,
    selFaceNames="",
    MainObject=None,
    maxExtendGap=5.0,
    mingap=0.1,
    automiter=True,
    sketch=None,
    extendType="Simple",
    LengthSpec="Leg",
    Perforate=False,
    PerforationAngle=0.0,
    PerforationInitialLength=5.0,
    PerforationMaxLength=5.0,
    NonperforationMaxLength=5.0,
):
    # if sketch is as wall.
    sketches = False
    if sketch:
        if sketch.Shape.Wires[0].isClosed():
            sketches = True
        else:
            pass

    # Add Bend Type details.
    inside = False
    if BendType == "Material Outside":
        offset = 0.0
        inside = False
    elif BendType == "Material Inside":
        offset = -(thk + bendR)
        inside = True
    elif BendType == "Thickness Outside":
        offset = -bendR
        inside = True
    elif BendType == "Offset":
        if offset < 0.0:
            inside = True
        else:
            inside = False

    if LengthSpec == "Leg":
        pass
    elif LengthSpec == "Tangential":
        if bendA >= 90.0:
            extLen -= thk + bendR
        else:
            extLen -= (bendR + thk) / math.tan(math.radians(90.0 - bendA / 2))
    elif LengthSpec == "Inner Sharp":
        extLen -= bendR / math.tan(math.radians(90.0 - bendA / 2))
    elif LengthSpec == "Outer Sharp":
        extLen -= (bendR + thk) / math.tan(math.radians(90.0 - bendA / 2))

    nogaptrimedgelist = []
    if not (sketches):
        mainlist, trimedgelist, nogaptrimedgelist = getBendetail(
            selFaceNames, MainObject, bendR, bendA, flipped, offset, gap1, gap2
        )
        (
            miterA1List,
            miterA2List,
            gap1List,
            gap2List,
            extend1List,
            extend2List,
        ) = smMiter(
            mainlist,
            trimedgelist,
            bendR=bendR,
            miterA1=miterA1,
            miterA2=miterA2,
            extLen=extLen,  # gap1 = gap1, gap2 = gap2,
            offset=offset,
            automiter=automiter,
            extend1=extend1,
            extend2=extend2,
            mingap=mingap,
            maxExtendGap=maxExtendGap,
        )

        # print(miterA1List, miterA2List, gap1List, gap2List, extend1List, extend2List)
    else:
        (
            miterA1List,
            miterA2List,
            gap1List,
            gap2List,
            extend1List,
            extend2List,
            _reliefDList,
        ) = ([0.0], [0.0], [gap1], [gap2], [extend1], [extend2], [reliefD])
    agap1, agap2 = gap1, gap2
    # print([agap1, agap1])

    # mainlist = getBendetail(selFaceNames, MainObject, bendR, bendA, flipped)
    thk_faceList = []
    resultSolid = MainObject
    for i, sublist in enumerate(mainlist):
        # Find the narrow edge.
        (
            Cface,
            selFace,
            thk,
            AlenEdge,
            revAxisP,
            revAxisV,
            thkDir,
            FaceDir,
            bendA,
            flipped,
        ) = sublist
        gap1, gap2 = (gap1List[i], gap2List[i])
        # print([gap1,gap2])
        extend1, extend2 = (extend1List[i], extend2List[i])
        # Part.show(lenEdge, "lenEdge1")
        selFace = smModifiedFace(selFace, resultSolid)
        # Part.show(selFace, "selFace")
        Cface = smModifiedFace(Cface, resultSolid)
        # Part.show(Cface, "Cface")
        # main Length Edge
        MlenEdge = SheetMetalTools.smGetIntersectingEdge(AlenEdge, resultSolid)
        # Part.show(MlenEdge, "MlenEdge")
        lenEdge = trimedgelist[i]
        noGap_lenEdge = nogaptrimedgelist[i]
        leng = lenEdge.Length
        # Part.show(lenEdge, "lenEdge")

        # Add as offset to set any distance.
        if UseReliefFactor:
            reliefW = thk * ReliefFactor
            reliefD = thk * ReliefFactor

        # If sketch is as wall.
        sketches = False
        if sketch:
            if sketch.Shape.Wires[0].isClosed():
                sketches = True
            else:
                pass

        if sketches:
            sketch_face = Part.makeFace(sketch.Shape.Wires, "Part::FaceMakerBullseye")
            sketch_face.translate(thkDir * -thk)
            if inside:
                sketch_face.translate(FaceDir * offset)
            sketch_Shape = lenEdge.common(sketch_face)
            sketch_Edge = sketch_Shape.Edges[0]
            gap1 = (lenEdge.valueAt(lenEdge.FirstParameter)
                    - sketch_Edge.valueAt(sketch_Edge.FirstParameter)
            ).Length
            gap2 = (lenEdge.valueAt(lenEdge.LastParameter)
                    - sketch_Edge.valueAt(sketch_Edge.LastParameter)
            ).Length

        # `CutSolids` list for collecting Solids.
        CutSolids = []

        # Calculate cut gap to avoid small faces.
        # Ref_lenEdge = noGap_lenEdge.copy().translate(FaceDir * -offset)
        # Remove relief if needed.
        if reliefD > 0.0 and reliefW > 0.0:
            if agap1 > minReliefgap: # and cutgap1 > 0.0:
                reliefFace1 = smMakeReliefFace(lenEdge, FaceDir * -1, gap1 - reliefW, reliefW,
                                               reliefD, reliefType, op="SMF")
                reliefSolid1 = reliefFace1.extrude(thkDir * thk)
                # Part.show(reliefSolid1, "reliefSolid1a")
                CutSolids.append(reliefSolid1)
                if inside:
                    reliefFace1 = smMakeReliefFace(lenEdge, FaceDir * -1, gap1 - reliefW, reliefW,
                                                   offset, "Rectangle", op="SMF")
                    reliefSolid1 = reliefFace1.extrude(thkDir * thk)
                    # Part.show(reliefSolid1, "reliefSolid1b")
                    CutSolids.append(reliefSolid1)
            if agap2 > minReliefgap: # and cutgap2 > 0.0:
                reliefFace2 = smMakeReliefFace(lenEdge, FaceDir * -1, lenEdge.Length - gap2,
                                               reliefW, reliefD, reliefType, op="SMFF")
                reliefSolid2 = reliefFace2.extrude(thkDir * thk)
                # Part.show(reliefSolid2, "reliefSolid2")
                CutSolids.append(reliefSolid2)
                if inside:
                    reliefFace2 = smMakeReliefFace(lenEdge, FaceDir * -1, lenEdge.Length - gap2,
                                                   reliefW, offset, "Rectangle", op="SMFF")
                    reliefSolid2 = reliefFace2.extrude(thkDir * thk)
                    # Part.show(reliefSolid2, "reliefSolid2")
                    CutSolids.append(reliefSolid2)

        # Remove bend face if present.
        if inside:
            if (
                MlenEdge.Vertexes[0].Point - MlenEdge.valueAt(MlenEdge.FirstParameter)
            ).Length < smEpsilon:
                vertex0 = MlenEdge.Vertexes[0]
                vertex1 = MlenEdge.Vertexes[1]
            else:
                vertex1 = MlenEdge.Vertexes[0]
                vertex0 = MlenEdge.Vertexes[1]
            Noffset_1 = abs(
                (
                    MlenEdge.valueAt(MlenEdge.FirstParameter)
                    - noGap_lenEdge.valueAt(noGap_lenEdge.FirstParameter)
                ).Length
            )
            Noffset_2 = abs(
                (
                    MlenEdge.valueAt(MlenEdge.FirstParameter)
                    - noGap_lenEdge.valueAt(noGap_lenEdge.LastParameter)
                ).Length
            )
            Noffset1 = min(Noffset_1, Noffset_2)
            Noffset_1 = abs(
                (
                    MlenEdge.valueAt(MlenEdge.LastParameter)
                    - noGap_lenEdge.valueAt(noGap_lenEdge.FirstParameter)
                ).Length
            )
            Noffset_2 = abs(
                (
                    MlenEdge.valueAt(MlenEdge.LastParameter)
                    - noGap_lenEdge.valueAt(noGap_lenEdge.LastParameter)
                ).Length
            )
            Noffset2 = min(Noffset_1, Noffset_2)
            # print([Noffset1, Noffset1])
            if agap1 <= minReliefgap:
                Edgelist = selFace.ancestorsOfType(vertex0, Part.Edge)
                for ed in Edgelist:
                    if not (MlenEdge.isSame(ed)):
                        list1 = resultSolid.ancestorsOfType(ed, Part.Face)
                        for Rface in list1:
                            # print(type(Rface.Surface))
                            if not (selFace.isSame(Rface)):
                                for edge in Rface.Edges:
                                    # print(type(edge.Curve))
                                    if issubclass(type(edge.Curve),
                                                  (Part.Circle or Part.BSplineSurface)):
                                        RfaceE = Rface.makeOffsetShape(-Noffset1, 0.0, fill=True)
                                        # Part.show(RfaceE, "RfaceSolid1")
                                        CutSolids.append(RfaceE)
                                        break
            if agap2 <= minReliefgap:
                Edgelist = selFace.ancestorsOfType(vertex1, Part.Edge)
                for ed in Edgelist:
                    if not (MlenEdge.isSame(ed)):
                        list1 = resultSolid.ancestorsOfType(ed, Part.Face)
                        for Rface in list1:
                            # print(type(Rface.Surface))
                            if not (selFace.isSame(Rface)):
                                for edge in Rface.Edges:
                                    # print(type(edge.Curve))
                                    if issubclass(type(edge.Curve),
                                                  (Part.Circle or Part.BSplineSurface)):
                                        RfaceE = Rface.makeOffsetShape(-Noffset2, 0.0, fill=True)
                                        # Part.show(RfaceE, "RfaceSolid2")
                                        CutSolids.append(RfaceE)
                                        break
            
            # print([cutgap1, cutgap2])
            Ref_lenEdge = AlenEdge.copy().translate(FaceDir * -offset)
            cutgap1 = smCalcCutGap(lenEdge, Ref_lenEdge, Ref_lenEdge.FirstParameter, 
                                    agap1, extend1, reliefW, reliefD)
            cutgap2 = smCalcCutGap(lenEdge, Ref_lenEdge, Ref_lenEdge.LastParameter, 
                                    agap2, extend2, reliefW, reliefD)
            CutFace = smMakeFace(lenEdge, thkDir, thk, cutgap1, cutgap2, op="SMC")
            # Part.show(CutFace, "CutFace")
            CutSolid = CutFace.extrude(FaceDir * -offset)
            # Part.show(CutSolid, "CutSolid")
            CfaceSolid = Cface.extrude(thkDir * thk)
            # Part.show(CfaceSolid, "CfaceSolid")
            CutSolid = CutSolid.common(CfaceSolid)
            # Part.show(CutSolid, "CutSolid")
            CutSolids.append(CutSolid)

        # Produce Main Solid for Inside Bends.
        if CutSolids:
            if len(CutSolids) == 1:
                resultSolid = resultSolid.cut(CutSolids[0])
            else:
                Solid = CutSolids[0].multiFuse(CutSolids[1:])
                Solid.removeSplitter()
                # Part.show(Solid)
                resultSolid = resultSolid.cut(Solid)
        # Produce Offset Solid.
        if offset > 0.0:
            # Create wall.
            offset_face = smMakeFace(lenEdge, FaceDir, -offset, op="SMO")
            OffsetSolid = offset_face.extrude(thkDir * thk)
            resultSolid = resultSolid.fuse(OffsetSolid)

        # Adjust revolving center to new point.
        if not flipped:
            revAxisP = lenEdge.valueAt(lenEdge.FirstParameter) + thkDir * (bendR + thk)
        else:
            revAxisP = lenEdge.valueAt(lenEdge.FirstParameter) + thkDir * -bendR

        wallSolid = None
        if sketches:
            Wall_face = Part.makeFace(sketch.Shape.Wires, "Part::FaceMakerBullseye")
            if inside:
                Wall_face.translate(FaceDir * offset)
            FaceAxisP = sketch_Edge.valueAt(sketch_Edge.FirstParameter) + thkDir * thk
            FaceAxisV = (sketch_Edge.valueAt(sketch_Edge.FirstParameter)
                         - sketch_Edge.valueAt(sketch_Edge.LastParameter))
            Wall_face.rotate(FaceAxisP, FaceAxisV, -90.0)
            wallSolid = Wall_face.extrude(thkDir * -thk)
            # Part.show(wallSolid)
            wallSolid.rotate(revAxisP, revAxisV, bendA)
        elif extLen > 0.0:
            # Create wall.
            Wall_face = smMakeFace(lenEdge, FaceDir, extLen, gap1 - extend1, gap2 - extend2,
                                   miterA1List[i], miterA2List[i], op="SMW")
            # Part.show(Wall_face, "Wall_face")
            wallSolid = Wall_face.extrude(thkDir * thk)
            # Part.show(wallSolid, "wallSolid")
            wallSolid.rotate(revAxisP, revAxisV, bendA)
            # Part.show(wallSolid, "wallSolid-rotated")
            thk_faceList.append(wallSolid.Faces[2])

        if not unfold:
            # Produce bend Solid.
            if bendA > 0.0:
                # Create bend.
                # Narrow the wall if we have gaps.
                revFace = smMakeFace(lenEdge, thkDir, thk, gap1, gap2, op="SMR")
                if revFace.normalAt(0, 0) != FaceDir:
                    revFace.reverse()
                bendSolid = revFace.revolve(revAxisP, revAxisV, bendA)
                # Part.show(bendSolid)
                resultSolid = resultSolid.fuse(bendSolid)
            if wallSolid:
                resultSolid = resultSolid.fuse(wallSolid)
                # Part.show(resultSolid, "resultSolid")
            # Remove perforation
            if Perforate:
                # CHECK I'm not sure about flipped - the main one gets
                # overwritten for each sublist item.
                perfFace = smMakePerforationFace(lenEdge, thkDir, bendR, bendA, PerforationAngle,
                                                 flipped, thk, gap1, gap2,
                                                 PerforationInitialLength,
                                                 PerforationInitialLength, PerforationMaxLength,
                                                 NonperforationMaxLength, op="SMR")
                # Part.show(perfFace)
                # CHECK `Part.Compound` object has no attribute
                # `normalAt`; might need it:
                # if perfFace.normalAt(0, 0) != FaceDir:
                #     perfFace.reverse()
                if PerforationAngle > 0.0:
                    perfFace = perfFace.rotate(revAxisP, revAxisV, (bendA/2)-(PerforationAngle/2))
                    perfSolid = perfFace.revolve(revAxisP, revAxisV, PerforationAngle)
                else:
                    perfSolid = perfFace.revolve(revAxisP, revAxisV, bendA)
                # Part.show(perfSolid)
                resultSolid = resultSolid.cut(perfSolid)
        else:
            # Produce unfold Solid.
            if bendA > 0.0:
                # Create bend.
                unfoldLength = (bendR + kfactor * thk) * bendA * math.pi / 180.0
                # Narrow the wall if we have gaps.
                unfoldFace = smMakeFace(lenEdge, thkDir, thk, gap1, gap2, op="SMR")
                if unfoldFace.normalAt(0, 0) != FaceDir:
                    unfoldFace.reverse()
                unfoldSolid = unfoldFace.extrude(FaceDir * unfoldLength)
                # Part.show(unfoldSolid)
                resultSolid = resultSolid.fuse(unfoldSolid)
            if extLen > 0.0:
                # Flatten the wall back out.
                wallSolid.rotate(revAxisP, revAxisV, -bendA)
                # Part.show(wallSolid, "wallSolid")
                wallSolid.translate(FaceDir * unfoldLength)
                resultSolid = resultSolid.fuse(wallSolid)
            # Remove perforation.
            if Perforate:
                perfFace = smMakePerforationFace(lenEdge, thkDir, bendR, bendA, PerforationAngle,
                                                 flipped, thk, gap1, gap2,
                                                 PerforationInitialLength,
                                                 PerforationInitialLength, PerforationMaxLength,
                                                 NonperforationMaxLength, op="SMR")

                # CHECK 'Part.Compound' object has no attribute
                # 'normalAt' ; might need it:
                # if perfFace.normalAt(0, 0) != FaceDir:
                #     perfFace.reverse()
                if PerforationAngle > 0.0:
                    perfUnfoldLength = (bendR + kfactor * thk) * PerforationAngle * math.pi / 180.0
                    perfFace = perfFace.translate(FaceDir * (unfoldLength/2 - perfUnfoldLength/2))
                    perfSolid = perfFace.extrude(FaceDir * perfUnfoldLength)
                else:
                    perfSolid = perfFace.extrude(FaceDir * unfoldLength)
                # Part.show(perfSolid)
                resultSolid = resultSolid.cut(perfSolid)

    # Part.show(resultSolid, "resultSolid")
    return resultSolid, thk_faceList


class SMBendWall:
    def __init__(self, obj, selobj, sel_items, refAngOffset=None, checkRefFace=False):
        """Add Wall with radius bend."""
        self.addVerifyProperties(obj, refAngOffset, checkRefFace)

        _tip_ = translate("App::Property", "Base Object")
        obj.addProperty("App::PropertyLinkSub", "baseObject", "Parameters", _tip_
        ).baseObject = (selobj, sel_items)
        obj.Proxy = self
        SheetMetalTools.taskRestoreDefaults(obj, smAddWallDefaultVars)

    def addVerifyProperties(self, obj, refAngOffset=None, checkRefFace=False):
        SheetMetalTools.smAddLengthProperty(obj,
                "radius",
                translate("App::Property", "Bend Radius"),
                1.0)
        SheetMetalTools.smAddLengthProperty(obj,
                "length",
                translate("App::Property", "Length of Wall"),
                10.0)
        SheetMetalTools.smAddDistanceProperty(obj,
                "gap1",
                translate("App::Property", "Gap from Left Side"),
                0.0)
        SheetMetalTools.smAddDistanceProperty(obj,
                "gap2",
                translate("App::Property", "Gap from Right Side"),
                0.0)
        SheetMetalTools.smAddBoolProperty(obj,
                "invert",
                translate("App::Property", "Invert Bend Direction"),
                False)
        SheetMetalTools.smAddAngleProperty(obj,
                "angle",
                translate("App::Property", "Bend Angle"),
                90.0)
        SheetMetalTools.smAddDistanceProperty(obj,
                "extend1",
                translate("App::Property", "Extend from Left Side"),
                0.0)
        SheetMetalTools.smAddDistanceProperty(obj,
                "extend2",
                translate("App::Property", "Extend from Right Side"),
                0.0)
        SheetMetalTools.smAddEnumProperty(obj,
                "BendType",
                translate("App::Property", "Bend Type"),
                ["Material Outside", "Material Inside", "Thickness Outside", "Offset"])
        SheetMetalTools.smAddEnumProperty(obj,
                "LengthSpec",
                translate("App::Property", "Type of Length Specification"),
                ["Leg", "Outer Sharp", "Inner Sharp", "Tangential"])
        SheetMetalTools.smAddLengthProperty(obj,
                "reliefw",
                translate("App::Property", "Relief Width"),
                0.8,
                "ParametersRelief")
        SheetMetalTools.smAddLengthProperty(obj,
                "reliefd",
                translate("App::Property", "Relief Depth"),
                1.0,
                "ParametersRelief")
        SheetMetalTools.smAddBoolProperty(obj,
                "UseReliefFactor",
                translate("App::Property", "Use Relief Factor"),
                False,
                "ParametersRelief")
        SheetMetalTools.smAddEnumProperty(obj,
                "reliefType",
                translate("App::Property", "Relief Type"),
                ["Rectangle", "Round"],
                None,
                "ParametersRelief")
        SheetMetalTools.smAddFloatProperty(obj,
                "ReliefFactor",
                translate("App::Property", "Relief Factor"),
                0.7,
                "ParametersRelief")
        SheetMetalTools.smAddAngleProperty(obj,
                "miterangle1",
                translate("App::Property", "Bend Miter Angle from Left Side"),
                0.0,
                "ParametersMiterangle")
        SheetMetalTools.smAddAngleProperty(obj,
                "miterangle2",
                translate("App::Property", "Bend Miter Angle from Right Side"),
                0.0,
                "ParametersMiterangle")
        SheetMetalTools.smAddLengthProperty(obj,
                "minGap",
                translate("App::Property", "Auto Miter Minimum Gap"),
                0.2,
                "ParametersEx")
        SheetMetalTools.smAddLengthProperty(obj,
                "maxExtendDist",
                translate("App::Property", "Auto Miter maximum Extend Distance"),
                5.0,
                "ParametersEx")
        SheetMetalTools.smAddLengthProperty(obj,
                "minReliefGap",
                translate("App::Property", "Minimum Gap to Relief Cut"),
                1.0,
                "ParametersEx")
        SheetMetalTools.smAddDistanceProperty(obj,
                "offset",
                translate("App::Property", "Offset Bend"),
                0.0,
                "ParametersEx")
        SheetMetalTools.smAddBoolProperty(obj,
                "AutoMiter",
                translate("App::Property", "Enable Auto Miter"),
                True,
                "ParametersEx")
        SheetMetalTools.smAddBoolProperty(obj,
                "unfold",
                translate("App::Property", "Shows Unfold View of Current Bend"),
                False,
                "ParametersEx")
        SheetMetalTools.smAddProperty(obj,
                "App::PropertyFloatConstraint",
                "kfactor",
                translate("App::Property",
                          "Location of Neutral Line. Caution: Using ANSI standards, not DIN."),
                (0.5, 0.0, 1.0, 0.01),
                "ParametersEx")
        SheetMetalTools.smAddBoolProperty(obj,
                "sketchflip",
                translate("App::Property", "Flip Sketch Direction"),
                False,
                "ParametersEx2")
        SheetMetalTools.smAddBoolProperty(obj,
                "sketchinvert",
                translate("App::Property", "Invert Sketch Start"),
                False,
                "ParametersEx2")
        SheetMetalTools.smAddProperty(obj,
                "App::PropertyLink",
                "Sketch",
                translate("App::Property", "Sketch Object"),
                None,
                "ParametersEx2")
        SheetMetalTools.smAddProperty(obj,
                "App::PropertyFloatList",
                "LengthList",
                translate("App::Property", "Length of Wall List"),
                None,
                "ParametersEx3")
        SheetMetalTools.smAddProperty(obj,
                "App::PropertyFloatList",
                "bendAList",
                translate("App::Property", "Bend Angle List"),
                None,
                "ParametersEx3")
        SheetMetalTools.smAddBoolProperty(obj,
                "Perforate",
                FreeCAD.Qt.translate("App::Property", "Enable Perforation"),
                False,
                "ParametersPerforation")
        SheetMetalTools.smAddAngleProperty(obj,
                "PerforationAngle",
                FreeCAD.Qt.translate("App::Property", "Perforation Angle"),
                0.0,
                "ParametersPerforation")
        SheetMetalTools.smAddLengthProperty(obj,
                "PerforationInitialLength",
                FreeCAD.Qt.translate("App::Property", "Initial Perforation Length"),
                5.0,
                "ParametersPerforation")
        SheetMetalTools.smAddLengthProperty(obj,
                "PerforationMaxLength",
                FreeCAD.Qt.translate("App::Property", "Perforation Max Length"),
                5.0,
                "ParametersPerforation")
        SheetMetalTools.smAddLengthProperty(obj,
                "NonperforationMaxLength",
                FreeCAD.Qt.translate("App::Property", "Non-Perforation Max Length"),
                5.0,
                "ParametersPerforation")
        SheetMetalTools.smAddProperty(obj,
                "App::PropertyLinkSub",
                "OffsetFaceReference",
                "Face reference for offset",
                refAngOffset,
                "ParametersEx")
        SheetMetalTools.smAddProperty(obj,
                "App::PropertyLinkSub",
                "AngleFaceReference",
                "Face reference for angle",
                refAngOffset,
                "ParametersEx")
        SheetMetalTools.smAddAngleProperty(obj,
                "RelativeAngleToRef",
                "Relative angle to the face reference",
                0.0,
                "ParametersEx")
        SheetMetalTools.smAddEnumProperty(obj,
                "OffsetType",
                "Offset Type",
                ["Material Outside", "Material Inside", "Thickness Outside", "Offset"],
                "Material Inside",
                "ParametersEx")
        SheetMetalTools.smAddDistanceProperty(obj,
                "OffsetTypeOffset",
                "Works when offset face reference is on. It offsets by "
                "a normal distance from the offsets reference face.",
                0.0,
                "ParametersEx")
        SheetMetalTools.smAddBoolProperty(obj,
                "SupplAngleRef",
                "Supplementary angle reference",
                False,
                "ParametersEx")

    def getElementMapVersion(self, _fp, ver, _prop, restored):
        if not restored:
            return smElementMapVersion + ver
        return None

    def execute(self, fp):
        """Print a short message when doing a recomputation.

        Note:
            This method is mandatory.

        """
        self.addVerifyProperties(fp)

        # Restrict some params.
        fp.miterangle1.Value = smRestrict(fp.miterangle1.Value, -80.0, 80.0)
        fp.miterangle2.Value = smRestrict(fp.miterangle2.Value, -80.0, 80.0)

        bendAList = [fp.angle.Value]
        LengthList = [fp.length.Value]
        # print face

        # Pass selected object shape.
        Main_Object = fp.baseObject[0].Shape.copy()
        face = fp.baseObject[1]
        thk, thkDir = sheet_thk(Main_Object, face[0])

        if fp.Sketch:
            WireList = fp.Sketch.Shape.Wires[0]
            if not (WireList.isClosed()):
                LengthList, bendAList = getSketchDetails(fp.Sketch, fp.sketchflip, fp.sketchinvert,
                                                         fp.radius.Value, thk)
            else:
                if fp.Sketch.Support:
                    fp.baseObject = (fp.Sketch.Support[0][0], fp.Sketch.Support[0][1])
                LengthList = [10.0]
        fp.LengthList = LengthList
        fp.bendAList = bendAList
        # print(LengthList, bendAList)

        # Extend value needed for first bend set only.
        extend1_list = [0.0 for n in LengthList]
        extend2_list = [0.0 for n in LengthList]
        extend1_list[0] = fp.extend1.Value
        extend2_list[0] = fp.extend2.Value
        # print(extend1_list, extend2_list)

        # Gap value needed for first bend set only.
        gap1_list = [0.0 for n in LengthList]
        gap2_list = [0.0 for n in LengthList]
        gap1_list[0] = fp.gap1.Value
        gap2_list[0] = fp.gap2.Value
        # print(gap1_list, gap2_list)

        # Calculate the angle based on reference face.
        if fp.AngleFaceReference is not None:
            smObj, smSelItemName = fp.baseObject
            smSelItemName = smSelItemName[0]
            # Get the sheet metal reference face.
            smFace, refEdge, thkFace = GetSMComparisonFace(smObj, smSelItemName)
            refObj, refFace = fp.AngleFaceReference

            if "Plane" in refObj.TypeId:
                refFace = SheetMetalTools.smConvertPlaneToFace(refObj.Shape)
            else:
                refFace = refFace[0]
                refFace = refObj.Shape.getElement(refFace)

            # Angle calculation.
            angleParFace = relatAngleCalc(thkFace, refEdge, refFace, smFace)

            angle = angleParFace + fp.RelativeAngleToRef.Value

            if fp.invert == True:
                angle = 180 - angleParFace + fp.RelativeAngleToRef.Value

            # Supplementary angle option.
            if fp.SupplAngleRef == True:
                angle = 180 - angle + fp.RelativeAngleToRef.Value
            bendAList = [angle]

        offsetValue = fp.offset.Value
        # Calculate the offset based on reference face.
        if fp.BendType == "Offset" and fp.OffsetFaceReference is not None:
            smObj, smSelItemName = fp.baseObject
            smSelItemName = smSelItemName[0]
            # Get the sheet metal reference face and edge.
            smFace, refEdge, thkFace = GetSMComparisonFace(smObj, smSelItemName)
            refObj, refFace = fp.OffsetFaceReference

            if "Plane" in refObj.TypeId:
                refFace = SheetMetalTools.smConvertPlaneToFace(refObj.Shape)
            else:
                refFace = refFace[0]
                refFace = refObj.Shape.getElement(refFace)

            # Angle calculation.
            angleParFace = relatAngleCalc(thkFace, refEdge, refFace, smFace)

            if fp.invert:
                angleParFace = 180 - angleParFace

            if fp.SupplAngleRef:  # Supplementary angle option.
                angleParFace = 180 - angleParFace

            # Calculate the distance for the wall position.
            # This 'try' is needed when the reference face is 3D angle
            # rotated.
            try:
                angleParFace = angleParFace[0]
            except:
                pass

            # Get radians of the bend angle.
            radAngle = math.radians(angleParFace)
            # Get radians of half supplementary angle.
            halfSuplAngle = math.radians((180-angleParFace) / 2)

            # Calculate the distance for the wall to be inside.
            if fp.OffsetType == "Material Inside":
                distWall = (((fp.radius.Value + thk) * math.sqrt(2 - 2 * math.cos(radAngle)))/2)/math.sin(halfSuplAngle)

            # Calculate the distance for the wall (and bend radius) to
            # be outside.
            if fp.OffsetType == "Material Outside":
                distWall = 0.0

            # Calculate the distance for the wall to be one thickness
            # outside.
            if fp.OffsetType == "Thickness Outside":
                # Get radians of the complementary angle.
                wallThkAngle = math.radians(90 - angleParFace)

                distThkOut = thk / math.cos(wallThkAngle)
                distWall = (((fp.radius.Value + thk) * math.sqrt(2 - 2 * math.cos(radAngle)))/2)/math.sin(halfSuplAngle) - distThkOut

            # Calculate the distance for the wall to be normally
            # distanced from the reference face.
            if fp.OffsetType == "Offset":
                # Get radians of the complementary angle.
                wallThkAngle = math.radians(90 - angleParFace)

                distOffsetOut = (thk+fp.OffsetTypeOffset.Value) / math.cos(wallThkAngle)
                distWall = (((fp.radius.Value + thk) * math.sqrt(2 - 2 * math.cos(radAngle)))/2)/math.sin(halfSuplAngle) - distOffsetOut

            if fp.invert and fp.OffsetType != "Material Outside":
                # Get radians of the complementary angle.
                complAngle = math.radians(90 - angleParFace)

                distInvertComp = math.sin(complAngle) * (thk/math.cos(complAngle))
                distWall = distWall + distInvertComp

            offsetValue = offsetFaceDistance(smFace, refFace, refEdge, thkFace) - distWall

        for i, Length in enumerate(LengthList):
            s, f = smBend(
                thk,
                bendR=fp.radius.Value,
                bendA=bendAList[i],
                miterA1=fp.miterangle1.Value,
                miterA2=fp.miterangle2.Value,
                BendType=fp.BendType,
                flipped=fp.invert,
                unfold=fp.unfold,
                extLen=Length,
                reliefType=fp.reliefType,
                gap1=gap1_list[i],
                gap2=gap2_list[i],
                reliefW=fp.reliefw.Value,
                reliefD=fp.reliefd.Value,
                minReliefgap=fp.minReliefGap.Value,
                extend1=extend1_list[i],
                extend2=extend2_list[i],
                kfactor=fp.kfactor,
                offset=offsetValue,
                ReliefFactor=fp.ReliefFactor,
                UseReliefFactor=fp.UseReliefFactor,
                automiter=fp.AutoMiter,
                selFaceNames=face,
                MainObject=Main_Object,
                sketch=fp.Sketch,
                mingap=fp.minGap.Value,
                maxExtendGap=fp.maxExtendDist.Value,
                LengthSpec=fp.LengthSpec,
                Perforate=fp.Perforate,
                PerforationAngle=fp.PerforationAngle.Value,
                PerforationInitialLength=fp.PerforationInitialLength.Value,
                PerforationMaxLength=fp.PerforationMaxLength.Value,
                NonperforationMaxLength=fp.NonperforationMaxLength.Value)
            faces = smGetFace(f, s)
            face = faces
            Main_Object = s

        fp.Shape = s


###################################################################################################
# Gui code
###################################################################################################

if SheetMetalTools.isGuiLoaded():
    import os

    Gui = FreeCAD.Gui
    icons_path = SheetMetalTools.icons_path
    smEpsilon = SheetMetalTools.smEpsilon


    class SMViewProviderTree(SheetMetalTools.SMViewProvider):
        """Part WB style ViewProvider."""

        def getIcon(self):
            return os.path.join(icons_path, "SheetMetal_AddWall.svg")

        def getTaskPanel(self, obj):
            return SMBendWallTaskPanel(obj)


    class SMViewProviderFlat(SMViewProviderTree):
        """Part Design WB style ViewProvider.

        Note:
            Backward compatibility only.

        """


    class SMBendWallTaskPanel:
        """A TaskPanel for the SheetMetal."""

        def __init__(self, obj, checkRefFace=False):
            QtCore.QDir.addSearchPath("Icons", icons_path)
            self.obj = obj
            self.form = SheetMetalTools.taskLoadUI("FlangeParameters.ui")
            # Make sure all properties are added.
            obj.Proxy.addVerifyProperties(obj)
            # Variable to track which property should be filled when in
            # selection mode. And used to rename the form field (of face
            # reference) only when necessary.
            self.activeRefGeom = None
            self.updateForm()
            # Turn bend type to 'Offset', on case of automatic face
            # reference selection.
            self.onBendOffset(checkRefFace)

            # Flange parameters connects.
            self.selParams = SheetMetalTools.taskConnectSelection(
                self.form.AddRemove, self.form.tree, self.obj, ["Edge"], self.form.pushClearSel)
            SheetMetalTools.taskConnectEnum(obj, self.form.BendType, "BendType", self.bendTypeUpdated)
            SheetMetalTools.taskConnectSpin(obj, self.form.Offset, "offset")
            SheetMetalTools.taskConnectSpin(obj, self.form.Radius, "radius")
            SheetMetalTools.taskConnectSpin(obj, self.form.Angle, "angle")
            SheetMetalTools.taskConnectSpin(obj, self.form.Length, "length")
            SheetMetalTools.taskConnectEnum(obj, self.form.LengthSpec, "LengthSpec")
            SheetMetalTools.taskConnectCheck(obj, self.form.UnfoldCheckbox, "unfold")
            SheetMetalTools.taskConnectSpin(obj, self.form.gap1, "gap1")
            SheetMetalTools.taskConnectSpin(obj, self.form.gap2, "gap2")
            SheetMetalTools.taskConnectSpin(obj, self.form.extend1, "extend1")
            SheetMetalTools.taskConnectSpin(obj, self.form.extend2, "extend2")
            # Advanced flange parameters connects.
            self.form.reliefTypeButtonGroup.buttonToggled.connect(self.reliefTypeUpdated)
            SheetMetalTools.taskConnectSpin(obj, self.form.reliefWidth, "reliefw")
            SheetMetalTools.taskConnectSpin(obj, self.form.reliefDepth, "reliefd")
            SheetMetalTools.taskConnectCheck(obj, self.form.autoMiterCheckbox, "AutoMiter", self.autoMiterChanged)
            SheetMetalTools.taskConnectSpin(obj, self.form.minGap, "minGap")
            SheetMetalTools.taskConnectSpin(obj, self.form.maxExDist, "maxExtendDist")
            SheetMetalTools.taskConnectSpin(obj, self.form.miterAngle1, "miterangle1")
            SheetMetalTools.taskConnectSpin(obj, self.form.miterAngle2, "miterangle2")
            # Perforation.
            SheetMetalTools.taskConnectCheck(obj, self.form.checkPerforate, "Perforate", self.perforateChanged)
            SheetMetalTools.taskConnectSpin(obj, self.form.perforateAngle, "PerforationAngle")
            SheetMetalTools.taskConnectSpin(obj, self.form.perforateInitialCutLen, "PerforationInitialLength")
            SheetMetalTools.taskConnectSpin(obj, self.form.perforateMaxCutLen, "PerforationMaxLength")
            SheetMetalTools.taskConnectSpin(obj, self.form.perforateMaxTabLen, "NonperforationMaxLength")

            self.offsetRefSelParams = SheetMetalTools.taskConnectSelectionToggle(
                self.form.SelOffsetFace, self.form.OffsetFaceRef, self.obj, "OffsetFaceReference",
                [("Plane", []), ["Face"]])
            self.offsetRefSelParams.setVisibilityControlledWidgets(
                [(self.form.frameOffType, True)], [(self.form.Offset, False)])
            self.offsetRefSelParams.HideRefObject = False
            SheetMetalTools.taskConnectEnum(obj, self.form.OffsetTypes, "OffsetType", self.updateForm)
            SheetMetalTools.taskConnectSpin(obj, self.form.OffsetTypeOffset, "OffsetTypeOffset", self.updateForm)
            self.angleRefSelParams = SheetMetalTools.taskConnectSelectionToggle(
                self.form.SelAngleFace, self.form.AngleFaceRef, self.obj, "AngleFaceReference",
                [("Plane", []), ["Face"]])
            self.angleRefSelParams.setVisibilityControlledWidgets(
                [(self.form.frameRelatAngle, True)], [(self.form.Angle, False)])
            self.angleRefSelParams.HideRefObject = False
            SheetMetalTools.taskConnectSpin(obj, self.form.RelativeAngle, "RelativeAngleToRef", self.updateForm)

            # Button reversed wall.
            self.form.buttRevWall.clicked.connect(self.revWall)  # Button click action.

        # Turn bend type to 'Offset', on case of automatic face
        # reference selection.
        def onBendOffset(self, test):
            if test:
                self.obj.BendType = "Offset"
                self.updateForm()

        def revWall(self):  # Button to flip the wall side.
            self.obj.invert = not self.obj.invert
            self.updateForm()

        def isAllowedAlterSelection(self):
            return True

        def isAllowedAlterView(self):
            return True

        def bendTypeUpdated(self, value):
            if self.obj.BendType == "Offset":
                self.form.Offset.setEnabled(True)
            else:
                self.form.Offset.setEnabled(False)
                self.obj.OffsetType = "Material Inside"
                self.form.OffsetTypes.setCurrentIndex(1)

            # Updates of offset face reference mode.
            self.form.frameOffFaceRef.setVisible(self.obj.BendType == "Offset")
            self.form.frameOffType.setVisible(
                self.obj.BendType == "Offset" and self.obj.OffsetFaceReference is not None)

        def reliefTypeUpdated(self):
            self.obj.reliefType = (
                "Rectangle" if self.form.reliefRectangle.isChecked() else "Round")
            self.obj.Document.recompute()

        # `value` parameter just to easily use this function as
        # a callback in form connections.
        def updateForm(self, value=None):
            self.form.Offset.setEnabled(self.obj.BendType == "Offset")
            SheetMetalTools.taskPopulateSelectionList(self.form.tree, self.obj.baseObject)

            # Advanced parameters update.
            if self.obj.reliefType == "Rectangle":
                self.form.reliefRectangle.setChecked(True)
            else:
                self.form.reliefRound.setChecked(True)

            # Button flip the wall - updates check.
            self.form.buttRevWall.setChecked(self.obj.invert)

            # Updates of offset face reference mode.
            self.form.frameOffFaceRef.setVisible(self.obj.BendType == "Offset")
            self.form.frameOffType.setVisible(
                self.obj.BendType == "Offset" and self.obj.OffsetFaceReference is not None)
            self.form.frameOffOff.setVisible(
                self.form.frameOffType.isVisible() and self.obj.OffsetType == "Offset")

            # Updates the angle in model.
            self.obj.recompute()

            self.obj.Document.recompute()
            # Updates the value of angle form field when the face
            # reference is activated.
            self.form.Angle.setProperty("value", getattr(self.obj, "angle"))
            # Updates the value of offset form field when the face
            # reference is activated.
            self.form.Offset.setProperty("value", getattr(self.obj, "offset"))

        def perforateChanged(self, isPerforate):
            self.form.groupPerforate.setEnabled(isPerforate)

        def autoMiterChanged(self, isAutoMiter):
            self.form.groupAutoMiter.setEnabled(isAutoMiter)
            self.form.groupManualMiter.setEnabled(not isAutoMiter)

        def accept(self):
            SheetMetalTools.taskAccept(self)
            SheetMetalTools.taskSaveDefaults(self.obj, smAddWallDefaultVars)
            return True

        def reject(self):
            SheetMetalTools.taskReject(self)


    class AddWallCommandClass:
        """Add Wall command."""

        def GetResources(self):
            return {
                    # The name of a svg file available in the resources.
                    "Pixmap": os.path.join(icons_path, "SheetMetal_AddWall.svg"),
                    "MenuText": FreeCAD.Qt.translate("SheetMetal", "Make Wall"),
                    "Accel": "W",
                    "ToolTip": FreeCAD.Qt.translate(
                        "SheetMetal",
                        "Extends one or more face, connected by a bend on existing sheet metal.\n"
                        "1. Select edges to create bends with walls.\n"
                        "2. Use Property editor to modify other parameters",
                        ),
                    }

        def Activated(self):
            doc = FreeCAD.ActiveDocument
            view = Gui.ActiveDocument.ActiveView
            activeBody = None

            # Get the sheet metal object.
            try:
                for obj in Gui.Selection.getSelectionEx():
                    if not "Plane" in obj.Object.TypeId:
                        for subElem in obj.SubElementNames:
                            if type(obj.Object.Shape.getElement(subElem)) == Part.Edge:
                                sel = obj
                                break
                selobj = sel.Object
            except:
                raise Exception("At least one edge must be selected to create a wall.")

            selSubNames = list(sel.SubElementNames)
            selSubObjs = sel.SubObjects

            # Remove faces for wall creation reference because only
            # edges should be used as reference to create walls.
            for subObjName in selSubNames:
                if type(selobj.Shape.getElement(subObjName)) == Part.Face:
                    selSubNames.remove(subObjName)
                    if len(selSubNames) < 1:
                        raise Exception("At least one edge must be selected to create a wall.")

            # Get only one selected face to use for reference to angle
            # and offset.
            faceCount = 0
            refAngOffset = None
            checkRefFace = False
            for obj in Gui.Selection.getSelectionEx():
                for subObj in obj.SubObjects:
                    if type(subObj) == Part.Face and not "Plane" in obj.Object.TypeId:
                        faceCount += 1
                        if faceCount == 1:
                            for subObjName in obj.SubElementNames:
                                if obj.Object.Shape.getElement(subObjName).isEqual(subObj):
                                    refAngOffset = [obj.Object, subObjName]
                                    checkRefFace = True
                        else:
                            print("If more than one face is selected, "
                                  "only the first is used for "
                                  "reference to angle and offset.")
                if "Plane" in obj.Object.TypeId and faceCount == 0:
                    if obj.Object.TypeId == "App::Plane":
                        refAngOffset = [obj.Object, ""]
                    else:
                        refAngOffset = [obj.Object, obj.SubElementNames[0]]
                    checkRefFace = True

            viewConf = SheetMetalTools.GetViewConfig(selobj)
            if hasattr(view, "getActiveObject"):
                activeBody = view.getActiveObject("pdbody")
            if not SheetMetalTools.smIsOperationLegal(activeBody, selobj):
                return
            doc.openTransaction("Bend")
            if activeBody is None or not SheetMetalTools.smIsPartDesign(selobj):
                newObj = doc.addObject("Part::FeaturePython", "Bend")
                SMBendWall(newObj, selobj, selSubNames, refAngOffset, checkRefFace)
                SMViewProviderTree(newObj.ViewObject)
            else:
                # FreeCAD.Console.PrintLog("found active body: " + activeBody.Name)
                newObj = doc.addObject("PartDesign::FeaturePython", "Bend")
                SMBendWall(newObj, selobj, selSubNames, refAngOffset, checkRefFace)
                SMViewProviderFlat(newObj.ViewObject)
                activeBody.addObject(newObj)
            SheetMetalTools.SetViewConfig(newObj, viewConf)
            Gui.Selection.clearSelection()
            if SheetMetalTools.is_autolink_enabled():
                root = SheetMetalTools.getOriginalBendObject(newObj)
                if root:
                    if hasattr(root, "Radius"):
                        newObj.setExpression("radius", root.Label + ".Radius")
                    elif hasattr(root, "radius"):
                        newObj.setExpression("radius", root.Label + ".radius")
            newObj.baseObject[0].ViewObject.Visibility = False
            doc.recompute()
            # 'checkRefFace' turn bendtype to 'Offset' when face
            # reference is before the command.
            dialog = SMBendWallTaskPanel(newObj, checkRefFace)
            SheetMetalTools.updateTaskTitleIcon(dialog)
            Gui.Control.showDialog(dialog)
            return

        def IsActive(self):
            selobj = Gui.Selection.getSelectionEx()
            objSM = None
            # In this iteration, we will find which selected object is
            # the sheet metal part, 'cause the user can select a face
            # for reference (this object will not be the sheet metal
            # part).
            for obj in selobj:
                for subObj in obj.SubObjects:
                    if type(subObj) == Part.Edge:
                        objSM = obj

            # Test if any selected subObject in the sheet metal
            # isn't edge.
            geomTest = []
            for subObj in objSM.SubObjects:
                if type(subObj) == Part.Edge:
                    geomTest.append(True)
                else:
                    geomTest.append(False)

            if not False in geomTest and geomTest is not None:
                return True
            return None


    Gui.addCommand("SheetMetal_AddWall", AddWallCommandClass())
