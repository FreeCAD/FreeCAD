# -*- coding: utf8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 Yorik van Havre <yorik@uncreated.net>              *
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

"""The BIM Convert command"""


import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP


class BIM_Convert:

    def GetResources(self):
        return {
            "Pixmap": "Arch_Component",
            "MenuText": QT_TRANSLATE_NOOP("BIM_Convert", "Convert to BIM"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_Convert", "Converts any object to a BIM component"
            ),
        }

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):
        sel = FreeCADGui.Selection.getSelection()
        if sel:
            FreeCADGui.Control.showDialog(BIM_Convert_TaskPanel(sel))


class BIM_Convert_TaskPanel:

    def __init__(self, objs):
        from PySide import QtGui
        self.types = [
            "Wall",
            "Structure",
            "Rebar",
            "Window",
            "Stairs",
            "Roof",
            "Panel",
            "Frame",
            "Space",
            "Equipment",
            "Component",
        ]
        self.objs = objs
        self.form = QtGui.QListWidget()
        for t in self.types:
            ti = t + "_Tree"
            tx = t
            if t == "Component":
                ti = t
                tx = "Generic component"
            i = QtGui.QListWidgetItem(QtGui.QIcon(":/icons/Arch_" + ti + ".svg"), tx)
            i.setToolTip(t)
            self.form.addItem(i)
        self.form.itemDoubleClicked.connect(self.accept)

    def accept(self, idx=None):
        i = self.form.currentItem()
        if i:
            import Arch

            FreeCAD.ActiveDocument.openTransaction("Convert to BIM")
            for o in self.objs:
                getattr(Arch, "make" + i.toolTip())(o)
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()
        if idx:
            from draftutils import todo

            todo.ToDo.delay(FreeCADGui.Control.closeDialog, None)
        return True


FreeCADGui.addCommand("BIM_Convert", BIM_Convert())
