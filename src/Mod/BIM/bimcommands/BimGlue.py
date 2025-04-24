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

"""The BIM Glue command"""

import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP


class BIM_Glue:

    def GetResources(self):
        return {
            "Pixmap": "BIM_Glue",
            "MenuText": QT_TRANSLATE_NOOP("BIM_Glue", "Glue"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_Glue", "Joins selected shapes into one non-parametric shape"
            ),
        }

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):
        import Part
        sel = FreeCADGui.Selection.getSelection()
        if sel:
            rem = []
            shapes = []
            for obj in sel:
                if obj.isDerivedFrom("Part::Feature"):
                    if obj.Shape:
                        shapes.append(obj.Shape)
                        rem.append(obj.Name)
            if shapes:
                comp = Part.makeCompound(shapes)
                FreeCAD.ActiveDocument.openTransaction("Glue")
                Part.show(comp)
                for name in rem:
                    FreeCAD.ActiveDocument.removeObject(name)
                FreeCAD.ActiveDocument.commitTransaction()


FreeCADGui.addCommand("BIM_Glue", BIM_Glue())
