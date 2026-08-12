########################################################################
#
#  SketchOnSheetMetalCmd.py
#
#  Copyright 2015 Shai Seger <shaise at gmail dot com>
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

import SheetMetalBendSolid
import SheetMetalTools
from SheetMetalCornerReliefCmd import bendAngle

smEpsilon = SheetMetalTools.smEpsilon


def smSketchOnSheetMetal(kfactor=0.5, sketch="", flipped=False, selFaceNames="", MainObject=None):
    resultSolid = MainObject.Shape.copy()
    selElement = resultSolid.getElement(SheetMetalTools.getElementFromTNP(selFaceNames[0]))
    LargeFace = SheetMetalTools.smGetFaceByEdge(selElement, resultSolid)
    sketch_face = Part.makeFace(sketch.Shape.Wires, "Part::FaceMakerBullseye")
    # To get thk of sheet, top face normal.
    thk = SheetMetalTools.smGetThickness(resultSolid, LargeFace)
    # print(thk)
    # # To get top face normal, flatsolid
    solidlist = []
    normal = LargeFace.normalAt(0, 0)
    # To check face direction.
    coeff = normal.dot(sketch_face.Faces[0].normalAt(0, 0))
    if coeff < 0:
        sketch_face.reverse()
    Flatface = sketch_face.common(LargeFace)
    BalanceFaces = sketch_face.cut(Flatface)
    # Part.show(BalanceFace,"BalanceFace")
    Flatsolid = Flatface.extrude(normal * -thk)
    # Part.show(Flatsolid,"Flatsolid")
    solidlist.append(Flatsolid)
    if BalanceFaces.Faces:
        for BalanceFace in BalanceFaces.Faces:
            # Part.show(BalanceFace, "BalanceFace")
            TopFace = LargeFace
            # Part.show(TopFace, "TopFace")
            # flipped = False
            while BalanceFace.Faces:
                BendEdge = SheetMetalTools.smGetIntersectingEdge(BalanceFace, TopFace)
                # Part.show(BendEdge, "BendEdge")
                facelist = resultSolid.ancestorsOfType(BendEdge, Part.Face)

                # To get bend radius, bend angle.
                for cylface in facelist:
                    if issubclass(type(cylface.Surface), Part.Cylinder):
                        break
                if not (issubclass(type(cylface.Surface), Part.Cylinder)):
                    break
                # Part.show(cylface,"cylface")
                for planeface in facelist:
                    if issubclass(type(planeface.Surface), Part.Plane):
                        break
                # Part.show(planeface,"planeface")
                normal = planeface.normalAt(0, 0)
                revAxisV = cylface.Surface.Axis
                revAxisP = cylface.Surface.Center
                bendA = bendAngle(cylface, revAxisP)
                # print([bendA, revAxisV, revAxisP, cylface.Orientation])

                # To check bend direction.
                offsetface = cylface.makeOffsetShape(-thk, 0.0, fill=False)
                # Part.show(offsetface, "offsetface")
                if offsetface.Area < cylface.Area:
                    bendR = cylface.Surface.Radius - thk
                    flipped = True
                else:
                    bendR = cylface.Surface.Radius
                    flipped = False
                # To arrive unfold Length, neutralRadius.
                unfoldLength = (bendR + kfactor*thk) * abs(bendA) * math.pi / 180.0
                neutralRadius = bendR + kfactor*thk
                # print([unfoldLength, neutralRadius])

                # To get faceNormal, bend face.
                faceNormal = normal.cross(revAxisV).normalize()
                # print(faceNormal)
                if bendR < cylface.Surface.Radius:
                    offsetSolid = cylface.makeOffsetShape(bendR / 2.0, 0.0, fill=True)
                else:
                    offsetSolid = cylface.makeOffsetShape(-bendR / 2.0, 0.0, fill=True)
                # Part.show(offsetSolid,"offsetSolid")
                tool = BendEdge.copy()
                FaceArea = tool.extrude(faceNormal * -unfoldLength)
                # Part.show(FaceArea,"FaceArea")
                # Part.show(BalanceFace,"BalanceFace")
                SolidFace = offsetSolid.common(FaceArea)
                # Part.show(BendSolidFace,"BendSolidFace")
                if not SolidFace.Faces:
                    faceNormal *= -1
                    FaceArea = tool.extrude(faceNormal * -unfoldLength)
                BendSolidFace = BalanceFace.common(FaceArea)
                # Part.show(FaceArea,"FaceArea")
                # Part.show(BendSolidFace,"BendSolidFace")
                # print([bendR, bendA, revAxisV, revAxisP, normal, flipped,
                #        BendSolidFace.Faces[0].normalAt(0, 0)])

                bendsolid = SheetMetalBendSolid.bend_solid(BendSolidFace.Faces[0], BendEdge, bendR,
                                                           thk, neutralRadius, revAxisV, flipped)
                # Part.show(bendsolid,"bendsolid")
                solidlist.append(bendsolid)

                if flipped:
                    bendA = -bendA
                if not SolidFace.Faces:
                    revAxisV *= -1
                sketch_face = BalanceFace.cut(BendSolidFace)
                sketch_face.translate(faceNormal * unfoldLength)
                # Part.show(sketch_face,"sketch_face")
                sketch_face.rotate(revAxisP, -revAxisV, bendA)
                # Part.show(sketch_face,"Rsketch_face")
                TopFace = SheetMetalTools.smGetIntersectingFace(sketch_face, resultSolid)
                # Part.show(TopFace,"TopFace")

                # To get top face normal, flatsolid.
                normal = TopFace.normalAt(0, 0)
                Flatface = sketch_face.common(TopFace)
                BalanceFace = sketch_face.cut(Flatface)
                # Part.show(BalanceFace,"BalanceFace")
                Flatsolid = Flatface.extrude(normal * -thk)
                # Part.show(Flatsolid,"Flatsolid")
                solidlist.append(Flatsolid)
    # To get relief Solid fused.
    if len(solidlist) > 1:
        SMSolid = solidlist[0].multiFuse(solidlist[1:])
        # Part.show(SMSolid,"SMSolid")
        SMSolid = SMSolid.removeSplitter()
    else:
        SMSolid = solidlist[0]
    # Part.show(SMSolid,"SMSolid")
    resultSolid = resultSolid.cut(SMSolid)
    SheetMetalTools.smHideObjects(MainObject, sketch)
    return resultSolid


