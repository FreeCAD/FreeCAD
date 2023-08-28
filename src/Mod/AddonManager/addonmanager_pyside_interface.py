# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022 FreeCAD Project Association                        *
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

"""Wrap QtCore imports so that can be replaced when running outside of FreeCAD (e.g. for
unit tests, etc.) Only provides wrappers for the things commonly used by the Addon
Manager."""

try:
    from PySide import QtCore

    QObject = QtCore.QObject
    Signal = QtCore.Signal

    def is_interruption_requested() -> bool:
        return QtCore.QThread.currentThread().isInterruptionRequested()

except ImportError:
    QObject = object

    class Signal:
        """A purely synchronous signal. emit() does not use queued slots so cannot be
        used across threads."""

        def __init__(self, *args):
            self.expected_types = args
            self.connections = []

        def connect(self, func):
            self.connections.append(func)

        def disconnect(self, func):
            if func in self.connections:
                self.connections.remove(func)

        def emit(self, *args):
            for connection in self.connections:
                connection(args)

    def is_interruption_requested() -> bool:
        return False
