# ***************************************************************************
# *   Copyright (c) 2016 Victor Titov (DeepSOIC) <vv.titov@gmail.com>       *
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

import FreeCAD, Part

# if FreeCAD.GuiUp:
# import FreeCADGui
# from PySide import QtCore, QtGui

__title__ = "JoinFeatures module (legacy)"
__author__ = "DeepSOIC"
__url__ = "http://www.freecad.org"
__doc__ = "Legacy JoinFeatures module provided for ability to load projects made with \
FreeCAD v0.16. Do not use. Use BOPTools.JoinFeatures instead."


def getParamRefine():
    return FreeCAD.ParamGet(
        "User parameter:BaseApp/Preferences/Mod/Part/Boolean"
    ).GetBool("RefineModel")


def shapeOfMaxVol(compound):
    if compound.ShapeType == "Compound":
        maxVol = 0
        cntEq = 0
        shMax = None
        for sh in compound.childShapes():
            v = sh.Volume
            if v > maxVol + 1e-8:
                maxVol = v
                shMax = sh
                cntEq = 1
            elif abs(v - maxVol) <= 1e-8:
                cntEq = cntEq + 1
        if cntEq > 1:
            raise ValueError("Equal volumes, can't figure out what to cut off!")
        return shMax
    else:
        return compound


#
# def makePartJoinFeature(name, mode="bypass"):
# """makePartJoinFeature(name, mode = 'bypass'): makes an PartJoinFeature object."""
# obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython", name)
# _PartJoinFeature(obj)
# obj.Mode = mode
# obj.Refine = getParamRefine()
# _ViewProviderPartJoinFeature(obj.ViewObject)
# return obj


class _PartJoinFeature:
    "The PartJoinFeature object"

    def __init__(self, obj):
        self.Type = "PartJoinFeature"
        obj.addProperty(
            "App::PropertyEnumeration",
            "Mode",
            "Join",
            "The mode of operation. bypass = make compound (fast)",
        )
        obj.Mode = ["bypass", "Connect", "Embed", "Cutout"]
        obj.addProperty("App::PropertyLink", "Base", "Join", "First object")
        obj.addProperty("App::PropertyLink", "Tool", "Join", "Second object")
        obj.addProperty(
            "App::PropertyBool",
            "Refine",
            "Join",
            "True = refine resulting shape. False = output as is.",
        )

        obj.Proxy = self

    def execute(self, obj):
        rst = None
        if obj.Mode == "bypass":
            rst = Part.makeCompound([obj.Base.Shape, obj.Tool.Shape])
        else:
            cut1 = obj.Base.Shape.cut(obj.Tool.Shape)
            cut1 = shapeOfMaxVol(cut1)
            if obj.Mode == "Connect":
                cut2 = obj.Tool.Shape.cut(obj.Base.Shape)
                cut2 = shapeOfMaxVol(cut2)
                rst = cut1.multiFuse([cut2, obj.Tool.Shape.common(obj.Base.Shape)])
            elif obj.Mode == "Embed":
                rst = cut1.fuse(obj.Tool.Shape)
            elif obj.Mode == "Cutout":
                rst = cut1
            if obj.Refine:
                rst = rst.removeSplitter()
        obj.Shape = rst
        return


class _ViewProviderPartJoinFeature:
    "A View Provider for the PartJoinFeature object"

    def __init__(self, vobj):
        vobj.Proxy = self

    def getIcon(self):
        if self.Object is None:
            return getIconPath("Part_JoinConnect.svg")
        else:
            return getIconPath(
                {
                    "bypass": "Part_JoinBypass.svg",
                    "Connect": "Part_JoinConnect.svg",
                    "Embed": "Part_JoinEmbed.svg",
                    "Cutout": "Part_JoinCutout.svg",
                }[self.Object.Mode]
            )

    def attach(self, vobj):
        self.ViewObject = vobj
        self.Object = vobj.Object

    def setEdit(self, vobj, mode):
        return False

    def unsetEdit(self, vobj, mode):
        return

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def claimChildren(self):
        return [self.Object.Base, self.Object.Tool]

    def onDelete(self, feature, subelements):
        try:
            self.Object.Base.ViewObject.show()
            self.Object.Tool.ViewObject.show()
        except Exception as err:
            FreeCAD.Console.PrintError("Error in onDelete: " + str(err))
        return True


