# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2026 Jesse Trutna <jesse@trutna.com>                    *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Lesser General Public License for more details.                   *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

"""Keeps the task panel as a pane docked on the right of the main window."""

import FreeCADGui
from PySide import QtCore, QtWidgets

WIDTH = 340


def _task_dock(mw):
    for dock in mw.findChildren(QtWidgets.QDockWidget):
        widget = dock.widget()
        if widget is not None and widget.objectName() == "Tasks":
            return dock
    return None


def dock_on_right():
    """Undo a saved floating, overlay or other-side position of the task panel."""
    mw = FreeCADGui.getMainWindow()
    dock = _task_dock(mw)
    if dock is None:
        return
    if dock.isFloating():
        dock.setFloating(False)
    if mw.dockWidgetArea(dock) != QtCore.Qt.RightDockWidgetArea:
        mw.addDockWidget(QtCore.Qt.RightDockWidgetArea, dock)
    dock.setFeatures(dock.features() & ~QtWidgets.QDockWidget.DockWidgetFloatable)
    # Size the pane while shown, so it reopens at this width; it is only visible during a task.
    dock.show()
    if dock.width() < WIDTH:
        mw.resizeDocks([dock], [WIDTH], QtCore.Qt.Horizontal)
    if not FreeCADGui.Control.activeDialog():
        dock.hide()
