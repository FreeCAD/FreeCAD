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

"""The BIM MoveView command"""

import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP


class BIM_MoveView:

    def GetResources(self):
        return {
            "Pixmap": "BIM_MoveView",
            "MenuText": QT_TRANSLATE_NOOP("BIM_MoveView", "Move view..."),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_MoveView", "Moves this view to an existing page"
            ),
        }

    def Activated(self):
        self.selection = FreeCADGui.Selection.getSelection()
        self.pages = FreeCAD.ActiveDocument.findObjects(Type="TechDraw::DrawPage")
        self.labels = [obj.Label for obj in self.pages]
        FreeCADGui.draftToolBar.sourceCmd = self
        FreeCADGui.draftToolBar.popupMenu(self.labels)

    def proceed(self, labelname):
        FreeCADGui.draftToolBar.sourceCmd = None
        if labelname in self.labels:
            page = self.pages[self.labels.index(labelname)]
            for view in self.selection:
                for p in view.InList:
                    if hasattr(p, "Views") and view in p.Views:
                        p.removeView(view)
                x = view.X
                y = view.Y
                page.addView(view)
                FreeCAD.ActiveDocument.recompute()
                view.X = x
                view.Y = y


FreeCADGui.addCommand("BIM_MoveView", BIM_MoveView())
