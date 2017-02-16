# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 - Victor Titov (DeepSOIC) <vv.titov@gmail.com>     *
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

__title__ = "CompoundTools.CompoundFilter"
__author__ = "DeepSOIC, Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"
__doc__ = "Compound Filter: remove some children from a compound (features)."


import FreeCAD
import Part
import math
if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtGui


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


# OCC's Precision::Confusion; should have taken this from FreeCAD but haven't found; unlikely to ever change (DeepSOIC)
DistConfusion = 1e-7
ParaConfusion = 1e-8


# -------------------------- common stuff --------------------------------------------------
def makeCompoundFilter(name):
    '''makeCompoundFilter(name): makes a CompoundFilter object.'''
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython", name)
    _CompoundFilter(obj)
    _ViewProviderCompoundFilter(obj.ViewObject)
    return obj


class _CompoundFilter:
    "The CompoundFilter object"
    def __init__(self, obj):
        obj.addProperty("App::PropertyLink", "Base", "CompoundFilter", "Compound to be filtered")

        obj.addProperty("App::PropertyEnumeration", "FilterType", "CompoundFilter", "")
        obj.FilterType = ['bypass', 'specific items', 'collision-pass', 'window-volume', 'window-area', 'window-length', 'window-distance']
        obj.FilterType = 'bypass'

        # properties controlling "specific items" mode
        obj.addProperty("App::PropertyString", "items", "CompoundFilter", "list of indexes of childs to be returned (like this: 1,4,8:10).")

        obj.addProperty("App::PropertyLink", "Stencil", "CompoundFilter", "Object that defines filtering")

        obj.addProperty("App::PropertyFloat", "WindowFrom", "CompoundFilter", "Value of threshold, expressed as a percentage of maximum value.")
        obj.WindowFrom = 80.0
        obj.addProperty("App::PropertyFloat", "WindowTo", "CompoundFilter", "Value of threshold, expressed as a percentage of maximum value.")
        obj.WindowTo = 100.0

        obj.addProperty("App::PropertyFloat", "OverrideMaxVal", "CompoundFilter", "Volume threshold, expressed as percentage of the volume of largest child")
        obj.OverrideMaxVal = 0

        obj.addProperty("App::PropertyBool", "Invert", "CompoundFilter", "Output shapes that are rejected by filter, instead")
        obj.Invert = False

        self.Type = "CompoundFilter"
        obj.Proxy = self

    def execute(self, obj):
        # When operating on the object, it is to be treated as a lattice object. If False, treat as a regular shape.'''
        if hasattr(obj, "isLattice"):
            if 'On' in obj.isLattice:
                print(obj.Name + " A generic shape is expected, but an array of placements was supplied. It will be treated as a generic shape.\n")

        rst = []  # variable to receive the final list of shapes
        shps = obj.Base.Shape.childShapes()
        if obj.FilterType == 'bypass':
            rst = shps
        elif obj.FilterType == 'specific items':
            rst = []
            flags = [False] * len(shps)
            ranges = obj.items.split(';')
            for r in ranges:
                r_v = r.split(':')
                if len(r_v) == 1:
                    i = int(r_v[0])
                    rst.append(shps[i])
                    flags[i] = True
                elif len(r_v) == 2 or len(r_v) == 3:
                    if len(r_v) == 2:
                        r_v.append("")  # fix issue #1: instead of checking length here and there, simply add the missing field =) (DeepSOIC)
                    ifrom = None if len(r_v[0].strip()) == 0 else int(r_v[0])
                    ito = None if len(r_v[1].strip()) == 0 else int(r_v[1])
                    istep = None if len(r_v[2].strip()) == 0 else int(r_v[2])
                    rst = rst + shps[ifrom:ito:istep]
                    for b in flags[ifrom:ito:istep]:
                        b = True
                else:
                    raise ValueError('index range cannot be parsed:' + r)
            if obj.Invert:
                rst = []
                for i in xrange(0, len(shps)):
                    if not flags[i]:
                        rst.append(shps[i])
        elif obj.FilterType == 'collision-pass':
            stencil = obj.Stencil.Shape
            for s in shps:
                d = s.distToShape(stencil)
                if bool(d[0] < DistConfusion) ^ bool(obj.Invert):
                    rst.append(s)
        elif obj.FilterType == 'window-volume' or obj.FilterType == 'window-area' or obj.FilterType == 'window-length' or obj.FilterType == 'window-distance':
            vals = [0.0] * len(shps)
            for i in xrange(0, len(shps)):
                if obj.FilterType == 'window-volume':
                    vals[i] = shps[i].Volume
                elif obj.FilterType == 'window-area':
                    vals[i] = shps[i].Area
                elif obj.FilterType == 'window-length':
                    vals[i] = shps[i].Length
                elif obj.FilterType == 'window-distance':
                    vals[i] = shps[i].distToShape(obj.Stencil.Shape)[0]

            maxval = max(vals)
            if obj.Stencil:
                if obj.FilterType == 'window-volume':
                    maxval = obj.Stencil.Shape.Volume
                elif obj.FilterType == 'window-area':
                    maxval = obj.Stencil.Shape.Area
                elif obj.FilterType == 'window-length':
                    maxval = obj.Stencil.Shape.Length
            if obj.OverrideMaxVal:
                maxval = obj.OverrideMaxVal

            valFrom = obj.WindowFrom / 100.0 * maxval
            valTo = obj.WindowTo / 100.0 * maxval

            for i in xrange(0, len(shps)):
                if bool(vals[i] >= valFrom and vals[i] <= valTo) ^ obj.Invert:
                    rst.append(shps[i])
        else:
            raise ValueError('Filter mode not implemented:' + obj.FilterType)

        if len(rst) == 0:
            scale = 1.0
            if not obj.Base.Shape.isNull():
                scale = obj.Base.Shape.BoundBox.DiagonalLength / math.sqrt(3) / math.sqrt(len(shps))
            if scale < DistConfusion * 100:
                scale = 1.0
            print scale
            obj.Shape = getNullShapeShape(scale)
            raise ValueError('Nothing passes through the filter')  # Feeding empty compounds to FreeCAD seems to cause rendering issues, otherwise it would have been a good idea to output nothing.

        if len(rst) > 1:
            obj.Shape = Part.makeCompound(rst)
        else:  # don't make compound of one shape, output it directly
            sh = rst[0]
            sh.transformShape(sh.Placement.toMatrix(), True)  # True = make copy
            sh.Placement = FreeCAD.Placement()
            obj.Shape = sh

        return


