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

"""The BIM TDPage command"""


import FreeCAD
import FreeCADGui
import os

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
        from PySide import QtCore, QtGui
        import TechDraw

        templatedir = FreeCAD.ParamGet(
            "User parameter:BaseApp/Preferences/Mod/BIM"
        ).GetString("TDTemplateDir", "")
        if not templatedir:
            templatedir = None
        filename = QtGui.QFileDialog.getOpenFileName(
            QtGui.QApplication.activeWindow(),
            translate("BIM", "Select page template"),
            templatedir,
            "SVG file (*.svg)",
        )
        if filename:
            filename = filename[0]
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
            FreeCAD.ActiveDocument.recompute()


FreeCADGui.addCommand("BIM_TDPage", BIM_TDPage())
