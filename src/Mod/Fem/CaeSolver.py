#***************************************************************************
#*   Copyright (c) 2015 - FreeCAD Developers                               *
#*   Author: Przemo Firszt <przemo@firszt.eu>                              *
#*                                                                         *
#*   This file is part of the FreeCAD CAx development system.              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   FreeCAD is distributed in the hope that it will be useful,            *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with FreeCAD; if not, write to the Free Software        *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************/

import os

from PreImport import *

#could be gen by python code by check subfolders
registered_solvers={
"Calculix":{ "name":"Calculix", "catogory":"Fem", "module": "ccxFemSolver","version": (2,3,4), "reader":"","writer":""}
}

#ViewProvider and CaeSolver DocumentObject should be gen

def makeCaeSolver(solverName):
    """CaeSolver Factory method, solverName string  "Calbulix"
    """
    #obj = FreeCAD.ActiveDocument.addObject("App::CaeSolverPython", name)  #todo
    #solverName=FemGui.getActiveAnalysis().solverName  #ActiveAnalysis object is not constructed yet!
    if solverName in registered_solvers.keys():
        sinfo=registered_solvers[solverName]
        
        import ccxFemSolver
        solver=ccxFemSolver.Solver(FemGui.getActiveAnalysis())
        #FemGui.getActiveAnalysis().Proxy.solver=solver  #CaeAnalysis instance has no attribute 
        #print "makeCaeSolver() type of FemGui.getActiveAnalysis().Proxy.solver ", type(FemGui.getActiveAnalysis().Proxy.solver)
        return solver
        """
        if FreeCAD.GuiUp:
            #FreeCADGui.loadModule("FemGui")
            FreeCADGui.loadModule(sinfo["module"])
            FreeCADGui.doCommand("FemGui.getActiveAnalysis().Proxy.solver={}.Solver()"%sinfo["module"])
            #ViewProviderCaeSolver(obj.ViewObject)
            
        else:
            raise Exception('NonGui mode makeCaeSolver() is not implemented')
        """
            

class CaeSolver(QtCore.QRunnable, QtCore.QObject):
    """base for CFD and FEM solver, it must have a run() method as a Runnable derived
    It also has the QProcess as member to run the external solver
    """
    started = QtCore.Signal()  
    finished = QtCore.Signal(int)  #why declare here?
    #stateChanged=QtCore.Signal(QtCore.QProcess.ProcessState)
    #hasError= QtCore.Signal(QtCore.QProcess.ProcessError) 
    
    def __init__(self):
        QtCore.QRunnable.__init__(self)
        QtCore.QObject.__init__(self)
        
        self.name="BaseCaeSolver"
        self.minVersion=()
        self.case_file_name = "" #
        self.results_present = False
        
        #self.process_object=QtCore.QProcess()
        self.catogory="Fem"
        self.prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/"+self.catogory)
        #
        #setup_solver() should be called during construction of derived solver!
        
    def check_prerequisites(self):
        message = ""
        if not self.analysis:
            message += "No active Analysis\n"
        if not self.mesh:
            message += "No mesh object in the Analysis\n"
        if not self.material:  #material for fluid is not done yet
            message += "No material object in the Analysis\n"
        
        #boundary condition or constraint is solver specific 
        return message
        
    def setup_solver(self):
        """ call in derived class after all parameters are set, followed by solver specific setup
        #this class need not know about analysis, so it work in nonGui mode
        """
        self.setup_working_dir()
        self.setup_solver_binary_path()
        self.update_objects() #implemented in derived class, this should be call before write_case_file, need to reset_all()
        """
        if analysis != None: # other test like type
            self.analysis=analysis
        else:
            raise Exception('No active analysis found!')
        """
            
            
    ################# property set and get ################    
    def set_case_file_name(self, case_file_name=None):
        """this is case name for solver to identify setup, could be folder name
        """
        if case_file_name is None:
            self.case_file_name = ""
        else:
            self.case_file_name = case_file_name

    def setup_working_dir(self, working_dir=None):
        if working_dir is None:
            self.working_dir = self.prefs.GetString("WorkingDir", "/tmp")
        else:
            #should give more testing here, especially for nonGui mode
            self.working_dir = working_dir
            
    def start_solver(self):
        """ p.communicate(), p.returncode  are main IPC methods.
        IPC could be improved if there is better IPC standard
        """
        import subprocess
        if self.case_file_name != "":  #base case name, it is a dir for OpenFOAM
            # change cwd because ccx may crash if directory has no write permission
            # there is also a limit of the length of file names so jump to the document directory
            cwd = QtCore.QDir.currentPath()
            f = QtCore.QFileInfo(self.case_file_name)
            QtCore.QDir.setCurrent(f.path())
            
            self.set_solver_env(self.parallel)
            #is that possible to replace Popen with QProcess()
            FreeCAD.Console.PrintMessage("Debug info: start Popen: {} \n".format(self.solver_command_string))
            p = subprocess.Popen(self.solver_command_string,
                                 stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                 shell=False, env=self._env)
            self.solver_stdout, self.solver_stderr = p.communicate() #
            
            self.set_solver_env(True)
            QtCore.QDir.setCurrent(cwd) # change back curent working dir
            
            return p.returncode
        else:
            FreeCAD.Console.PrintMessage("Error: solver's case file name does not exist!");  
            return -1
 
    def run(self):
        """ QRunnable, should work without GUI
        """
        ret_code = 0
        message = self.check_prerequisites()
        if not message:
            self.write_case_file()
            from FreeCAD import Base
            progress_bar = Base.ProgressIndicator() # Does this work in both cmd and gui mode?
            progress_bar.start("Running Solver "+self.case_file_name, 0)
            
            self.started.emit()
            ret_code = self.start_solver() #
            self.finished.emit(ret_code)
            
            progress_bar.stop()
        else:
            print "Running analysis failed! " + message
        if ret_code or self.solver_stderr:
            print "Analysis failed with exit code {}".format(ret_code)
            print "--------start of stderr-------"
            print self.solver_stderr
            print "--------end of stderr---------"
            print "--------start of stdout-------"
            print self.solver_stdout
            print "--------end of stdout---------"           
    
    ## Resets mesh color, deformation and removes all result objects
    def reset_all(self):
        self.purge_results()
        self.reset_mesh_color()
        self.reset_mesh_deformation()      
        
        
class ViewProviderCaeSolver:
    """A View Provider for the CaeSolver object, base class for all derived solver
    derived solver should implement  a specific TaskPanel and set up solver and override setEdit()"""

    def __init__(self, vobj):
        vobj.Proxy = self

    def getIcon(self):
        return ":/icons/fem-material.svg"

    def attach(self, vobj):
        self.ViewObject = vobj
        self.Object = vobj.Object

    def updateData(self, obj, prop):
        return

    def onChanged(self, vobj, prop):
        return

    def setEdit(self, vobj, mode):
        #taskd = _MechanicalMaterialTaskPanel(self.Object) #Todo
        #taskd.obj = vobj.Object
        #FreeCADGui.Control.showDialog(taskd)
        return True

    def unsetEdit(self, vobj, mode):
        #FreeCADGui.Control.closeDialog() #Todo
        return

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None