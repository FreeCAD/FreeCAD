#/***************************************************************************
# *   Copyright (c) Victor Titov (DeepSOIC)                                 *
# *                                           (vv.titov@gmail.com) 2016     *
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

__title__="BOPTools.SplitFeatures module"
__author__ = "DeepSOIC"
__url__ = "http://www.freecadweb.org"
__doc__ = "Shape splitting document objects (features)."

from . import SplitAPI
import FreeCAD
import Part

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui

#-------------------------- translation-related code ----------------------------------------
#(see forum thread "A new Part tool is being born... JoinFeatures!"
#http://forum.freecadweb.org/viewtopic.php?f=22&t=11112&start=30#p90239 )
    try:
        _fromUtf8 = QtCore.QString.fromUtf8
    except Exception:
        def _fromUtf8(s):
            return s
    try:
        _encoding = QtGui.QApplication.UnicodeUTF8
        def _translate(context, text, disambig):
            return QtGui.QApplication.translate(context, text, disambig, _encoding)
    except AttributeError:
        def _translate(context, text, disambig):
            return QtGui.QApplication.translate(context, text, disambig)
#--------------------------/translation-related code ----------------------------------------

def getIconPath(icon_dot_svg):
    return ":/icons/" + icon_dot_svg

# -------------------------- /common stuff --------------------------------------------------

# -------------------------- BooleanFragments --------------------------------------------------

def makeBooleanFragments(name):
    '''makeBooleanFragments(name): makes an BooleanFragments object.'''
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
    FeatureBooleanFragments(obj)
    if FreeCAD.GuiUp:
        ViewProviderBooleanFragments(obj.ViewObject)
    return obj

class FeatureBooleanFragments:
    "The BooleanFragments feature object"
    def __init__(self,obj):
        obj.addProperty("App::PropertyLinkList","Objects","BooleanFragments","Object to compute intersections between.")
        obj.addProperty("App::PropertyEnumeration","Mode","BooleanFragments","Standard: wires, shells, compsolids remain in one piece. Split: wires, shells, compsolids are split. CompSolid: make compsolid from solid fragments.")
        obj.Mode = ["Standard", "Split", "CompSolid"]
        obj.addProperty("App::PropertyLength","Tolerance","BooleanFragments","Tolerance when intersecting (fuzzy value). In addition to tolerances of the shapes.")

        obj.Proxy = self
        self.Type = "FeatureBooleanFragments"

    def execute(self,selfobj):
        shapes = [obj.Shape for obj in selfobj.Objects]
        if len(shapes) == 1 and shapes[0].ShapeType == "Compound":
            shapes = shapes[0].childShapes()
        if len(shapes) < 2:
            raise ValueError("At least two shapes are needed for computing boolean fragments. Got only {num}.".format(num= len(shapes)))
        selfobj.Shape = SplitAPI.booleanFragments(shapes, selfobj.Mode, selfobj.Tolerance)


class ViewProviderBooleanFragments:
    "A View Provider for the Part BooleanFragments feature"

    def __init__(self,vobj):
        vobj.Proxy = self

    def getIcon(self):
        return getIconPath("Part_BooleanFragments.svg")

    def attach(self, vobj):
        self.ViewObject = vobj
        self.Object = vobj.Object

    def __getstate__(self):
        return None

    def __setstate__(self,state):
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

def cmdCreateBooleanFragmentsFeature(name, mode):
    """cmdCreateBooleanFragmentsFeature(name, mode): implementation of GUI command to create
    BooleanFragments feature (GFA). Mode can be "Standard", "Split", or "CompSolid"."""
    sel = FreeCADGui.Selection.getSelectionEx()
    FreeCAD.ActiveDocument.openTransaction("Create Boolean Fragments")
    FreeCADGui.addModule("BOPTools.SplitFeatures")
    FreeCADGui.doCommand("j = BOPTools.SplitFeatures.makeBooleanFragments(name= '{name}')".format(name= name))
    FreeCADGui.doCommand("j.Objects = {sel}".format(
       sel= "["  +  ", ".join(["App.ActiveDocument."+so.Object.Name for so in sel])  +  "]"
       ))
    FreeCADGui.doCommand("j.Mode = {mode}".format(mode= repr(mode)))

    try:
        FreeCADGui.doCommand("j.Proxy.execute(j)")
        FreeCADGui.doCommand("j.purgeTouched()")
    except Exception as err:
        mb = QtGui.QMessageBox()
        mb.setIcon(mb.Icon.Warning)
        mb.setText(_translate("Part_SplitFeatures","Computing the result failed with an error: \n\n{err}\n\nClick 'Continue' to create the feature anyway, or 'Abort' to cancel.", None)
                   .format(err= str(err)))
        mb.setWindowTitle(_translate("Part_SplitFeatures","Bad selection", None))
        btnAbort = mb.addButton(QtGui.QMessageBox.StandardButton.Abort)
        btnOK = mb.addButton(_translate("Part_SplitFeatures","Continue",None), QtGui.QMessageBox.ButtonRole.ActionRole)
        mb.setDefaultButton(btnOK)

        mb.exec_()

        if mb.clickedButton() is btnAbort:
            FreeCAD.ActiveDocument.abortTransaction()
            return

    FreeCADGui.doCommand("for obj in j.ViewObject.Proxy.claimChildren():\n"
                         "    obj.ViewObject.hide()")

    FreeCAD.ActiveDocument.commitTransaction()

