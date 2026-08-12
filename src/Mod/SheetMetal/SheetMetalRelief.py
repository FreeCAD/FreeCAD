########################################################################
#
#  SheetMetalRelief.py
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

import os

import FreeCAD
import Part

import SheetMetalTools

# IMPORTANT: please remember to change the element map version in case
# of any changes in modeling logic.
smElementMapVersion = "sm1."

smEpsilon = SheetMetalTools.smEpsilon
smSolidCornerReliefDefaultVars = [("relief", "defaultCornerRelief")]


def smMakeFace(vertex, face, edges, relief):
    if edges[0].Vertexes[0].isSame(vertex):
        Edgedir1 = edges[0].Vertexes[1].Point - edges[0].Vertexes[0].Point
    else:
        Edgedir1 = edges[0].Vertexes[0].Point - edges[0].Vertexes[1].Point
    Edgedir1.normalize()

    if edges[1].Vertexes[0].isSame(vertex):
        Edgedir2 = edges[1].Vertexes[1].Point - edges[1].Vertexes[0].Point
    else:
        Edgedir2 = edges[1].Vertexes[0].Point - edges[1].Vertexes[1].Point
    Edgedir2.normalize()
    normal = face.normalAt(0, 0)
    Edgedir3 = normal.cross(Edgedir1)
    Edgedir4 = normal.cross(Edgedir2)

    p1 = vertex.Point
    p2 = p1 + relief * Edgedir1
    p3 = p2 + relief * Edgedir3
    if not (face.isInside(p3, 0.0, True)):
        p3 = p2 + relief * Edgedir3 * -1
    p6 = p1 + relief * Edgedir2
    p5 = p6 + relief * Edgedir4
    if not (face.isInside(p5, 0.0, True)):
        p5 = p6 + relief * Edgedir4 * -1
    # print([p1,p2,p3,p5,p6,p1])

    e1 = Part.makeLine(p2, p3)
    # Part.show(e1, "e1")
    e2 = Part.makeLine(p5, p6)
    # Part.show(e2, "e2")
    section = e1.section(e2)
    # Part.show(section1, "section1")

    if section.Vertexes:
        wire = Part.makePolygon([p1, p2, p3, p6, p1])
    else:
        p41 = p3 + relief * Edgedir1 * -1
        p42 = p5 + relief * Edgedir2 * -1
        e1 = Part.Line(p3, p41).toShape()
        # Part.show(e1, "e1")
        e2 = Part.Line(p42, p5).toShape()
        # Part.show(e2, "e2")
        section = e1.section(e2)
        # Part.show(section1, "section1")
        p4 = section.Vertexes[0].Point
        wire = Part.makePolygon([p1, p2, p3, p4, p5, p6, p1])

    extface = Part.Face(wire)
    return extface


def smRelief(relief=2.0, selVertexNames=" ", MainObject=None):
    resultSolid = MainObject
    for selVertexName in selVertexNames:
        vertex = MainObject.getElement(SheetMetalTools.getElementFromTNP(selVertexName))
        facelist = MainObject.ancestorsOfType(vertex, Part.Face)

        extsolidlist = []
        for face in facelist:
            # Part.show(face, "face")
            edgelist = face.ancestorsOfType(vertex, Part.Edge)
            # for edge in edgelist:
            #     Part.show(edge, "edge")
            extface = smMakeFace(vertex, face, edgelist, relief)
            # Part.show(extface, "extface")
            extsolid = extface.extrude(relief * face.normalAt(0, 0) * -1)
            extsolidlist.append(extsolid)

        cutsolid = extsolidlist[0].multiFuse(extsolidlist[1:])
        # Part.show(cutsolid, "cutsolid")
        cutsolid = cutsolid.removeSplitter()
        resultSolid = resultSolid.cut(cutsolid)
        # Part.show(resultsolid, "resultsolid")
    return resultSolid


