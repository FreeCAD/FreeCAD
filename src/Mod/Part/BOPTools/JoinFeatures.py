#/***************************************************************************
# *   Copyright (c) 2016 Victor Titov (DeepSOIC) <vv.titov@gmail.com>       *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This library is free software; you can redistribute it and/or         *
# *   modify it under the terms of the GNU Library General Public           *
# *   License as published by the Free Software Foundation; either          *
# *   version 2 of the License, or (at your option) any later version.      *
# *                                                                         *
# *   This library  is distributed in the hope that it will be useful,      *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this library; see the file COPYING.LIB. If not,    *
# *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
# *   Suite 330, Boston, MA  02111-1307, USA                                *
# *                                                                         *
# ***************************************************************************/

__title__ = "BOPTools.JoinFeatures module"
__author__ = "DeepSOIC"
__url__ = "http://www.freecad.org"
__doc__ = "Implementation of document objects (features) for connect, ebmed and cutout operations."

from . import JoinAPI
import FreeCAD
import Part

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui
# -------------------------- common stuff -------------------------------------

# -------------------------- translation-related code -------------------------
# Thanks, yorik! (see forum thread "A new Part tool is being born... JoinFeatures!"
# http://forum.freecad.org/viewtopic.php?f=22&t=11112&start=30#p90239 )

    try:
        _fromUtf8 = QtCore.QString.fromUtf8
    except Exception:
        def _fromUtf8(s):
            return s
    translate = FreeCAD.Qt.translate
# --------------------------/translation-related code -------------------------


def getParamRefine():
    return FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Part/Boolean").GetBool("RefineModel")


def cmdCreateJoinFeature(name, mode):
    """cmdCreateJoinFeature(name, mode): generalized implementation of GUI commands."""
    sel = FreeCADGui.Selection.getSelectionEx()

    FreeCAD.ActiveDocument.openTransaction("Create "+mode)
    FreeCADGui.addModule("BOPTools.JoinFeatures")
    FreeCADGui.doCommand("j = BOPTools.JoinFeatures.make{mode}(name='{name}')".format(mode=mode, name=name))
    if mode == "Embed" or mode == "Cutout":
        FreeCADGui.doCommand("j.Base = App.ActiveDocument."+sel[0].Object.Name)
        FreeCADGui.doCommand("j.Tool = App.ActiveDocument."+sel[1].Object.Name)
    elif mode == "Connect":
        FreeCADGui.doCommand("j.Objects = {sel}".format(
           sel= "["  +  ", ".join(["App.ActiveDocument."+so.Object.Name for so in sel])  +  "]"
           ))
    else:
        raise ValueError("cmdCreateJoinFeature: Unexpected mode {mode}".format(mode=repr(mode)))

    try:
        FreeCADGui.doCommand("j.Proxy.execute(j)")
        FreeCADGui.doCommand("j.purgeTouched()")
    except Exception as err:
        mb = QtGui.QMessageBox()
        mb.setIcon(mb.Icon.Warning)
        error_text1 = translate("Part_JoinFeatures", "Computing the result failed with an error:")
        error_text2 = translate("Part_JoinFeatures", "Click 'Continue' to create the feature anyway, or 'Abort' to cancel.")
        mb.setText(error_text1 + "\n\n" + str(err) + "\n\n" + error_text2)
        mb.setWindowTitle(translate("Part_JoinFeatures","Bad selection", None))
        btnAbort = mb.addButton(QtGui.QMessageBox.StandardButton.Abort)
        btnOK = mb.addButton(translate("Part_JoinFeatures","Continue",None),
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

def makeConnect(name):
    '''makeConnect(name): makes an Connect object.'''
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
    FeatureConnect(obj)
    if FreeCAD.GuiUp:
        ViewProviderConnect(obj.ViewObject)
    return obj


class FeatureConnect:
    """The PartJoinFeature object."""

    def __init__(self,obj):
        obj.addProperty("App::PropertyLinkList","Objects","Connect","Object to be connected.")
        obj.addProperty("App::PropertyBool","Refine","Connect",
                        "True = refine resulting shape. False = output as is.")
        obj.Refine = getParamRefine()
        obj.addProperty("App::PropertyLength","Tolerance","Connect",
                        "Tolerance when intersecting (fuzzy value). "
                        "In addition to tolerances of the shapes.")

        obj.Proxy = self
        self.Type = "FeatureConnect"

    def execute(self,selfobj):
        rst = JoinAPI.connect([obj.Shape for obj in selfobj.Objects], selfobj.Tolerance)
        if selfobj.Refine:
            rst = rst.removeSplitter()
        selfobj.Shape = rst


class ViewProviderConnect:
    """A View Provider for the Part Connect feature."""

    def __init__(self,vobj):
        vobj.Proxy = self

    def getIcon(self):
        return getIconPath("Part_JoinConnect.svg")

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


class CommandConnect:
    """Command to create Connect feature."""

    def GetResources(self):
        return {'Pixmap': getIconPath("Part_JoinConnect.svg"),
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Part_JoinConnect","Connect objects"),
                'Accel': "",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Part_JoinConnect",
                                                    "Fuses objects, taking care to preserve voids.")}

    def Activated(self):
        if len(FreeCADGui.Selection.getSelectionEx()) >= 1:
            cmdCreateJoinFeature(name="Connect", mode="Connect")
        else:
            mb = QtGui.QMessageBox()
            mb.setIcon(mb.Icon.Warning)
            mb.setText(translate("Part_JoinFeatures",
                                  "Select at least two objects, or one or more compounds", None))
            mb.setWindowTitle(translate("Part_JoinFeatures","Bad selection", None))
            mb.exec_()

    def IsActive(self):
        if FreeCAD.ActiveDocument:
            return True
        else:
            return False

