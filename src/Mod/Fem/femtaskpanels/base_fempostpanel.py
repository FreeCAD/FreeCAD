# ***************************************************************************
# *   Copyright (c) 2025 Stefan Tröger <stefantroeger@gmx.net>              *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
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

__title__ = "FreeCAD task panel base for post object task panels"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"

## @package base_fempostpanel
#  \ingroup FEM
#  \brief task panel base for post objects

from PySide import QtCore, QtGui

import FreeCAD

from . import base_femtaskpanel

translate = FreeCAD.Qt.translate


class _BasePostTaskPanel(base_femtaskpanel._BaseTaskPanel):
    """
    The TaskPanel for post objects, mimicking the c++ functionality
    """

    def __init__(self, obj):
        super().__init__(obj)

        # get the settings group
        self.__settings_grp = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem")

    # Implement parent functions
    # ##########################

    def getStandardButtons(self):
        return (
            QtGui.QDialogButtonBox.Apply | QtGui.QDialogButtonBox.Ok | QtGui.QDialogButtonBox.Cancel
        )

    def clicked(self, button):
        # apply button hit?
        if button == QtGui.QDialogButtonBox.Apply:
            self.obj.Document.recompute()

    def open(self):
        # open a new transaction if non is open
        if not FreeCAD.getActiveTransaction():
            FreeCAD.ActiveDocument.openTransaction(
                translate("FEM", "Edit {}").format(self.obj.Label)
            )

    # Helper functions
    # ################

    def _recompute(self):
        # only recompute if the user wants automatic recompute
        if self.__settings_grp.GetBool("PostAutoRecompute", True):
            self.obj.Document.recompute()

    def _enumPropertyToCombobox(self, obj, prop, cbox):
        cbox.blockSignals(True)
        cbox.clear()
        entries = obj.getEnumerationsOfProperty(prop)
        for entry in entries:
            cbox.addItem(entry)

        cbox.setCurrentText(getattr(obj, prop))
        cbox.blockSignals(False)