class _ViewProviderCompoundFilter:
    "A View Provider for the CompoundFilter object"

    def __init__(self, vobj):
        vobj.Proxy = self
        vobj.addProperty("App::PropertyBool", "DontUnhideOnDelete", "CompoundFilter", "When this object is deleted, Base and Stencil are unhidden. This flag stops it from happening.")
        vobj.setEditorMode("DontUnhideOnDelete", 2)  # set hidden

    def getIcon(self):
        return ":/icons/Part_CompoundFilter.svg"

    def attach(self, vobj):
        self.ViewObject = vobj
        self.Object = vobj.Object

    def setEdit(self, vobj, mode):
        return False

    def unsetEdit(self, vobj, mode):
        return

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def claimChildren(self):
        children = [self.Object.Base]
        if self.Object.Stencil:
            children.append(self.Object.Stencil)
        return children

    def onDelete(self, feature, subelements):  # subelements is a tuple of strings
        if not self.ViewObject.DontUnhideOnDelete:
            try:
                self.Object.Base.ViewObject.show()
                if self.Object.Stencil:
                    self.Object.Stencil.ViewObject.show()
            except Exception as err:
                FreeCAD.Console.PrintError("Error in onDelete: " + err.message)
        return True
# -------------------------- /common stuff --------------------------------------------------


# -------------------------- GUI command  --------------------------------------------------
def cmdCreateCompoundFilter(name):
    sel = FreeCADGui.Selection.getSelection()
    FreeCAD.ActiveDocument.openTransaction("Create CompoundFilter")
    FreeCADGui.addModule("CompoundTools.CompoundFilter")
    FreeCADGui.doCommand("f = CompoundTools.CompoundFilter.makeCompoundFilter(name = '" + name + "')")
    FreeCADGui.doCommand("f.Base = App.ActiveDocument." + sel[0].Name)
    if len(sel) == 2:
        FreeCADGui.doCommand("f.Stencil = App.ActiveDocument." + sel[1].Name)
        FreeCADGui.doCommand("f.Stencil.ViewObject.hide()")
        FreeCADGui.doCommand("f.FilterType = 'collision-pass'")
    else:
        FreeCADGui.doCommand("f.FilterType = 'window-volume'")

    try:
        FreeCADGui.doCommand("f.Proxy.execute(f)")
        FreeCADGui.doCommand("f.purgeTouched()")
    except Exception as err:
        mb = QtGui.QMessageBox()
        mb.setIcon(mb.Icon.Warning)
        mb.setText(_translate("Part_CompoundFilter", "Computing the result failed with an error: \n\n{err}\n\nClick 'Continue' to create the feature anyway, or 'Abort' to cancel.", None)
                   .format(err=err.message))
        mb.setWindowTitle(_translate("Part_CompoundFilter", "Bad selection", None))
        btnAbort = mb.addButton(QtGui.QMessageBox.StandardButton.Abort)
        btnOK = mb.addButton(_translate("Part_SplitFeatures", "Continue", None), QtGui.QMessageBox.ButtonRole.ActionRole)
        mb.setDefaultButton(btnOK)

        mb.exec_()

        if mb.clickedButton() is btnAbort:
            FreeCAD.ActiveDocument.abortTransaction()
            return

    FreeCADGui.doCommand("f.Base.ViewObject.hide()")
    FreeCADGui.doCommand("f = None")

    FreeCAD.ActiveDocument.commitTransaction()
# -------------------------- /GUI command  --------------------------------------------------


# helper
def getNullShapeShape(scale=1.0):
    """obtains a shape intended ad a placeholder in case null shape was produced by an operation"""

    # read shape from file, if not done this before
    global _nullShapeShape
    if not _nullShapeShape:
        _nullShapeShape = Part.Shape()
        import os
        shapePath = os.path.dirname(__file__) + os.path.sep + "shapes" + os.path.sep + "empty-shape.brep"
        f = open(shapePath)
        _nullShapeShape.importBrep(f)
        f.close()

    # scale the shape
    ret = _nullShapeShape
    if scale != 1.0:
        ret = _nullShapeShape.copy()
        ret.scale(scale)
