# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2024 Yorik van Havre <yorik@uncreated.net>              *
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

"""The BIM Building part-related commands"""

# TODO: Refactor the Site code so it becomes a BuildingPart too


import os
import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate
PARAMS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM")


class Arch_Level:

    """The command definition for the Arch workbench's gui tool, Arch Floor"""

    def GetResources(self):

        return {'Pixmap'  : 'Arch_Floor',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Level","Level"),
                'Accel': "L, V",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Level","Creates a Building Part object that represents a level.")}

    def IsActive(self):

        return not FreeCAD.ActiveDocument is None

    def Activated(self):

        FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Level"))
        FreeCADGui.addModule("Arch")
        FreeCADGui.addModule("Draft")
        FreeCADGui.addModule("WorkingPlane")
        FreeCADGui.doCommand("obj = Arch.makeFloor()")
        FreeCADGui.doCommand("obj.Placement = WorkingPlane.get_working_plane().get_placement()")
        FreeCADGui.doCommand("Draft.autogroup(obj)")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


class Arch_Building:

    "the Arch Building command definition"

    def GetResources(self):

        return {'Pixmap'  : 'Arch_Building',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Building","Building"),
                'Accel': "B, U",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Building","Creates a building object.")}

    def IsActive(self):

        return not FreeCAD.ActiveDocument is None

    def Activated(self):

        FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Building"))
        FreeCADGui.addModule("Arch")
        FreeCADGui.addModule("Draft")
        FreeCADGui.doCommand("obj = Arch.makeBuilding()")
        FreeCADGui.doCommand("Draft.autogroup(obj)")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


FreeCADGui.addCommand('Arch_Building', Arch_Building())
FreeCADGui.addCommand('Arch_Level', Arch_Level())
