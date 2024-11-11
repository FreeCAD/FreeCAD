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

"""The BIM Background command"""


import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP


class BIM_Background:

    def GetResources(self):
        return {
            "Pixmap": "BIM_Background",
            "MenuText": QT_TRANSLATE_NOOP("BIM_Background", "Toggle background"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_Background",
                "Toggles the background of the 3D view between simple and gradient",
            ),
        }

    def Activated(self):
        param = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View")
        if param.GetBool("Simple", True):
            param.SetBool("Simple", False)
            param.SetBool("Gradient", True)
            param.SetBool("RadialGradient", False)
        elif param.GetBool("Gradient", True):
            param.SetBool("Simple", False)
            param.SetBool("Gradient", False)
            param.SetBool("RadialGradient", True)
        else:
            param.SetBool("Simple", True)
            param.SetBool("Gradient", False)
            param.SetBool("RadialGradient", False)
        FreeCADGui.updateGui()


FreeCADGui.addCommand("BIM_Background", BIM_Background())
