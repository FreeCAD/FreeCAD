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

__title__ = "CompoundTools._CommandExplodeCompound"
__author__ = "DeepSOIC"
__url__ = "http://www.freecad.org"
__doc__ = "ExplodeCompound: create a bunch of CompoundFilter objects to split a compound into pieces."

from .Explode import explodeCompound

import FreeCAD
if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtGui
    from PySide import QtCore


# translation-related code
    try:
        _fromUtf8 = QtCore.QString.fromUtf8
    except Exception:
        def _fromUtf8(s):
            return s
    translate = FreeCAD.Qt.translate


# command class
class _CommandExplodeCompound:
    "Command to explode a compound"
    def GetResources(self):
        return {'Pixmap': "Part_ExplodeCompound",
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Part_ExplodeCompound", "Explode compound"),
                'Accel': "",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Part_ExplodeCompound",
                                                    "Split up a compound of shapes into separate objects.\n"
                                                    "It will create a 'Compound Filter' for each shape.")}

    def Activated(self):
        if len(FreeCADGui.Selection.getSelection()) == 1:
            cmdExplode()
        else:
            mb = QtGui.QMessageBox()
            mb.setIcon(mb.Icon.Warning)
            mb.setText(translate("Part_ExplodeCompound", "First select a shape that is a compound.", None))
            mb.setWindowTitle(translate("Part_ExplodeCompound", "Bad selection", None))
            mb.exec_()

    def IsActive(self):
        if FreeCAD.ActiveDocument:
            return True
        else:
            return False


if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Part_ExplodeCompound', _CommandExplodeCompound())




def cmdExplode():
    FreeCAD.ActiveDocument.openTransaction("Explode")
    try:
        sel = FreeCADGui.Selection.getSelectionEx()
        if len(sel) != 1:
            raise ValueError("Bad selection","More than one object selected. You have selected {num} objects.".format(num= len(sel)))
        obj = sel[0].Object
        FreeCADGui.addModule("CompoundTools.Explode")
        FreeCADGui.doCommand("input_obj = App.ActiveDocument."+obj.Name)
        FreeCADGui.doCommand("CompoundTools.Explode.explodeCompound(input_obj)")
        FreeCADGui.doCommand("input_obj.ViewObject.hide()")
    except Exception as ex:
        FreeCAD.ActiveDocument.abortTransaction()
        FreeCAD.Console.PrintError("{}\n".format(ex))

    FreeCAD.ActiveDocument.commitTransaction()
    FreeCADGui.doCommand("App.ActiveDocument.recompute()")

