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

#This list could be gen by python code by check subfolders
registered_solvers={
"Calculix":{ "name":"Calculix", "catogory":"Fem", "module": "ccxFemSolver","version": (2,7,0), "reader":"","writer":""}
}

from PreImport import *

#ViewProvider and CaeSolver DocumentObject should be gen
def makeCaeSolver(solverName, analysis=None):
    """CaeSolver Factory method, solverName string  "Calculix" "OpenFoam"
    """
    #obj = FreeCAD.ActiveDocument.addObject("App::CaeSolverPython", name)  #todo
    #solverName=FemGui.getActiveAnalysis().solverName  #ActiveAnalysis object is not constructed yet!
    if solverName in registered_solvers.keys():
        sinfo=registered_solvers[solverName]
        
        if solverName=='Calculix':
            import ccxFemSolver
            if analysis==None:
                solver=ccxFemSolver.Solver(FemGui.getActiveAnalysis()) #Should get fom Fem, not from GUI
            else:
                solver=ccxFemSolver.Solver(analysis)
            
        #FemGui.getActiveAnalysis().Proxy.solver=solver  #CaeAnalysis instance has no attribute 
        #print "makeCaeSolver() type of FemGui.getActiveAnalysis().Proxy.solver ", type(FemGui.getActiveAnalysis().Proxy.solver)
        
            FreeCAD.Console.PrintMessage("Solver {} is created\n".format(solverName))
            
        else:
            raise Exception('NonGui mode makeCaeSolver() is not implemented')            
        """
        if FreeCAD.GuiUp:
            #FreeCADGui.loadModule("FemGui")
            FreeCADGui.loadModule(sinfo["module"])
            FreeCADGui.doCommand("FemGui.getActiveAnalysis().Proxy.solver={}.Solver()"%sinfo["module"])
            #ViewProviderCaeSolver(obj.ViewObject)
        """
        return solver

