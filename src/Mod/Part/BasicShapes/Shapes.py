# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2019 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

__title__ = "BasicShapes.Shapes"
__author__ = "Werner Mayer"
__url__ = "http://www.freecadweb.org"
__doc__ = "Basic shapes"


import FreeCAD
from FreeCAD import Qt
import Part
import math
import sys

def makeTube(outerRadius, innerRadius, height):
    outer_cylinder = Part.makeCylinder(outerRadius, height)
    shape = outer_cylinder
    if innerRadius > 0 and innerRadius < outerRadius:
        inner_cylinder = Part.makeCylinder(innerRadius, height)
        shape = outer_cylinder.cut(inner_cylinder)
    return shape


class TubeFeature:
    def __init__(self, obj):
        obj.Proxy = self
        obj.addProperty("App::PropertyLength","OuterRadius","Tube","Outer radius").OuterRadius = 5.0
        obj.addProperty("App::PropertyLength","InnerRadius","Tube","Inner radius").InnerRadius = 2.0
        obj.addProperty("App::PropertyLength","Height","Tube", "Height of the tube").Height = 10.0
        obj.addExtension("Part::AttachExtensionPython", self)

    def execute(self, fp):
        if fp.InnerRadius >= fp.OuterRadius:
            raise ValueError("Inner radius must be smaller than outer radius")
        fp.Shape = makeTube(fp.OuterRadius, fp.InnerRadius, fp.Height)


# Only if GUI is running
if FreeCAD.GuiUp:
    import FreeCADGui


    class ViewProviderTube:
        def __init__(self, obj):
            ''' Set this object to the proxy object of the actual view provider '''
            obj.Proxy = self

        def attach(self, obj):
            ''' Setup the scene sub-graph of the view provider, this method is mandatory '''
            return

        def getIcon(self):
            return ":/icons/Tree_Part_Tube_Parametric.svg"

        def __getstate__(self):
            return None

        def __setstate__(self,state):
            return None


    class CommandTube:
        """Command for creating Tube."""
        def GetResources(self):
            return {'MenuText': Qt.QT_TRANSLATE_NOOP("Part_Tube","Create tube"),
                    'Accel': "",
                    'CmdType': "AlterDoc:Alter3DView:AlterSelection",
                    'Pixmap': ":/icons/Part_Tube.svg",
                    'ToolTip': Qt.QT_TRANSLATE_NOOP("Part_Tube","Creates a tube")}
        
        def Activated(self):
            FreeCAD.ActiveDocument.openTransaction("Create tube")
            tube = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Tube")
            TubeFeature(tube)
            ViewProviderTube(tube.ViewObject)
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()

        def IsActive(self):
            return not FreeCAD.ActiveDocument is None


    FreeCADGui.addCommand('Part_Tube', CommandTube())
