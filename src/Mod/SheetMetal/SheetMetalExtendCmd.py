########################################################################
#
#  SheetMetalExtendCmd.py
#
#  Copyright 2020 Jaise James <jaisekjames at gmail dot com>
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 2 of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#  MA 02110-1301, USA.
#
#
########################################################################

import math
import os

import FreeCAD
import Part

import SheetMetalTools
SMException = SheetMetalTools.SMException

Base = FreeCAD.Base

# IMPORTANT: please remember to change the element map version in case
# of any changes in modeling logic.
smElementMapVersion = "sm1."
smEpsilon = SheetMetalTools.smEpsilon
translate = FreeCAD.Qt.translate

# List of properties to be saved as defaults.
smExtrudeDefaultVars = ["Refine"]


def _createExtendFace(edge, dir, extLen, gap1=0.0, gap2=0.0, angle1=0.0, angle2=0.0, op=""):
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

def _getTopFaceBySketch(sketch, baseSolid):
    _attachedObj, attachedFaceNames = sketch.AttachmentSupport[0]
    attachedFace = baseSolid.getElement(
        SheetMetalTools.getElementFromTNP(attachedFaceNames[0])
    )
    # ToDo: make sure attachedObj is same as selObject
    #       make sure sketch is valid (sketch.Shape.Wires[0].isClosed())
    return attachedFace
    
def _getFacesAndEdgeBySelection(baseSolid, selItemName):
    selItem = baseSolid.getElement(SheetMetalTools.getElementFromTNP(selItemName))
    if type(selItem) == Part.Face:
        len = 0.
        sideFace = selItem
        for edge in selItem.Edges:
            if abs(edge.Length) > len:
                len = abs(edge.Length)
                flangeEdge = edge
        Facelist = baseSolid.ancestorsOfType(flangeEdge, Part.Face)
        topFace = Facelist[0] if not (Facelist[0].isSame(sideFace)) else Facelist[1]
    else:
        flangeEdge = selItem    
        Facelist = baseSolid.ancestorsOfType(flangeEdge, Part.Face)
        topFace = Facelist[0] if Facelist[0].Area > Facelist[1].Area else Facelist[1]
        sideFace = Facelist[1] if Facelist[0].Area > Facelist[1].Area else Facelist[0]

    return topFace, sideFace, flangeEdge

def _detectThicknessByFlangeFace(baseSolid, flangeFace):
    vert = flangeFace.Vertexes[0]
    norm = flangeFace.normalAt(0, 0)
    line = Part.makeLine(vert.Point, vert.Point - norm * 999999.0)
    intersect = line.common(baseSolid)
    for edges in intersect.Edges:
        if vert.Point.isEqual(edges.Vertexes[0].Point, smEpsilon) or \
           vert.Point.isEqual(edges.Vertexes[1].Point, smEpsilon):
            return edges.Length
    raise SMException("Unable to detect thickness from selected item.")

def _getExtendSolidBySketch(sketch, extrudeDir, thickness):
    sketchFace = Part.makeFace(sketch.Shape.Wires, "Part::FaceMakerBullseye")
    extrudeVector = extrudeDir * thickness
    extendSolid = sketchFace.extrude(extrudeVector)
    return extendSolid

def _getExtendSolidByEdge(sideFace, sideEdge, extrudeDir, thickness, length, reversed, gap1, gap2):
    faceDir = sideFace.normalAt(0, 0)
    if reversed:
        faceDir *= -1
    sketchFace = _createExtendFace(sideEdge, faceDir, length, gap1, gap2, op="SMW")
    # Part.show(sketchFace, "sketchFace")
    extendsolid = sketchFace.extrude(extrudeDir * thickness)
    return extendsolid

def _isSideFace(face, thickness):
    # Find the narrow edge.
    thk = 999999.
    thkEdge = None
    for edge in face.Edges:
        if abs(edge.Length - thickness) < smEpsilon:
            return True
    return False    

def _getOverlappingFaces(overlapSolid, nonWallSolid, thickness):
    for face in nonWallSolid.Faces:
        if _isSideFace(face, thickness):
            continue
        commonFace = face.common(overlapSolid)
        if not commonFace.Faces:
            continue
        return face, commonFace.Faces[0]
    return None, None

once = True 
def _findMatchinFaceOnSolid(face, solid):
    global once
    for solidFace in solid.Faces:
        if face.CenterOfMass.isEqual(solidFace.CenterOfMass, smEpsilon):
            return face
    return None