class SMRelief:
    def __init__(self, obj, selobj, sel_items):
        """Add Relief to Solid."""
        _tip_ = FreeCAD.Qt.translate("App::Property", "Relief Size")
        obj.addProperty("App::PropertyLength", "relief", "Parameters", _tip_).relief = 2.0
        _tip_ = FreeCAD.Qt.translate("App::Property", "Base Object")
        obj.addProperty("App::PropertyLinkSub", "baseObject", "Parameters",
                        _tip_).baseObject = (selobj, sel_items)
        SheetMetalTools.taskRestoreDefaults(obj, smSolidCornerReliefDefaultVars)
        obj.Proxy = self

    def addVerifyProperties(self, obj):
        pass

    def getElementMapVersion(self, _fp, ver, _prop, restored):
        if not restored:
            return smElementMapVersion + ver
        return None

    def execute(self, fp):
        """Print a short message when doing a recomputation.

        Note:
            This method is mandatory.

        """
        # Pass selected object shape.
        Main_Object = fp.baseObject[0].Shape.copy()
        fp.Shape = smRelief(relief=fp.relief.Value, selVertexNames=fp.baseObject[1],
                            MainObject=Main_Object)
        SheetMetalTools.smHideObjects(fp.baseObject[0])


###################################################################################################
# Gui code
###################################################################################################

if SheetMetalTools.isGuiLoaded():
    from PySide import QtCore, QtGui

    Gui = FreeCAD.Gui
    icons_path = SheetMetalTools.icons_path


    class SMReliefViewProviderTree(SheetMetalTools.SMViewProvider):
        """Part WB style ViewProvider."""

        def getIcon(self):
            return os.path.join(icons_path, "SheetMetal_AddRelief.svg")

        def getTaskPanel(self, obj):
            return SMReliefTaskPanel(obj)


    class SMReliefViewProviderFlat(SMReliefViewProviderTree):
        """Part Design WB style ViewProvider.

        Note:
            Backward compatibility only.

        """


    class SMReliefTaskPanel:
        """A TaskPanel for the SheetMetal relief on solid corner."""

        def __init__(self, obj):
            self.obj = obj
            self.form = SheetMetalTools.taskLoadUI("SolidCornerReliefPanel.ui")
            # Make sure all properties are added.
            obj.Proxy.addVerifyProperties(obj)
            self.selParams = SheetMetalTools.taskConnectSelection(self.form.AddRemove,
                                                                  self.form.tree, self.obj,
                                                                  ["Vertex"],
                                                                  self.form.pushClearSel)
            SheetMetalTools.taskConnectSpin(obj, self.form.CornerSize, "relief")
            # SheetMetalTools.taskConnectCheck(obj, self.form.RefineCheckbox, "Refine")

        def isAllowedAlterSelection(self):
            return True

        def isAllowedAlterView(self):
            return True

        def accept(self):
            SheetMetalTools.taskAccept(self)
            SheetMetalTools.taskSaveDefaults(self.obj, smSolidCornerReliefDefaultVars)
            return True

        def reject(self):
            SheetMetalTools.taskReject(self)


    class AddReliefCommandClass:
        """Add Relief command."""

        def GetResources(self):
            return {
                    # The name of a svg file available in the resources.
                    "Pixmap": os.path.join(icons_path, "SheetMetal_AddRelief.svg"),
                    "MenuText": FreeCAD.Qt.translate("SheetMetal", "Make Relief"),
                    "Accel": "S, R",
                    "ToolTip": FreeCAD.Qt.translate(
                        "SheetMetal",
                        "Modify an Individual solid corner to create Relief.\n"
                        "1. Select Vertex(es) to create Relief on Solid corner Vertex(es).\n"
                        "2. Use Property editor to modify default parameters"
                        )
                    }

        def Activated(self):
            sel = Gui.Selection.getSelectionEx()[0]
            selobj = Gui.Selection.getSelectionEx()[0].Object
            newObj, activeBody = SheetMetalTools.smCreateNewObject(selobj, "CornerRelief")
            if newObj is None:
                return
            SMRelief(newObj, selobj, sel.SubElementNames)
            SMReliefViewProviderTree(newObj.ViewObject)
            SheetMetalTools.smAddNewObject(selobj, newObj, activeBody, SMReliefTaskPanel)
            return

        def IsActive(self):
            if (len(Gui.Selection.getSelection()) < 1
                    or len(Gui.Selection.getSelectionEx()[0].SubElementNames) < 1):
                return False
            # selobj = Gui.Selection.getSelection()[0]
            for selVertex in Gui.Selection.getSelectionEx()[0].SubObjects:
                if type(selVertex) != Part.Vertex:
                    return False
            return True


    Gui.addCommand("SheetMetal_AddRelief", AddReliefCommandClass())
