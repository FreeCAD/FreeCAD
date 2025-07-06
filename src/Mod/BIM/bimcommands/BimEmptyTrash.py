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

"""The BIM EmptyTrash command"""

import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP


class BIM_EmptyTrash:

    def GetResources(self):
        return {
            "Pixmap": "BIM_Trash",
            "MenuText": QT_TRANSLATE_NOOP("BIM_EmptyTrash", "Empty Trash"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_EmptyTrash",
                "Deletes all objects from the trash bin that are not used by any other",
            ),
        }

    def Activated(self):
        trash = FreeCAD.ActiveDocument.getObject("Trash")
        if trash and trash.isDerivedFrom("App::DocumentObjectGroup"):
            deletelist = []
            for obj in trash.Group:
                if (len(obj.InList) == 1) and (obj.InList[0] == trash):
                    deletelist.append(obj.Name)
                    deletelist.extend(self.getDeletableChildren(obj))
            if deletelist:
                FreeCAD.ActiveDocument.openTransaction("Empty Trash")
                for name in deletelist:
                    FreeCAD.ActiveDocument.removeObject(name)
                FreeCAD.ActiveDocument.commitTransaction()

    def getDeletableChildren(self, obj):
        deletelist = []
        for child in obj.OutList:
            if (len(child.InList) == 1) and (child.InList[0] == obj):
                deletelist.append(child.Name)
                deletelist.extend(self.getDeletableChildren(child))
        return deletelist


FreeCADGui.addCommand("BIM_EmptyTrash", BIM_EmptyTrash())