def _projectFaceToPlane(face, planeFace):
    projFace = planeFace.project([face])
    wire = Part.Wire(projFace.Edges)
    if not wire.isClosed():
        return None
    return Part.makeFace(wire)

def smExtrude(extLength=10.0, gap1=0.0, gap2=0.0, reversed=False, enableClearance=False, offset=0.2,
              refine=True, sketch="", selItemNames="", selObject="" ):
    import BOPTools.SplitFeatures

    baseSolid = selObject.Shape.copy()
    itemList = []
    if sketch:
        topFace = _getTopFaceBySketch(sketch, baseSolid)
        thickness = _detectThicknessByFlangeFace(baseSolid, topFace)
        extrudeDir = topFace.normalAt(0, 0) * -1
        wallSolid = topFace.extrude(extrudeDir * thickness)
        extendSolid = _getExtendSolidBySketch(sketch, extrudeDir, thickness)
        itemList.append((wallSolid, extendSolid))
    else:
        for selItemName in selItemNames:

            topFace, sideFace, sideEdge = _getFacesAndEdgeBySelection(baseSolid, selItemName)
            extrudeDir = topFace.normalAt(0, 0) * -1
            thickness = _detectThicknessByFlangeFace(baseSolid, topFace)
            extendSolid = _getExtendSolidByEdge(sideFace, sideEdge, extrudeDir, 
                                                thickness, extLength, reversed, gap1, gap2)
            wallSolid = topFace.extrude(extrudeDir * thickness)
            itemList.append((wallSolid, extendSolid))
    #     return smExtendBySketch(sketch, selObject, refine, enableClearance, offset)

    finalShape = baseSolid
    for wallSolid, extendSolid in itemList:
        nonWallSolid = finalShape.cut(wallSolid)
        # easy case: simple negative extend
        if reversed and not sketch:
            cutWallSolid = wallSolid.cut(extendSolid)
            finalShape = nonWallSolid.fuse(cutWallSolid)
            continue

        # To find Overlapping Solid, non thickness side Face that
        # touch Overlapping Solid.
        if nonWallSolid.Volume > smEpsilon and enableClearance:
            overlapSolids = nonWallSolid.common(extendSolid)
            for overlapSolid in overlapSolids.Solids:
                fullFace, overlapFace = _getOverlappingFaces(overlapSolid, nonWallSolid, thickness)
                # Part.show(overlapSolid, "overlapSolid")
                if not fullFace or not overlapFace:
                    continue
                machingFace = _findMatchinFaceOnSolid(overlapFace, overlapSolid)
                if not machingFace:
                    continue
                #Part.show(machingFace, "machingFace")
                norm = overlapFace.normalAt(0, 0)
                extrudeVec = norm * -thickness
                subtructSolid = overlapFace.extrude(extrudeVec)
                for face in overlapSolid.Faces:
                    dotprod = norm.dot(face.normalAt(0, 0)) # dot product > 0 means face angle < 90 deg
                    if face.isSame(machingFace) or dotprod < smEpsilon:
                        continue
                    projFace = _projectFaceToPlane(face, fullFace)
                    if projFace:
                        # Part.show(projFace, "projFace")
                        extrudeFace = projFace.extrude(extrudeVec)
                        subtructSolid = subtructSolid.fuse(extrudeFace) 
                subtructSolid = subtructSolid.removeSplitter()
                cutSolid = subtructSolid.makeOffsetShape(offset, 0.0, fill=False, join=2)
                # Part.show(cutSolid, "cutSolid")
                finalShape = finalShape.cut(cutSolid)
        finalShape = finalShape.fuse(extendSolid)

    # Part.show(finalShape, "finalShape")
    if refine:
        finalShape = finalShape.removeSplitter()
    return finalShape

