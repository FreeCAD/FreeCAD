# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2021 Russell Johnson (russ4262) <russ4262@gmail.com>    *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
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

import FreeCAD
import Part
import PathScripts.PathGeom as PathGeom
import PathScripts.PathLog as PathLog
import math
from PySide import QtCore

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

DraftGeomUtils = LazyLoader("DraftGeomUtils", globals(), "DraftGeomUtils")
PathUtils = LazyLoader("PathScripts.PathUtils", globals(), "PathScripts.PathUtils")
TechDraw = LazyLoader("TechDraw", globals(), "TechDraw")


__title__ = "Path Selection Processing"
__author__ = "russ4262 (Russell Johnson)"
__url__ = "http://www.freecadweb.org"
__doc__ = (
    "Collection of classes and functions used to process and refine user selections."
)
__contributors__ = ""


PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
# PathLog.trackModule(PathLog.thisModule())

isRoughly = PathGeom.isRoughly
Tolerance = PathGeom.Tolerance
isVertical = PathGeom.isVertical
isHorizontal = PathGeom.isHorizontal

# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


def makeBoundBoxFace(bBox, offset=0.0, zHeight=0.0):
    """makeBoundBoxFace(bBox, offset=0.0, zHeight=0.0)...
    Function to create boundbox face, with possible extra offset and custom Z-height."""
    p1 = FreeCAD.Vector(bBox.XMin - offset, bBox.YMin - offset, zHeight)
    p2 = FreeCAD.Vector(bBox.XMax + offset, bBox.YMin - offset, zHeight)
    p3 = FreeCAD.Vector(bBox.XMax + offset, bBox.YMax + offset, zHeight)
    p4 = FreeCAD.Vector(bBox.XMin - offset, bBox.YMax + offset, zHeight)

    L1 = Part.makeLine(p1, p2)
    L2 = Part.makeLine(p2, p3)
    L3 = Part.makeLine(p3, p4)
    L4 = Part.makeLine(p4, p1)

    return Part.Face(Part.Wire([L1, L2, L3, L4]))


def combineHorizontalFaces(faces):
    """combineHorizontalFaces(faces)...
    This function successfully identifies and combines multiple connected faces and
    works on multiple independent faces with multiple connected faces within the list.
    The return value is list of simplifed faces.
    The Adaptive op is not concerned with which hole edges belong to which face.

    Attempts to do the same shape connecting failed with TechDraw.findShapeOutline() and
    PathGeom.combineConnectedShapes(), so this algorithm was created.
    """
    horizontal = list()
    offset = 10.0
    topFace = None
    innerFaces = list()

    # Verify all incomming faces are at Z=0.0
    for f in faces:
        if f.BoundBox.ZMin != 0.0:
            f.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - f.BoundBox.ZMin))

    # Make offset compound boundbox solid and cut incoming face extrusions from it
    allFaces = Part.makeCompound(faces)
    if hasattr(allFaces, "Area") and isRoughly(allFaces.Area, 0.0):
        msg = translate(
            "PathGeom",
            "Zero working area to process. Check your selection and settings.",
        )
        PathLog.info(msg)
        return horizontal

    afbb = allFaces.BoundBox
    bboxFace = makeBoundBoxFace(afbb, offset, -5.0)
    bboxSolid = bboxFace.extrude(FreeCAD.Vector(0.0, 0.0, 10.0))
    extrudedFaces = list()
    for f in faces:
        extrudedFaces.append(f.extrude(FreeCAD.Vector(0.0, 0.0, 6.0)))

    # Fuse all extruded faces together
    allFacesSolid = extrudedFaces.pop()
    for i in range(len(extrudedFaces)):
        temp = extrudedFaces.pop().fuse(allFacesSolid)
        allFacesSolid = temp
    cut = bboxSolid.cut(allFacesSolid)

    # Identify top face and floating inner faces that are the holes in incoming faces
    for f in cut.Faces:
        fbb = f.BoundBox
        if isRoughly(fbb.ZMin, 5.0) and isRoughly(fbb.ZMax, 5.0):
            if (
                isRoughly(afbb.XMin - offset, fbb.XMin)
                and isRoughly(afbb.XMax + offset, fbb.XMax)
                and isRoughly(afbb.YMin - offset, fbb.YMin)
                and isRoughly(afbb.YMax + offset, fbb.YMax)
            ):
                topFace = f
            else:
                innerFaces.append(f)

    if not topFace:
        return horizontal

    outer = [Part.Face(w) for w in topFace.Wires[1:]]

    if outer:
        for f in outer:
            f.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - f.BoundBox.ZMin))

        if innerFaces:
            inner = innerFaces

            for f in inner:
                f.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - f.BoundBox.ZMin))
            innerComp = Part.makeCompound(inner)
            outerComp = Part.makeCompound(outer)
            cut = outerComp.cut(innerComp)
            for f in cut.Faces:
                horizontal.append(f)
        else:
            horizontal = outer

    return horizontal


def flattenWireSingleLoop(wire, trgtDep=0.0):
    """_flattenWire(wire, trgtDep=0.0)...
    Return a flattened version of the wire loop passed in.
    It is assumed that when flattened, the wire contain no more
    than a single loop."""

    if not wire.isClosed():
        msg = translate("PathGeom", "Wire received is not closed.")
        PathLog.debug("flattenWireSingleLoop(): " + msg)

    wBB = wire.BoundBox
    if isRoughly(wBB.ZLength, 0.0):
        # Return copy of horizontal wire
        srtWire = Part.Wire(Part.__sortEdges__(wire.Edges))
        srtWire.translate(FreeCAD.Vector(0, 0, trgtDep - srtWire.BoundBox.ZMin))
        return srtWire

    # Extrude non-horizontal wire
    extFwdLen = (wBB.ZLength + 2.0) * 2.0
    wireExtrude = wire.extrude(FreeCAD.Vector(0, 0, extFwdLen))

    # Create cross-section of extrusion
    sliceZ = wire.BoundBox.ZMin + (extFwdLen / 2)
    sectionWires = wireExtrude.slice(FreeCAD.Vector(0, 0, 1), sliceZ)
    if len(sectionWires) == 0:
        return None

    # return translated wire
    flatWire = sectionWires[0]
    flatWire.translate(FreeCAD.Vector(0, 0, trgtDep - flatWire.BoundBox.ZMin))
    return flatWire


def getHorizFaceFromVertFaceLoop(vertFaces):
    """getHorizFaceFromVertFaceLoop(vertFaces)...
    Return the horizontal cross-section from a loop of vertical faces."""

    # Check if selected vertical faces form a loop
    if len(vertFaces) == 0:
        return list()

    horizFaces = list()
    vertical = PathGeom.combineConnectedShapes(vertFaces)
    vWires = [
        TechDraw.findShapeOutline(shape, 1, FreeCAD.Vector(0, 0, 1))
        for shape in vertical
    ]
    for wire in vWires:
        w = PathGeom.removeDuplicateEdges(wire)
        if w.isClosed():
            face = Part.Face(w)
            # face.tessellate(0.1)
            if isRoughly(face.Area, 0):
                PathLog.debug(
                    translate(
                        "PathPocket", "Vertical face(s) do not form a loop - ignoring"
                    )
                )
            else:
                horizFaces.append(face)
        else:
            PathLog.debug(translate("PathPocket", "Vertical face(s) loop not closed."))

    return horizFaces


def isVerticalExtrusionFace(face):
    """isVerticalExtrusionFace(face)...
    Return True if the face provided exhibits characteristics of a wire
    that has been vertically extruded, creating a vertical extrusion face.
    This method also attempts to identify bsplines that are vertically extruded.
    This method may require additional refinement at a later date."""

    fBB = face.BoundBox
    if isRoughly(fBB.ZLength, 0.0):
        return False
    if isRoughly(face.normalAt(0, 0).z, 0.0):
        return True

    extr = face.extrude(FreeCAD.Vector(0.0, 0.0, fBB.ZLength)).removeSplitter()
    if hasattr(extr, "Volume"):
        if isRoughly(extr.Volume, 0.0):
            return True
        if extr.Volume < face.Area * Tolerance:
            PathLog.debug(
                "isVerticalExtrusionFace() Check if extruded face is vertical"
            )
            return True
        else:
            # PathLog.debug("extr.Volume: {}".format(extr.Volume))
            # PathLog.debug("face.Area: {}".format(face.Area))
            # PathLog.debug("extr.Volume < face.Area * Tolerance: {}".format(face.Area * Tolerance))
            # PathLog.debug("Face count: {}".format(len(extr.Faces)))
            PathLog.debug("Face.normalAt(): {}".format(face.normalAt(0, 0)))

    return False


def extrudeNonVerticalFaces(faceList, extent):
    extVect = FreeCAD.Vector(0.0, 0.0, extent)
    extrudedFaces = list()
    for f in faceList:
        if not isVertical(f):
            extrudedFaces.append(f.extrude(extVect))
    return extrudedFaces


def fuseShapes(shapeList):
    fCnt = len(shapeList)
    if fCnt == 0:
        return None
    fusion = shapeList[0]
    for i in range(1, fCnt):
        fused = fusion.fuse(shapeList[i])
        fusion = fused
    # return fusion.removeSplitter()
    return fusion


def extrudeFacesToSolid(faceList, extent):
    # Extrude well beyond start depth
    extVect = FreeCAD.Vector(0.0, 0.0, extent)
    extrudeFaces = [shp.extrude(extVect) for shp in faceList]

    # Fuse faces together
    if len(faceList) == 1:
        return extrudeFaces[0]
    else:
        fused = fuseShapes(extrudeFaces)
        if fused:
            return fused.removeSplitter()
    return None


def get3DEnvelope(baseShape, faceList, envTargetHeight):
    """get3DEnvelope(baseShape, faceList, envTargetHeight)...
    Take list of faces pertaining to base shape provided and extrude them upward to envelop target height.
    Returns the envelope of extruded faces as a fused solid.
    """
    bsBB = baseShape.BoundBox

    # Extrude all non-vertical faces upward
    extrudedFaces = list()
    zLen = bsBB.ZLength
    extrLen = math.floor((2.0 * zLen) + 20.0)
    extVect = FreeCAD.Vector(0.0, 0.0, extrLen)
    for f in faceList:
        if not isVertical(f):
            extrudedFaces.append(f.extrude(extVect))
    extCnt = len(extrudedFaces)
    if extCnt == 0:
        return None

    # Fuse extrusions together into single solid
    if extCnt == 1:
        solid = extrudedFaces
    else:
        solid = extrudedFaces.pop()
        for i in range(extCnt - 1):
            fusion = solid.fuse(extrudedFaces.pop())
            solid = fusion

    # Cut off the top of the solid safely above original baseShape.ZMax height
    topBox = Part.makeBox(bsBB.XLength + 10.0, bsBB.YLength + 10.0, extrLen)
    topBox.translate(FreeCAD.Vector(bsBB.XMin - 5.0, bsBB.YMin - 5.0, envTargetHeight))
    targetShape = solid.cut(
        topBox
    )  # was `clean.cut(topBox)`, but removeSplitter() is causing issues

    return targetShape


def flattenFace(shape):
    """flattenFace(shape)...
    This method attempts to return a horizontal cross-section of a single face - a vertical projection of the face.
    """
    if not isinstance(shape, Part.Face):
        return None

    fBB = shape.BoundBox
    zLen = fBB.ZLength
    extrLen = math.floor((2.0 * zLen) + 10.0)
    extrusion = shape.extrude(FreeCAD.Vector(0.0, 0.0, extrLen))

    clean = extrusion.removeSplitter()

    # Cut off the top of the extrusion safely above original baseShape.ZMax height
    topBox = Part.makeBox(fBB.XLength + 10.0, fBB.YLength + 10.0, extrLen)
    cutZ = math.floor(fBB.ZMin + (extrLen / 2.0))
    topBox.translate(FreeCAD.Vector(fBB.XMin - 5.0, fBB.YMin - 5.0, cutZ))
    targetShape = clean.cut(topBox).removeSplitter()

    for f in targetShape.Faces:
        if isRoughly(f.BoundBox.ZMin, cutZ):
            f.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - f.BoundBox.ZMin))
            return f

    return None


def flattenVerticalFace(face):
    """flattenVerticalFace(shape)...
    This method attempts to return a horizontal cross-section of a single vertical face - a vertical projection of the face.
    """
    if not isinstance(face, Part.Face):
        return None

    wire = TechDraw.findShapeOutline(face, 1, FreeCAD.Vector(0, 0, 1))
    wire.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - wire.BoundBox.ZMin))
    return wire


