# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2019 sliptonic <shopinthewoods@gmail.com>               *
# *                 2025 Samuel Abels <knipknap@gmail.com>                  *
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

from PySide import QtGui
import FreeCADGui
import Path
from Path.Base.Gui import IconViewProvider
from Path.Tool.toolbit.ui.panel import TaskPanel


class ViewProvider(object):
    """
    ViewProvider for a ToolBit DocumentObject.
    It's sole job is to provide an icon and invoke the TaskPanel
    on edit.
    """

    def __init__(self, vobj, name):
        Path.Log.track(name, vobj.Object)
        self.panel = None
        self.icon = name
        self.obj = vobj.Object
        self.vobj = vobj
        vobj.Proxy = self

    def attach(self, vobj):
        Path.Log.track(vobj.Object)
        self.vobj = vobj
        self.obj = vobj.Object

    def getIcon(self):
        try:
            png_data = self.obj.Proxy.get_thumbnail()
        except AttributeError:  # Proxy not initialized
            png_data = None
        if png_data:
            pixmap = QtGui.QPixmap()
            pixmap.loadFromData(png_data, "PNG")
            return QtGui.QIcon(pixmap)
        return ":/icons/CAM_ToolBit.svg"

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def onDelete(self, vobj, arg2=None):
        Path.Log.track(vobj.Object.Label)
        vobj.Object.Proxy.onDelete(vobj.Object)

    def getDisplayMode(self, mode):
        return "Default"

    def _openTaskPanel(self, vobj, deleteOnReject):
        Path.Log.track()
        self.panel = TaskPanel(vobj, deleteOnReject)
        FreeCADGui.Control.closeDialog()
        FreeCADGui.Control.showDialog(self.panel)
        self.panel.setupUi()

    def setCreate(self, vobj):
        Path.Log.track()
        self._openTaskPanel(vobj, True)

    def setEdit(self, vobj, mode=0):
        self._openTaskPanel(vobj, False)
        return True

    def unsetEdit(self, vobj, mode):
        FreeCADGui.Control.closeDialog()
        self.panel = None
        return

    def claimChildren(self):
        if self.obj.BitBody:
            return [self.obj.BitBody]
        return []

    def doubleClicked(self, vobj):
        pass

    def setupContextMenu(self, vobj, menu):
        # Override the base class method to prevent adding the "Edit" action
        # for ToolBit objects.
        pass  # TODO: call setEdit here once we have a new editor panel


IconViewProvider.RegisterViewProvider("ToolBit", ViewProvider)