class SMSketchOnSheet:
    def __init__(self, obj, selobj, sel_items, selsketch):
        """Add Sketch based cut On Sheet metal."""
        _tip_ = FreeCAD.Qt.translate("App::Property", "Base Object")
        obj.addProperty("App::PropertyLinkSub", "baseObject", "Parameters", _tip_).baseObject = (
                selobj, sel_items)
        _tip_ = FreeCAD.Qt.translate("App::Property", "Sketch on Sheetmetal")
        obj.addProperty("App::PropertyLink", "Sketch", "Parameters", _tip_).Sketch = selsketch
        _tip_ = FreeCAD.Qt.translate("App::Property", "Gap from Left Side")
        obj.addProperty("App::PropertyFloatConstraint", "kfactor", "Parameters", _tip_).kfactor = (
                0.5, 0.0, 1.0, 0.01)
        self.addVerifyProperties(obj)
        obj.Proxy = self

    def addVerifyProperties(self, obj):
        """Add new properties to the object here and not on init."""
        pass

    def execute(self, fp):
        """Print a short message when doing a recomputation.

        Note:
            This method is mandatory.

        """
        fp.Shape = smSketchOnSheetMetal(kfactor=fp.kfactor,
                                        sketch=fp.Sketch,
                                        selFaceNames=fp.baseObject[1],
                                        MainObject=fp.baseObject[0])


###################################################################################################
#  Gui code
###################################################################################################

if SheetMetalTools.isGuiLoaded():
    Gui = FreeCAD.Gui
    icons_path = SheetMetalTools.icons_path


    class SMSketchOnSheetVP(SheetMetalTools.SMViewProvider):
        """Part WB style ViewProvider."""

        def getIcon(self):
            return os.path.join(icons_path, "SheetMetal_SketchOnSheet.svg")

        def getTaskPanel(self, obj):
            return SMWrappedCutoutTaskPanel(obj)


    class SMSketchOnSheetPDVP(SMSketchOnSheetVP):
        """Part Design WB style ViewProvider.

        Note:
            Backward compatibility only.

        """


    class SMWrappedCutoutTaskPanel:
        """A TaskPanel for the SheetMetal Wrapped Cutout."""

        def __init__(self, obj):
            self.obj = obj
            self.form = SheetMetalTools.taskLoadUI("WrappedCutoutPanel.ui")

            # Make sure all properties are added.
            obj.Proxy.addVerifyProperties(obj)

            self.faceSelParams = SheetMetalTools.taskConnectSelectionSingle(
                    self.form.pushFace, self.form.txtFace, obj, "baseObject", ["Face"])
            self.sketchSelParams = SheetMetalTools.taskConnectSelectionSingle(
                    self.form.pushSketch,
                    self.form.txtSketch,
                    obj,
                    "Sketch",
                    ("Sketcher::SketchObject", [])
            )
            SheetMetalTools.taskConnectSpin(obj, self.form.floatKFactor, "kfactor")

        def isAllowedAlterSelection(self):
            return True

        def isAllowedAlterView(self):
            return True

        def accept(self):
            SheetMetalTools.taskAccept(self)
            # Hide sketch after click OK button.
            self.obj.Sketch.ViewObject.hide()
            return True

        def reject(self):
            SheetMetalTools.taskReject(self)


    class AddSketchOnSheetCommandClass:
        """Add Wrap cutout command."""

        def GetResources(self):
            return {
                    # The name of a svg file available in the resources.
                    "Pixmap": os.path.join(icons_path, "SheetMetal_SketchOnSheet.svg"),
                    "MenuText": FreeCAD.Qt.translate("SheetMetal", "Wrap Cutout"),
                    "Accel": "M, S",
                    "ToolTip": FreeCAD.Qt.translate(
                        "SheetMetal",
                        "Wrap cutout from a Sketch On Sheet metal faces\n"
                        "1. Select a flat face on sheet metal and\n"
                        "2. Select a sketch on same face to create sheetmetal wrapped cut.\n"
                        "3. Use Property editor to modify other parameters",
                        ),
                    }

        def Activated(self):
            sel = Gui.Selection.getSelectionEx()[0]
            selobj = Gui.Selection.getSelectionEx()[0].Object
            selected_faces = sel.SubElementNames[0]
            selected_sketch = Gui.Selection.getSelectionEx()[1].Object
            newObj, activeBody = SheetMetalTools.smCreateNewObject(selobj, "WrappedCutout")
            if newObj is None:
                return
            SMSketchOnSheet(newObj, selobj, selected_faces, selected_sketch)
            SMSketchOnSheetVP(newObj.ViewObject)
            SheetMetalTools.smAddNewObject(selobj, newObj, activeBody, SMWrappedCutoutTaskPanel)

        def IsActive(self):
            if len(Gui.Selection.getSelection()) < 2:
                return False
            #    selobj = Gui.Selection.getSelection()[1]
            #    if str(type(selobj)) != "<type 'Sketcher.SketchObject'>" :
            #      return False
            return True


    Gui.addCommand("SheetMetal_SketchOnSheet", AddSketchOnSheetCommandClass())
