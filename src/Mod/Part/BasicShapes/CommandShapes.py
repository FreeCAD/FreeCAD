# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2020 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

__title__ = "BasicShapes.CommandShapes"
__author__ = "Werner Mayer"
__url__ = "http://www.freecad.org"
__doc__ = "Basic shapes"


import FreeCAD
from FreeCAD import Qt
import FreeCADGui

import Part
from . import Shapes
from . import ViewProviderShapes
import math
import sys



class CommandTube:
    """Command for creating Tube."""
    def GetResources(self):
        return {'MenuText': Qt.QT_TRANSLATE_NOOP("Part_Tube","Create tube"),
                'Accel': "",
                'CmdType': "AlterDoc:Alter3DView:AlterSelection",
                'Pixmap': "Part_Tube",
                'ToolTip': Qt.QT_TRANSLATE_NOOP("Part_Tube","Creates a tube")}

    def Activated(self):
        text = FreeCAD.Qt.translate("QObject", "Create tube")
        FreeCAD.ActiveDocument.openTransaction(text)
        tube = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Tube")
        Shapes.TubeFeature(tube)
        vp = ViewProviderShapes.ViewProviderTube(tube.ViewObject)
        activePart = FreeCADGui.activeView().getActiveObject('part')
        if activePart:
            activePart.addObject(tube)
        FreeCAD.ActiveDocument.recompute()
        vp.startDefaultEditMode(tube.ViewObject)

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None


FreeCADGui.addCommand('Part_Tube', CommandTube())
