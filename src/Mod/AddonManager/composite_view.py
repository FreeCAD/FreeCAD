# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022-2024 FreeCAD Project Association                   *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

""" Provides a class for showing the list view and detail view at the same time. """

import addonmanager_freecad_interface

# Get whatever version of PySide we can
try:
    import PySide  # Use the FreeCAD wrapper
except ImportError:
    try:
        import PySide6  # Outside FreeCAD, try Qt6 first

        PySide = PySide6
    except ImportError:
        import PySide2  # Fall back to Qt5 (if this fails, Python will kill this module's import)

        PySide = PySide2

from PySide import QtCore, QtWidgets


class CompositeView(QtWidgets.QWidget):
    """A widget that displays the Addon Manager's top bar, the list of Addons, and the detail
    view, all on a single pane (with no switching). Detail view is shown in its "icon-only" mode
    for the installation, etc. buttons. The bottom bar remains visible throughout."""

    def __init__(self, parent=None):
        super().__init__(parent)

    # TODO: Refactor the Addon Manager's display into four custom widgets:
    # 1) The top bar showing the filter and search
    # 2) The package list widget, which can take three forms (expanded, compact, and list)
    # 3) The installer bar, which can take two forms (text and icon)
    # 4) The bottom bar