def getBaseCrossSection(baseShape, includeInternals=True, isDebug=False):
    bsBB = baseShape.BoundBox
    extrLen = math.floor(bsBB.ZLength * 4.0 + 10.0)
    extrudedFaces = extrudeNonVerticalFaces(baseShape.Faces, extrLen)
    fusion = fuseShapes(extrudedFaces)

    if not fusion:
        if isDebug:
            PathLog.error("no fusion")
        return None

    # Cut off the top of the solid safely above original baseShape.ZMax height
    xLen = bsBB.XLength + 10.0
    yLen = bsBB.YLength + 10.0
    extrLen2 = math.floor(extrLen / 2.0)
    topBox = Part.makeBox(xLen, yLen, extrLen)
    topBox.translate(
        FreeCAD.Vector(bsBB.XMin - 5.0, bsBB.YMin - 5.0, bsBB.ZMin + extrLen2)
    )

    if includeInternals:
        cut = fusion.cut(topBox).removeSplitter()
        cut.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - cut.BoundBox.ZMax))

        faceComp = None
        faceList = [f for f in cut.Faces if isRoughly(f.BoundBox.ZMin, 0.0)]
        if faceList:
            faceComp = Part.makeCompound(faceList)
        return faceComp
    else:
        cut = topBox.cut(fusion).removeSplitter()
        cut.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - cut.BoundBox.ZMin))

        faceComp = None
        for f in cut.Faces:
            fBB = f.BoundBox
            if (
                isRoughly(fBB.ZMin, 0.0)
                and isRoughly(fBB.XLength, xLen)
                and isRoughly(fBB.YLength, yLen)
            ):
                faceComp = Part.makeCompound([Part.Face(f.Wires[1])])
                break
        return faceComp


def splitClosedWireAtTwoVertexes(closedWire, vertA, vertB, tolerance):
    """splitClosedWireAtTwoVertexes(self, wire, vertA, vertB) ...
    Returns two parts of a closed wire, split at the provided vertexes.  The vertexes must now be the same.
    """
    PathLog.track()

    if not closedWire.isClosed():
        PathLog.debug("closedWire is not closed")
        return (None, None)

    pnt_A = vertA.Point
    pnt_B = vertB.Point

    # Check if points are the same
    if isRoughly(pnt_A.sub(pnt_B).Length, 0.0):
        PathLog.debug("Two vertexes are roughly the same point")
        return (closedWire, closedWire)

    lenE = len(closedWire.Edges)
    edgesIdxs = [i for i in range(0, lenE)]
    indexesA = list()
    missingVertA = -1
    missingVertB = -1

    # Cycle through edges in wire to identify the edge that aligns with point A
    for idx in range(lenE):
        i = edgesIdxs.pop(0)
        edge = closedWire.Edges[i]
        # Check if first edge vertex matches first point
        if edge.Vertexes[0].Point.sub(pnt_A).Length <= tolerance:
            # PathLog.debug("missingVertA: {}".format(i))
            indexesA.append(i)
            missingVertA = i
            # Check if target wire section is a single edge
            if edge.Vertexes[1].Point.sub(pnt_B).Length <= tolerance:
                wireA = Part.Wire([edge])
                wireB = Part.Wire(
                    Part.__sortEdges__([closedWire.Edges[i] for i in edgesIdxs])
                )
                return (wireA, wireB)
            break
        else:
            edgesIdxs.append(i)

    # Exit on failure
    if missingVertA == -1:
        PathLog.debug("Did not find vertA")
        return (None, None)

    # Cycle through edges in wire to identify the edge that aligns with point B
    for idx in range(len(edgesIdxs)):
        i = edgesIdxs.pop(0)
        edge = closedWire.Edges[i]
        indexesA.append(i)
        # Check if last edge vertex matches second point
        if edge.Vertexes[1].Point.sub(pnt_B).Length <= tolerance:
            # PathLog.debug("missingVertB: {}".format(i))
            missingVertB = i
            break

    # Exit on failure
    if missingVertB == -1:
        PathLog.debug("Did not find vertB")
        return (None, None)

    wireA = Part.Wire(Part.__sortEdges__([closedWire.Edges[i] for i in indexesA]))
    wireB = Part.Wire(Part.__sortEdges__([closedWire.Edges[i] for i in edgesIdxs]))

    return (wireA, wireB)


# Collision avoidance related method
def getOverheadRegionsAboveHeight(baseShape, height, isDebug=False):
    """getOverheadRegionsAboveHeight(baseShape, height)...
    This method tries to determine if any overhead regions exist on the baseShape above provided height.
    Determine vertical projection of entire baseShape down to height provided.
    The face(s) are used for collision avoidance.
    Return value is None or a Part.Compound() object.
    """
    bsBB = baseShape.BoundBox
    extra = 10.0

    if height <= bsBB.ZMin:
        return getBaseCrossSection(baseShape)

    if height >= bsBB.ZMax:
        return None

    # Cut off all baseShape below height
    extDist1 = height - (bsBB.ZMin - extra)
    bottomBox = Part.makeBox(bsBB.XLength + 2.0, bsBB.YLength + 2.0, extDist1)
    bottomBox.translate(
        FreeCAD.Vector(bsBB.XMin - 1.0, bsBB.YMin - 1.0, bsBB.ZMin - extra)
    )
    overheadShape = baseShape.cut(
        bottomBox
    )  # base shape cut off bottom portion at height

    # If nothing remains above final depth, return None
    if not hasattr(overheadShape, "Volume"):
        return None
    if isRoughly(overheadShape.Volume, 0.0):
        return None

    if isDebug:
        Part.show(overheadShape)
        FreeCAD.ActiveDocument.ActiveObject.Label = "overheadShape"
        FreeCAD.ActiveDocument.ActiveObject.purgeTouched()

    # Relocate bottom of overhead shape to Z=0.0
    overheadShape.translate(
        FreeCAD.Vector(0.0, 0.0, 0.0 - overheadShape.BoundBox.ZMin)
    )  # Place bottom at zero

    # Extrude all non-vertical faces upward
    zLen = bsBB.ZLength
    extrLen = math.floor((4.0 * zLen) + 20.0)
    extrudedFaces = extrudeNonVerticalFaces(overheadShape.Faces, extrLen)
    extCnt = len(extrudedFaces)
    if extCnt == 0:
        return None

    # Fuse extrusions together into single solid and remove splitters
    if extCnt == 1:
        solid = extrudedFaces[0]
    else:
        solid = extrudedFaces.pop()
        for i in range(extCnt - 1):
            fusion = solid.fuse(extrudedFaces.pop())
            solid = fusion

    if isDebug:
        Part.show(solid)
        FreeCAD.ActiveDocument.ActiveObject.Label = "solid"
        FreeCAD.ActiveDocument.ActiveObject.purgeTouched()

    # Cut off the top of the solid safely above original baseShape.ZMax height
    topBox = Part.makeBox(bsBB.XLength + 10.0, bsBB.YLength + 10.0, extrLen)
    cutZ = math.floor(height + (extrLen / 2.0))
    topBox.translate(FreeCAD.Vector(bsBB.XMin - 5.0, bsBB.YMin - 5.0, cutZ))
    targetShape = solid.cut(topBox)

    # DO NOT REMOVE
    # This `addObject()` seems to be required in order to force internal shape recompute/update of targetShape
    obj = FreeCAD.ActiveDocument.addObject("Part::Feature", "TmpShape")
    obj.Shape = targetShape
    obj.purgeTouched()
    FreeCAD.ActiveDocument.removeObject(obj.Name)

    if isDebug:
        Part.show(targetShape)
        FreeCAD.ActiveDocument.ActiveObject.Label = "targetShape"
        FreeCAD.ActiveDocument.ActiveObject.purgeTouched()

    targetShape.translate(
        FreeCAD.Vector(0.0, 0.0, 0.0 - targetShape.BoundBox.ZMax)
    )  # Place target face(s) at Z=0.0
    # filter out only top horizontal faces of interest
    overheadFaces = list()
    for f in targetShape.Faces:
        if isRoughly(f.BoundBox.ZMin, 0.0):
            overheadFaces.append(f)

    if overheadFaces:
        comp = Part.makeCompound(overheadFaces)
        return comp

    return None


def getRegionsBelowHeight(baseShape, height, isDebug=False):
    """getRegionsBelowHeight(baseShape, height)...
    This method returns a solid representing all regions that exist on the baseShape below provided height.
    Determine vertical projection of entire baseShape starting at height provided.
    The face(s) are used for collision avoidance.
    Return value is None or a Part.Compound() object.
    """
    bsBB = baseShape.BoundBox
    extra = 10.0

    if height >= bsBB.ZMax:
        return getBaseCrossSection(baseShape)

    if height <= bsBB.ZMin:
        return None

    # Cut off all baseShape below height
    extDist1 = (bsBB.ZMax - height) + extra
    bottomBox = Part.makeBox(bsBB.XLength + 2.0, bsBB.YLength + 2.0, extDist1)
    bottomBox.translate(FreeCAD.Vector(bsBB.XMin - 1.0, bsBB.YMin - 1.0, height))
    lowerShape = baseShape.cut(bottomBox)  # base shape cut off bottom portion at height

    # If nothing remains above final depth, return None
    if not hasattr(lowerShape, "Volume"):
        return None
    if isRoughly(lowerShape.Volume, 0.0):
        return None

    if isDebug:
        Part.show(lowerShape)
        FreeCAD.ActiveDocument.ActiveObject.Label = "lowerShape"
        FreeCAD.ActiveDocument.ActiveObject.purgeTouched()

    # Relocate bottom of overhead shape to Z=0.0
    lowerShape.translate(
        FreeCAD.Vector(0.0, 0.0, -10.0 - lowerShape.BoundBox.ZMax)
    )  # Place top at -10

    # Extrude all non-vertical faces upward
    zLen = bsBB.ZLength
    extrLen = math.floor((4.0 * zLen) + 20.0)
    extrudedFaces = extrudeNonVerticalFaces(lowerShape.Faces, extrLen)
    extCnt = len(extrudedFaces)
    if extCnt == 0:
        return None

    # Fuse extrusions together into single solid and remove splitters
    if extCnt == 1:
        solid = extrudedFaces[0]
    else:
        solid = extrudedFaces.pop()
        for i in range(extCnt - 1):
            fusion = solid.fuse(extrudedFaces.pop())
            solid = fusion

    if isDebug:
        Part.show(solid)
        FreeCAD.ActiveDocument.ActiveObject.Label = "solid"
        FreeCAD.ActiveDocument.ActiveObject.purgeTouched()

    # clean = solid.removeSplitter()  # removeSplitter() causes errors with some shapes

    # Cut off the top of the solid safely above original baseShape.ZMax height
    topBox = Part.makeBox(bsBB.XLength + 10.0, bsBB.YLength + 10.0, extrLen)
    # cutZ = math.floor(height + (extrLen / 2.0))
    # topBox.translate(FreeCAD.Vector(bsBB.XMin - 5.0, bsBB.YMin - 5.0, cutZ))
    targetShape = solid.cut(topBox)

    # DO NOT REMOVE
    # This `addObject()` seems to be required in order to force internal shape recompute/update of targetShape
    obj = FreeCAD.ActiveDocument.addObject("Part::Feature", "TmpShape")
    obj.Shape = targetShape
    obj.purgeTouched()
    FreeCAD.ActiveDocument.removeObject(obj.Name)

    if isDebug:
        Part.show(targetShape)
        FreeCAD.ActiveDocument.ActiveObject.Label = "targetShape"
        FreeCAD.ActiveDocument.ActiveObject.purgeTouched()

    # targetShape.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - targetShape.BoundBox.ZMax))  # Place target face(s) at Z=0.0
    # filter out only top horizontal faces of interest
    overheadFaces = list()
    for f in targetShape.Faces:
        if isRoughly(f.BoundBox.ZMin, 0.0):
            overheadFaces.append(f)

    if overheadFaces:
        comp = Part.makeCompound(overheadFaces)
        return comp

    return None


def getThickCrossSection(baseShape, bottom, top, isDebug=False):
    """getThickCrossSection(baseShape, bottom, top, isDebug=False)...
    This method returns a solid representing all regions that exist on the baseShape below provided height.
    Determine vertical projection of entire baseShape starting at height provided.
    The face(s) are used for collision avoidance.
    Return value is None or a Part.Compound() object.
    """
    bsBB = baseShape.BoundBox

    if top < bottom:
        return None
    if bottom > top:
        return None

    if bottom <= bsBB.ZMin:
        return getRegionsBelowHeight(baseShape, top)

    if top >= bsBB.ZMax:
        return getOverheadRegionsAboveHeight(baseShape, bottom)

    # Cut off all baseShape below height
    extDist1 = top - bottom
    bottomBox = Part.makeBox(bsBB.XLength + 2.0, bsBB.YLength + 2.0, extDist1)
    bottomBox.translate(FreeCAD.Vector(bsBB.XMin - 1.0, bsBB.YMin - 1.0, bottom))
    return baseShape.common(bottomBox)


