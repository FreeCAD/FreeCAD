# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2015 - Przemo Firszt <przemo@firszt.eu>                 *
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


__title__ = "Fem Tools super class"
__author__ = "Przemo Firszt, Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"


import FreeCAD
from PySide import QtCore


class FemTools(QtCore.QRunnable, QtCore.QObject):
    ## The constructor
    #  @param analysis - analysis object to be used as the core object.
    #  "__init__" tries to use current active analysis in analysis is left empty.
    #  Rises exception if analysis is not set and there is no active analysis
    #  The constructur of FemTools is for use of analysis without solver object
    def __init__(self, analysis=None, solver=None):

        if analysis:
            ## @var analysis
            #  FEM analysis - the core object. Has to be present.
            #  It's set to analysis passed in "__init__" or set to current active analysis by default if nothing has been passed to "__init__".
            self.analysis = analysis
        else:
            import FemGui
            self.analysis = FemGui.getActiveAnalysis()
        if solver:
            ## @var solver
            #  solver of the analysis. Used to store the active solver and analysis parameters
            self.solver = solver
        else:
            self.solver = None
        if self.analysis:
            self.update_objects()
            self.results_present = False
            self.result_object = None
        else:
            raise Exception('FEM: No active analysis found!')

    ## Removes all result objects
    #  @param self The python object self
    def purge_results(self):
        for m in self.analysis.Member:
            if (m.isDerivedFrom('Fem::FemResultObject')):
                self.analysis.Document.removeObject(m.Name)
        self.results_present = False

    ## Resets mesh deformation
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

    ## Resets mesh color, deformation and removes all result objects if preferences to keep them is not set
    #  @param self The python object self
    def reset_mesh_purge_results_checked(self):
        self.fem_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem")
        keep_results_on_rerun = self.fem_prefs.GetBool("KeepResultsOnReRun", False)
        if not keep_results_on_rerun:
            self.purge_results()
        self.reset_mesh_color()
        self.reset_mesh_deformation()

    ## Resets mesh color, deformation and removes all result objects
    #  @param self The python object self
    def reset_all(self):
        self.purge_results()
        self.reset_mesh_color()
        self.reset_mesh_deformation()

    ## Sets mesh color using selected type of results (Sabs by default)
    #  @param self The python object self
    #  @param result_type Type of FEM result, allowed are:
    #  - U1, U2, U3 - deformation
    #  - Uabs - absolute deformation
    #  - Sabs - Von Mises stress
    #  @param limit cutoff value. All values over the limit are treated as equel to the limit. Useful for filtering out hot spots.
    def show_result(self, result_type="Sabs", limit=None):
        self.update_objects()
        if result_type == "None":
            self.reset_mesh_color()
            return
        if self.result_object:
            if FreeCAD.GuiUp:
                if self.result_object.Mesh.ViewObject.Visibility is False:
                    self.result_object.Mesh.ViewObject.Visibility = True
            if result_type == "Sabs":
                values = self.result_object.StressValues
            elif result_type == "Uabs":
                values = self.result_object.DisplacementLengths
            else:
                match = {"U1": 0, "U2": 1, "U3": 2}
                d = zip(*self.result_object.DisplacementVectors)
                values = list(d[match[result_type]])
            self.show_color_by_scalar_with_cutoff(values, limit)

    ## Sets mesh color using list of values. Internally used by show_result function.
    #  @param self The python object self
    #  @param values list of values
    #  @param limit cutoff value. All values over the limit are treated as equel to the limit. Useful for filtering out hot spots.
    def show_color_by_scalar_with_cutoff(self, values, limit=None):
        if limit:
            filtered_values = []
            for v in values:
                if v > limit:
                    filtered_values.append(limit)
                else:
                    filtered_values.append(v)
        else:
            filtered_values = values
        self.mesh.ViewObject.setNodeColorByScalars(self.result_object.NodeNumbers, filtered_values)

    def show_displacement(self, displacement_factor=0.0):
        self.mesh.ViewObject.setNodeDisplacementByVectors(self.result_object.NodeNumbers,
                                                          self.result_object.DisplacementVectors)
        self.mesh.ViewObject.applyDisplacement(displacement_factor)

    def update_objects(self):
        # [{'Object':materials}, {}, ...]
        # [{'Object':fixed_constraints, 'NodeSupports':bool}, {}, ...]
        # [{'Object':force_constraints, 'NodeLoad':value}, {}, ...
        # [{'Object':pressure_constraints, 'xxxxxxxx':value}, {}, ...]
        # [{'Object':beam_sections, 'xxxxxxxx':value}, {}, ...]
        # [{'Object':shell_thicknesses, 'xxxxxxxx':value}, {}, ...]

        ## @var mesh
        #  mesh of the analysis. Used to generate .inp file and to show results
        self.mesh = None
        ## @var materials
        # set of materials from the analysis. Updated with update_objects
        # Induvidual materials are "App::MaterialObjectPython" type
        self.materials = []
        ## @var fixed_constraints
        #  set of fixed constraints from the analysis. Updated with update_objects
        #  Individual constraints are "Fem::ConstraintFixed" type
        self.fixed_constraints = []
        ## @var force_constraints
        #  set of force constraints from the analysis. Updated with update_objects
        #  Individual constraints are "Fem::ConstraintForce" type
        self.force_constraints = []
        ## @var pressure_constraints
        #  set of pressure constraints from the analysis. Updated with update_objects
        #  Individual constraints are "Fem::ConstraintPressure" type
        self.pressure_constraints = []
        ## @var beam_sections
        # set of beam sections from the analysis. Updated with update_objects
        # Individual beam sections are Proxy.Type "FemBeamSection"
        self.beam_sections = []
        ## @var shell_thicknesses
        # set of shell thicknesses from the analysis. Updated with update_objects
        # Individual shell thicknesses are Proxy.Type "FemShellThickness"
        self.shell_thicknesses = []
        ## @var displacement_constraints
        # set of displacements for the analysis. Updated with update_objects
        # Individual displacement_constraints are Proxy.Type "FemConstraintDisplacement"
        self.displacement_constraints = []

        found_solver_for_use = False
        for m in self.analysis.Member:
            if m.isDerivedFrom("Fem::FemSolverObjectPython"):
                # for some methods no solver is needed (purge_results) --> solver could be none
                # analysis has one solver and no solver was set --> use the one solver
                # analysis has more than one solver and no solver was set --> use solver none
                # analysis has no solver --> use solver none
                if not found_solver_for_use and not self.solver:
                    # no solver was found before and no solver was set by constructor
                    self.solver = m
                    found_solver_for_use = True
                elif found_solver_for_use:
                    self.solver = None
                    # another solver was found --> We have more than one solver
                    # we do not know which one to use, so we use none !
                    # print('FEM: More than one solver in the analysis and no solver given to analys. No solver is set!')
            elif m.isDerivedFrom("Fem::FemMeshObject"):
                if not self.mesh:
                    self.mesh = m
                else:
                    raise Exception('FEM: Multiple mesh in analysis not yet supported!')
            elif m.isDerivedFrom("App::MaterialObjectPython"):
                material_dict = {}
                material_dict['Object'] = m
                self.materials.append(material_dict)
            elif m.isDerivedFrom("Fem::ConstraintFixed"):
                fixed_constraint_dict = {}
                fixed_constraint_dict['Object'] = m
                self.fixed_constraints.append(fixed_constraint_dict)
            elif m.isDerivedFrom("Fem::ConstraintForce"):
                force_constraint_dict = {}
                force_constraint_dict['Object'] = m
                self.force_constraints.append(force_constraint_dict)
            elif m.isDerivedFrom("Fem::ConstraintPressure"):
                PressureObjectDict = {}
                PressureObjectDict['Object'] = m
                self.pressure_constraints.append(PressureObjectDict)
            elif m.isDerivedFrom("Fem::ConstraintDisplacement"):  # OvG: Replacement reference to C++ implementation of Displacement Constraint
                displacement_constraint_dict = {}
                displacement_constraint_dict['Object'] = m
                self.displacement_constraints.append(displacement_constraint_dict)
            elif hasattr(m, "Proxy") and m.Proxy.Type == "FemBeamSection":
                beam_section_dict = {}
                beam_section_dict['Object'] = m
                self.beam_sections.append(beam_section_dict)
            elif hasattr(m, "Proxy") and m.Proxy.Type == "FemShellThickness":
                shell_thickness_dict = {}
                shell_thickness_dict['Object'] = m
                self.shell_thicknesses.append(shell_thickness_dict)

    def check_prerequisites(self):
        message = ""
        if not self.analysis:
            message += "No active Analysis\n"
        if self.analysis_type not in self.known_analysis_types:
            message += "Unknown analysis type: {}\n".format(self.analysis_type)
        if not self.working_dir:
            message += "Working directory not set\n"
        import os
        if not (os.path.isdir(self.working_dir)):
                message += "Working directory \'{}\' doesn't exist.".format(self.working_dir)
        if not self.mesh:
            message += "No mesh object defined in the analysis\n"
        if not self.materials:
            message += "No material object defined in the analysis\n"
        has_no_references = False
        for m in self.materials:
            if len(m['Object'].References) == 0:
                if has_no_references is True:
                    message += "More than one material has an empty references list (Only one empty references list is allowed!).\n"
                has_no_references = True
        if self.analysis_type == "static":
            if not (self.fixed_constraints or self.displacement_constraints):
                message += "Neither a constraint fixed nor a contraint displacement defined in the static analysis\n"
        if self.analysis_type == "static":
            if not (self.force_constraints or self.pressure_constraints):
                message += "Neither constraint force nor constraint pressure defined in the static analysis\n"
        if self.beam_sections:
            has_no_references = False
            for b in self.beam_sections:
                if len(b['Object'].References) == 0:
                    if has_no_references is True:
                        message += "More than one beam section has an empty references list (Only one empty references list is allowed!).\n"
                    has_no_references = True
        if self.shell_thicknesses:
            has_no_references = False
            for s in self.shell_thicknesses:
                if len(s['Object'].References) == 0:
                    if has_no_references is True:
                        message += "More than one shell thickness has an empty references list (Only one empty references list is allowed!).\n"
                    has_no_references = True
        return message

    ## Sets eigenmode parameters for CalculiX frequency analysis
    #  @param self The python object self
    #  @param number number of eigenmodes that wll be calculated, default read for FEM prefs or 10 if not set in the FEM prefs
    #  @param limit_low lower value of requested eigenfrequency range, default read for FEM prefs or 0.0 if not set in the FEM prefs
    #  @param limit_high higher value of requested eigenfrequency range, default read for FEM prefs or 1000000.o if not set in the FEM prefs
    def set_eigenmode_parameters(self, number=None, limit_low=None, limit_high=None):
        self.fem_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem")
        if number is not None:
            _number = number
        else:
            try:
                _number = self.solver.NumberOfEigenmodes
            except:
                #Not yet in prefs, so it will always default to 10
                _number = self.fem_prefs.GetInteger("NumberOfEigenmodes", 10)
        if _number < 1:
            _number = 1

        if limit_low is not None:
            _limit_low = limit_low
        else:
            try:
                _limit_low = self.solver.EigenmodeLowLimit
            except:
                #Not yet in prefs, so it will always default to 0.0
                _limit_low = self.fem_prefs.GetFloat("EigenmodeLowLimit", 0.0)

        if limit_high is not None:
            _limit_high = limit_high
        else:
            try:
                _limit_high = self.solver.EigenmodeHighLimit
            except:
                #Not yet in prefs, so it will always default to 1000000.0
                _limit_high = self.fem_prefs.GetFloat("EigenmodeHighLimit", 1000000.0)
        self.eigenmode_parameters = (_number, _limit_low, _limit_high)

    ## Sets base_name
    #  @param self The python object self
    #  @param base_name  base name of .inp/.frd file (without extension). It is used to construct .inp file path that is passed to CalculiX ccx
    def set_base_name(self, base_name=None):
        if base_name is None:
            self.base_name = ""
        else:
            self.base_name = base_name
        # Update inp file name
        self.set_inp_file_name()

    ## Sets inp file name that is used to determine location and name of frd result file.
    # Normally inp file name is set set by write_inp_file
    # Can be used to read mock calculations file
    #  @param self The python object self
    #  @inp_file_name .inp file name. If empty the .inp file path is constructed from working_dir, base_name and string ".inp"
    def set_inp_file_name(self, inp_file_name=None):
        if inp_file_name is not None:
            self.inp_file_name = inp_file_name
        else:
            self.inp_file_name = self.working_dir + '/' + self.base_name + '.inp'

    ## Sets analysis type.
    #  @param self The python object self
    #  @param analysis_type type of the analysis. Allowed values are:
    #  - static
    #  - frequency
    def set_analysis_type(self, analysis_type=None):
        if analysis_type is not None:
            self.analysis_type = analysis_type
        else:
            try:
                self.analysis_type = self.solver.AnalysisType
            except:
                self.fem_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem")
                self.analysis_type = self.fem_prefs.GetString("AnalysisType", "static")

    ## Sets working dir for solver execution. Called with no working_dir uses WorkingDir from FEM preferences
    #  @param self The python object self
    #  @working_dir directory to be used for writing solver input file or files and executing solver
    def setup_working_dir(self, working_dir=None):
        import os
        if working_dir is not None:
            self.working_dir = working_dir
        else:
            self.working_dir = ''
            self.fem_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem")
            if self.fem_prefs.GetString("WorkingDir"):
                try:
                    self.working_dir = self.fem_prefs.GetString("WorkingDir")
                except:
                    print('Could not set working directory to FEM Preferences working directory.')
            else:
                print('FEM preferences working dir is not set, the solver working directory is used.')
                if self.solver.WorkingDir:
                    try:
                        self.working_dir = self.solver.WorkingDir
                    except:
                        print('Could not set working directory to solver working directory.')

        if not (os.path.isdir(self.working_dir)):
            try:
                os.makedirs(self.working_dir)
            except:
                print("Dir \'{}\' doesn't exist and cannot be created.".format(self.working_dir))
                import tempfile
                self.working_dir = tempfile.gettempdir()
                print("Dir \'{}\' will be used instead.".format(self.working_dir))
        print('FemTools.setup_working_dir()  -->  self.working_dir = ' + self.working_dir)
        # Update inp file name
        self.set_inp_file_name()

    ## Set the analysis result object
    #  if no result object is provided, check if the analysis has result objects
    #  if the analysis has exact one result object use this result object
    #  @param self The python object self
    #  @param result object name
    def use_results(self, results_name=None):
        self.result_object = None
        if results_name is not None:
            for m in self.analysis.Member:
                if m.isDerivedFrom("Fem::FemResultObject") and m.Name == results_name:
                    self.result_object = m
                    break
            if not self.result_object:
                raise Exception("{} doesn't exist".format(results_name))
        else:
            has_results = False
            for m in self.analysis.Member:
                if m.isDerivedFrom("Fem::FemResultObject"):
                    self.result_object = m
                    if has_results is True:
                        self.result_object = None
                        raise Exception("No result name was provided, but more than one result objects in the analysis.")
                    has_results = True
            if not self.result_object:
                raise Exception("No result object found in the analysis")

    ## Returns minimum, average and maximum value for provided result type
    #  @param self The python object self
    #  @param result_type Type of FEM result, allowed are:
    #  - U1, U2, U3 - deformation
    #  - Uabs - absolute deformation
    #  - Sabs - Von Mises stress
    #  - None - always return (0.0, 0.0, 0.0)
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