# def CreateJoinFeature(name, mode):
# sel = FreeCADGui.Selection.getSelectionEx()
# FreeCAD.ActiveDocument.openTransaction("Create " + mode + "ObjectsFeature")
# FreeCADGui.addModule("JoinFeatures")
# FreeCADGui.doCommand(
# "j = JoinFeatures.makePartJoinFeature(name = '"
# + name
# + "', mode = '"
# + mode
# + "' )"
# )
# FreeCADGui.doCommand("j.Base = App.ActiveDocument." + sel[0].Object.Name)
# FreeCADGui.doCommand("j.Tool = App.ActiveDocument." + sel[1].Object.Name)
# try:
# FreeCADGui.doCommand("j.Proxy.execute(j)")
# FreeCADGui.doCommand("j.purgeTouched()")
# except Exception as err:
# mb = QtGui.QMessageBox()
# mb.setIcon(mb.Icon.Warning)
# mb.setText(
# translate(
# "Part_JoinFeatures",
# "Computing the result failed with an error: {err}. Click 'Continue' to create the feature anyway, or 'Abort' to cancel.",
# None,
# ).format(err=str(err))
# )
# mb.setWindowTitle(translate("Part_JoinFeatures", "Bad selection", None))
# btnAbort = mb.addButton(QtGui.QMessageBox.StandardButton.Abort)
# btnOK = mb.addButton(
# translate("Part_JoinFeatures", "Continue", None),
# QtGui.QMessageBox.ButtonRole.ActionRole,
# )
# mb.setDefaultButton(btnOK)

# mb.exec_()

# if mb.clickedButton() is btnAbort:
# FreeCAD.ActiveDocument.abortTransaction()
# return

# FreeCADGui.doCommand("j.Base.ViewObject.hide()")
# FreeCADGui.doCommand("j.Tool.ViewObject.hide()")

# FreeCAD.ActiveDocument.commitTransaction()


def getIconPath(icon_dot_svg):
    return ":/icons/" + icon_dot_svg


# class _CommandConnectFeature:
# "Command to create PartJoinFeature in Connect mode"

# def GetResources(self):
# return {
# "Pixmap": getIconPath("Part_JoinConnect.svg"),
# "MenuText": QtCore.QT_TRANSLATE_NOOP(
# "Part_ConnectFeature", "Connect objects"
# ),
# "Accel": "",
# "ToolTip": QtCore.QT_TRANSLATE_NOOP(
# "Part_ConnectFeature", "Fuses objects, taking care to preserve voids."
# ),
# }

# def Activated(self):
# if len(FreeCADGui.Selection.getSelectionEx()) == 2:
# CreateJoinFeature(name="Connect", mode="Connect")
# else:
# mb = QtGui.QMessageBox()
# mb.setIcon(mb.Icon.Warning)
# mb.setText(
# translate(
# "Part_JoinFeatures", "Two solids need to be selected, first!", None
# )
# )
# mb.setWindowTitle(translate("Part_JoinFeatures", "Bad selection", None))
# mb.exec_()

# def IsActive(self):
# if FreeCAD.ActiveDocument:
# return True
# else:
# return False


# FreeCADGui.addCommand("Part_JoinConnect", _CommandConnectFeature())


# class _CommandEmbedFeature:
# "Command to create PartJoinFeature in Embed mode"

# def GetResources(self):
# return {
# "Pixmap": getIconPath("Part_JoinEmbed.svg"),
# "MenuText": QtCore.QT_TRANSLATE_NOOP("Part_EmbedFeature", "Embed object"),
# "Accel": "",
# "ToolTip": QtCore.QT_TRANSLATE_NOOP(
# "Part_EmbedFeature",
# "Fuses one object into another, taking care to preserve voids.",
# ),
# }

# def Activated(self):
# if len(FreeCADGui.Selection.getSelection()) == 2:
# CreateJoinFeature(name="Embed", mode="Embed")
# else:
# mb = QtGui.QMessageBox()
# mb.setIcon(mb.Icon.Warning)
# mb.setText(
# translate(
# "Part_JoinFeatures",
# "Select base object, then the object to embed, and invoke this tool.",
# None,
# )
# )
# mb.setWindowTitle(translate("Part_JoinFeatures", "Bad selection", None))
# mb.exec_()

# def IsActive(self):
# if FreeCAD.ActiveDocument:
# return True
# else:
# return False


# FreeCADGui.addCommand("Part_JoinEmbed", _CommandEmbedFeature())


# class _CommandCutoutFeature:
# "Command to create PartJoinFeature in Cutout mode"

# def GetResources(self):
# return {
# "Pixmap": getIconPath("Part_JoinCutout.svg"),
# "MenuText": QtCore.QT_TRANSLATE_NOOP(
# "Part_CutoutFeature", "Cutout for object"
# ),
# "Accel": "",
# "ToolTip": QtCore.QT_TRANSLATE_NOOP(
# "Part_CutoutFeature",
# "Makes a cutout in one object to fit another object.",
# ),
# }

# def Activated(self):
# if len(FreeCADGui.Selection.getSelection()) == 2:
# CreateJoinFeature(name="Cutout", mode="Cutout")
# else:
# mb = QtGui.QMessageBox()
# mb.setIcon(mb.Icon.Warning)
# mb.setText(
# translate(
# "Part_JoinFeatures",
# "Select the object to make a cutout in, then the object that should fit into the cutout, and invoke this tool.",
# None,
# )
# )
# mb.setWindowTitle(translate("Part_JoinFeatures", "Bad selection", None))
# mb.exec_()

# def IsActive(self):
# if FreeCAD.ActiveDocument:
# return True
# else:
# return False


# FreeCADGui.addCommand("Part_JoinCutout", _CommandCutoutFeature())