def getOverheadRegions3D(baseShape, faceList):
    faceComp = Part.makeCompound(faceList)
    bsBB = baseShape.BoundBox

    # extrude faces downward and cut away from base
    extent1 = -1 * math.floor(faceComp.BoundBox.ZMax - bsBB.ZMin + 10.0)
    removalSolid = extrudeFacesToSolid(faceList, extent1)
    if not removalSolid:
        return None

    voidBase = baseShape.cut(removalSolid)

    # move voided base down
    startDep = math.floor(bsBB.ZMin - (bsBB.ZLength / 2.0) - faceComp.BoundBox.ZMax)
    voidBase.translate(FreeCAD.Vector(0.0, 0.0, startDep - voidBase.BoundBox.ZMin))

    # extrude nonvertical faces upward at least twice height, and fuse.
    extent2 = math.floor(bsBB.ZLength * 10.0)
    extrudedFaces = extrudeNonVerticalFaces(voidBase.Faces, extent2)
    fused = fuseShapes(extrudedFaces)
    if fused:
        return fused

    return None


def getCrossSectionOfSolid(baseShape, height):
    """getCrossSectionOfSolid(baseShape, height)..."""
    bsBB = baseShape.BoundBox

    if height < bsBB.ZMin:
        return None

    if height > bsBB.ZMax:
        return None

    cuttingFace = makeBoundBoxFace(bsBB, offset=2.0, zHeight=height)
    return baseShape.common(cuttingFace)


