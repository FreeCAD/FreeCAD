# SPDX-License-Identifier: LGPL-2.1-or-later

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

import FreeCAD
import FreeCADGui
import Path
from Path.Tool.toolbit.ui import ToolBitEditorPanel


class TaskPanel:
    """TaskPanel for the SetupSheet - if it is being edited directly."""

    def __init__(self, vobj, deleteOnReject):
        Path.Log.track(vobj.Object.Label)
        self.vobj = vobj
        self.obj = vobj.Object
        # The editor switches the global schema to the bit's units, and the
        # panel outlives it, so the panel is what has to put it back.
        self._entry_schema = FreeCAD.Units.getSchema()
        self.editor = ToolBitEditorPanel(self.obj.Proxy)
        # The task dialog supplies its own OK/Cancel.
        self.editor._button_box.hide()
        self.form = self.editor
        self.deleteOnReject = deleteOnReject
        FreeCAD.ActiveDocument.openTransaction("Edit ToolBit")

    def reject(self):
        # The transaction holds every edit made in the panel, so aborting it
        # is the undo; the editor has nothing of its own to roll back.
        FreeCAD.ActiveDocument.abortTransaction()
        FreeCAD.Units.setSchema(self._entry_schema)
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCADGui.Control.closeDialog()
        if self.deleteOnReject:
            FreeCAD.ActiveDocument.openTransaction("Uncreate ToolBit")
            FreeCAD.ActiveDocument.removeObject(self.obj.Name)
            FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()
        return True

    def accept(self):
        self.editor.save_toolbit()
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.Units.setSchema(self._entry_schema)
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()
        return True

    def setupUi(self):
        pass
