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

"""The BIM TD View command"""

import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate


class BIM_TDView:
    def GetResources(self):
        return {
            "Pixmap": "BIM_InsertView",
            "MenuText": QT_TRANSLATE_NOOP("BIM_TDView", "New View"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_TDView",
                "Inserts a drawing view on a page.\n"
                "To choose where to insert the view when multiple pages are available,\n"
                "select both the view and the page before executing the command.",
            ),
            'Accel': "V, I",
        }

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):
        import Draft
        sections = []
        page = None
        drafts = []
        for obj in FreeCADGui.Selection.getSelection():
            t = Draft.getType(obj)
            if t == "SectionPlane":
                sections.append(obj)
            elif t == "TechDraw::DrawPage":
                page = obj
            else:
                drafts.append(obj)
        if not page:
            pages = FreeCAD.ActiveDocument.findObjects(Type="TechDraw::DrawPage")
            if pages:
                page = pages[0]
        if (not page) or ((not sections) and (not drafts)):
            FreeCAD.Console.PrintError(
                translate(
                    "BIM",
                    "No section view, Draft object, or page found or selected in the document",
                )
                + "\n"
            )
            return
        FreeCAD.ActiveDocument.openTransaction("Create view")
        for section in sections:
            view = FreeCAD.ActiveDocument.addObject(
                "TechDraw::DrawViewArch", "BIM view"
            )
            view.Label = section.Label
            view.Source = section
            page.addView(view)
            if page.Scale:
                view.Scale = page.Scale
        for draft in drafts:
            view = FreeCAD.ActiveDocument.addObject(
                "TechDraw::DrawViewDraft", "DraftView"
            )
            view.Label = draft.Label
            view.Source = draft
            page.addView(view)
            if page.Scale:
                view.Scale = page.Scale
            if "ShapeMode" in draft.PropertiesList:
                draft.ShapeMode = "Shape"
            for child in draft.OutListRecursive:
                if "ShapeMode" in child.PropertiesList:
                    child.ShapeMode = "Shape"
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


FreeCADGui.addCommand("BIM_TDView", BIM_TDView())