class Working2DAreas:
    """class Working2DAreas
    This class processes user inputs through both the Base Geometry and Extensions features,
    combining connected or overlapping regions when necessary, and returns a list
    of working areas represented by faces."""

    def __init__(
        self,
        baseObjectList,
        extensions=None,
        processPerimeter=False,
        processHoles=False,
        processCircles=False,
        handleMultipleFeatures="Collectively",
        boundaryShape="Face Region",
        stockShape=None,
        finalDepth=None,
    ):
        """__init__(baseObjectList, extensions=None, otherFaces=list())
        The baseObjectList is expected to be a pointer to obj.Base.
        """
        PathLog.debug("Working2DAreas.__init__()")

        self.baseObjectList = baseObjectList
        self.extensions = extensions
        self.processCircles = processCircles
        self.processHoles = processHoles
        self.processPerimeter = processPerimeter
        self.handleMultipleFeatures = handleMultipleFeatures
        self.boundaryShape = boundaryShape
        self.stockShape = stockShape
        self.finalDepth = finalDepth
        self.zMin = finalDepth
        self.overheadFaces = None
        self.workingAreas = list()
        self.extensionFaces = list()
        self.allOuters = list()
        self.workingHoles = list()
        self.avoidFeatures = list()
        self.horiz = list()
        self.vert = list()
        self.allVert = list()
        self.oblique = list()
        self.holes = list()
        self.edges = list()
        self.baseObj = None
        self.baseSubsTups = None
        self.processInternals = False
        self.subs = dict()
        self.stockProcessed = False
        self.avoidOverhead = False
        self.baseFacesDict = dict()
        self.baseOutersDict = dict()
        self.overheadRegionsDict = (
            dict()
        )  # Save overhead regions by base.Name for external class access
        self.basePerimeterDict = dict()  # Save base perimeter by base.Name
        self.disableOverheadCheck = False

        # Open edge usage
        self.rawOpenEdgeBaseTups = list()
        self.openWireTups = list()
        self.openEdges = list()

        if processHoles or processCircles:
            self.processInternals = True

        # Identify extension faces to avoid
        if self.extensions:
            for e in self.extensions:
                if e.avoid:
                    self.avoidFeatures.append(e.feature)

        # Debugging attributes
        self.isDebug = False
        self.showDebugShapes = False

    # Private methods
    def _debugMsg(self, msg):
        """_debugMsg(msg)
        If `self.isDebug` flag is True, the provided message is printed in the Report View.
        If not, then the message is assigned a debug status.
        """
        if self.isDebug:
            # PathLog.info(msg)
            FreeCAD.Console.PrintMessage(
                "SelectionProcessing.Working2DAreas: " + msg + "\n"
            )
        else:
            PathLog.debug(msg)

    def _addDebugObject(self, objShape, objName="shape"):
        """_addDebugObject(objShape, objName='shape')
        If `self.isDebug` and `self.showDebugShapes` flags are True, the provided
        debug shape will be added to the active document with the provided name.
        """
        if self.isDebug and self.showDebugShapes:
            O = FreeCAD.ActiveDocument.addObject("Part::Feature", "debug_" + objName)
            O.Shape = objShape
            O.purgeTouched()

    def _processBaseSubsList(self):
        """_processBaseSubsList()...
        This method processes the base objects list (obj.Base). It assesses selected
        faces and edges to determine if they are acceptable.
        A vertical face loop is converted to a horizontal working area.
        A set of closed edges are converted to a horizontal working area.
        Non-horizontally planar faces are regected.
        """
        self._debugMsg("_processBaseSubsList()")

        self.disableOverheadCheck = False

        # Get faces selected by user
        for base, subs in self.baseSubsTups:
            self.baseObj = base
            self.subs[base.Name] = subs

            for sub in subs:
                # Sort features in Base Geometry selection
                subShp = base.Shape.getElement(sub)
                if sub.startswith("Face"):
                    self.baseFacesDict[base.Name].append(subShp)
                    if sub not in self.avoidFeatures:
                        if not self._clasifyFace(base, sub):
                            msg = translate(
                                "PathPocket", "Pocket does not support shape %s.%s"
                            ) % (base.Label, sub)
                            PathLog.error("getWorkingAreas(): " + msg)
                    else:
                        msg = translate("PathPocket", "Avoiding %s.%s") % (
                            base.Label,
                            sub,
                        )
                        self._debugMsg("getWorkingAreas(): " + msg)
                elif sub.startswith("Edge"):
                    self.edges.append(subShp)

            if len(subs) == 0:
                self._debugMsg("{}: No subs".format(base.Name))
                # if self.boundaryShape == 'Face Region':
                #    self.boundaryShape = 'Perimeter'
                if self.boundaryShape == "Boundbox":
                    self.allOuters.append(makeBoundBoxFace(base.Shape.BoundBox))
                else:
                    bcs = self._getBaseCrossSection(base)
                    if bcs:
                        self.horiz.extend(bcs.Faces)
                self.avoidOverhead = False

            self._identifyLoopedEdges()
            self._processVertFaces()
            self._processHorizFaces()
            self._processHoles()
            self._applyBoundaryShape()

            # Reset lists
            self.horiz = list()
            self.vert = list()
            self.edges = list()
            # self.baseObj = None  # Commented out due to possible need later in successive class methods

            # Disable overhead check for horizontal face only bases
            if isRoughly(base.Shape.BoundBox.ZLength, 0.0):
                self.disableOverheadCheck = True

        # Efor

    def _clasifyFace(self, baseObject, sub):
        """_clasifyFace(baseObject, sub)...
        Given a base object, a sub-feature name,
        this function returns True if the sub-feature is a horizontally or vertically
        oriented face. The face need not be flat to be considered vertically oriented,
        such as a bspline that is vertically extruded. All other faces converted to a
        horizontal cross-section.
        """
        self._debugMsg("_clasifyFace({})".format(sub))

        face = baseObject.Shape.getElement(sub)
        if isHorizontal(face):
            self.horiz.append(face)
        elif isVertical(face):
            self.vert.append(face)
        else:
            csFace = flattenFace(face)
            if csFace:
                # Move cross-section face to ZMax of face
                csFace.translate(FreeCAD.Vector(0.0, 0.0, face.BoundBox.ZMax))
                self.horiz.append(csFace)
            else:
                return False

        return True

    def _identifyLoopedEdges(self):
        """_identifyLoopedEdges()... Attempt to identify a closed wire
        from a set of edges.  If closed, return a horizontal cross-section
        of the closed wire."""
        self._debugMsg("_identifyLoopedEdges()")

        if not self.edges:
            return

        self.openWireTups = list()
        horizFaces = list()
        sourceCompound = Part.makeCompound(self.edges)
        wires = DraftGeomUtils.findWires(self.edges)
        if wires:
            for w in wires:
                if w.isClosed():
                    flatWire = flattenWireSingleLoop(w)
                    face = Part.Face(flatWire)
                    if face.Area > 0.0:
                        self._debugMsg("Working2DAreas: Found edge loop")
                        horizFaces.append(face)
                    else:
                        self._debugMsg("Working2DAreas: No face from selected edges.")
                else:
                    self.openWireTups.append(
                        (self.baseObj, w)
                    )  # Later processing of open edges in Profile module
        else:
            msg = translate("PathGeom", "No wire from selected edges.")
            PathLog.error("Working2DAreas: " + msg)

        if horizFaces:
            self._categorizeFaces(horizFaces, sourceCompound)

        # process open wire data
        self._identifyProjectedWireLoop()

    def _identifyProjectedWireLoop(self):
        """_identifyProjectedWireLoop()... Attempt to identify a closed wire
        from a multi-height open wires.  The wires are flattened, then a check for closed wires is completed.
        If found, working faces are created from the closed wires."""
        if not self.openWireTups:
            return

        loopFaces = list()
        flatEdges = list()
        rawOpenEdgeBaseTups = list()
        openWires = [tup[1] for tup in self.openWireTups]
        sourceCompound = Part.makeCompound(openWires)

        # flatten each wire and extract edges
        for w in [flattenWireSingleLoop(w) for w in openWires]:
            flatEdges.extend(w.Edges)

        # Check for closed wires
        wires = DraftGeomUtils.findWires(flatEdges)
        if wires:
            for w in wires:
                if w.isClosed():
                    face = Part.Face(w)
                    if face.Area > 0.0:
                        self._debugMsg("Working2DAreas: Found edge loop")
                        loopFaces.append(face)
                    else:
                        self._debugMsg("Working2DAreas: No face from flattened edges.")
                else:
                    self._debugMsg(
                        "Working2DAreas: Flattened wire is not a closed wire."
                    )
                    rawOpenEdgeBaseTups.append(
                        (self.baseObj, w)
                    )  # Later processing of open edges in Profile module
        else:
            self._debugMsg("Working2DAreas: No wires from flattened edges.")

        if rawOpenEdgeBaseTups:
            self.rawOpenEdgeBaseTups.extend(rawOpenEdgeBaseTups)

        # Identify relationship of loopFaces - some may be holes within larger loop face
        if loopFaces:
            self._categorizeFaces(loopFaces, sourceCompound)

    def _categorizeFaces(self, faceList, sourceCompound):
        """_categorizeFaces(faceList, sourceCompound)...
        Reconstruct proper faces from faces provided in faceList.  Identify if any faces are holes in other faces,
        and apply those holes to those faces.  Reconstructed faces are properly assigned to class variable lists."""
        faces = list()
        holes = list()
        trueFaces = list()
        faceList.sort(key=lambda face: face.Area)  # small to big order

        def findHole(outer, faceList):
            for i in range(0, len(faceList)):
                inner = faceList[i]
                cmn = outer.common(inner)
                if cmn.Area > 0.0:
                    # inner is hole in outer
                    return i
            return -1

        while len(faceList) > 0:
            outer = faceList.pop()
            faces.append(outer)
            idx = 0
            cutFace = False
            while idx >= 0:
                idx = findHole(outer, faceList)
                if idx >= 0:
                    inner = faceList.pop(idx)
                    holes.append(inner)
                    cut = outer.cut(inner)
                    cutFace = True
                    outer = cut
            if cutFace:
                trueFaces.append(outer)

        if trueFaces:
            self.horiz.extend(trueFaces)
            for tf in trueFaces:
                tf.translate(
                    FreeCAD.Vector(
                        0.0, 0.0, sourceCompound.BoundBox.ZMax - tf.BoundBox.ZMin
                    )
                )
                self.baseFacesDict[self.baseObj.Name].append(tf)
        elif faces:
            self.horiz.extend(faces)
            for f in faces:
                f.translate(
                    FreeCAD.Vector(
                        0.0, 0.0, sourceCompound.BoundBox.ZMax - f.BoundBox.ZMin
                    )
                )
                self.baseFacesDict[self.baseObj.Name].append(f)

    def _processVertFaces(self):
        """_processVertFaces()... Attempt to identify a horizontal cross-section
        from a set of vertical faces provided in base subs list."""
        if not self.vert:
            return

        self.allVert.extend(self.vert)

        vert = Part.makeCompound(self.vert)
        horizFaces = getHorizFaceFromVertFaceLoop(self.vert)
        if horizFaces:
            # Translate horizontal faces to bottom of vertical loop
            for f in horizFaces:
                f.translate(
                    FreeCAD.Vector(0.0, 0.0, vert.BoundBox.ZMin - f.BoundBox.ZMin)
                )
            self.horiz.extend(horizFaces)
            return

        self._debugMsg(
            "Working2DAreas: Attempting to convert non-looped vertical faces to open edges."
        )
        lowZMax = min(
            [f.BoundBox.ZMax for f in self.vert]
        )  # determine lowest ZMax in face list
        # Flatten all vertical faces to cross-sectional wires at Z=0.0 height
        flatEdges = list()
        for f in self.vert:
            wire = flattenVerticalFace(f)
            if wire:
                flatEdges.extend(wire.Edges)
            else:
                self._debugMsg(
                    "flattenVerticalFace() failed to return edge(s) for a vertical face."
                )

        # Identify continuous wires from all flat edges of flattend faces above
        wires = DraftGeomUtils.findWires(flatEdges)
        if wires:
            for w in wires:
                w.translate(FreeCAD.Vector(0.0, 0.0, lowZMax - w.BoundBox.ZMin))
                self.rawOpenEdgeBaseTups.append(
                    (self.baseObj, w)
                )  # Later processing of open edges in Profile module

    def _processHorizFaces(self):
        """_processHorizFaces()... Process horizontal faces provided in base subs list."""
        self._debugMsg(
            "_processHorizFaces({})".format(
                len(self.horiz) if len(self.horiz) > 0 else 0
            )
        )

        if not self.horiz:
            return

        for f in self.horiz:
            self.baseOutersDict[self.baseObj.Name].append(Part.Face(f.Wires[0]))

        # Translate horizontal faces to final depth
        for f in self.horiz:
            f.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - f.BoundBox.ZMin))

        if self.processPerimeter:
            for f in self.horiz:
                self.allOuters.append(Part.Face(f.Wires[0]))

        if self.processInternals:
            # Identify all holes
            for f in self.horiz:
                for wire in f.Wires[1:]:
                    self.holes.append(wire)

    def _processHoles(self):
        """_processHoles()... Process holes in horizontal faces provided in base subs list."""
        if not self.holes:
            return

        for wire in self.holes:
            self._saveHole(wire)

    def _saveHole(self, wire):
        """_saveHole(wire)... Analyze hole and save accordingly."""
        self._debugMsg("_saveHole()")

        cont = False
        drillable = PathUtils.isDrillable(self.baseObj, wire)

        if self.processCircles:
            if drillable:
                cont = True
        if self.processHoles:
            if not drillable:
                cont = True

        if cont:
            try:
                face = Part.Face(wire)
            except Exception as ee:
                PathLog.error("{}".format(ee))
                return
            face.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - face.BoundBox.ZMin))
            self.workingHoles.append(face)

    def _getCrossSectionFace(self, shape):
        """_getCrossSectionFace(shape)... Return a cross-sectional
        face of the 3D shape provided."""
        self._debugMsg("_getCrossSectionFace()")

        shapeBB = shape.BoundBox
        max = shapeBB.ZMin + (shapeBB.ZMax * 2.0 + 2.0)
        tmpDepthParams = PathUtils.depth_params(
            clearance_height=max + 5.0,
            safe_height=max + 3.0,
            start_depth=max,
            step_down=5.0,
            z_finish_step=0.0,
            final_depth=shapeBB.ZMin,
            user_depths=None,
        )
        stockEnv = PathUtils.getEnvelope(partshape=shape, depthparams=tmpDepthParams)
        sliceZ = (stockEnv.BoundBox.ZMax + stockEnv.BoundBox.ZMin) / 2.0
        sectionWires = stockEnv.slice(FreeCAD.Vector(0, 0, 1), sliceZ)
        sectFace = Part.Face(sectionWires[0])
        sectFace.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - sectFace.BoundBox.ZMin))
        return sectFace

    def _applyBoundaryShape(self):
        """_applyBoundaryShape()... Apply user request for boundary shape of selected region(s)."""
        self._debugMsg("_applyBoundaryShape({})".format(self.boundaryShape))

        if self.boundaryShape == "BoundBox":
            comp = Part.makeCompound(self.allOuters)
            self.allOuters = [makeBoundBoxFace(comp.BoundBox, offset=0.0, zHeight=0.0)]
        elif self.boundaryShape == "Face Region":
            # Default setting, processing faces normally
            pass
        elif self.boundaryShape == "Perimeter":  # Referred to as Outline sometimes
            # Two methods for getting outline: TechDraw.findShapeOutline and self._get_getCrossSectionFace
            # perimeterFace = Part.Face(TechDraw.findShapeOutline(self.baseObj.Shape, 1, FreeCAD.Vector(0, 0, 1)))
            # perimeterFace.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - perimeterFace.BoundBox.ZMin))
            # self.allOuters.append(perimeterFace)

            perimeterFace2 = self._getCrossSectionFace(self.baseObj.Shape)
            self.allOuters.append(perimeterFace2)
        elif self.boundaryShape == "Stock" and not self.stockProcessed:
            csFace = self._getCrossSectionFace(self.stockShape)
            self.allOuters = [csFace]
            self.stockProcessed = True

    def _processExtensions(self):
        """_processExtensions()...
        This method processes any available extension faces from the Extensions feature.
        """
        if not self.extensions:
            return

        # Apply regular Extensions
        for ext in self.extensions:
            # verify extension is not an avoided face
            # verify extension pertains to active base object (model)
            # verify extension feature is in active subs list
            if (
                not ext.avoid
                and ext.obj.Name == self.baseObj.Name
                and ext.feature in self.subs[self.baseObj.Name]
            ):

                wire = ext.getWire()
                if wire:
                    for f in ext.getExtensionFaces(wire):
                        self.allOuters.append(f)
                        self.extensionFaces.append(f)

    def _identifyWorkAreas(self):
        """_identifyWorkAreas()...
        This method attempts to combine(fuse, merge) all the identified areas
        when possible. This method is what produces the final pocket areas
        that are requested from this class.
        """
        self._debugMsg("_identifyWorkAreas()")

        if len(self.allOuters) == 0 or not self.processPerimeter:
            self._debugMsg("no raw faces for _identifyWorkAreas()")
            return

        # Place all faces into same working plane
        for h in self.allOuters:
            h.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - h.BoundBox.ZMin))

        # Second face-combining method attempted
        chf = combineHorizontalFaces(self.allOuters)
        if chf:
            self._debugMsg("combineHorizontalFaces count: {}".format(len(chf)))
            i = 0
            for f in chf:
                i += 1
                self._addDebugObject(f, "combineHorizontalFaces_{}".format(i))

                if self.processPerimeter and self.processHoles and self.processCircles:
                    self.workingAreas.append(f)
                else:
                    self.workingAreas.append(Part.Face(f.Wires[0]))

                if self.processInternals:
                    # save any holes
                    for wire in f.Wires[1:]:
                        self._saveHole(wire)

    def _applyAvoidOverhead(self, base):
        """_applyAvoidOverhead(base)...
        Create overhead regions and apply collision therewith to working shapes.
        """
        self._debugMsg("_applyAvoidOverhead({})".format(base.Name))

        if (
            not self.avoidOverhead
            or (not self.workingAreas and not self.workingHoles)
            or self.disableOverheadCheck
        ):
            return

        faceList = self.baseFacesDict[base.Name]
        faceComp = Part.makeCompound(faceList)
        self.zMin = faceComp.BoundBox.ZMax

        if isRoughly(self.zMin, base.Shape.BoundBox.ZMax):
            # cancel overhead collision if faceList is at top of model
            self._debugMsg("Canceling overhead collision check.")
            return

        height = self.finalDepth
        # if not isRoughly(self.zMin - self.finalDepth, 0.0):
        if self.finalDepth < self.zMin:
            height = self.zMin
            # msg = translate('PathGeom', 'Please verify Final Depth.')
            # PathLog.warning(msg)
            msg2 = translate(
                "PathGeom",
                "High point of selection: {} mm.".format(round(self.zMin, 6)),
            )
            PathLog.debug(msg2)

        overhead = self.getOverheadRegions(base, height)
        if not overhead:
            self._debugMsg("_applyAvoidOverhead() No overhead regions")
            return

        self._addDebugObject(faceComp, objName="pre_overheadRegions_facesCompound")
        self._addDebugObject(
            Part.makeCompound(self.workingAreas),
            objName="pre_overheadRegions_workingAreas",
        )
        self._addDebugObject(
            overhead, objName="{}_overheadRegions_{}".format(base.Name, height)
        )

        # PathLog.info('_applyAvoidOverhead() forced return in place before applying overhead regions for collision avoidance')
        # return

        # Cut overhead shape from working shapes
        if self.workingAreas:
            self._debugMsg("pre-overhead area count: {}".format(len(self.workingAreas)))
            safeAreas = [ws.cut(overhead) for ws in self.workingAreas]
            self.workingAreas = safeAreas

        if self.processInternals and self.workingHoles:
            self._debugMsg(
                "pre-overhead holes count: {}".format(len(self.workingHoles))
            )
            safeHoles = [ws.cut(overhead) for ws in self.workingHoles]
            self.workingHoles = safeHoles

    def _getBaseCrossSection(self, base):
        if base.Name in self.basePerimeterDict.keys():
            return self.basePerimeterDict[base.Name]

        bcs = getBaseCrossSection(base.Shape, self.processInternals)
        self.basePerimeterDict[base.Name] = bcs
        return bcs

    # Public method
    def getWorkingAreas(self, avoidOverhead=False):
        """getWorkingAreas()...
        This is the main class control function for identifying pocket areas,
        including extensions to selected faces using input from the Base Geometry
        and Extensions features.
        """
        PathLog.track(self.handleMultipleFeatures)

        # Force debug in this class
        # self.isDebug = True
        # self.showDebugShapes = True

        self.avoidOverhead = avoidOverhead

        if not self.baseObjectList:
            return list()

        def processBaseSubs():
            self._processBaseSubsList()
            if self.allOuters:
                if self.processInternals and self.workingHoles:
                    pa = Part.Compound(self.allOuters)
                    ah = Part.Compound(self.workingHoles)
                    self.allOuters = [f for f in pa.cut(ah).Faces]
            self._processExtensions()
            self._identifyWorkAreas()
            self.allOuters = list()

        self._debugMsg(
            "HandleMultipleFeatures = {}".format(self.handleMultipleFeatures)
        )

        # Procede with processing based upon handleMultipleFeatures setting
        if self.handleMultipleFeatures == "Collectively":
            for tup in self.baseObjectList:
                self.baseFacesDict[tup[0].Name] = list()
                self.baseOutersDict[tup[0].Name] = list()
                self.baseSubsTups = [tup]
                processBaseSubs()
                self._applyAvoidOverhead(tup[0])
        else:
            for base, subsList in self.baseObjectList:
                self.baseFacesDict[base.Name] = list()
                self.baseOutersDict[base.Name] = list()
                for sub in subsList:
                    self.baseSubsTups = [(base, [sub])]
                    processBaseSubs()
                self._applyAvoidOverhead(base)

        if self.workingAreas:
            if self.isDebug:
                self._debugMsg(
                    "returning {} self.workingAreas".format(len(self.workingAreas))
                )
                self._addDebugObject(Part.Compound(self.workingAreas), "workingAreas")
                pass
            return self.workingAreas

        if self.processInternals and self.workingHoles:
            return self.getHoleAreas()

        if (
            self.handleMultipleFeatures == "Individually"
            and self.allVert
            and not self.workingAreas
        ):
            PathLog.info(
                translate(
                    "PathSelectionProcessing",
                    "Verify `HandleMultipleFeatures` property.",
                )
            )

        self._debugMsg("No working areas to return.")
        return None

    def getWorkingSolids(self, avoidOverhead=False):
        """getWorkingSolids()...
        This is a ghost method in place unitl a parent class is created for working areas.
        """
        return list()

    def getExtensionFaces(self):
        """getExtensionFaces()... Returns list of extension faces identified"""
        return self.extensionFaces

    def getHoleAreas(self):
        """getHoleAreas()... Returns list of holes (any internal openning in face) identified"""
        if self.processInternals and self.workingHoles:
            if self.isDebug:
                self._debugMsg("returning self.workingHoles")
                self._addDebugObject(Part.Compound(self.workingHoles), "workingHoles")
                pass
            fusedHoles = fuseShapes(self.workingHoles)
            if fusedHoles:
                self.workingHoles = fusedHoles.Faces
                self._debugMsg(
                    "self.workingHoles count: {}".format(len(self.workingHoles))
                )
                return self.workingHoles

        return None

    def getOpenEdges(
        self, toolRadius, offsetRadius, useToolComp, jobTolerance, jobLabel="Job"
    ):
        """setOpenEdgeAttributes(toolRadius, offsetRadius, jobTolerance, jobLabel='Job')...
        Call this method with arguments after calling `getWorkingAreas()` method.
        This method processes any identified open edges, returning a list
        of offset wires ready for path processing.
        """
        self._debugMsg("getOpenEdges()")

        for (base, wire) in self.rawOpenEdgeBaseTups:
            oe = OpenEdge(
                base.Shape,
                wire,
                self.finalDepth,
                toolRadius,
                offsetRadius,
                useToolComp,
                jobTolerance,
                jobLabel,
            )
            oe.isDebug = self.isDebug  # Transfer debug status
            oe.showDebugShapes = (
                self.showDebugShapes
            )  # Transfer show debug shapes status
            openEdges = oe.getOpenEdges()
            if openEdges:
                self.openEdges.extend(openEdges)

        return self.openEdges

    def getOverheadRegions(self, base, height):
        self._debugMsg("getOverheadRegions({}, height={}mm)".format(base.Name, height))
        # This version uses all above height collision avoidance

        if base.Name in self.overheadRegionsDict.keys():
            return self.overheadRegionsDict[base.Name]

        # orah = getOverheadRegionsAboveHeight(base.Shape, height, self.isDebug and self.showDebugShapes)
        orah = getOverheadRegionsAboveHeight(base.Shape, height)
        if orah:
            self._addDebugObject(orah, "overheadRegion_{}".format(base.Name))
        else:
            self._debugMsg("No overhead regions identified.")
        self.overheadRegionsDict[base.Name] = orah
        return orah

    def getOverheadRegions_3D(self, base, height):
        self._debugMsg(
            "getOverheadRegions_3D({}, height={}mm)".format(base.Name, height)
        )
        # This version uses 3D overhead collision

        if base.Name in self.overheadRegionsDict.keys():
            return self.overheadRegionsDict[base.Name]
        else:
            self.baseFacesDict[base.Name] = list()

        faceList = self.baseFacesDict[base.Name]
        if len(faceList) > 0:
            orah = getOverheadRegionsAboveHeight(base.Shape, height)
            # orah = getOverheadRegions3D(base.Shape, faceList)
            self.overheadRegionsDict[base.Name] = orah
            return orah
        else:
            PathLog.error("No faces for {}".format(base.Name))
        return None


