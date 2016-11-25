# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2015 - Bernd Hahnebach <bernd@bimstatik.org>            *
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

__title__ = "_CommandSolverCalculix"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package CommandSolverCalculix
#  \ingroup FEM

import FreeCAD
from FemCommands import FemCommands
import FreeCADGui
import FemGui
from PySide import QtCore


class _CommandSolverCalculix(FemCommands):
    "The Fem_SolverCalculix command definition"
    def __init__(self):
        super(_CommandSolverCalculix, self).__init__()
        self.resources = {'Pixmap': 'fem-solver',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("Fem_SolverCalculix", "Solver CalculiX"),
                          'Accel': "S, C",
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("Fem_SolverCalculix", "Creates a FEM solver CalculiX")}
        self.is_active = 'with_analysis'

    def Activated(self):
        has_nonlinear_material_obj = False
        for m in FemGui.getActiveAnalysis().Member:
            if hasattr(m, "Proxy") and m.Proxy.Type == "FemMaterialMechanicalNonlinear":
                has_nonlinear_material_obj = True
        FreeCAD.ActiveDocument.openTransaction("Create SolverCalculix")
        FreeCADGui.addModule("FemSolverCalculix")
        if has_nonlinear_material_obj:
            FreeCADGui.doCommand("solver = FemSolverCalculix.makeFemSolverCalculix()")
            FreeCADGui.doCommand("solver.MaterialNonlinearity = 'nonlinear'")
            FreeCADGui.doCommand("FemGui.getActiveAnalysis().Member = FemGui.getActiveAnalysis().Member + [solver]")
        else:
            FreeCADGui.doCommand("FemGui.getActiveAnalysis().Member = FemGui.getActiveAnalysis().Member + [FemSolverCalculix.makeFemSolverCalculix()]")


FreeCADGui.addCommand('Fem_SolverCalculix', _CommandSolverCalculix())