# -------------------------- /Connect -----------------------------------------


# -------------------------- Embed --------------------------------------------


def makeEmbed(name):
    '''makeEmbed(name): makes an Embed object.'''
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
    FeatureEmbed(obj)
    if FreeCAD.GuiUp:
        ViewProviderEmbed(obj.ViewObject)
    return obj


class FeatureEmbed:
    """The Part Embed object."""

    def __init__(self,obj):
        obj.addProperty("App::PropertyLink","Base","Embed","Object to embed into.")
        obj.addProperty("App::PropertyLink","Tool","Embed","Object to be embedded.")
        obj.addProperty("App::PropertyBool","Refine","Embed",
                        "True = refine resulting shape. False = output as is.")
        obj.Refine = getParamRefine()
        obj.addProperty("App::PropertyLength","Tolerance","Embed",
                        "Tolerance when intersecting (fuzzy value). "
                        "In addition to tolerances of the shapes.")

        obj.Proxy = self
        self.Type = "FeatureEmbed"

    def execute(self,selfobj):
        rst = JoinAPI.embed_legacy(selfobj.Base.Shape, selfobj.Tool.Shape, selfobj.Tolerance)
        if selfobj.Refine:
            rst = rst.removeSplitter()
        selfobj.Shape = rst


class ViewProviderEmbed:
    """A View Provider for the Part Embed feature."""

    def __init__(self,vobj):
        vobj.Proxy = self

    def getIcon(self):
        return getIconPath("Part_JoinEmbed.svg")

    def attach(self, vobj):
        self.ViewObject = vobj
        self.Object = vobj.Object

    def dumps(self):
        return None

    def loads(self,state):
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


class CommandEmbed:
    """Command to create Part Embed feature."""

    def GetResources(self):
        return {'Pixmap': getIconPath("Part_JoinEmbed.svg"),
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Part_JoinEmbed","Embed object"),
                'Accel': "",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Part_JoinEmbed",
                                                    "Fuses one object into another, taking care to preserve voids.")}

    def Activated(self):
        if len(FreeCADGui.Selection.getSelectionEx()) == 2:
            cmdCreateJoinFeature(name = "Embed", mode = "Embed")
        else:
            mb = QtGui.QMessageBox()
            mb.setIcon(mb.Icon.Warning)
            mb.setText(translate("Part_JoinFeatures",
                                 "Select base object, then the object to embed, and then invoke this tool.", None))
            mb.setWindowTitle(translate("Part_JoinFeatures","Bad selection", None))
            mb.exec_()

    def IsActive(self):
        if FreeCAD.ActiveDocument:
            return True
        else:
            return False

# -------------------------- /Embed -------------------------------------------


# -------------------------- Cutout -------------------------------------------

def makeCutout(name):
    '''makeCutout(name): makes an Cutout object.'''
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
    FeatureCutout(obj)
    if FreeCAD.GuiUp:
        ViewProviderCutout(obj.ViewObject)
    return obj


class FeatureCutout:
    """The Part Cutout object."""

    def __init__(self,obj):
        obj.addProperty("App::PropertyLink","Base","Cutout","Object to be cut.")
        obj.addProperty("App::PropertyLink","Tool","Cutout","Object to make cutout for.")
        obj.addProperty("App::PropertyBool","Refine","Cutout",
                        "True = refine resulting shape. False = output as is.")
        obj.Refine = getParamRefine()
        obj.addProperty("App::PropertyLength","Tolerance","Cutout",
                        "Tolerance when intersecting (fuzzy value). In addition to tolerances of the shapes.")

        obj.Proxy = self
        self.Type = "FeatureCutout"

    def execute(self,selfobj):
        rst = JoinAPI.cutout_legacy(selfobj.Base.Shape, selfobj.Tool.Shape, selfobj.Tolerance)
        if selfobj.Refine:
            rst = rst.removeSplitter()
        selfobj.Shape = rst


class ViewProviderCutout:
    """A View Provider for the Part Cutout feature."""

    def __init__(self,vobj):
        vobj.Proxy = self

    def getIcon(self):
        return getIconPath("Part_JoinCutout.svg")

    def attach(self, vobj):
        self.ViewObject = vobj
        self.Object = vobj.Object

    def dumps(self):
        return None

    def loads(self,state):
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


class CommandCutout:
    """Command to create PartJoinFeature in Cutout mode."""

    def GetResources(self):
        return {'Pixmap': getIconPath("Part_JoinCutout.svg"),
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Part_JoinCutout","Cutout for object"),
                'Accel': "",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Part_JoinCutout",
                                                    "Makes a cutout in one object to fit another object.")}

    def Activated(self):
        if len(FreeCADGui.Selection.getSelectionEx()) == 2:
            cmdCreateJoinFeature(name="Cutout", mode="Cutout")
        else:
            mb = QtGui.QMessageBox()
            mb.setIcon(mb.Icon.Warning)
            mb.setText(translate("Part_JoinFeatures",
                                  "Select the object to make a cutout in, then the object that should fit into the cutout, and then invoke this tool.", None))
            mb.setWindowTitle(translate("Part_JoinFeatures","Bad selection", None))
            mb.exec_()

    def IsActive(self):
        if FreeCAD.ActiveDocument:
            return True
        else:
            return False

# -------------------------- /Cutout ------------------------------------------

def addCommands():
    FreeCADGui.addCommand('Part_JoinCutout', CommandCutout())
    FreeCADGui.addCommand('Part_JoinEmbed', CommandEmbed())
    FreeCADGui.addCommand('Part_JoinConnect', CommandConnect())
