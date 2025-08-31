# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 Yorik van Havre <yorik@uncreated.net>              *
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
            "MenuText": QT_TRANSLATE_NOOP("BIM_MoveView", "Move View"),
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
