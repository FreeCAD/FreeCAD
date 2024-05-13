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

"""BIM Schedule command"""


import os
import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate
PARAMS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM")


class Arch_Schedule:

    "the Arch Schedule command definition"

    def GetResources(self):
        return {'Pixmap': 'Arch_Schedule',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Schedule","Schedule"),
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Schedule","Creates a schedule to collect data from the model")}

    def Activated(self):
        if hasattr(self,"taskd"):
            if self.taskd:
                self.taskd.form.hide()
        import ArchSchedule
        self.taskd = ArchSchedule.ArchScheduleTaskPanel()

    def IsActive(self):
        if FreeCAD.ActiveDocument:
            return True
        else:
            return False


FreeCADGui.addCommand('Arch_Schedule',Arch_Schedule())
