# -*- coding: utf8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 Yorik van Havre <yorik@uncreated.net>              *
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

"""The BIM Project command - Includes previous Arch Project"""


import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP


class BIM_Project:

    def GetResources(self):
        return {
            "Pixmap": "BIM_Project",
            "MenuText": QT_TRANSLATE_NOOP("BIM_Project", "Project"),
            "ToolTip": QT_TRANSLATE_NOOP("BIM_Project",
                                         "Create an empty NativeIFC project"),
        }

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v and not hasattr(FreeCAD.ActiveDocument, "IfcFilePath")

    def Activated(self):
        from nativeifc import ifc_tools
        project = ifc_tools.create_document(FreeCAD.ActiveDocument)
        project.Modified = True
        FreeCAD.ActiveDocument.recompute()


FreeCADGui.addCommand("BIM_Project", BIM_Project())
