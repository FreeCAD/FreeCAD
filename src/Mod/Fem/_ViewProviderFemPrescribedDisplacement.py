#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2013-2015 - Juergen Riegel <FreeCAD@juergen-riegel.net> *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

__title__ = "Command Prescribed Displacement"
__author__ = "Alfred Bogaers and Michael Hindley"
__url__ = "http://www.freecadweb.org"

class viewProviderPrescribedDisplacement:
    def __init__(self, vobj):
        vobj.Proxy = self

    def getIcon(self):
        return ':/icons/fem-constraint-displacement.svg'

    # Show boundary condition on GUI
    def attach(self, vobj):
        self.ViewObject = vobj
        self.Object = vobj.Object
        self.ViewObject.ShapeColor = (1.0, 0.0, 0.0)
        self.ViewObject.LineColor = (1.0, 0.0, 0.0)
        self.ViewObject.PointColor = (1.0, 0.0, 0.0)
        self.ViewObject.LineWidth = 5
        self.ViewObject.Transparency = 90
        self.ViewObject.PointSize = 5

        return

    def updateData(self, obj, prop):
        return

    def onChanged(self, vobj, prop):
        return

    def setEdit(self, vobj, mode):
        import FreeCAD
        import FreeCADGui
        from _FemPrescribedDisplacement import TaskPanelPrescribedDisplacement
        taskd = TaskPanelPrescribedDisplacement(self.Object)
        taskd.obj = vobj.Object
        FreeCADGui.Control.showDialog(taskd)
        return True

    def getDefaultDisplayMode(self):
        return "Flat Lines"

    def unsetEdit(self, vobj, mode):
        import FreeCAD
        import FreeCADGui
        # return
        FreeCADGui.Control.closeDialog()
        return

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None



