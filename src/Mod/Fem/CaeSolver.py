#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2015 - Qingfeng Xia @iesensor.com                 *
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

import FreeCAD
if FreeCAD.GuiUp:
    import FemGui

# This list could be gen by python from preference page in the future,
# each key has same name Property in Fem::FemSolverObjectPython
REGISTERED_SOLVERS = {
    "Calculix": {"SolverName": "Calculix", "Category": "FEM", "Module": "ccxFemSolver",
                 "ExternalResultViewer": "", "ExternalCaseEditor": ""},
    "OpenFOAM": {"SolverName": "OpenFOAM", "Category": "CFD", "Module": "FoamCfdSolver",
                 "ExternalResultViewer": "paraFoam", "ExternalCaseEditor": ""}
}


def _setSolverInfo(solver_obj, solverInfo):
    solver_obj.SolverName = solverInfo["SolverName"]
    solver_obj.Category = solverInfo["Category"]
    solver_obj.Module = solverInfo["Module"]
    solver_obj.ExternalResultViewer = solverInfo["ExternalResultViewer"]
    solver_obj.ExternalCaseEditor = solverInfo["ExternalCaseEditor"]


def makeCaeSolver(solverName, analysis=None):
    """CaeSolver Factory method, solverName string  "Calculix" "Openfoam"
    """
    if (solverName is None) and solverName in REGISTERED_SOLVERS.keys():
        solverInfo = REGISTERED_SOLVERS[solverName]
        obj = _createCaeSolver(solverInfo, analysis)
        _setSolverInfo(obj, solverInfo)
    else:
        raise Exception('Solver: {} is not registered or found'.format(solverName))


def _createCaeSolver(solverInfo, analysis=None, solver_object=None):
    if FreeCAD.GuiUp:
        if (analysis is not None) and analysis.isDerivedFrom("Fem::FemAnalysisObject"):
            _analysis = analysis
        else:
            _analysis = FemGui.getActiveAnalysis()
    else:
        if (analysis is not None) and analysis.isDerivedFrom("Fem::FemAnalysisObject"):
            _analysis = analysis
        else:
            raise Exception('Analysis type is not the valid type')

    if solverInfo["SolverName"] == "Calculix":
        from ccxFemSolver import ViewProviderCaeSolver, CaeSolver
    else:
        from FoamCfdSolver import ViewProviderCaeSolver, CaeSolver

    if solver_object is None:
        obj = FreeCAD.ActiveDocument.addObject("Fem::FemSolverObjectPython", solverInfo["SolverName"])
    else:
        obj = solver_object
    CaeSolver(obj)
    _analysis.Member = _analysis.Member + [obj]
    if FreeCAD.GuiUp:
        ViewProviderCaeSolver(obj.ViewObject)

    _analysis.Category = solverInfo["Category"]
    _analysis.SolverName = solverInfo["SolverName"]

    FreeCAD.Console.PrintMessage("Solver {} is created\n".format(solverInfo["SolverName"]))
    return obj

"""
# Currently, one analysis has only one solver, once analysis is created, solver is added auto
class _CommandNewCaeSolver:
    def GetResources(self):
        return {'Pixmap': 'fem-solver',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Fem_Solver", "Create solver"),
                'Accel': "S, R",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Fem_Solver", "Create a solver object to an analysis")}

    def Activated(self):
        #
        if not self.solver_object:
            QtGui.QMessageBox.critical(None, "Missing prerequisite", "No solver is created in active Analysis")
            return
        # not implemented yet! pop a dialog to select solver
        #_CreateCaeSolver(???)
        #import _SolverControlTaskPanel
        #taskd = _SolverControlTaskPanel._ResultControlTaskPanel()
        #FreeCADGui.Control.showDialog(taskd)

    def IsActive(self):
        return FreeCADGui.ActiveDocument is not None and solver_present()

if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Fem_CreateSolver', _CommandNewCaeSolver())
"""
