#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2022 Mario Passaglia                                    *
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


"""Provides the viewprovider code for the Shapestring object."""

import FreeCADGui as Gui

from draftviewproviders.view_base import ViewProviderDraft
from drafttaskpanels.task_shapestring import ShapeStringTaskPanelEdit

class ViewProviderShapeString(ViewProviderDraft):

    def __init__(self,vobj):

        vobj.Proxy = self

    def getIcon(self):

        return ":/icons/Draft_ShapeString.svg"

    def setEdit(self, vobj, mode):
        if mode != 0:
            return None

        if not "Draft_Edit" in Gui.listCommands(): # Using Draft_Edit to detect if the Draft, Arch or BIM WB has been loaded.
            self.wb_before_edit = Gui.activeWorkbench()
            Gui.activateWorkbench("DraftWorkbench")
        self.task = ShapeStringTaskPanelEdit(vobj)
        Gui.Control.showDialog(self.task)
        return True

    def unsetEdit(self, vobj, mode):
        if mode != 0:
            return None

        self.task.finish()
        if hasattr(self, "wb_before_edit"):
            Gui.activateWorkbench(self.wb_before_edit.name())
            delattr(self, "wb_before_edit")
        return True
