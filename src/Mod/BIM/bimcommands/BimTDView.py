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

"""The BIM TD View command"""


import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate


class BIM_TDView:
    def GetResources(self):
        return {
            "Pixmap": "BIM_InsertView",
            "MenuText": QT_TRANSLATE_NOOP("BIM_TDView", "Insert view"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_TDView",
                "Inserts a drawing view on a page",
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
                    "No section view or Draft objects selected, or no page selected, or no page found in document",
                )
                + "\n"
            )
            return
        FreeCAD.ActiveDocument.openTransaction("Create view")
        for section in sections:
            view = FreeCAD.ActiveDocument.addObject(
                "TechDraw::DrawViewArch", "ArchView"
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
