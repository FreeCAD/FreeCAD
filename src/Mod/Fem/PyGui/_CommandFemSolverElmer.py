# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 - Markus Hovorka <m.hovorka@live.de>               *
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


__title__ = "Elmer"
__author__ = "Markus Hovorka"
__url__ = "http://www.freecadweb.org"


from PySide import QtCore

import FreeCAD as App
import FreeCADGui as Gui
import FemGui


class Command(QtCore.QObject):

    def Activated(self):
        analysis = FemGui.getActiveAnalysis()
        App.ActiveDocument.openTransaction("Create Elmer solver object")
        Gui.addModule("femsolver.elmer.solver")
        Gui.doCommand(
            "App.ActiveDocument.%s.Member += "
            "[femsolver.elmer.solver.create(App.ActiveDocument)]"
            % analysis.Name)
        App.ActiveDocument.commitTransaction()
        App.ActiveDocument.recompute()

    def GetResources(self):
        return {
            'Pixmap': 'fem-elmer',
            'MenuText': "Solver Elmer",
            'Accel': "S, E",
            'ToolTip': "Creates a FEM solver Elmer"
        }

    def IsActive(self):
        analysis = FemGui.getActiveAnalysis()
        return (analysis is not None
                and analysis.Document == App.ActiveDocument)


Gui.addCommand('FEM_SolverElmer', Command())
