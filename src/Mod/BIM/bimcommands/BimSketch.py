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

"""The Bim Sketch command"""


import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP


class BIM_Sketch:

    def GetResources(self):
        return {
            "Pixmap": "Sketch",
            "MenuText": QT_TRANSLATE_NOOP("BIM_Sketch", "Sketch"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_Sketch", "Creates a new sketch in the current working plane"
            ),
            "Accel": "S,K",
        }

    def IsActive(self):
        if FreeCAD.ActiveDocument:
            return True
        else:
            return False

    def Activated(self):
        from draftutils import params
        issnap = False
        if hasattr(FreeCAD, "DraftWorkingPlane"):
            FreeCAD.DraftWorkingPlane.setup()
        if hasattr(FreeCADGui, "Snapper"):
            FreeCADGui.Snapper.setGrid()
            issnap = FreeCADGui.Snapper.isEnabled("Grid")
        sk = FreeCAD.ActiveDocument.addObject("Sketcher::SketchObject", "Sketch")
        if issnap and hasattr(sk.ViewObject, "GridSnap"):
            s = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft").GetInt(
                "gridSize", 100
            )
            sk.ViewObject.GridSize = s
            sk.ViewObject.GridSnap = True
        sk.MapMode = "Deactivated"
        sk.ViewObject.LineColor = params.get_param_view("DefaultShapeLineColor")
        sk.ViewObject.PointColor = params.get_param_view("DefaultShapeLineColor")
        sk.ViewObject.LineWidth = params.get_param_view("DefaultShapeLineWidth")
        p = FreeCAD.DraftWorkingPlane.getPlacement()
        p.Base = FreeCAD.DraftWorkingPlane.position
        sk.Placement = p
        FreeCADGui.ActiveDocument.setEdit(sk.Name)
        FreeCADGui.activateWorkbench("SketcherWorkbench")


FreeCADGui.addCommand("BIM_Sketch", BIM_Sketch())
