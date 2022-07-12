#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2021 Yorik van Havre <yorik@uncreated.net>              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************


"""This module contains FreeCAD commands for the Draft workbench"""

from draftguitools.gui_hatch import Draft_Hatch_TaskPanel

class ViewProviderDraftHatch:


    def __init__(self,vobj):

        vobj.Proxy = self

    def getIcon(self):

        return ":/icons/Draft_Hatch.svg"

    def __getstate__(self):

        return None

    def __setstate__(self,state):

        return None

    def setEdit(self,vobj,mode):
        if mode != 0:
            return None

        import FreeCADGui

        taskd = Draft_Hatch_TaskPanel(vobj.Object)
        taskd.form.File.setFileName(vobj.Object.File)
        taskd.form.Pattern.setCurrentText(vobj.Object.Pattern)
        taskd.form.Scale.setValue(vobj.Object.Scale)
        taskd.form.Rotation.setValue(vobj.Object.Rotation)
        FreeCADGui.Control.showDialog(taskd)
        return True

    def unsetEdit(self,vobj,mode):
        if mode != 0:
            return None

        import FreeCADGui

        FreeCADGui.Control.closeDialog()
        return True