class SMExtrudeWall:
    def __init__(self, obj, selobj, sel_items, selSketch=None):
        """Add SheetMetal Wall by Extending."""
        _tip_ = translate("App::Property", "Length of Wall")
        obj.addProperty("App::PropertyLength", "length", "Parameters", _tip_).length = 10.0
        _tip_ = translate("App::Property", "Gap from left side")
        obj.addProperty("App::PropertyDistance", "gap1", "Parameters", _tip_).gap1 = 0.0
        _tip_ = translate("App::Property", "Gap from right side")
        obj.addProperty("App::PropertyDistance", "gap2", "Parameters", _tip_).gap2 = 0.0
        _tip_ = translate("App::Property", "Base object")
        obj.addProperty("App::PropertyLinkSub", "baseObject", "Parameters",
                        _tip_).baseObject = (selobj, sel_items)
        self.addVerifyProperties(obj, selSketch)
        obj.Proxy = self
        SheetMetalTools.taskRestoreDefaults(obj, smExtrudeDefaultVars)

    def getElementMapVersion(self, _fp, ver, _prop, restored):
        if not restored:
            return smElementMapVersion + ver
        return None

    def addVerifyProperties(self, obj, selSketch=None):
        SheetMetalTools.smAddBoolProperty(obj, "reversed",
                translate("App::Property", "Reverse extend direction (cut)"), False, "Parameters")
        SheetMetalTools.smAddProperty(obj, "App::PropertyLink", "Sketch", 
                translate("App::Property", "Wall Sketch"), selSketch, "ParametersExt")
        SheetMetalTools.smAddBoolProperty(obj, "UseSubtraction",
                translate("App::Property", "Use Subtraction"), True, "ParametersExt")
        SheetMetalTools.smAddDistanceProperty(obj, "Offset",
                translate("App::Property", "Offset for subtraction"), 0.2, "ParametersExt")
        SheetMetalTools.smAddBoolProperty(obj, "Refine",
                translate("App::Property", "Use Refine"), True, "ParametersExt")
        if hasattr(obj, "UseSubstraction"):
                obj.UseSubtraction = obj.UseSubstraction  # Compatibility with old files.
        

    def execute(self, fp):
        """Execute SheetMetal Wall by Extruding."""
        self.addVerifyProperties(fp)

        # Pass selected object shape.
        Main_Object = fp.baseObject[0]
        face = fp.baseObject[1]
        fp.Shape = smExtrude(extLength=fp.length.Value,
                             gap1=fp.gap1.Value,
                             gap2=fp.gap2.Value,
                             reversed=fp.reversed,
                             enableClearance=fp.UseSubtraction,
                             offset=fp.Offset.Value,
                             refine=fp.Refine,
                             sketch=fp.Sketch,
                             selItemNames=face,
                             selObject=Main_Object)
        if fp.Sketch :
            fp.Sketch.ViewObject.Visibility = False



###################################################################################################
# Gui code
###################################################################################################

