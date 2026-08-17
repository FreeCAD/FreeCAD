########################################################################
#
#  SheetMetalJunction.py
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
########################################################################

import os

import FreeCAD
import Part

import SheetMetalTools

# IMPORTANT: please remember to change the element map version in case
# of any changes in modeling logic.
smElementMapVersion = "sm1."

# List of properties to be saved as defaults.
smJunctionDefaultVars = [("gap", "defaultJunctionGap")]


def smJunction(gap=2.0, selEdgeNames="", MainObject=None):
    import BOPTools.JoinFeatures
    import BOPTools.SplitFeatures

    resultSolid = MainObject
    for selEdgeName in selEdgeNames:
        edge = MainObject.getElement(SheetMetalTools.getElementFromTNP(selEdgeName))
        facelist = MainObject.ancestorsOfType(edge, Part.Face)
        # for face in facelist:
        #     Part.show(face, "face")
        joinface = facelist[0].fuse(facelist[1])
        # Part.show(joinface, "joinface")
        filletedface = joinface.makeFillet(gap, joinface.Edges)
        # Part.show(filletedface, "filletedface")
        cutface1 = facelist[0].cut(filletedface)
        # Part.show(cutface1, "cutface1")
        offsetsolid1 = cutface1.makeOffsetShape(-gap, 0.0, fill=True)
        # Part.show(offsetsolid1, "offsetsolid1")
        cutface2 = facelist[1].cut(filletedface)
        # Part.show(cutface2, "cutface2")
        offsetsolid2 = cutface2.makeOffsetShape(-gap, 0.0, fill=True)
        # Part.show(offsetsolid2, "offsetsolid2")
        cutsolid = offsetsolid1.fuse(offsetsolid2)
        # Part.show(cutsolid, "cutsolid")
        resultSolid = resultSolid.cut(cutsolid)
        # Part.show(resultsolid, "resultsolid")
    return resultSolid


class SMJunction:
    def __init__(self, obj, selobj, sel_items):
        """Add Gap to Solid."""
        _tip_ = FreeCAD.Qt.translate("App::Property", "Junction Gap")
        obj.addProperty("App::PropertyLength", "gap", "Parameters", _tip_).gap = 2.0
        _tip_ = FreeCAD.Qt.translate("App::Property", "Base Object")
        obj.addProperty("App::PropertyLinkSub", "baseObject", "Parameters",
                        _tip_).baseObject = (selobj, sel_items)
        obj.Proxy = self
        SheetMetalTools.taskRestoreDefaults(obj, smJunctionDefaultVars)

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
        fp.Shape = smJunction(gap=fp.gap.Value, selEdgeNames=fp.baseObject[1],
                              MainObject=Main_Object)
        SheetMetalTools.smHideObjects(fp.baseObject[0])


###################################################################################################
# Gui code
###################################################################################################

if SheetMetalTools.isGuiLoaded():
    from PySide import QtCore, QtGui

    Gui = FreeCAD.Gui
    icons_path = SheetMetalTools.icons_path


    class SMJViewProviderTree(SheetMetalTools.SMViewProvider):
        """Part WB style ViewProvider."""

        def getIcon(self):
            return os.path.join(icons_path, "SheetMetal_AddJunction.svg")

        def getTaskPanel(self, obj):
            return SMJunctionTaskPanel(obj)


    class SMJViewProviderFlat(SMJViewProviderTree):
        """Part Design WB style ViewProvider.

        Note:
            Backward compatibility only.

        """


    class SMJunctionTaskPanel:
        """A TaskPanel for the SheetMetal addJunction command."""

        def __init__(self, obj):
            self.obj = obj
            self.form = SheetMetalTools.taskLoadUI("AddJunctionPanel.ui")

            # Make sure all properties are added.
            obj.Proxy.addVerifyProperties(obj)

            self.selParams = SheetMetalTools.taskConnectSelection(
                    self.form.AddRemove, self.form.tree, self.obj, ["Edge"], self.form.pushClearSel
            )
            SheetMetalTools.taskConnectSpin(obj, self.form.JunctionWidth, "gap")
            # SheetMetalTools.taskConnectCheck(obj, self.form.RefineCheckbox, "Refine")

        def isAllowedAlterSelection(self):
            return True

        def isAllowedAlterView(self):
            return True

        def accept(self):
            SheetMetalTools.taskAccept(self)
            SheetMetalTools.taskSaveDefaults(self.obj, smJunctionDefaultVars)
            return True

        def reject(self):
            SheetMetalTools.taskReject(self)

        def retranslateUi(self, TaskPanel):
            self.addButton.setText(QtGui.QApplication.translate("draft", "Update", None))


    class AddJunctionCommandClass:
        """Add Junction command."""

        def GetResources(self):
            return {
                    # The name of a svg file available in the resources.
                    "Pixmap": os.path.join(icons_path, "SheetMetal_AddJunction.svg"),
                    "MenuText": FreeCAD.Qt.translate("SheetMetal", "Make Junction"),
                    "Accel": "S, J",
                    "ToolTip": FreeCAD.Qt.translate(
                        "SheetMetal",
                        "Create a rip where two walls come together on solids.\n"
                        "1. Select edge(s) to create rip on corner edge(s).\n"
                        "2. Use Property editor to modify parameters"
                        )
                    }

        def Activated(self):
            sel = Gui.Selection.getSelectionEx()[0]
            selobj = sel.Object
            newObj, activeBody = SheetMetalTools.smCreateNewObject(selobj, "Junction")
            if newObj is None:
                return
            SMJunction(newObj, selobj, sel.SubElementNames)
            SMJViewProviderTree(newObj.ViewObject)
            SheetMetalTools.smAddNewObject(selobj, newObj, activeBody, SMJunctionTaskPanel)
            return

        def IsActive(self):
            sel = Gui.Selection.getSelectionEx()[0]
            if len(Gui.Selection.getSelection()) < 1 or len(sel.SubElementNames) < 1:
                return False
                # selobj = Gui.Selection.getSelection()[0]
            for selEdge in sel.SubObjects:
                if not isinstance(selEdge, Part.Edge):
                    return False
            return True


    Gui.addCommand("SheetMetal_AddJunction", AddJunctionCommandClass())
