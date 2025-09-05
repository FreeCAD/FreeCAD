#/***************************************************************************
# *   Copyright (c) 2024 Eric Price (CorvusCorax)                           *
# *                      <eric.price[at]tuebingen.mpg.de>                   *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# ***************************************************************************/

__title__ = "BOPTools.ToleranceFeatures module"
__author__ = "CorvusCorax"
__url__ = "https://www.freecad.org"
__doc__ = "Implementation of document objects (features) to adjust/manipulate tolerances."

import FreeCAD
import Part

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui
# -------------------------- common stuff -------------------------------------

# -------------------------- translation-related code -------------------------

    try:
        _fromUtf8 = QtCore.QString.fromUtf8
    except Exception:
        def _fromUtf8(s):
            return s
    translate = FreeCAD.Qt.translate
# --------------------------/translation-related code -------------------------


def getParamRefine():
    return FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Part/Boolean").GetBool("RefineModel")


def cmdCreateToleranceSetFeature(name, minTolerance=1e-7, maxTolerance=0):
    """cmdCreateToleranceSetFeature(name, minTolerance, maxTolerance): generalized implementation of GUI commands."""
    sel = FreeCADGui.Selection.getSelectionEx()

    FreeCAD.ActiveDocument.openTransaction("Create ToleranceSet")
    FreeCADGui.addModule("BOPTools.ToleranceFeatures")
    FreeCADGui.doCommand("j = BOPTools.ToleranceFeatures.makeToleranceSet(name='{name}')".format(name=name))
    FreeCADGui.doCommand("j.minTolerance = {minTolerance}".format(minTolerance=minTolerance))
    FreeCADGui.doCommand("j.maxTolerance = {maxTolerance}".format(maxTolerance=maxTolerance))
    FreeCADGui.doCommand("j.Objects = {sel}".format(
       sel= "["  +  ", ".join(["App.ActiveDocument."+so.Object.Name for so in sel])  +  "]"
    ))

    try:
        FreeCADGui.doCommand("j.Proxy.execute(j)")
        FreeCADGui.doCommand("j.purgeTouched()")
    except Exception as err:
        mb = QtGui.QMessageBox()
        mb.setIcon(mb.Icon.Warning)
        error_text1 = translate("Part_ToleranceFeatures", "Computing the result failed with an error:")
        error_text2 = translate("Part_ToleranceFeatures", "Click 'Continue' to create the feature anyway, or 'Abort' to cancel.")
        mb.setText(error_text1 + "\n\n" + str(err) + "\n\n" + error_text2)
        mb.setWindowTitle(translate("Part_ToleranceFeatures","Bad Selection", None))
        btnAbort = mb.addButton(QtGui.QMessageBox.StandardButton.Abort)
        btnOK = mb.addButton(translate("Part_ToleranceFeatures","Continue",None),
                             QtGui.QMessageBox.ButtonRole.ActionRole)
        mb.setDefaultButton(btnOK)
        mb.exec_()

        if mb.clickedButton() is btnAbort:
            FreeCAD.ActiveDocument.abortTransaction()
            return

    FreeCADGui.doCommand("for obj in j.ViewObject.Proxy.claimChildren():\n"
                         "    obj.ViewObject.hide()")

    FreeCAD.ActiveDocument.commitTransaction()


def getIconPath(icon_dot_svg):
    return icon_dot_svg

# -------------------------- /common stuff ------------------------------------

# -------------------------- Connect ------------------------------------------

def makeToleranceSet(name):
    '''makeToleranceSet(name): makes an ToleranceSet object.'''
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
    FeatureToleranceSet(obj)
    if FreeCAD.GuiUp:
        ViewProviderToleranceSet(obj.ViewObject)
    return obj


class FeatureToleranceSet:
    """The PartToleranceSetFeature object."""

    def __init__(self,obj):
        obj.addProperty("App::PropertyLinkList","Objects","ToleranceSet","Objects to have tolerance adjusted.", locked=True)
        obj.addProperty("App::PropertyBool","Refine","ToleranceSet",
                        "True = refine resulting shape. False = output as is.", locked=True)
        obj.addProperty("App::PropertyLength","minTolerance","ToleranceSet", "0.1 nm", locked=True)
        obj.addProperty("App::PropertyLength","maxTolerance","ToleranceSet", "0", locked=True)
        obj.Refine = getParamRefine()

        obj.Proxy = self
        self.Type = "FeatureToleranceSet"

    def onDocumentRestored(self, obj):
        if not hasattr(obj, 'maxTolerance'):
            obj.addProperty("App::PropertyLength","maxTolerance","ToleranceSet", "0", locked=True)

    def execute(self,selfobj):
        shapes = []
        for obj in selfobj.Objects:
            sh = obj.Shape.copy(True,False)
            sh.limitTolerance(selfobj.minTolerance,selfobj.maxTolerance)
            if selfobj.Refine:
                sh.fix(selfobj.minTolerance,selfobj.minTolerance,selfobj.maxTolerance)
                sh = sh.removeSplitter()
                sh.fix(selfobj.minTolerance,selfobj.minTolerance,selfobj.maxTolerance)
            shapes.append(sh)

        if len(shapes)>1:
            rst = Part.makeCompound(shapes)
        else:
            rst = shapes[0]
        selfobj.Shape = rst


class ViewProviderToleranceSet:
    """A View Provider for the Part ToleranceSet feature."""

    def __init__(self,vobj):
        vobj.Proxy = self

    def getIcon(self):
        return ":/icons/preferences-part_design.svg"

    def attach(self, vobj):
        self.ViewObject = vobj
        self.Object = vobj.Object

    def dumps(self):
        return None

    def loads(self,state):
        return None

    def claimChildren(self):
        return self.Object.Objects

    def onDelete(self, feature, subelements):
        try:
            for obj in self.claimChildren():
                obj.ViewObject.show()
        except Exception as err:
            FreeCAD.Console.PrintError("Error in onDelete: " + str(err))
        return True

    def canDragObjects(self):
        return True
    def canDropObjects(self):
        return True
    def canDragObject(self, dragged_object):
        return True
    def canDropObject(self, incoming_object):
        return hasattr(incoming_object, 'Shape')
    def dragObject(self, selfvp, dragged_object):
        objs = self.Object.Objects
        objs.remove(dragged_object)
        self.Object.Objects = objs
    def dropObject(self, selfvp, incoming_object):
        self.Object.Objects = self.Object.Objects + [incoming_object]


class CommandToleranceSet:
    """Command to create ToleranceSet feature."""

    def GetResources(self):
        return {'Pixmap': getIconPath("preferences-part_design.svg"),
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Part_ToleranceSet","Set Tolerance"),
                'Accel': "",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Part_ToleranceSet",
                                                    "Creates a parametric copy of the selected object with all contained tolerances set to at least a certain minimum value")}

    def Activated(self):
        if len(FreeCADGui.Selection.getSelectionEx()) >= 1:
            cmdCreateToleranceSetFeature(name="Tolerance")
        else:
            mb = QtGui.QMessageBox()
            mb.setIcon(mb.Icon.Warning)
            mb.setText(translate("Part_ToleranceSet",
                                  "Select at least one object or compounds", None))
            mb.setWindowTitle(translate("Part_ToleranceSet","Bad Selection", None))
            mb.exec_()

    def IsActive(self):
        if FreeCAD.ActiveDocument:
            return True
        else:
            return False

# -------------------------- /Connect -----------------------------------------


def addCommands():
    FreeCADGui.addCommand('Part_ToleranceSet', CommandToleranceSet())
