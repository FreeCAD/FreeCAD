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

"""This module contains BIM wrappers for commands from other workbenches"""

import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP


class BIM_Text:
    def GetResources(self):
        return {
            "Pixmap": "Draft_Text",
            "MenuText": QT_TRANSLATE_NOOP("BIM_Text", "Text"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_Text", "Create a text in the current 3D view or TechDraw page"
            ),
            "Accel": "T, E",
        }

    def IsActive(self):
        act_win = FreeCADGui.getMainWindow().getActiveWindow()
        return hasattr(act_win, "getSceneGraph") or hasattr(act_win, "getPage")

    def Activated(self):
        import draftutils.utils as utils

        self.view = FreeCADGui.ActiveDocument.ActiveView
        if hasattr(self.view, "getPage") and self.view.getPage():
            self.text = ""
            FreeCADGui.draftToolBar.sourceCmd = self
            FreeCADGui.draftToolBar.taskUi()
            FreeCADGui.draftToolBar.textUi()
        else:
            FreeCADGui.runCommand("Draft_Text")

    def createObject(self):
        import TechDraw

        if self.text:
            page = self.view.getPage()
            pagescale = page.Scale
            if not pagescale:
                pagescale = 1
            anno = FreeCAD.ActiveDocument.addObject(
                "TechDraw::DrawViewAnnotation", "Annotation"
            )
            anno.Text = self.text
            page.addView(anno)
            param = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
            anno.TextSize = param.GetFloat("textheight", 10) * pagescale
            anno.Font = param.GetString("textfont", "Sans")
            c = param.GetUnsigned("DefaultTextColor", 255)
            r = ((c >> 24) & 0xFF) / 255.0
            g = ((c >> 16) & 0xFF) / 255.0
            b = ((c >> 8) & 0xFF) / 255.0
            anno.TextColor = (r, g, b)
            self.finish()

    def finish(self, arg=False):
        FreeCADGui.draftToolBar.sourceCmd = None
        FreeCADGui.draftToolBar.offUi()


FreeCADGui.addCommand("BIM_Text", BIM_Text())
