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
    
#This list could be gen by python from preference page in the future
registered_solvers={
"Calculix":{ "Name":"Calculix", "Category":"Fem", "Module": "ccxFemSolver",
                     "Version": (2,7,0), "ResultReader":"FreeCAD","ExternalCaseEditor":""},
"OpenFOAM":{ "Name":"OpenFOAM", "Category":"Cfd", "Module": "FoamCfdSolver",
                     "Version": (2,0,0), "ResultReader":"praraview","ExternalCaseEditor":""}
}
def _SetSolverInfo(solver_obj, solverInfo):
    """Fill registered solver into FemSolverObject' Properties defined in C++"""
    pass

def makeCaeSolver(solverName, analysis=None):
    """CaeSolver Factory method, solverName string  "Calculix" "Openfoam"
    """
    if solverName != None and solverName in registered_solvers.keys():
        sovlerInfo=registered_solvers[solverName]
        _CreateCaeSolver(sovlerInfo, analysis)
    else:
        raise Exception('Solver: {} is not registered or found'.format(solverName)) 
        
def _CreateCaeSolver(sovlerInfo,analysis=None):
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
            
    if FreeCAD.GuiUp:
        #FreeCADGui.addModule(sovlerInfo["Module"])
        FreeCADGui.doCommand("from {} import ViewProviderCaeSolver, CaeSolver".format(sovlerInfo["Module"]))
    else:
        pass # import 
    
    obj=FreeCAD.ActiveDocument.addObject("Fem::FemSolverObjectPython", sovlerInfo["Name"])
    CaeSolver(obj)
    ViewProviderCaeSolver(obj.ViewObject)
    _analysis.Category=sovlerInfo["Category"]
    _analysis.SolverName=sovlerInfo["Name"]
    
    solverObj=obj.Proxy
    solverObj.SolverInfo=solverInfo #dict should be add into property
    FreeCAD.Console.PrintMessage("Solver {} is created\n".format(solverObj.SolverInfo["Name"]))
    
    _SetSolverInfo(obj, solverInfo)
    FreeCAD.Console.PrintMessage("Solver {} is created\n".format(sovlerInfo["Name"]))
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