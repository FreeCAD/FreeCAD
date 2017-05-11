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

        sel = FreeCADGui.Selection.getSelection()
        if len(sel) == 1 and sel[0].isDerivedFrom("Fem::FemSolverObjectPython"):
            self.solver = sel[0]
        if self.solver.SolverType == "FemSolverCalculix":
            import FemToolsCcx
            self.fea = FemToolsCcx.FemToolsCcx(None, self.solver)
            self.fea.reset_mesh_purge_results_checked()
            message = self.fea.check_prerequisites()
            if message:
                QtGui.QMessageBox.critical(None, "Missing prerequisite", message)
                return
            self.fea.finished.connect(load_results)
            QtCore.QThreadPool.globalInstance().start(self.fea)
        elif self.solver.SolverType == "FemSolverZ88":
            import FemToolsZ88
            self.fea = FemToolsZ88.FemToolsZ88(None, self.solver)
            self.fea.reset_mesh_purge_results_checked()
            message = self.fea.check_prerequisites()
            if message:
                QtGui.QMessageBox.critical(None, "Missing prerequisite", message)
                return
            self.fea.run()  # test z88
            # self.fea.finished.connect(load_results)
            # QtCore.QThreadPool.globalInstance().start(self.fea)
        else:
            QtGui.QMessageBox.critical(None, "Not known solver type", message)


FreeCADGui.addCommand('FEM_SolverRun', _CommandFemSolverRun())