# Eclass


class Working3DFaces:
    """class Working3DFaces
    This class processes user inputs through both the Base Geometry and Extensions features,
    combining connected or overlapping regions when necessary, and returns a list
    of working areas represented by faces."""

    def __init__(
        self,
        baseObjectList,
        extensions=None,
        processPerimeter=False,
        processHoles=False,
        processCircles=False,
        handleMultipleFeatures="Collectively",
        startDepth=None,
        finalDepth=None,
    ):
        """__init__(baseObjectList, extensions=None, otherFaces=list())
        The baseObjectList is expected to be a pointer to obj.Base.
        """
        self.baseObjectList = baseObjectList
        self.extensions = extensions
        self.processCircles = processCircles
        self.processHoles = processHoles
        self.processPerimeter = processPerimeter
        self.handleMultipleFeatures = handleMultipleFeatures
        self.startDepth = startDepth
        self.finalDepth = finalDepth
        self.rawSolids = list()
        self.rawHoles = list()
        self.workingSolids = list()
        self.extensionFaces = list()
        self.workingHoles = list()
        self.fullBaseDict = dict()
        self.subs = dict()
        self.vert = list()
        self.nonVerticalFaces = list()
        self.edges = list()
        self.avoidFeatures = list()
        self.baseObj = None
        self.baseSubsTups = None
        self.processInternals = False
        self.stockProcessed = False
        self.baseFacesDict = dict()
        self.overheadRegionsDict = (
            dict()
        )  # Save overhead regions by base.Name for external class access
        self.avoidOverhead = False

        if processHoles or processCircles:
            self.processInternals = True

        # Sort bases and related faces
        for base, subsList in baseObjectList:
            self.baseFacesDict[base.Name] = [
                base.Shape.getElement(sub) for sub in subsList if sub.startswith("Face")
            ]

        # Identify extension faces to avoid
        if self.extensions:
            for e in self.extensions:
                if e.avoid:
                    self.avoidFeatures.append(e.feature)

        # Debugging attributes
        self.isDebug = False
        self.showDebugShapes = False

    # Private methods
    def _debugMsg(self, msg):
        """_debugMsg(msg)
        If `self.isDebug` flag is True, the provided message is printed in the Report View.
        If not, then the message is assigned a debug status.
        """
        if self.isDebug:
            # PathLog.info(msg)
            FreeCAD.Console.PrintMessage(
                "SelectionProcessing.Working3DFaces: " + msg + "\n"
            )
        else:
            PathLog.debug(msg)

    def _addDebugObject(self, objShape, objName="shape"):
        """_addDebugObject(objShape, objName='shape')
        If `self.isDebug` and `self.showDebugShapes` flags are True, the provided
        debug shape will be added to the active document with the provided name.
        """
        if self.isDebug and self.showDebugShapes:
            O = FreeCAD.ActiveDocument.addObject("Part::Feature", "debug_" + objName)
            O.Shape = objShape
            O.purgeTouched()

    def _processBaseSubsList(self):
        """_processBaseSubsList()...
        This method processes the base objects list (obj.Base). It assesses selected
        faces and edges to determine if they are acceptable.
        A vertical face loop is converted to a horizontal working area.
        A set of closed edges are converted to a horizontal working area.
        Non-horizontally planar faces are regected.
        """

        # Get faces selected by user
        for base, subs in self.baseSubsTups:
            self.baseObj = base
            self.subs[base.Name] = subs
            for sub in subs:
                # Sort features in Base Geometry selection
                if sub.startswith("Face"):
                    if sub not in self.avoidFeatures:
                        if not self._clasifyFace(base, sub):
                            msg = "%s.%s is vertical. Ignoring." % (base.Label, sub)
                            self._debugMsg("Working3DFaces: " + msg)
                elif sub.startswith("Edge"):
                    self.edges.append(base.Shape.getElement(sub))
                else:
                    msgNoSupport = translate(
                        "PathPocket", "Pocket does not support shape %s.%s"
                    ) % (base.Label, sub)
                    PathLog.info(msgNoSupport)

            subCnt = len(subs)
            if subCnt == 0:
                self._debugMsg("_processBaseSubsList() {}: No subs".format(base.Name))
                self._processEntireBase()
            else:
                self._debugMsg(
                    "_processBaseSubsList() {}: {} subs".format(base.Name, subCnt)
                )
                self._processEdges()
                self._processFaces()

            # Reset lists
            self.nonVerticalFaces = list()
            self.edges = list()
        # Efor

    def _clasifyFace(self, baseObject, sub):
        """_clasifyFace(baseObject, sub)...
        Given a base object, a sub-feature name,
        this function returns True if the sub-feature is a horizontally or vertically
        oriented face. The face need not be flat to be considered vertically oriented,
        such as a bspline that is vertically extruded. All other faces are placed
        in a `neither` list.
        """
        self._debugMsg("_clasifyFace({})".format(sub))

        face = baseObject.Shape.getElement(sub)
        if isVertical(face):
            self.vert.append(face)
            return False

        self.nonVerticalFaces.append(face)
        return True

    def _processEntireBase(self):
        """_processEntireBase()... Process entire base shape into working 3D solid."""
        self._debugMsg("_processEntireBase()")

        name = self.baseObj.Name
        if name not in self.fullBaseDict.keys():
            solidBase = self._processSolid(self.baseObj.Shape)
            self.fullBaseDict[name] = solidBase

            self.rawSolids.append(solidBase)
            self.avoidOverhead = False  # Cancel overhead collision detection

    def _processSolid(self, solidShape):
        """_processSolid(solidShape)... Process solid shape into working solid base.
        This process eliminates overhangs and voids in solid below top surfaces.
        """
        self._debugMsg("_processSolid()")

        bsBB = solidShape.BoundBox
        extent = math.floor(bsBB.ZLength + 2.0)
        extrudedFaces = extrudeNonVerticalFaces(solidShape.Faces, -1 * extent)
        fused = fuseShapes(extrudedFaces)
        box = Part.makeBox(bsBB.XLength + 10.0, bsBB.YLength + 10.0, extent + 5.0)
        box.translate(
            FreeCAD.Vector(bsBB.XMin - 5.0, bsBB.YMin - 5.0, -1.0 * (extent + 5.0))
        )
        surfaceBase = fused.cut(box)

        baseEnv = getBaseCrossSection(solidShape, includeInternals=False)
        baseEnv.translate(FreeCAD.Vector(0.0, 0.0, bsBB.ZMin - baseEnv.BoundBox.ZMin))
        baseEnvExt = baseEnv.extrude(FreeCAD.Vector(0.0, 0.0, bsBB.ZLength))
        solidBase = baseEnvExt.cut(surfaceBase).removeSplitter()
        return solidBase

    def _processEdges(self):
        """_processEdges()... Process selected edges.
        Attempt to identify closed areas from selected edges.
        Project closed areas onto base shape and add common 3D solid to working solids list."""
        self._debugMsg("_processEdges()")

        if not self.edges:
            return

        # Attempt to identify closed areas from selected edges
        closedAreas = self._identifyClosedAreas()
        if not closedAreas:
            return

        # Translate closed areas to bottom of base shape and extrude closed areas upward
        boBB = self.baseObj.Shape.BoundBox
        caComp = Part.makeCompound(closedAreas)
        caComp.translate(FreeCAD.Vector(0.0, 0.0, boBB.ZMin))
        extent = math.floor((boBB.ZLength) * 10.0)
        fusion = extrudeFacesToSolid(caComp.Faces, extent)

        # Get common solid with extrusion and base shape
        targetSolid = self.baseObj.Shape.common(fusion)

        # Process solid into usable 3D envelope shape for working solid
        solidBase = self._processSolid(targetSolid)

        self.rawSolids.append(solidBase)
        self.avoidOverhead = False  # Cancel overhead collision detection

    def _identifyClosedAreas(self):
        """_identifyClosedAreas()... Attempt to identify a closed wire
        from a set of edges.  If closed, return a horizontal cross-section
        of the closed wire."""
        self._debugMsg("_identifyClosedAreas()")

        closedAreas = list()
        edges = Part.makeCompound(self.edges)
        wires = DraftGeomUtils.findWires(self.edges)
        if wires:
            for w in wires:
                if w.isClosed():
                    face = Part.Face(flattenWireSingleLoop(w))
                    if face.Area > 0.0:
                        closedAreas.append(face)
                        # self.baseFacesDict[self.baseObj.Name].append(face)
                    else:
                        msg = translate("PathGeom", "No face from selected edges.")
                        PathLog.error("Working3DAreas: " + msg)
                else:
                    msg = translate("PathGeom", "Not a closed wire.")
                    self._debugMsg("Working3DAreas: " + msg)
                    # self.rawOpenEdgeBaseTups.append((self.baseObj, w))  # Later processing of open edges in Profile module
        else:
            msg = translate("PathGeom", "No wire from selected edges.")
            PathLog.error("Working2DAreas: " + msg)

        return closedAreas

    def _processFaces(self):
        """_processFaces()... Process selected faces into working 3D solid."""
        self._debugMsg("_processFaces()")

        if not self.nonVerticalFaces:
            return

        self._processExtensions()

        self._addDebugObject(
            Part.makeCompound(self.nonVerticalFaces), "nonVerticalFaces"
        )

        # Extrude well beyond start depth, and fuse to solid
        extent = math.floor((self.baseObj.Shape.BoundBox.ZLength) * 10.0)
        fusion = extrudeFacesToSolid(self.nonVerticalFaces, extent)
        if not fusion:
            return

        self._addDebugObject(fusion, "fusion")
        # Get cross-sectional faces
        # csFaces = self._getCrossSections(fusion)  # for later feature/capability expansion

        # Trim extrusion to start height
        trimmed = self._trimExtrusion(fusion, self.startDepth)
        self.rawSolids.append(trimmed)

    def _processExtensions(self):
        """_processExtensions()...
        This method processes any available extension faces from the Extensions feature.
        """
        if not self.extensions:
            return

        # Apply regular Extensions
        for ext in self.extensions:
            # verify extension is not an avoided face
            # verify extension pertains to active base object (model)
            # verify extension feature is in active subs list
            if (
                not ext.avoid
                and ext.obj.Name == self.baseObj.Name
                and ext.feature in self.subs[self.baseObj.Name]
            ):

                self._debugMsg("Adding extension for {}".format(ext.feature))
                wire = ext.getWire()
                if wire:
                    self.avoidOverhead = False  # disable overhead check, temporarily
                    self._debugMsg("Overhead check disabled due to extensions.")
                    for f in ext.getExtensionFaces(wire):
                        self.nonVerticalFaces.append(f)

    def _getCrossSections(self, fusion):
        csFaces = list()
        boBB = self.baseObj.Shape.BoundBox
        height = math.floor((boBB.ZLength) * 5.0 + boBB.ZMin)
        trimmed = self._trimExtrusion(fusion, height)
        for f in trimmed.Faces:
            if isRoughly(f.BoundBox.ZMin, height):
                csFaces.append(f)
        return csFaces

    def _trimExtrusion(self, extrusion, height):
        extBB = extrusion.BoundBox
        box = Part.makeBox(
            extBB.XLength + 2.0, extBB.YLength + 2.0, math.floor(extBB.ZLength)
        )
        move = FreeCAD.Vector(extBB.XMin - 1.0, extBB.YMin - 1.0, height)
        box.translate(move)
        return extrusion.cut(box)

    def _applyAvoidOverhead(self, base):
        """_applyAvoidOverhead(base)...
        Create overhead regions and apply collision therewith to working shapes.
        """
        self._debugMsg("_applyAvoidOverhead({})".format(base.Name))

        if not self.avoidOverhead or (not self.rawSolids and not self.rawHoles):
            return

        # Save overhead regions by base.Name for external class access
        faceList = self.baseFacesDict[base.Name]
        overheadRegions = getOverheadRegions3D(base.Shape, faceList)
        self.overheadRegionsDict[base.Name] = overheadRegions
        if not overheadRegions:
            return

        self._addDebugObject(
            overheadRegions, objName="{}_overheadRegions".format(base.Name)
        )

        # Cut overhead shape from working shapes
        if self.rawSolids:
            self._debugMsg("pre-overhead solids count: {}".format(len(self.rawSolids)))
            safeSolids = [ws.cut(overheadRegions) for ws in self.rawSolids]
            self.rawSolids = safeSolids

        if self.processInternals and self.rawHoles:
            self._debugMsg("pre-overhead holes count: {}".format(len(self.rawHoles)))
            safeHoles = [ws.cut(overheadRegions) for ws in self.rawHoles]
            self.rawHoles = safeHoles

    def _finishProcessing(self):
        # Move bottom of solids to final depth is the idea.  This will likely require more complexity to correctly implement.
        if self.workingSolids:
            for s in self.workingSolids:
                s.translate(FreeCAD.Vector(0.0, 0.0, self.finalDepth - s.BoundBox.ZMin))

    # Public method
    def getWorkingAreas(self, avoidOverhead=False):
        """getWorkingAreas()...
        This is a ghost method in place unitl a parent class is created for working areas.
        """
        return list()

    def getWorkingSolids(self, avoidOverhead=False):
        """getWorkingSolids()...
        This is the main class control function for identifying pocket areas,
        including extensions to selected faces using input from the Base Geometry
        and Extensions features.
        """
        # PathLog.track(self.handleMultipleFeatures)
        # self.isDebug = True  #################################### Force debug mode for class
        # self.showDebugShapes = True  #################################### Force showing debug shapes for class

        self.avoidOverhead = avoidOverhead
        if not self.baseObjectList:
            return list()

        # Procede with processing based upon handleMultipleFeatures setting
        if self.handleMultipleFeatures == "Collectively":
            for tup in self.baseObjectList:
                self.avoidOverhead = avoidOverhead
                self.rawSolids = list()
                self.rawHoles = list()
                self.baseSubsTups = [tup]
                self._processBaseSubsList()
                self._applyAvoidOverhead(tup[0])
                self.workingSolids.extend(self.rawSolids)
                self.workingHoles.extend(self.rawHoles)
        else:
            for base, subsList in self.baseObjectList:
                for sub in subsList:
                    self.avoidOverhead = avoidOverhead
                    self.rawSolids = list()
                    self.rawHoles = list()
                    self.baseSubsTups = [(base, [sub])]
                    self._processBaseSubsList()
                    self._applyAvoidOverhead(base)
                    self.workingSolids.extend(self.rawSolids)
                    self.workingHoles.extend(self.rawHoles)

        # self._finishProcessing()

        if self.workingSolids:
            if self.isDebug:
                self._debugMsg("returning self.workingSolids")
                self._addDebugObject(Part.Compound(self.workingSolids), "workingSolids")
                pass
            return self.workingSolids

        if self.processInternals and self.workingHoles:
            if self.isDebug:
                self._debugMsg("returning self.workingHoles")
                self._addDebugObject(Part.Compound(self.workingHoles), "workingHoles")
                pass
            return self.workingHoles

        PathLog.error("No 3D pocket envelopes to return!")
        return list()

    def getExtensionFaces(self):
        """getExtensionFaces()... Returns list of extension faces identified"""
        return self.extensionFaces

    def getHoleSolids(self):
        """getHoleSolids()... Returns list of holes (any internal openning in face) identified"""
        return self.workingHoles


