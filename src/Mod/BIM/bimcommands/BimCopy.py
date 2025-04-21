# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 Yorik van Havre <yorik@uncreated.net>              *
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

"""The BIM Copy command"""

import FreeCAD
import FreeCADGui
import DraftTools

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP


class BIM_Copy(DraftTools.Move):

    def __init__(self):
        DraftTools.Move.__init__(self)
        self.copymode = True

    def GetResources(self):
        return {
            "Pixmap": "BIM_Copy",
            "MenuText": QT_TRANSLATE_NOOP("BIM_Copy", "Copy"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_Copy", "Copies selected objects to another location"
            ),
            "Accel": "C,P",
        }

    def Activated(self):
        from draftutils import params
        self.cmode = params.get_param("CopyMode")
        DraftTools.Move.Activated(self)

    def finish(self, cont=False):
        from draftutils import params
        DraftTools.Move.finish(self, cont)
        params.set_param("CopyMode", self.cmode)

FreeCADGui.addCommand("BIM_Copy", BIM_Copy())
