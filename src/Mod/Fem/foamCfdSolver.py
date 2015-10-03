#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2015 - Przemo Firszt <przemo@firszt.eu>                 *
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
      
import os
import FreeCAD
from PySide import QtCore
#
from CaeSolver import CaeSolver
            
class FoamCfdSolver(CaeSolver):
    """ Using OpenFoam solver derived from base class:  CaeSolver
    This class focus on mesh and boundary condition setup, 
    attach to existed case but modify mesh should be possible
    """
    
    def __init__(self,analysis=None):
        super(self.__class__, self).__init__()
        
        self.analysis = FemGui.getActiveAnalysis()
        self.category="CFD"
        self.name="OpenFoam"
        self.version=(2.3.0)
        self.parallel=False

        self.known_analysis_types = ["icoFoam", ""] #specific solver type

        self.prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/"+self.catogory+"/"+self.name)
        
        setup_analysis(analysis)
        
        
    def check_prerequisites(self):
        """analysis and solver check"""
        message = super(self.__class__, self).check_prerequisites()
        
        #if not self.fixed_constraints:
        #    message += "No fixed-constraint nodes defined in the Analysis\n"
        #if self.analysis_type == "static":
        #    if not (self.force_constraints or self.pressure_constraints):
        #        message += "No force-constraint or pressure-constraint defined in the Analysis\n"
        
        return message
        
    def set_analysis_type(self, analysis_type=None):
        if analysis_type is None:
            self.analysis_type = "icoFoam"
        else:
            self.analysis_type = analysis_type

    def setup_solver(self, solver_name=None):
        """ if not specify, setup binary solver name from prefs a default one
        """
        if not solver_name:
            solver_name = self.prefs.GetString("solverBinaryPath", "icoFoam")
        if not solver_name: #set a default solver binary
            from platform import system
            if system() == "Linux":
                solver_binary = solver_name
            elif system() == "Windows":
                solver_binary = FreeCAD.getHomePath() + solver_name+".exe"
            else:
                solver_binary = solver_name
        self.solver_binary = solver_binary
        
        
    def write_case_file(self):
        """conversion of mesh could happen here, need progressbar for call external program
        """
        import ccxInpWriter as iw
        import sys
        self.base_name = ""
        try:
            inp_writer = iw.inp_writer(self.analysis, self.mesh, self.material,
                                       self.fixed_constraints, self.force_constraints,
                                       self.pressure_constraints, self.analysis_type,
                                       self.eigenmode_parameters, self.working_dir)
            self.base_name = inp_writer.write_calculix_input_file()
        except:
            print "Unexpected error when writing CalculiX input file:", sys.exc_info()[0]
            raise
            
    def start_ext_editor(self, ext_editor_path, filename):
        if not hasattr(self, "ext_editor_process"):
            self.ext_editor_process = QtCore.QProcess()
        if self.ext_editor_process.state() != QtCore.QProcess.Running:
            self.ext_editor_process.start(ext_editor_path, [filename])
    
    #
    def editSolverInputFile(self):
        """ FEM Calculix or Abaqus specific input file editing
        """
        filename = self.base_name + '.inp'
        print 'editSolverInputFile {}'.format(filename)
        if self.fem_prefs.GetBool("UseInternalEditor", True):
            FemGui.open(filename)
        else:
            ext_editor_path = self.fem_prefs.GetString("ExternalEditorPath", "")
            if ext_editor_path:
                self.start_ext_editor(ext_editor_path, filename)
            else:
                print "External editor is not defined in FEM preferences. Falling back to internal editor"
                FemGui.open(filename)
                
    def set_solver_env(self):
        """solver specific env variable setup, this 
        """           
        if self.parallel:
            import multiprocessing
            self._ont_backup = os.environ.get('OMP_NUM_THREADS') #OPEM-MPI
            if not self._ont_backup:
                self.ont_backup = ""
            self._env = os.putenv('OMP_NUM_THREADS', str(multiprocessing.cpu_count()))
        else:
            self._env=None #
        
    def unset_solver_env(self):
        if self.parallel:
            os.putenv('OMP_NUM_THREADS', self._ont_backup)

    ##########################################################
    def load_results(self, solverTime=0.0):
        import foamResultReader
        #import os
        self.results_present = False
        #need to generate result file first,
        
        result_file = self.working_dir + '/' + self.base_name + ".frd"
        if os.path.isfile(result_file):
            foamResultReader.importFrd(result_file, self.analysis) #
            for m in self.analysis.Member:
                if m.isDerivedFrom("Fem::FemResultObject"): #<FemToCae>
                    self.result_object = m
            if self.result_object is not None:
                self.results_present = True
        else:
            raise Exception(__name__ + 'No results found at {}!'.format(result_file))

    def use_results(self, results_name=None):
        for m in self.analysis.Member:
            if m.isDerivedFrom("Fem::FemResultObject") and m.Name == results_name:
                self.result_object = m
                break
        if not self.result_object:
            raise ("{} doesn't exist".format(results_name))


    ## returns minimum, average and maximum value for provided result type
    #  @param self The python object self
    #  @result_type Type of FEM result, allowed U1, U2, U3, Uabs, Sabs and None
    def get_stats(self, result_type):
        stats = (0.0, 0.0, 0.0)
        for m in self.analysis.Member:
            if m.isDerivedFrom("Fem::FemResultObject"):
                match = {"U1": (m.Stats[0], m.Stats[1], m.Stats[2]),
                         "U2": (m.Stats[3], m.Stats[4], m.Stats[5]),
                         "U3": (m.Stats[6], m.Stats[7], m.Stats[8]),
                         "Uabs": (m.Stats[9], m.Stats[10], m.Stats[11]),
                         "Sabs": (m.Stats[12], m.Stats[13], m.Stats[14]),
                         "None": (0.0, 0.0, 0.0)}
                stats = match[result_type]
        return stats
        
        
    def show_result(self, result_type="Sabs", limit=None):
        """ solver specific result field
        Sabs (Stress), Uabs (Displacement), U1, U2,U3 (displacement vector)
        """
        self.update_objects()
        if result_type == "None":
            self.reset_mesh_color()
            return
        if self.result_object:
            if result_type == "Sabs":
                values = self.result_object.StressValues
            elif result_type == "Uabs":
                values = self.result_object.DisplacementLengths
            else:
                match = {"U1": 0, "U2": 1, "U3": 2}
                d = zip(*self.result_object.DisplacementVectors)
                values = list(d[match[result_type]])
            self.show_scalar_color_by_with_cutoff(values, limit)

    def show_displacement(self, displacement_factor=0.0):
        self.mesh.ViewObject.setNodeDisplacementByVectors(self.result_object.ElementNumbers,
                                                          self.result_object.DisplacementVectors)
        self.mesh.ViewObject.applyDisplacement(displacement_factor)
        

################################################
# Removes all result objects,  define only one API  reset_all() to clean analysis
    #  @param self The python object self
    def purge_results(self):
        for m in self.analysis.Member:
            if (m.isDerivedFrom('Fem::FemResultObject')):  # <FemToCae> this CFD module still using FEM object to render 
                FreeCAD.ActiveDocument.removeObject(m.Name)
        self.results_present = False

    ## Resets mesh deformation, 
    #  @param self The python object self
    def reset_mesh_deformation(self):
        if self.mesh:
            self.mesh.ViewObject.applyDisplacement(0.0)

    ## Resets mesh color
    #  @param self The python object self
    def reset_mesh_color(self):
        if self.mesh:
            self.mesh.ViewObject.NodeColor = {}
            self.mesh.ViewObject.ElementColor = {}
            self.mesh.ViewObject.setNodeColorByScalars()




    