# Eclass


class OpenEdge:
    def __init__(
        self,
        baseShape,
        wire,
        finalDepth,
        toolRadius,
        offsetRadius,
        useToolComp,
        jobTolerance,
        jobLabel="Job",
    ):
        self.baseShape = baseShape
        self.wire = wire
        self.finalDepth = finalDepth
        self.toolRadius = abs(toolRadius)
        self.offsetRadius = offsetRadius
        self.useToolComp = useToolComp
        self.jobTolerance = jobTolerance
        self.jobLabel = jobLabel
        self.geomFinalDepth = finalDepth
        self.wireBoundBoxFaceAtZero = None
        self.useOtherSide = False
        self.pathStops = None
        self.otherPathStops = None
        self.sortedFlatWire = None
        self.errorshape = None
        self.inaccessibleMsg = translate(
            "PathProfile",
            "The selected edge(s) are inaccessible. If multiple, re-ordering selection might work.",
        )

        if finalDepth < baseShape.BoundBox.ZMin:
            self.geomFinalDepth = baseShape.BoundBox.ZMin

        if finalDepth > wire.BoundBox.ZMin:
            self.geomFinalDepth = wire.BoundBox.ZMin

        if toolRadius < 0.0:
            self.useOtherSide = True

        # Debugging attributes
        self.isDebug = False
        self.showDebugShapes = False

    # Method to add temporary debug object
    def _debugMsg(self, msg):
        """_debugMsg(msg)
        If `self.isDebug` flag is True, the provided message is printed in the Report View.
        If not, then the message is assigned a debug status.
        """
        if self.isDebug:
            # PathLog.info(msg)
            FreeCAD.Console.PrintMessage("SelectionProcessing.OpenEdge: " + msg + "\n")
        else:
            PathLog.debug(msg)

    def _addDebugObject(self, objName, objShape):
        if self.isDebug and self.showDebugShapes:
            O = FreeCAD.ActiveDocument.addObject("Part::Feature", "tmp_" + objName)
            O.Shape = objShape
            O.purgeTouched()

    # Private methods
    def _getCutAreaCrossSection(self, origWire, flatWire):
        self._debugMsg("_getCutAreaCrossSection()")
        # FCAD = FreeCAD.ActiveDocument
        tolerance = self.jobTolerance
        toolDiam = 2 * self.toolRadius  # self.toolRadius defined in PathOp
        minBfr = toolDiam * 1.25
        bbBfr = (self.offsetRadius * 2) * 1.25
        if bbBfr < minBfr:
            bbBfr = minBfr
        # fwBB = flatWire.BoundBox
        wBB = origWire.BoundBox
        minArea = (self.offsetRadius - tolerance) ** 2 * math.pi
        self.wireBoundBoxFaceAtZero = self._makeExtendedBoundBox(wBB, bbBfr, 0.0)

        useWire = origWire.Wires[0]
        numOrigEdges = len(useWire.Edges)
        sdv = wBB.ZMax
        fdv = self.geomFinalDepth
        extLenFwd = sdv - fdv
        if extLenFwd <= 0.0:
            msg = translate(
                "PathProfile", "For open edges, verify Final Depth for this operation."
            )
            FreeCAD.Console.PrintError(msg + "\n")
            # return False
            extLenFwd = 0.1

        # Identify first/last edges and first/last vertex on wire
        numEdges = len(self.sortedFlatWire.Edges)
        begE = self.sortedFlatWire.Edges[0]  # beginning edge
        endE = self.sortedFlatWire.Edges[numEdges - 1]  # ending edge
        blen = begE.Length
        elen = endE.Length
        Vb = begE.Vertexes[0]  # first vertex of wire
        Ve = endE.Vertexes[1]  # last vertex of wire
        pb = FreeCAD.Vector(Vb.X, Vb.Y, fdv)
        pe = FreeCAD.Vector(Ve.X, Ve.Y, fdv)

        # Obtain beginning point perpendicular points
        if blen > 0.1:
            bcp = begE.valueAt(
                begE.getParameterByLength(0.1)
            )  # point returned 0.1 mm along edge
        else:
            bcp = FreeCAD.Vector(begE.Vertexes[1].X, begE.Vertexes[1].Y, fdv)
        if elen > 0.1:
            ecp = endE.valueAt(
                endE.getParameterByLength(elen - 0.1)
            )  # point returned 0.1 mm along edge
        else:
            ecp = FreeCAD.Vector(endE.Vertexes[1].X, endE.Vertexes[1].Y, fdv)

        # Create intersection tags for determining which side of wire to cut
        (begInt, begExt, iTAG, eTAG) = self._makeIntersectionTags(
            useWire, numOrigEdges, fdv
        )
        if not begInt or not begExt:
            return False
        self.iTAG = iTAG
        self.eTAG = eTAG

        # COLLISTION
        # collision = getOverheadRegionsAboveHeight(self.baseShape, fdv)  # was `fdv`

        # Identify working layer of baseShape for wire(s)
        if isRoughly(wBB.ZLength, 0.0):
            wireLayer = getCrossSectionOfSolid(self.baseShape, wBB.ZMin)
        else:
            thickCS = getThickCrossSection(
                self.baseShape, wBB.ZMin, wBB.ZMax, isDebug=False
            )
            projCS = getBaseCrossSection(thickCS)
            wireLayer = fuseShapes(projCS.Faces)
        wireLayer.translate(FreeCAD.Vector(0, 0, 0.0 - wireLayer.BoundBox.ZMin))
        self._addDebugObject("wireLayer", wireLayer)

        # Isolate working layer to expanded boundbox around wire
        rawComFC = self.wireBoundBoxFaceAtZero.cut(wireLayer)
        rawComFC.translate(FreeCAD.Vector(0, 0, 0.0 - rawComFC.BoundBox.ZMin))
        self._addDebugObject("rawComFC", rawComFC)

        # Invert the working face for Other Side as needed
        if (
            self.useOtherSide
        ):  # alternate Inside/Outside application in `_extractPathWire()` in progress...
            comFC = self.wireBoundBoxFaceAtZero.cut(rawComFC)
        else:
            comFC = rawComFC
        self._addDebugObject("comFC", comFC)

        # Determine with which set of intersection tags the model intersects
        tagCOM = self._checkTagIntersection(iTAG, eTAG, "QRY", comFC, begInt, begExt)

        # Make two beginning style(oriented) 'L' shape stops
        begStop = self._makeStop("BEG", bcp, pb, "BegStop")
        altBegStop = self._makeStop("END", bcp, pb, "BegStop")

        # Identify to which style 'L' stop the beginning intersection tag is closest,
        # and create partner end 'L' stop geometry, and save for application later
        lenBS_extETag = begStop.CenterOfMass.sub(tagCOM).Length
        lenABS_extETag = altBegStop.CenterOfMass.sub(tagCOM).Length
        if lenBS_extETag < lenABS_extETag:
            endStop = self._makeStop("END", ecp, pe, "EndStop")
            self.pathStops = Part.makeCompound([begStop, endStop])
            # save other Path Stops
            altEndStop = self._makeStop("BEG", ecp, pe, "EndStop")
            self.otherPathStops = Part.makeCompound([altBegStop, altEndStop])
        else:
            altEndStop = self._makeStop("BEG", ecp, pe, "EndStop")
            self.pathStops = Part.makeCompound([altBegStop, altEndStop])
            # save other Path Stops
            endStop = self._makeStop("END", ecp, pe, "EndStop")
            self.otherPathStops = Part.makeCompound([begStop, endStop])
        self.pathStops.translate(
            FreeCAD.Vector(0, 0, 0.0 - self.pathStops.BoundBox.ZMin)
        )
        self.otherPathStops.translate(
            FreeCAD.Vector(0, 0, 0.0 - self.otherPathStops.BoundBox.ZMin)
        )

        # Identify closed wire in cross-section that corresponds to user-selected edge(s)
        workShp = comFC
        fcShp = workShp
        wire = origWire
        WS = workShp.Wires
        lenWS = len(WS)
        if lenWS < 3:
            wi = 0
        else:
            wi = None
            for wvt in wire.Vertexes:
                for w in range(0, lenWS):
                    twr = WS[w]
                    for v in range(0, len(twr.Vertexes)):
                        V = twr.Vertexes[v]
                        if abs(V.X - wvt.X) < tolerance:
                            if abs(V.Y - wvt.Y) < tolerance:
                                # Same vertex found.  This wire to be used for offset
                                wi = w
                                break
            # Efor

            if wi is None:
                PathLog.error(
                    "The cut area cross-section wire does not coincide with selected edge. Wires[] index is None."
                )
                return False
            else:
                self._debugMsg("Cross-section Wires[] index is {}.".format(wi))

            nWire = Part.Wire(Part.__sortEdges__(workShp.Wires[wi].Edges))
            fcShp = Part.Face(nWire)
            fcShp.translate(FreeCAD.Vector(0, 0, fdv - workShp.BoundBox.ZMin))
        # Eif

        # verify that wire chosen is not inside the physical model
        # Recent changes above with identifying working layer might render this section obsolete - testing necessary
        if wi > 0:  # and isInterior is False:
            self._debugMsg(
                "Multiple wires in cut area. First choice is not 0. Testing."
            )
            testArea = fcShp.cut(self.baseShape)

            isReady = self._checkTagIntersection(iTAG, eTAG, self.cutSide, testArea)
            self._debugMsg("isReady {}.".format(isReady))

            if isReady is False:
                self._debugMsg("Using wire index {}.".format(wi - 1))
                pWire = Part.Wire(Part.__sortEdges__(workShp.Wires[wi - 1].Edges))
                pfcShp = Part.Face(pWire)
                pfcShp.translate(FreeCAD.Vector(0, 0, fdv - workShp.BoundBox.ZMin))
                workShp = pfcShp.cut(fcShp)

            if testArea.Area < minArea:
                self._debugMsg(
                    "offset area is less than minArea of {}.".format(minArea)
                )
                self._debugMsg("Using wire index {}.".format(wi - 1))
                pWire = Part.Wire(Part.__sortEdges__(workShp.Wires[wi - 1].Edges))
                pfcShp = Part.Face(pWire)
                pfcShp.translate(FreeCAD.Vector(0, 0, fdv - workShp.BoundBox.ZMin))
                workShp = pfcShp.cut(fcShp)
        # Eif

        # Add path stops at ends of wire
        cutShp = workShp.cut(self.pathStops)
        self._addDebugObject("CutShape", cutShp)

        return cutShp

    def _checkTagIntersection(
        self, iTAG, eTAG, cutSide, tstObj, begInt=None, begExt=None
    ):
        self._debugMsg("_checkTagIntersection()")
        # Identify intersection of Common area and Interior Tags
        intCmn = tstObj.common(iTAG)

        # Identify intersection of Common area and Exterior Tags
        extCmn = tstObj.common(eTAG)

        # Calculate common intersection (solid model side, or the non-cut side) area with tags, to determine physical cut side
        cmnIntArea = intCmn.Area
        cmnExtArea = extCmn.Area
        if cutSide == "QRY":
            # return (cmnIntArea, cmnExtArea)
            if cmnExtArea > cmnIntArea:
                self._debugMsg("Cutting on Ext side.")
                self.cutSide = "E"
                self.cutSideTags = eTAG
                tagCOM = begExt.CenterOfMass
            else:
                self._debugMsg("Cutting on Int side.")
                self.cutSide = "I"
                self.cutSideTags = iTAG
                tagCOM = begInt.CenterOfMass
            return tagCOM

        if cmnExtArea > cmnIntArea:
            self._debugMsg("Cutting on Ext side.")
            if cutSide == "E":
                return True
        else:
            self._debugMsg("Cutting on Int side.")
            if cutSide == "I":
                return True
        return False

    def _extractPathWire(self, flatWire, cutShp):
        self._debugMsg("_extractPathWire()")

        subLoops = list()
        rtnWIRES = list()
        osWrIdxs = list()
        subDistFactor = (
            1.0  # Raise to include sub wires at greater distance from original
        )
        wire = flatWire
        lstVrtIdx = len(wire.Vertexes) - 1
        lstVrt = wire.Vertexes[lstVrtIdx]
        frstVrt = wire.Vertexes[0]
        cent0 = FreeCAD.Vector(frstVrt.X, frstVrt.Y, 0.0)
        cent1 = FreeCAD.Vector(lstVrt.X, lstVrt.Y, 0.0)

        # Calculate offset shape, containing cut region
        ofstShp = self._getOffsetArea(cutShp, False)
        if not ofstShp:
            self._addDebugObject("error_cutShape", cutShp)
            return list()

        self._addDebugObject("OffsetShape", ofstShp)
        """
        # Alternative method to better switch sides for Inside/Outside profile
        if self.useOtherSide and False:
            # save original offset value
            origOffsetValue = self.offsetRadius
            self.offsetRadius *= 2.0
            # offset current offset twice original value in opposite direction
            newOfstShp = self._getOffsetArea(ofstShp, True)
            if not newOfstShp:
                PathLog.error("failed reverse offset for other side")
                return list()
            # Restore original offsetRadius value
            self.offsetRadius = origOffsetValue
            self._addDebugObject('newOfstShp', newOfstShp)
            # make common of wire BB face and new offset
            newOfstTrimmed = self.wireBoundBoxFaceAtZero.common(newOfstShp)
            # cut out other Path Stops
            newEndsIn = newOfstTrimmed.cut(self.pathStops)
            # re-assign as new offset shape
            self._addDebugObject('newEndsIn', newEndsIn)
        """

        numOSWires = len(ofstShp.Wires)
        for w in range(0, numOSWires):
            osWrIdxs.append(w)

        # Identify two vertexes for dividing offset loop
        NEAR0 = self._findNearestVertex(ofstShp, cent0)
        min0i = 0
        min0 = NEAR0[0][4]
        for n in range(0, len(NEAR0)):
            N = NEAR0[n]
            if N[4] < min0:
                min0 = N[4]
                min0i = n
        (w0, vi0, pnt0, _, _) = NEAR0[0]  # min0i
        # self._addDebugObject('Near0', Part.makeLine(cent0, pnt0))

        NEAR1 = self._findNearestVertex(ofstShp, cent1)
        min1i = 0
        min1 = NEAR1[0][4]
        for n in range(0, len(NEAR1)):
            N = NEAR1[n]
            if N[4] < min1:
                min1 = N[4]
                min1i = n
        (w1, vi1, pnt1, _, _) = NEAR1[0]  # min1i
        # self._addDebugObject('Near1', Part.makeLine(cent1, pnt1))

        if w0 != w1:
            PathLog.warning(
                "Offset wire endpoint indexes are not equal - w0, w1: {}, {}".format(
                    w0, w1
                )
            )

        if self.isDebug and False:  # remove False to add these comments when debugging
            self._debugMsg("min0i is {}.".format(min0i))
            self._debugMsg("min1i is {}.".format(min1i))
            self._debugMsg("NEAR0[{}] is {}.".format(w0, NEAR0[w0]))
            self._debugMsg("NEAR1[{}] is {}.".format(w1, NEAR1[w1]))
            self._debugMsg("NEAR0 is {}.".format(NEAR0))
            self._debugMsg("NEAR1 is {}.".format(NEAR1))

        mainWire = ofstShp.Wires[w0]

        # Check for additional closed loops in offset wire by checking distance to iTAG or eTAG elements
        if numOSWires > 1:
            self._debugMsg("Number of offset wires > 1")
            # check all wires for proximity(children) to intersection tags
            tagsComList = list()
            for T in self.cutSideTags.Faces:
                tcom = T.CenterOfMass
                tv = FreeCAD.Vector(tcom.x, tcom.y, 0.0)
                tagsComList.append(tv)
            subDist = self.offsetRadius * subDistFactor
            for w in osWrIdxs:
                if w != w0:
                    cutSub = False
                    VTXS = ofstShp.Wires[w].Vertexes
                    for V in VTXS:
                        v = FreeCAD.Vector(V.X, V.Y, 0.0)
                        for t in tagsComList:
                            if t.sub(v).Length < subDist:
                                cutSub = True
                                break
                        if cutSub is True:
                            break
                    if cutSub is True:
                        sub = Part.Wire(Part.__sortEdges__(ofstShp.Wires[w].Edges))
                        subLoops.append(sub)
                # Eif

        # Break offset loop into two wires - one of which is the desired profile path wire.
        (part0, part1) = splitClosedWireAtTwoVertexes(
            mainWire, mainWire.Vertexes[vi0], mainWire.Vertexes[vi1], self.jobTolerance
        )

        # Determine which part is nearest original edge(s) by using distance between wire midpoints
        # Calculate midpoints of wires
        mpA = self._findWireMidpoint(self.sortedFlatWire.Wires[0])
        mp0 = self._findWireMidpoint(part0.Wires[0])
        mp1 = self._findWireMidpoint(part1.Wires[0])
        if mpA.sub(mp0).Length < mpA.sub(mp1).Length:
            rtnWIRES.append(part0)
        else:
            rtnWIRES.append(part1)
        rtnWIRES.extend(subLoops)

        for w in rtnWIRES:
            w.translate(FreeCAD.Vector(0.0, 0.0, 0.0 - w.BoundBox.ZMin))

        return rtnWIRES

    def _getOffsetArea(self, fcShape, isHole):
        """Get an offset area for a shape. Wrapper around
        PathUtils.getOffsetArea."""
        self._debugMsg("_getOffsetArea()")

        offset = self.offsetRadius

        if isHole is False:
            offset = 0 - offset

        ofstShp = PathUtils.getOffsetArea(
            fcShape, offset, plane=Part.makeCircle(5.0), tolerance=self.jobTolerance
        )

        # CHECK for ZERO area of offset shape
        try:
            if not hasattr(ofstShp, "Area") or not ofstShp.Area:
                PathLog.error("No area to offset shape returned. 1")
                self.errorshape = fcShape
                return None
        except Exception as ee:
            PathLog.error("No area to offset shape returned. 2 {}".format(ee))
            self.errorshape = fcShape
            return None

        return ofstShp

    def _findNearestVertex(self, shape, point):
        self._debugMsg("_findNearestVertex()")
        PT = FreeCAD.Vector(point.x, point.y, 0.0)

        def sortDist(tup):
            return tup[4]

        PNTS = list()
        for w in range(0, len(shape.Wires)):
            WR = shape.Wires[w]
            V = WR.Vertexes[0]
            P = FreeCAD.Vector(V.X, V.Y, 0.0)
            dist = P.sub(PT).Length
            vi = 0
            pnt = P
            vrt = V
            for v in range(0, len(WR.Vertexes)):
                V = WR.Vertexes[v]
                P = FreeCAD.Vector(V.X, V.Y, 0.0)
                d = P.sub(PT).Length
                if d < dist:
                    dist = d
                    vi = v
                    pnt = P
                    vrt = V
            PNTS.append((w, vi, pnt, vrt, dist))
        PNTS.sort(key=sortDist)
        return PNTS

    def _makeCrossSection(self, shape, sliceZ, zHghtTrgt=False):
        """_makeCrossSection(shape, sliceZ, zHghtTrgt=None)...
        Creates cross-section objectc from shape.  Translates cross-section to zHghtTrgt if available.
        Makes face shape from cross-section object. Returns face shape at zHghtTrgt."""
        self._debugMsg("_makeCrossSection()")
        # Create cross-section of shape and translate
        wires = list()
        slcs = shape.slice(FreeCAD.Vector(0, 0, 1), sliceZ)
        if len(slcs) > 0:
            for i in slcs:
                wires.append(i)
            comp = Part.Compound(wires)
            if zHghtTrgt is not False:
                comp.translate(FreeCAD.Vector(0, 0, zHghtTrgt - comp.BoundBox.ZMin))
            return comp

        return False

    def _makeExtendedBoundBox(self, wBB, bbBfr, zDep):
        self._debugMsg("_makeExtendedBoundBox()")
        p1 = FreeCAD.Vector(wBB.XMin - bbBfr, wBB.YMin - bbBfr, zDep)
        p2 = FreeCAD.Vector(wBB.XMax + bbBfr, wBB.YMin - bbBfr, zDep)
        p3 = FreeCAD.Vector(wBB.XMax + bbBfr, wBB.YMax + bbBfr, zDep)
        p4 = FreeCAD.Vector(wBB.XMin - bbBfr, wBB.YMax + bbBfr, zDep)

        L1 = Part.makeLine(p1, p2)
        L2 = Part.makeLine(p2, p3)
        L3 = Part.makeLine(p3, p4)
        L4 = Part.makeLine(p4, p1)

        return Part.Face(Part.Wire([L1, L2, L3, L4]))

    def _makeIntersectionTags(self, useWire, numOrigEdges, fdv):
        self._debugMsg("_makeIntersectionTags()")
        # Create circular probe tags around perimiter of wire
        extTags = list()
        intTags = list()
        tagRad = self.toolRadius / 2
        tagCnt = 0
        begInt = False
        begExt = False
        for e in range(0, numOrigEdges):
            E = useWire.Edges[e]
            LE = E.Length
            if LE > (self.toolRadius * 2):
                nt = math.ceil(
                    LE / (tagRad * math.pi)
                )  # (tagRad * 2 * math.pi) is circumference
            else:
                nt = 4  # desired + 1
            mid = LE / nt
            spc = self.toolRadius / 10
            for i in range(0, int(nt)):
                if i == 0:
                    if e == 0:
                        if LE > 0.2:
                            aspc = 0.1
                        else:
                            aspc = LE * 0.75
                        cp1 = E.valueAt(E.getParameterByLength(0))
                        cp2 = E.valueAt(E.getParameterByLength(aspc))
                        (intTObj, extTObj) = self._makeOffsetCircleTag(
                            cp1, cp2, tagRad, fdv, "BeginEdge[{}]_".format(e)
                        )
                        if intTObj and extTObj:
                            begInt = intTObj
                            begExt = extTObj
                else:
                    d = i * mid
                    negTestLen = d - spc
                    if negTestLen < 0:
                        negTestLen = d - (LE * 0.25)
                    posTestLen = d + spc
                    if posTestLen > LE:
                        posTestLen = d + (LE * 0.25)
                    cp1 = E.valueAt(E.getParameterByLength(negTestLen))
                    cp2 = E.valueAt(E.getParameterByLength(posTestLen))
                    (intTObj, extTObj) = self._makeOffsetCircleTag(
                        cp1, cp2, tagRad, fdv, "Edge[{}]_".format(e)
                    )
                    if intTObj and extTObj:
                        tagCnt += nt
                        intTags.append(intTObj)
                        extTags.append(extTObj)
        # tagArea = math.pi * tagRad**2 * tagCnt
        iTAG = Part.makeCompound(intTags)
        eTAG = Part.makeCompound(extTags)

        return (begInt, begExt, iTAG, eTAG)

    def _makeOffsetCircleTag(self, p1, p2, cutterRad, depth, lbl, reverse=False):
        # self._debugMsg('_makeOffsetCircleTag()')
        pb = FreeCAD.Vector(p1.x, p1.y, 0.0)
        pe = FreeCAD.Vector(p2.x, p2.y, 0.0)

        toMid = pe.sub(pb).multiply(0.5)
        lenToMid = toMid.Length
        if lenToMid == 0.0:
            # Probably a vertical line segment
            return (False, False)

        cutFactor = (
            cutterRad / 2.1
        ) / lenToMid  # = 2 is tangent to wire; > 2 allows tag to overlap wire; < 2 pulls tag away from wire
        perpE = FreeCAD.Vector(-1 * toMid.y, toMid.x, 0.0).multiply(
            -1 * cutFactor
        )  # exterior tag
        extPnt = pb.add(toMid.add(perpE))

        # make exterior tag
        eCntr = extPnt.add(FreeCAD.Vector(0, 0, depth))
        ecw = Part.Wire(Part.makeCircle((cutterRad / 2), eCntr).Edges[0])
        extTag = Part.Face(ecw)

        # make interior tag
        perpI = FreeCAD.Vector(-1 * toMid.y, toMid.x, 0.0).multiply(
            cutFactor
        )  # interior tag
        intPnt = pb.add(toMid.add(perpI))
        iCntr = intPnt.add(FreeCAD.Vector(0, 0, depth))
        icw = Part.Wire(Part.makeCircle((cutterRad / 2), iCntr).Edges[0])
        intTag = Part.Face(icw)

        return (intTag, extTag)

    def _makeStop(self, sType, pA, pB, lbl):
        # self._debugMsg('_makeStop()')
        ofstRad = self.offsetRadius
        extra = self.toolRadius / 5.0
        lng = 0.05
        med = lng / 2.0
        shrt = lng / 5.0

        E = FreeCAD.Vector(pB.x, pB.y, 0)  # endpoint
        C = FreeCAD.Vector(pA.x, pA.y, 0)  # checkpoint

        if self.useToolComp is True or (
            self.useToolComp is False and self.offsetExtra != 0
        ):
            # 'L' stop shape and edge map
            # --1--
            # |   |
            # 2   6
            # |   |
            # |   ----5----|
            # |            4
            # -----3-------|
            # positive dist in _makePerp2DVector() is CCW rotation
            p1 = E
            if sType == "BEG":
                p2 = self._makePerp2DVector(C, E, -1 * shrt)  # E1
                p3 = self._makePerp2DVector(p1, p2, ofstRad + lng + extra)  # E2
                p4 = self._makePerp2DVector(p2, p3, shrt + ofstRad + extra)  # E3
                p5 = self._makePerp2DVector(p3, p4, lng + extra)  # E4
                p6 = self._makePerp2DVector(p4, p5, ofstRad + extra)  # E5
            elif sType == "END":
                p2 = self._makePerp2DVector(C, E, shrt)  # E1
                p3 = self._makePerp2DVector(p1, p2, -1 * (ofstRad + lng + extra))  # E2
                p4 = self._makePerp2DVector(p2, p3, -1 * (shrt + ofstRad + extra))  # E3
                p5 = self._makePerp2DVector(p3, p4, -1 * (lng + extra))  # E4
                p6 = self._makePerp2DVector(p4, p5, -1 * (ofstRad + extra))  # E5
            p7 = E  # E6
            L1 = Part.makeLine(p1, p2)
            L2 = Part.makeLine(p2, p3)
            L3 = Part.makeLine(p3, p4)
            L4 = Part.makeLine(p4, p5)
            L5 = Part.makeLine(p5, p6)
            L6 = Part.makeLine(p6, p7)
            wire = Part.Wire([L1, L2, L3, L4, L5, L6])
        else:
            # 'L' stop shape and edge map
            # :
            # |----2-------|
            # 3            1
            # |-----4------|
            # positive dist in _makePerp2DVector() is CCW rotation
            p1 = E
            if sType == "BEG":
                p2 = self._makePerp2DVector(
                    C, E, -1 * (shrt + abs(self.offsetExtra))
                )  # left, shrt
                p3 = self._makePerp2DVector(p1, p2, shrt + abs(self.offsetExtra))
                p4 = self._makePerp2DVector(
                    p2, p3, (med + abs(self.offsetExtra))
                )  #      FIRST POINT
                p5 = self._makePerp2DVector(
                    p3, p4, shrt + abs(self.offsetExtra)
                )  # E1                SECOND
            elif sType == "END":
                p2 = self._makePerp2DVector(
                    C, E, (shrt + abs(self.offsetExtra))
                )  # left, shrt
                p3 = self._makePerp2DVector(p1, p2, -1 * (shrt + abs(self.offsetExtra)))
                p4 = self._makePerp2DVector(
                    p2, p3, -1 * (med + abs(self.offsetExtra))
                )  #      FIRST POINT
                p5 = self._makePerp2DVector(
                    p3, p4, -1 * (shrt + abs(self.offsetExtra))
                )  # E1                SECOND
            p6 = p1  # E4
            L1 = Part.makeLine(p1, p2)
            L2 = Part.makeLine(p2, p3)
            L3 = Part.makeLine(p3, p4)
            L4 = Part.makeLine(p4, p5)
            L5 = Part.makeLine(p5, p6)
            wire = Part.Wire([L1, L2, L3, L4, L5])
        # Eif
        face = Part.Face(wire)
        self._addDebugObject(lbl, face)

        return face

    def _makePerp2DVector(self, v1, v2, dist):
        p1 = FreeCAD.Vector(v1.x, v1.y, 0.0)
        p2 = FreeCAD.Vector(v2.x, v2.y, 0.0)
        toEnd = p2.sub(p1)
        factor = dist / toEnd.Length
        perp = FreeCAD.Vector(-1 * toEnd.y, toEnd.x, 0.0).multiply(factor)
        return p1.add(toEnd.add(perp))

    def _findWireMidpoint(self, wire):
        midPnt = None
        dist = 0.0
        wL = wire.Length
        midW = wL / 2

        for e in range(0, len(wire.Edges)):
            E = wire.Edges[e]
            elen = E.Length
            d_ = dist + elen
            if dist < midW and midW <= d_:
                dtm = midW - dist
                midPnt = E.valueAt(E.getParameterByLength(dtm))
                break
            else:
                dist += elen
        return midPnt

    def _altOffsetMethod(self):
        import PathScripts.PathOpTools as PathOpTools

        try:
            altOffsetWire = PathOpTools.offsetWire(
                wire=self.wire,
                base=self.baseShape,
                offset=self.offsetRadius,
                forward=True,
            )
            self._addDebugObject("altOffsetWire", altOffsetWire)
        except:
            self._debugMsg("Failed to offset wire using PathOpTools.offsetWire().")
            return

    # Public method
    def getOpenEdges(self):
        openEdges = list()
        wire = self.wire

        # Uncommnet to debug OpenEdge class only
        # self.isDebug = True  ##############################################################################

        if self.jobTolerance == 0.0:
            msg = self.jobLabel + "GeometryTolerance = 0.0. "
            msg += translate(
                "PathSelectionProcessing",
                "Please set to an acceptable value greater than zero.",
            )
            PathLog.error(msg)
            return openEdges

        zDiff = math.fabs(wire.BoundBox.ZMin - self.geomFinalDepth)
        if zDiff < self.jobTolerance:
            msg = translate(
                "PathProfile",
                "Check edge selection and Final Depth requirements for profiling open edge(s).",
            )
            PathLog.debug(msg)

        flatWire = flattenWireSingleLoop(wire, self.geomFinalDepth)
        if not flatWire:
            PathLog.error(self.inaccessibleMsg + " 2")
            return openEdges
        self._addDebugObject("FlatWire", flatWire)

        # Start process for extracting openEdge offset wire
        self.sortedFlatWire = Part.Wire(
            Part.__sortEdges__(flatWire.Wires[0].Edges)
        )  # complex selections need edges sorted
        cutShp = self._getCutAreaCrossSection(wire, flatWire)

        if cutShp:
            cutWireObjs = self._extractPathWire(flatWire, cutShp)

            if cutWireObjs:
                for cW in cutWireObjs:
                    openEdges.append(cW)
            else:
                PathLog.error(self.inaccessibleMsg + " 3")

        # self._altOffsetMethod()  # fails on bspline wires and other situations

        return openEdges