if SheetMetalTools.isGuiLoaded():
    from PySide import QtCore, QtGui

    Gui = FreeCAD.Gui
    icons_path = SheetMetalTools.icons_path


    class SMViewProviderTree(SheetMetalTools.SMViewProvider):
        """Part WB style ViewProvider."""

        def getIcon(self):
            if self.Object.Sketch:
                return os.path.join(icons_path, "SheetMetal_ExtendBySketch.svg")
            else:
                return os.path.join(icons_path, "SheetMetal_Extrude.svg")

        def getTaskPanel(self, obj):
            return SMExtendWallTaskPanel(obj)


    class SMViewProviderFlat(SMViewProviderTree):
        """Part Design WB style ViewProvider.

        Note:
            Backward compatibility only.

        """


    class SMExtendWallTaskPanel:
        """A TaskPanel for the SheetMetal."""

        def __init__(self, obj):
            self.obj = obj
            self.form = SheetMetalTools.taskLoadUI("ExtendTaskPanel.ui")

            # Make sure all properties are added.
            obj.Proxy.addVerifyProperties(obj)

            self.selParams = SheetMetalTools.taskConnectSelection(self.form.AddRemove,
                                                                  self.form.tree, self.obj,
                                                                  ["Face", "Edge"], self.form.pushClearSel)
            SheetMetalTools.taskConnectSelectionSingle(self.form.buttSelSketch,
                                                       self.form.txtSelSketch, obj, "Sketch",
                                                       ("Sketcher::SketchObject", []))
            SheetMetalTools.taskConnectSpin(obj, self.form.OffsetA, "gap1")
            SheetMetalTools.taskConnectSpin(obj, self.form.OffsetB, "gap2")
            SheetMetalTools.taskConnectSpin(obj, self.form.Length, "length")
            SheetMetalTools.taskConnectSpin(obj, self.form.Offset, "Offset")
            SheetMetalTools.taskConnectCheck(obj, self.form.RefineCheckbox, "Refine")
            SheetMetalTools.taskConnectCheck(obj, self.form.checkIntersectClear, "UseSubtraction")
            SheetMetalTools.taskConnectCheck(obj, self.form.checkReversed, "reversed")
            isStandardExtend = self.obj.Sketch is None
            self.form.groupExtend.setVisible(isStandardExtend)
            self.form.groupExtendBySketch.setVisible(not isStandardExtend)

            # for now disable clear sketch button since we have a seperate command for that
            self.form.buttClearSketch.setVisible(False)
            self.form.buttClearSketch.clicked.connect(self.clearPressed)

        def isAllowedAlterSelection(self):
            return True

        def isAllowedAlterView(self):
            return True
        
        def clearPressed(self):
            if self.obj.Sketch is None:
                return
            self.obj.Sketch.ViewObject.Visibility = True
            self.obj.Sketch = None
            self.form.txtSelSketch.setText("")
            self.obj.recompute()

        def accept(self):
            SheetMetalTools.taskAccept(self)
            SheetMetalTools.taskSaveDefaults(self.obj, smExtrudeDefaultVars)
            return True

        def reject(self):
            SheetMetalTools.taskReject(self)


    class SMExtrudeCommandClass:
        """Extrude face."""

        def GetResources(self):
            return {
                    # The name of a svg file available in the resources.
                    "Pixmap": os.path.join(icons_path, "SheetMetal_Extrude.svg"),
                    "MenuText": translate("SheetMetal", "Extend Face"),
                    "Accel": "E",
                    "ToolTip": translate(
                        "SheetMetal",
                        "Extends one or more face, on existing sheet metal.\n"
                        "1. Select edges or thickness side faces to extend walls.\n"
                        "2. Use Property-editor / Task-panel to modify other parameters",
                        ),
                    }

        def Activated(self):
            sel = Gui.Selection.getSelectionEx()[0]
            selobj = sel.Object
            newObj, activeBody = SheetMetalTools.smCreateNewObject(selobj, "Extend")
            if newObj is None:
                return
            SMExtrudeWall(newObj, selobj, sel.SubElementNames, None)
            SMViewProviderTree(newObj.ViewObject)
            SheetMetalTools.smAddNewObject(selobj, newObj, activeBody, SMExtendWallTaskPanel)
            return

        def IsActive(self):
            if (
                len(Gui.Selection.getSelection()) != 1
                or len(Gui.Selection.getSelectionEx()[0].SubElementNames) < 1
            ):
                return False
            selobj = Gui.Selection.getSelection()[0]
            if selobj.isDerivedFrom("Sketcher::SketchObject"):
                return False
            for selFace in Gui.Selection.getSelectionEx()[0].SubObjects:
                if isinstance(selFace, Part.Vertex):
                    return False
            return True

    class SMExtendBySketchCommandClass:
        """Extrude by sketch."""

        def GetResources(self):
            return {
                    # The name of a svg file available in the resources.
                    "Pixmap": os.path.join(icons_path, "SheetMetal_ExtendBySketch.svg"),
                    "MenuText": translate("SheetMetal", "Extend by Sketch"),
                    "Accel": "T",
                    "ToolTip": translate(
                        "SheetMetal",
                        "Extends a side face using a sketch.\n"
                        "1. Select a sketch to shape the extension (Good for creating tabs).\n"
                        "2. Make sure the sketch is attached to a non-edge face.\n"
                        "3. Use Property-editor / Task-panel to modify other parameters",
                        ),
                    }

        def GetValidSelection(self):
            if len(Gui.Selection.getSelection()) != 1:
                return None
            selSketch = Gui.Selection.getSelection()[0]
            if not selSketch.isDerivedFrom("Sketcher::SketchObject"):
                return None
            selobj, selFaceNames = selSketch.AttachmentSupport[0]
            if not selobj.isDerivedFrom("Part::Feature"):
                return None
            return (selSketch, selobj, selFaceNames)

        def Activated(self):
            sel = self.GetValidSelection()
            if not sel:
                FreeCAD.Console.PrintError("Please select a valid sketch attached to a face.\n")
                return
            selSketch, selobj, selFaceNames = sel
            newObj, activeBody = SheetMetalTools.smCreateNewObject(selobj, "Extend")
            SMExtrudeWall(newObj, selobj, selFaceNames, selSketch)
            SMViewProviderTree(newObj.ViewObject)
            SheetMetalTools.smAddNewObject(selobj, newObj, activeBody, SMExtendWallTaskPanel)
            return

        def IsActive(self):
            if not self.GetValidSelection():
                return False
            return True
 
    Gui.addCommand("SheetMetal_Extrude", SMExtrudeCommandClass())
    Gui.addCommand("SheetMetal_ExtendBySketch", SMExtendBySketchCommandClass())
