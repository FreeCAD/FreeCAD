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
    import FreeCADGui
    import FemGui
    from PySide import QtCore, QtGui
    
#This list could be gen by python from preference page in the future,
# each key has same name Property in Fem::FemSolverObjectPython
registered_solvers={
"Calculix":{ "SolverName":"Calculix", "Category":"FEM", "Module": "ccxFemSolver",
                     "ExternalResultReader":"","ExternalCaseEditor":""},
"OpenFOAM":{ "SolverName":"OpenFOAM", "Category":"CFD", "Module": "FoamCfdSolver",
                     "ExternalResultViewer":"paraFoam","ExternalCaseEditor":""}
}

def _SetSolverInfo(solver_obj, solverInfo):
    #for key, value in d.iteritems(): solver_obj[k] = solverInfo[k]
    solver_obj.SolverName = solverInfo["SolverName"]
    solver_obj.Category = solverInfo["Category"]
    solver_obj.Module = solverInfo["Module"]
    solver_obj.ExternalResultViewer = solverInfo["ExternalResultViewer"]
    solver_obj.ExternalCaseEditor = solverInfo["ExternalCaseEditor"]
    

def makeCaeSolver(solverName, analysis=None):
    """CaeSolver Factory method, solverName string  "Calculix" "Openfoam"
    """
    if solverName != None and solverName in registered_solvers.keys():
        solverInfo=registered_solvers[solverName]
        obj=_CreateCaeSolver(solverInfo, analysis)
        _SetSolverInfo(obj, solverInfo)
    else:
        raise Exception('Solver: {} is not registered or found'.format(solverName)) 
        
def _CreateCaeSolver(solverInfo,analysis=None):
    if FreeCAD.GuiUp:
        if analysis != None: # other test like type
            _analysis=analysis
        else:
            _analysis=FemGui.getActiveAnalysis()
    else: #nonGui
        if analysis != None: # todo: other test like type
            _analysis=analysis
        else:
            raise Exception('Analysis type is not the valid type')
    
    #eval is not compatible with python 3?
    #eval("from {} import ViewProviderCaeSolver, CaeSolver".format(solverInfo["Module"]))
    if solverInfo["SolverName"] == "Calculix":
        from ccxFemSolver import ViewProviderCaeSolver, CaeSolver
    else:
        from FoamCfdSolver import ViewProviderCaeSolver, CaeSolver
        
    obj = FreeCAD.ActiveDocument.addObject("Fem::FemSolverObjectPython", solverInfo["SolverName"])
    CaeSolver(obj)
    ViewProviderCaeSolver(obj.ViewObject)
    _analysis.Member = _analysis.Member + [obj] #FreeCAD.ActiveDocument.ActiveObject
    
    _analysis.Category = solverInfo["Category"]
    _analysis.SolverName = solverInfo["SolverName"]

    FreeCAD.Console.PrintMessage("Solver {} is created\n".format(solverInfo["SolverName"]))
    return obj
        
        
"""
#Currently, one analysis has only one solver, once analysis is created, solver is added auto
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
        #not implemented yet!
        #pop a dialog to select solver
        #_CreateCaeSolver(???)
        #import _SolverControlTaskPanel
        #taskd = _SolverControlTaskPanel._ResultControlTaskPanel()
        #FreeCADGui.Control.showDialog(taskd)

    def IsActive(self):
        return FreeCADGui.ActiveDocument is not None and solver_present()
    
if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Fem_CreateSolver', _CommandNewCaeSolver())
"""