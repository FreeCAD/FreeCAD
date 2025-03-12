# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2024 Mario Passaglia <mpassaglia[at]cbc.uba.ar>         *
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

__title__ = "FreeCAD FEM base task panel object"
__author__ = "Mario Passaglia"
__url__ = "https://www.freecad.org"

## @package base_femtaskpanel
#  \ingroup FEM
#  \brief base object for FEM task panels


class _BaseTaskPanel:
    """
    Base task panel
    """

    def __init__(self, obj):
        self.obj = obj

    def accept(self):
        gui_doc = self.obj.ViewObject.Document
        gui_doc.Document.recompute()
        gui_doc.resetEdit()
        gui_doc.Document.commitTransaction()

        return True

    def reject(self):
        gui_doc = self.obj.ViewObject.Document
        gui_doc.Document.abortTransaction()
        gui_doc.resetEdit()
        gui_doc.Document.recompute()

        return True