# Eclass


class ItemManager:
    def __init__(self):
        self.subGroups = dict()
        self.groupCounter = 0
        self.vertexDict = dict()
        self.edgeDict = dict()
        self.wireDict = dict()
        self.faceDict = dict()
        self.modelDict = dict()

    def addItem(self, base, sub, siblingList=None):
        i = self.groupCounter + 1
        item = Item(base, sub, siblingList)
        key = "Group{}".format(i)
        self.subGroups[key] = item
        if sub:
            catDict = getattr(self, item.itemType.lower() + "Dict")
            catDict[sub] = item
        else:
            self.modelDict[base.Name] = item


class Item:
    def __init__(self, base, sub, siblingList=None):
        self.base
        self.sub
        self.itemType = "Base"
        self.siblingList = (
            siblingList if siblingList and isinstance(siblingList, list) else None
        )
        self.subGoupKey = ""
        self.shape = None
        self.vertexes = list()
        self.flatVertexes = list()
        self.isCircle = False

        if sub is not None and isinstance(sub, str):
            self._setItemType()
            self.shape = base.Shape.getElement(sub)
            self.vertexes = [v.Point for v in self.shape.Vertexes]
            self.flatVertexes = [
                FreeCAD.Vector(v.X, v.Y, 0.0) for v in self.shape.Vertexes
            ]
            if len(self.shape.Vertexes) == 1 and self.isEdge:
                self.isCircle = True

    def isFace(self):
        if self.sub.startswith("Face"):
            return "Face"
        return None

    def isWire(self):
        if self.sub.startswith("Wire"):
            return "Wire"
        return None

    def isEdge(self):
        if self.sub.startswith("Edge"):
            return "Edge"
        return None

    def isVertex(self):
        if self.sub.startswith("Vertex"):
            return "Vertex"
        return None

    def _setItemType(self):
        types = ["Vertex", "Edge", "Wire", "Face"]
        for t in types:
            isType = getattr(self, "is" + t)
            if isType():
                self.itemType = t
                return

    def isOnWire(self, wire):
        """WORK  IN  PROGRESS - INCOMPLETE"""
        if not self.isEdge():
            return False

        if self.isCircle:
            e = self.shape
            if len(wire.Edges) == 1:
                edg = wire.Edges[0]
                p0 = edg.Vertexes[0].Point
                if isRoughly(p0.sub(self.vertexes[0]).Length, 0.0):
                    if isRoughly(edg.Center.sub(e.Center).Length, 0.0):
                        return True
            for e in wire.Edges:
                if len(e.Vertexes) == 1:
                    pass