class CommandBooleanFragments:
    "Command to create BooleanFragments feature"
    def GetResources(self):
        return {'Pixmap'  : getIconPath("Part_BooleanFragments.svg"),
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Part_SplitFeatures","Boolean Fragments"),
                'Accel': "",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Part_SplitFeatures","Split objects where they intersect")}

    def Activated(self):
        if len(FreeCADGui.Selection.getSelectionEx()) >= 1 :
            cmdCreateBooleanFragmentsFeature(name= "BooleanFragments", mode= "Standard")
        else:
            mb = QtGui.QMessageBox()
            mb.setIcon(mb.Icon.Warning)
            mb.setText(_translate("Part_SplitFeatures", "Select at least two objects, or one or more compounds, first! If only one compound is selected, the compounded shapes will be intersected between each other (otherwise, compounds with self-intersections are invalid).", None))
            mb.setWindowTitle(_translate("Part_SplitFeatures","Bad selection", None))
            mb.exec_()

    def IsActive(self):
        if FreeCAD.ActiveDocument:
            return True
        else:
            return False

# -------------------------- /BooleanFragments --------------------------------------------------

# -------------------------- Slice --------------------------------------------------

def makeSlice(name):
    '''makeSlice(name): makes an Slice object.'''
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
    FeatureSlice(obj)
    if FreeCAD.GuiUp:
        ViewProviderSlice(obj.ViewObject)
    return obj

class FeatureSlice:
    "The Slice feature object"
    def __init__(self,obj):
        obj.addProperty("App::PropertyLink","Base","Slice","Object to be sliced.")
        obj.addProperty("App::PropertyLinkList","Tools","Slice","Objects that slice.")
        obj.addProperty("App::PropertyEnumeration","Mode","Slice","Standard: wires, shells, compsolids remain in one piece. Split: wires, shells, compsolids are split. CompSolid: make compsolid from solid fragments.")
        obj.Mode = ["Standard", "Split", "CompSolid"]
        obj.addProperty("App::PropertyLength","Tolerance","Slice","Tolerance when intersecting (fuzzy value). In addition to tolerances of the shapes.")

        obj.Proxy = self
        self.Type = "FeatureSlice"

    def execute(self,selfobj):
        if len(selfobj.Tools) < 1:
            raise ValueError("No slicing objects supplied!")
        selfobj.Shape = SplitAPI.slice(selfobj.Base.Shape, [obj.Shape for obj in selfobj.Tools], selfobj.Mode, selfobj.Tolerance)


class ViewProviderSlice:
    "A View Provider for the Part Slice feature"

    def __init__(self,vobj):
        vobj.Proxy = self

    def getIcon(self):
        return getIconPath("Part_Slice.svg")

    def attach(self, vobj):
        self.ViewObject = vobj
        self.Object = vobj.Object

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None

    def claimChildren(self):
        return [self.Object.Base] + self.Object.Tools

    def onDelete(self, feature, subelements):
        try:
            for obj in self.claimChildren():
                obj.ViewObject.show()
        except Exception as err:
            FreeCAD.Console.PrintError("Error in onDelete: " + str(err))
        return True

def cmdCreateSliceFeature(name, mode, transaction= True):
    """cmdCreateSliceFeature(name, mode): implementation of GUI command to create
    Slice feature. Mode can be "Standard", "Split", or "CompSolid"."""
    sel = FreeCADGui.Selection.getSelectionEx()
    if transaction: FreeCAD.ActiveDocument.openTransaction("Create Slice")
    FreeCADGui.addModule("BOPTools.SplitFeatures")
    FreeCADGui.doCommand("f = BOPTools.SplitFeatures.makeSlice(name= '{name}')".format(name= name))
    FreeCADGui.doCommand("f.Base = {sel}[0]\n"
                         "f.Tools = {sel}[1:]".format(
       sel= "["  +  ", ".join(["App.ActiveDocument."+so.Object.Name for so in sel])  +  "]"
       ))
    FreeCADGui.doCommand("f.Mode = {mode}".format(mode= repr(mode)))

    try:
        FreeCADGui.doCommand("f.Proxy.execute(f)")
        FreeCADGui.doCommand("f.purgeTouched()")
    except Exception as err:
        mb = QtGui.QMessageBox()
        mb.setIcon(mb.Icon.Warning)
        mb.setText(_translate("Part_SplitFeatures","Computing the result failed with an error: \n\n{err}\n\nClick 'Continue' to create the feature anyway, or 'Abort' to cancel.", None)
                   .format(err= str(err)))
        mb.setWindowTitle(_translate("Part_SplitFeatures","Bad selection", None))
        btnAbort = mb.addButton(QtGui.QMessageBox.StandardButton.Abort)
        btnOK = mb.addButton(_translate("Part_SplitFeatures","Continue",None), QtGui.QMessageBox.ButtonRole.ActionRole)
        mb.setDefaultButton(btnOK)

        mb.exec_()

        if mb.clickedButton() is btnAbort:
            if transaction: FreeCAD.ActiveDocument.abortTransaction()
            return False

    FreeCADGui.doCommand("for obj in f.ViewObject.Proxy.claimChildren():\n"
                         "    obj.ViewObject.hide()")

    if transaction: FreeCAD.ActiveDocument.commitTransaction()
    return True

def cmdSliceApart():
    FreeCAD.ActiveDocument.openTransaction("Slice apart")
    made = cmdCreateSliceFeature(name= "Slice", mode= "Split", transaction= False)
    
    if made:
        FreeCADGui.addModule("CompoundTools.Explode")
        FreeCADGui.doCommand("CompoundTools.Explode.explodeCompound(f)")
        FreeCADGui.doCommand("f.ViewObject.hide()")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCADGui.doCommand("App.ActiveDocument.recompute()")
    else:
        FreeCAD.ActiveDocument.abortTransaction()
        

class CommandSlice:
    "Command to create Slice feature"
    def GetResources(self):
        return {'Pixmap'  : getIconPath("Part_Slice.svg"),
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Part_SplitFeatures","Slice to compound"),
                'Accel': "",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Part_SplitFeatures","Split object by intersections with other objects, and pack the pieces into a compound.")}

    def Activated(self):
        if len(FreeCADGui.Selection.getSelectionEx()) > 1 :
            cmdCreateSliceFeature(name= "Slice", mode= "Split")
        else:
            mb = QtGui.QMessageBox()
            mb.setIcon(mb.Icon.Warning)
            mb.setText(_translate("Part_SplitFeatures", "Select at least two objects, first! First one is the object to be sliced; the rest are objects to slice with.", None))
            mb.setWindowTitle(_translate("Part_SplitFeatures","Bad selection", None))
            mb.exec_()

    def IsActive(self):
        if FreeCAD.ActiveDocument:
            return True
        else:
            return False

class CommandSliceApart:
    "Command to create exploded Slice feature"
    def GetResources(self):
        return {'Pixmap'  : getIconPath("Part_SliceApart.svg"),
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Part_SplitFeatures","Slice apart"),
                'Accel': "",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Part_SplitFeatures","Split object by intersections with other objects.")}

    def Activated(self):
        if len(FreeCADGui.Selection.getSelectionEx()) > 1 :
            cmdSliceApart()
        else:
            mb = QtGui.QMessageBox()
            mb.setIcon(mb.Icon.Warning)
            mb.setText(_translate("Part_SplitFeatures", "Select at least two objects, first! First one is the object to be sliced; the rest are objects to slice with.", None))
            mb.setWindowTitle(_translate("Part_SplitFeatures","Bad selection", None))
            mb.exec_()

    def IsActive(self):
        if FreeCAD.ActiveDocument:
            return True
        else:
            return False

# -------------------------- /Slice --------------------------------------------------

# -------------------------- XOR --------------------------------------------------

def makeXOR(name):
    '''makeXOR(name): makes an XOR object.'''
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
    FeatureXOR(obj)
    if FreeCAD.GuiUp:
        ViewProviderXOR(obj.ViewObject)
    return obj

class FeatureXOR:
    "The XOR feature object"
    def __init__(self,obj):
        obj.addProperty("App::PropertyLinkList","Objects","XOR","Object to compute intersections between.")
        obj.addProperty("App::PropertyLength","Tolerance","XOR","Tolerance when intersecting (fuzzy value). In addition to tolerances of the shapes.")

        obj.Proxy = self
        self.Type = "FeatureXOR"

    def execute(self,selfobj):
        shapes = [obj.Shape for obj in selfobj.Objects]
        if len(shapes) == 1 and shapes[0].ShapeType == "Compound":
            shapes = shapes[0].childShapes()
        if len(shapes) < 2:
            raise ValueError("At least two shapes are needed for computing XOR. Got only {num}.".format(num= len(shapes)))
        selfobj.Shape = SplitAPI.xor(shapes, selfobj.Tolerance)


class ViewProviderXOR:
    "A View Provider for the Part XOR feature"

    def __init__(self,vobj):
        vobj.Proxy = self

    def getIcon(self):
        return getIconPath("Part_XOR.svg")

    def attach(self, vobj):
        self.ViewObject = vobj
        self.Object = vobj.Object

    def __getstate__(self):
        return None

    def __setstate__(self,state):
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

def cmdCreateXORFeature(name):
    """cmdCreateXORFeature(name): implementation of GUI command to create
    XOR feature (GFA). Mode can be "Standard", "Split", or "CompSolid"."""
    sel = FreeCADGui.Selection.getSelectionEx()
    FreeCAD.ActiveDocument.openTransaction("Create Boolean XOR")
    FreeCADGui.addModule("BOPTools.SplitFeatures")
    FreeCADGui.doCommand("j = BOPTools.SplitFeatures.makeXOR(name= '{name}')".format(name= name))
    FreeCADGui.doCommand("j.Objects = {sel}".format(
       sel= "["  +  ", ".join(["App.ActiveDocument."+so.Object.Name for so in sel])  +  "]"
       ))

    try:
        FreeCADGui.doCommand("j.Proxy.execute(j)")
        FreeCADGui.doCommand("j.purgeTouched()")
    except Exception as err:
        mb = QtGui.QMessageBox()
        mb.setIcon(mb.Icon.Warning)
        mb.setText(_translate("Part_SplitFeatures","Computing the result failed with an error: \n\n{err}\n\nClick 'Continue' to create the feature anyway, or 'Abort' to cancel.", None)
                   .format(err= str(err)))
        mb.setWindowTitle(_translate("Part_SplitFeatures","Bad selection", None))
        btnAbort = mb.addButton(QtGui.QMessageBox.StandardButton.Abort)
        btnOK = mb.addButton(_translate("Part_SplitFeatures","Continue",None), QtGui.QMessageBox.ButtonRole.ActionRole)
        mb.setDefaultButton(btnOK)

        mb.exec_()

        if mb.clickedButton() is btnAbort:
            FreeCAD.ActiveDocument.abortTransaction()
            return

    FreeCADGui.doCommand("for obj in j.ViewObject.Proxy.claimChildren():\n"
                         "    obj.ViewObject.hide()")

    FreeCAD.ActiveDocument.commitTransaction()

class CommandXOR:
    "Command to create XOR feature"
    def GetResources(self):
        return {'Pixmap'  : getIconPath("Part_XOR.svg"),
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Part_SplitFeatures","Boolean XOR"),
                'Accel': "",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Part_SplitFeatures","Remove intersection fragments")}

    def Activated(self):
        if len(FreeCADGui.Selection.getSelectionEx()) >= 1 :
            cmdCreateXORFeature(name= "XOR")
        else:
            mb = QtGui.QMessageBox()
            mb.setIcon(mb.Icon.Warning)
            mb.setText(_translate("Part_SplitFeatures", "Select at least two objects, or one or more compounds, first! If only one compound is selected, the compounded shapes will be intersected between each other (otherwise, compounds with self-intersections are invalid).", None))
            mb.setWindowTitle(_translate("Part_SplitFeatures","Bad selection", None))
            mb.exec_()

    def IsActive(self):
        if FreeCAD.ActiveDocument:
            return True
        else:
            return False

# -------------------------- /XOR --------------------------------------------------

def addCommands():
    FreeCADGui.addCommand('Part_BooleanFragments',CommandBooleanFragments())
    FreeCADGui.addCommand('Part_Slice',CommandSlice())
    FreeCADGui.addCommand('Part_SliceApart',CommandSliceApart())
    FreeCADGui.addCommand('Part_XOR',CommandXOR())
