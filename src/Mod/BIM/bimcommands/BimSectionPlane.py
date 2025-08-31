# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2024 Yorik van Havre <yorik@uncreated.net>              *
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

"""BIM Schedule command"""

import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate

PARAMS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM")


class Arch_SectionPlane:

    "the Arch SectionPlane command definition"

    def GetResources(self):

        return {'Pixmap'  : 'Arch_SectionPlane_Tree',
                'Accel': "S, E",
                'MenuText': QT_TRANSLATE_NOOP("Arch_SectionPlane","Section Plane"),
                'ToolTip': QT_TRANSLATE_NOOP("Arch_SectionPlane","Creates a section plane object, including the selected objects")}

    def IsActive(self):

        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):

        sel = FreeCADGui.Selection.getSelection()
        ss = "["
        for o in sel:
            if len(ss) > 1:
                ss += ","
            ss += "FreeCAD.ActiveDocument."+o.Name
        ss += "]"
        FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Section Plane"))
        FreeCADGui.addModule("Arch")
        FreeCADGui.doCommand("section = Arch.makeSectionPlane("+ss+")")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()
        if len(sel) == 1 and getattr(sel[0], "IfcClass", None) == "IfcProject":
            # remove the IFC project, otherwise we can't aggregate (circular loop)
            FreeCADGui.doCommand("section.Objects = []")
            #FreeCADGui.addModule("nativeifc.ifc_tools")
            #p = "FreeCAD.ActiveDocument."+sel[0].Name
            #FreeCADGui.doCommand("nativeifc.ifc_tools.aggregate(section,"+p+")")



FreeCADGui.addCommand('Arch_SectionPlane', Arch_SectionPlane())
