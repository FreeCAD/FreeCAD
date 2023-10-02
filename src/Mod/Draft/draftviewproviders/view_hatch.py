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


"""Provides the viewprovider code for the Hatch object."""

import PySide.QtCore as QtCore
import PySide.QtGui as QtGui

import FreeCADGui as Gui

from draftguitools.gui_hatch import Draft_Hatch_TaskPanel
from draftutils.translate import translate

class ViewProviderDraftHatch:

    def __init__(self, vobj):

        vobj.Proxy = self

    def getIcon(self):

        return ":/icons/Draft_Hatch.svg"

    def dumps(self):

        return None

    def loads(self, state):

        return None

    def attach(self, vobj):

        self.Object = vobj.Object
        return

    def setEdit(self, vobj, mode):
        # EditMode 1 and 2 are handled by the Part::FeaturePython code.
        # EditMode 3 (Color) does not make sense for hatches (which do not
        # have faces) and we let that default to EditMode 0.

        if mode == 1 or mode == 2:
            return None

        taskd = Draft_Hatch_TaskPanel(vobj.Object)
        taskd.form.File.setFileName(vobj.Object.File)
        taskd.form.Pattern.setCurrentText(vobj.Object.Pattern)
        taskd.form.Scale.setValue(vobj.Object.Scale)
        taskd.form.Rotation.setValue(vobj.Object.Rotation)
        Gui.Control.showDialog(taskd)
        return True

    def unsetEdit(self, vobj, mode):
        # See setEdit.

        if mode == 1 or mode == 2:
            return None

        return True

    def setupContextMenu(self, vobj, menu):
        action_edit = QtGui.QAction(translate("draft", "Edit"),
                                    menu)
        QtCore.QObject.connect(action_edit,
                               QtCore.SIGNAL("triggered()"),
                               self.edit)
        menu.addAction(action_edit)

        action_transform = QtGui.QAction(Gui.getIcon("Std_TransformManip.svg"),
                                         translate("Command", "Transform"), # Context `Command` instead of `draft`.
                                         menu)
        QtCore.QObject.connect(action_transform,
                               QtCore.SIGNAL("triggered()"),
                               self.transform)
        menu.addAction(action_transform)

        return True # Removes `Transform` and `Set colors` from the default
                    # Part::FeaturePython context menu. See view_base.py.

    def edit(self):
        Gui.ActiveDocument.setEdit(self.Object, 0)

    def transform(self):
        Gui.ActiveDocument.setEdit(self.Object, 1)
