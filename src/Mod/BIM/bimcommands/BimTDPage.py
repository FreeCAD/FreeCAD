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

"""The BIM TDPage command"""

import os

import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate


class BIM_TDPage:
    def GetResources(self):
        return {
            "Pixmap": "BIM_PageDefault",
            "MenuText": QT_TRANSLATE_NOOP("BIM_TDPage", "Page"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_TDPage", "Creates a new TechDraw page from a template"
            ),
            'Accel': "T, P",
        }

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):
        from PySide import QtGui
        import TechDraw

        templatedir = FreeCAD.ParamGet(
            "User parameter:BaseApp/Preferences/Mod/BIM"
        ).GetString("TDTemplateDir", "")
        if not templatedir:
            templatedir = None
        filename, _ = QtGui.QFileDialog.getOpenFileName(
            QtGui.QApplication.activeWindow(),
            translate("BIM", "Select page template"),
            templatedir,
            "SVG file (*.svg)",
        )
        if filename:
            name = os.path.splitext(os.path.basename(filename))[0]
            FreeCAD.ActiveDocument.openTransaction("Create page")
            page = FreeCAD.ActiveDocument.addObject("TechDraw::DrawPage", "Page")
            page.Label = name
            template = FreeCAD.ActiveDocument.addObject(
                "TechDraw::DrawSVGTemplate", "Template"
            )
            template.Template = filename
            template.Label = translate("BIM", "Template")
            page.Template = template
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM").SetString(
                "TDTemplateDir", filename.replace("\\", "/")
            )
            for txt in ["scale", "Scale", "SCALE", "scaling", "Scaling", "SCALING"]:
                if txt in page.Template.EditableTexts:
                    val = page.Template.EditableTexts[txt]
                    if val:
                        if ":" in val:
                            val.replace(":", "/")
                        if "/" in val:
                            try:
                                page.Scale = eval(val)
                            except:
                                pass
                            else:
                                break
                        else:
                            try:
                                page.Scale = float(val)
                            except:
                                pass
                            else:
                                break
            else:
                page.Scale = FreeCAD.ParamGet(
                    "User parameter:BaseApp/Preferences/Mod/BIM"
                ).GetFloat("DefaultPageScale", 0.01)
            page.ViewObject.show()
            FreeCAD.ActiveDocument.recompute()


FreeCADGui.addCommand("BIM_TDPage", BIM_TDPage())
