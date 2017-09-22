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


__title__ = "Command Solver Elmer"
__author__ = "Markus Hovorka"
__url__ = "http://www.freecadweb.org"

## @package CommandFemSolverZ88
#  \ingroup FEM

import FreeCAD
from .FemCommands import FemCommands
import FreeCADGui
import FemGui
from PySide import QtCore


class _CommandFemSolverElmer(FemCommands):
    "The FEM_SolverElmer command definition"
    def __init__(self):
        super(_CommandFemSolverElmer, self).__init__()
        self.resources = {'Pixmap': 'fem-elmer',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("FEM_SolverElmer", "Solver Elmer"),
                          'Accel': "S, E",
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("FEM_SolverElmer", "Creates a FEM solver Elmer")}
        self.is_active = 'with_analysis'

    def Activated(self):
        analysis = FemGui.getActiveAnalysis()
        FreeCAD.ActiveDocument.openTransaction("Create Elmer solver object")
        FreeCADGui.addModule("ObjectsFem")
        FreeCADGui.doCommand(
            "FreeCAD.ActiveDocument.%s.Member += "
            "[ObjectsFem.makeSolverElmer(FreeCAD.ActiveDocument)]"
            % analysis.Name)
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


FreeCADGui.addCommand('FEM_SolverElmer', _CommandFemSolverElmer())
