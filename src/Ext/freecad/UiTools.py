# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   Copyright (c) 2021 Werner Mayer                                            #
#   Copyright (c) 2022 FreeCAD Project Association                             #
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

from PySide import QtUiTools
import FreeCADGui as Gui


class QUiLoader(QtUiTools.QUiLoader):
    """
    This is an extension of Qt's QUiLoader to also create custom widgets
    """
    def __init__(self, arg = None):
        super(QUiLoader, self).__init__(arg)
        self.ui = Gui.PySideUic

    def createWidget(self, className, parent = None, name = ""):
        widget = self.ui.createCustomWidget(className, parent, name)
        if not widget:
            widget = super(QUiLoader, self).createWidget(className, parent, name)
        return widget

