# SPDX-License-Identifier: LGPL-2.1-or-later
# /**************************************************************************
#                                                                           *
#    Copyright (c) 2026 AstoCAD     <hello@astocad.com>                     *
#                                                                           *
#    This file is part of FreeCAD.                                          *
#                                                                           *
#    FreeCAD is free software: you can redistribute it and/or modify it     *
#    under the terms of the GNU Lesser General Public License as            *
#    published by the Free Software Foundation, either version 2.1 of the   *
#    License, or (at your option) any later version.                        *
#                                                                           *
#    FreeCAD is distributed in the hope that it will be useful, but         *
#    WITHOUT ANY WARRANTY; without even the implied warranty of             *
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
#    Lesser General Public License for more details.                        *
#                                                                           *
#    You should have received a copy of the GNU Lesser General Public       *
#    License along with FreeCAD. If not, see                                *
#    <https://www.gnu.org/licenses/>.                                       *
#                                                                           *
# **************************************************************************/

"""Edit-state toolbar switching for the Forms workbench."""

import FreeCAD as App
import FreeCADGui as Gui
from PySide import QtCore, QtWidgets

CREATE_TOOLBAR = "Forms Create"
MODIFY_TOOLBAR = "Forms Modify"
FORMS_WORKBENCH = "FormsWorkbench"


def _toolbar_names(name):
    return {name, App.Qt.translate("Workbench", name)}


def _find_toolbar(name):
    expected = _toolbar_names(name)
    for toolbar in Gui.getMainWindow().findChildren(QtWidgets.QToolBar):
        if toolbar.objectName() in expected or toolbar.windowTitle() in expected:
            return toolbar
    return None


def set_forms_toolbar_mode(editing):
    """Show only the Forms toolbar appropriate to the current edit state."""

    def apply():
        try:
            if Gui.activeWorkbench().name() != FORMS_WORKBENCH:
                return
        except (AttributeError, RuntimeError):
            return
        create_toolbar = _find_toolbar(CREATE_TOOLBAR)
        modify_toolbar = _find_toolbar(MODIFY_TOOLBAR)
        if create_toolbar is not None:
            create_toolbar.setVisible(not editing)
        if modify_toolbar is not None:
            modify_toolbar.setVisible(bool(editing))
            # The Form editor is a custom task dialog. FreeCAD disables normal
            # workbench toolbars when that dialog opens, so merely showing the
            # edit toolbar leaves every action visually inactive. Re-enable the
            # toolbar itself; each command still applies its own IsActive().
            modify_toolbar.setEnabled(bool(editing))

    # Workbench activation creates/restores Qt toolbars asynchronously.
    QtCore.QTimer.singleShot(0, apply)
