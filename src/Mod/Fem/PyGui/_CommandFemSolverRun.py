# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2013-2015 - Juergen Riegel <FreeCAD@juergen-riegel.net> *
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

__title__ = "Command Run Solver"
__author__ = "Juergen Riegel"
__url__ = "http://www.freecadweb.org"

## @package CommandFemSolverRun
#  \ingroup FEM

from .FemCommands import FemCommands
import FreeCADGui
from PySide import QtCore, QtGui
import femsolver.run
import FemUtils


class _CommandFemSolverRun(FemCommands):
    # the FEM_SolverRun command definition
    def __init__(self):
        super(_CommandFemSolverRun, self).__init__()
        self.resources = {'Pixmap': 'fem-run-solver',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("FEM_SolverRun", "Run solver calculations"),
                          'Accel': "R, C",
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("FEM_SolverRun", "Runs the calculations for the selected solver")}
        self.is_active = 'with_solver'

    def Activated(self):
        def load_results(ret_code):
            if ret_code == 0:
                self.fea.load_results()
            else:
                print ("CalculiX failed ccx finished with error {}".format(ret_code))

        self.solver = FreeCADGui.Selection.getSelection()[0]  # see 'with_solver' in FemCommands for selection check
        if FemUtils.isDerivedFrom(self.solver, "Fem::FemSolverObjectZ88"):
            self._newActivated()
        elif self.solver.SolverType == "FemSolverCalculix":
            import FemToolsCcx
            self.fea = FemToolsCcx.FemToolsCcx(None, self.solver)
            self.fea.reset_mesh_purge_results_checked()
            message = self.fea.check_prerequisites()
            if message:
                QtGui.QMessageBox.critical(None, "Missing prerequisite", message)
                return
            self.fea.finished.connect(load_results)
            QtCore.QThreadPool.globalInstance().start(self.fea)
        else:
            QtGui.QMessageBox.critical(None, "Not known solver type", message)

    def _newActivated(self):
        solver = self._getSelectedSolver()
        if solver is not None:
            try:
                machine = femsolver.run.getMachine(solver)
            except femsolver.run.MustSaveError:
                QtGui.QMessageBox.critical(
                    FreeCADGui.getMainWindow(),
                    "Can't start Solver",
                    "Please save the file before executing the solver. "
                    "This must be done because the location of the working "
                    "directory is set to \"Beside .fcstd File\".")
                return
            except femsolver.run.DirectoryDoesNotExist:
                QtGui.QMessageBox.critical(
                    FreeCADGui.getMainWindow(),
                    "Can't start Solver",
                    "Selected working directory doesn't exist.")
                return
            if not machine.running:
                machine.reset()
                machine.target = femsolver.run.RESULTS
                machine.start()

    def _getSelectedSolver(self):
        sel = FreeCADGui.Selection.getSelection()
        if len(sel) == 1 and sel[0].isDerivedFrom("Fem::FemSolverObjectPython"):
            return sel[0]
        return None


FreeCADGui.addCommand('FEM_SolverRun', _CommandFemSolverRun())
