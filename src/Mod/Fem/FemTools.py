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

## \addtogroup FEM
#  @{

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
        self.fem_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/General")
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
    #  @param limit cutoff value. All values over the limit are treated as equal to the limit. Useful for filtering out hot spots.
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
        # [{'Object':materials_linear}, {}, ...]
        # [{'Object':materials_nonlinear}, {}, ...]
        # [{'Object':fixed_constraints, 'NodeSupports':bool}, {}, ...]
        # [{'Object':force_constraints, 'NodeLoad':value}, {}, ...
        # [{'Object':pressure_constraints, 'xxxxxxxx':value}, {}, ...]
        # [{'Object':temerature_constraints, 'xxxxxxxx':value}, {}, ...]
        # [{'Object':heatflux_constraints, 'xxxxxxxx':value}, {}, ...]
        # [{'Object':initialtemperature_constraints, 'xxxxxxxx':value}, {}, ...]
        # [{'Object':beam_sections, 'xxxxxxxx':value}, {}, ...]
        # [{'Object':fluid_sections, 'xxxxxxxx':value}, {}, ...]
        # [{'Object':shell_thicknesses, 'xxxxxxxx':value}, {}, ...]
        # [{'Object':contact_constraints, 'xxxxxxxx':value}, {}, ...]

        ## @var mesh
        #  mesh of the analysis. Used to generate .inp file and to show results
        self.mesh = None
        ## @var materials_linear
        # set of linear materials from the analysis. Updated with update_objects
        #  Individual materials are "App::MaterialObjectPython" type
        self.materials_linear = []
        ## @var materials_nonlinear
        # set of nonlinear materials from the analysis. Updated with update_objects
        #  Individual materials are Proxy.Type "FemMaterialMechanicalNonlinear"
        self.materials_nonlinear = []
        ## @var fixed_constraints
        #  set of fixed constraints from the analysis. Updated with update_objects
        #  Individual constraints are "Fem::ConstraintFixed" type
        self.fixed_constraints = []
        ## @var selfweight_constraints
        #  set of selfweight constraints from the analysis. Updated with update_objects
        #  Individual constraints are Proxy.Type "FemConstraintSelfWeight"
        self.selfweight_constraints = []
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
        # Individual beam sections are Proxy.Type "FemElementGeometry1D"
        self.beam_sections = []
        ## @var fluid_sections
        # set of fluid sections from the analysis. Updated with update_objects
        # Individual fluid sections are Proxy.Type "FemElementFluid1D"
        self.fluid_sections = []
        ## @var shell_thicknesses
        # set of shell thicknesses from the analysis. Updated with update_objects
        # Individual shell thicknesses are Proxy.Type "FemElementGeometry2D"
        self.shell_thicknesses = []
        ## @var displacement_constraints
        # set of displacements for the analysis. Updated with update_objects
        # Individual displacement_constraints are Proxy.Type "FemConstraintDisplacement"
        self.displacement_constraints = []
        ## @var temperature_constraints
        # set of temperatures for the analysis. Updated with update_objects
        # Individual temperature_constraints are Proxy.Type "FemConstraintTemperature"
        self.temperature_constraints = []
        ## @var heatflux_constraints
        # set of heatflux constraints for the analysis. Updated with update_objects
        # Individual heatflux_constraints are Proxy.Type "FemConstraintHeatflux"
        self.heatflux_constraints = []
        ## @var initialtemperature_constraints
        # set of initial temperatures for the analysis. Updated with update_objects
        # Individual initialTemperature_constraints are Proxy.Type "FemConstraintInitialTemperature"
        self.initialtemperature_constraints = []
        ## @var planerotation_constraints
        #  set of plane rotation constraints from the analysis. Updated with update_objects
        #  Individual constraints are "Fem::ConstraintPlaneRotation" type
        self.planerotation_constraints = []
        ## @var contact_constraints
        #  set of contact constraints from the analysis. Updated with update_objects
        #  Individual constraints are "Fem::ConstraintContact" type
        self.contact_constraints = []
        ## @var transform_constraints
        #  set of transform constraints from the analysis. Updated with update_objects
        #  Individual constraints are "Fem::ConstraintTransform" type
        self.transform_constraints = []

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
                material_linear_dict = {}
                material_linear_dict['Object'] = m
                self.materials_linear.append(material_linear_dict)
            elif hasattr(m, "Proxy") and m.Proxy.Type == "FemMaterialMechanicalNonlinear":
                material_nonlinear_dict = {}
                material_nonlinear_dict['Object'] = m
                self.materials_nonlinear.append(material_nonlinear_dict)
            elif m.isDerivedFrom("Fem::ConstraintFixed"):
                fixed_constraint_dict = {}
                fixed_constraint_dict['Object'] = m
                self.fixed_constraints.append(fixed_constraint_dict)
            elif hasattr(m, "Proxy") and m.Proxy.Type == "FemConstraintSelfWeight":
                selfweight_dict = {}
                selfweight_dict['Object'] = m
                self.selfweight_constraints.append(selfweight_dict)
            elif m.isDerivedFrom("Fem::ConstraintForce"):
                force_constraint_dict = {}
                force_constraint_dict['Object'] = m
                self.force_constraints.append(force_constraint_dict)
            elif m.isDerivedFrom("Fem::ConstraintPressure"):
                PressureObjectDict = {}
                PressureObjectDict['Object'] = m
                self.pressure_constraints.append(PressureObjectDict)
            elif m.isDerivedFrom("Fem::ConstraintDisplacement"):
                displacement_constraint_dict = {}
                displacement_constraint_dict['Object'] = m
                self.displacement_constraints.append(displacement_constraint_dict)
            elif m.isDerivedFrom("Fem::ConstraintTemperature"):
                temperature_constraint_dict = {}
                temperature_constraint_dict['Object'] = m
                self.temperature_constraints.append(temperature_constraint_dict)
            elif m.isDerivedFrom("Fem::ConstraintHeatflux"):
                heatflux_constraint_dict = {}
                heatflux_constraint_dict['Object'] = m
                self.heatflux_constraints.append(heatflux_constraint_dict)
            elif m.isDerivedFrom("Fem::ConstraintInitialTemperature"):
                initialtemperature_constraint_dict = {}
                initialtemperature_constraint_dict['Object'] = m
                self.initialtemperature_constraints.append(initialtemperature_constraint_dict)
            elif m.isDerivedFrom("Fem::ConstraintPlaneRotation"):
                planerotation_constraint_dict = {}
                planerotation_constraint_dict['Object'] = m
                self.planerotation_constraints.append(planerotation_constraint_dict)
            elif m.isDerivedFrom("Fem::ConstraintContact"):
                contact_constraint_dict = {}
                contact_constraint_dict['Object'] = m
                self.contact_constraints.append(contact_constraint_dict)
            elif m.isDerivedFrom("Fem::ConstraintTransform"):
                transform_constraint_dict = {}
                transform_constraint_dict['Object'] = m
                self.transform_constraints.append(transform_constraint_dict)
            elif hasattr(m, "Proxy") and m.Proxy.Type == "FemElementGeometry1D":
                beam_section_dict = {}
                beam_section_dict['Object'] = m
                self.beam_sections.append(beam_section_dict)
            elif hasattr(m, "Proxy") and m.Proxy.Type == "FemElementFluid1D":
                fluid_section_dict = {}
                fluid_section_dict['Object'] = m
                self.fluid_sections.append(fluid_section_dict)
            elif hasattr(m, "Proxy") and m.Proxy.Type == "FemElementGeometry2D":
                shell_thickness_dict = {}
                shell_thickness_dict['Object'] = m
                self.shell_thicknesses.append(shell_thickness_dict)

    def check_prerequisites(self):
        from FreeCAD import Units
        message = ""
        # analysis
        if not self.analysis:
            message += "No active Analysis\n"
        if self.analysis_type not in self.known_analysis_types:
            message += "Unknown analysis type: {}\n".format(self.analysis_type)
        if not self.working_dir:
            message += "Working directory not set\n"
        import os
        if not (os.path.isdir(self.working_dir)):
                message += "Working directory \'{}\' doesn't exist.".format(self.working_dir)
        # solver
        if not self.solver:
            message += "No solver object defined in the analysis\n"
        else:
            if self.analysis_type == "frequency":
                if not hasattr(self.solver, "EigenmodeHighLimit"):
                    message += "Frequency analysis: Solver has no EigenmodeHighLimit.\n"
                elif not hasattr(self.solver, "EigenmodeLowLimit"):
                    message += "Frequency analysis: Solver has no EigenmodeLowLimit.\n"
                elif not hasattr(self.solver, "EigenmodesCount"):
                    message += "Frequency analysis: Solver has no EigenmodesCount.\n"
            if hasattr(self.solver, "MaterialNonlinearity") and self.solver.MaterialNonlinearity == "nonlinear" and not self.materials_nonlinear:
                message += "Solver is set to nonlinear materials, but there is no nonlinear material in the analysis. \n"
        # mesh
        if not self.mesh:
            message += "No mesh object defined in the analysis\n"
        if self.mesh:
            if self.mesh.FemMesh.VolumeCount == 0 and self.mesh.FemMesh.FaceCount > 0 and not self.shell_thicknesses:
                message += "FEM mesh has no volume elements, either define a shell thicknesses or provide a FEM mesh with volume elements.\n"
            if self.mesh.FemMesh.VolumeCount == 0 and self.mesh.FemMesh.FaceCount == 0 and self.mesh.FemMesh.EdgeCount > 0 and not self.beam_sections and not self.fluid_sections:
                message += "FEM mesh has no volume and no shell elements, either define a beam/fluid section or provide a FEM mesh with volume elements.\n"
            if self.mesh.FemMesh.VolumeCount == 0 and self.mesh.FemMesh.FaceCount == 0 and self.mesh.FemMesh.EdgeCount == 0:
                message += "FEM mesh has neither volume nor shell or edge elements. Provide a FEM mesh with elements!\n"
        # materials linear and nonlinear
        if not self.materials_linear:
            message += "No material object defined in the analysis\n"
        has_no_references = False
        for m in self.materials_linear:
            if len(m['Object'].References) == 0:
                if has_no_references is True:
                    message += "More than one material has an empty references list (Only one empty references list is allowed!).\n"
                has_no_references = True
        for m in self.materials_linear:
            mat_map = m['Object'].Material
            mat_obj = m['Object']
            if mat_obj.Category == 'Solid':
                if 'YoungsModulus' in mat_map:
                    # print Units.Quantity(mat_map['YoungsModulus']).Value
                    if not Units.Quantity(mat_map['YoungsModulus']).Value:
                        message += "Value of YoungsModulus is set to 0.0.\n"
                else:
                    message += "No YoungsModulus defined for at least one material.\n"
                if 'PoissonRatio' not in mat_map:
                    message += "No PoissonRatio defined for at least one material.\n"  # PoissonRatio is allowed to be 0.0 (in ccx), but it should be set anyway.
            if self.analysis_type == "frequency" or self.selfweight_constraints:
                if 'Density' not in mat_map:
                    message += "No Density defined for at least one material.\n"
            if self.analysis_type == "thermomech":
                if 'ThermalConductivity' in mat_map:
                    if not Units.Quantity(mat_map['ThermalConductivity']).Value:
                        message += "Value of ThermalConductivity is set to 0.0.\n"
                else:
                    message += "Thermomechanical analysis: No ThermalConductivity defined for at least one material.\n"
                if 'ThermalExpansionCoefficient' not in mat_map:
                    message += "Thermomechanical analysis: No ThermalExpansionCoefficient defined for at least one material.\n"  # allowed to be 0.0 (in ccx)
                if 'SpecificHeat' not in mat_map:
                    message += "Thermomechanical analysis: No SpecificHeat defined for at least one material.\n"  # allowed to be 0.0 (in ccx)
        for m in self.materials_linear:
            has_nonlinear_material = False
            for nlm in self.materials_nonlinear:
                if nlm['Object'].LinearBaseMaterial == m['Object']:
                    if has_nonlinear_material is False:
                        has_nonlinear_material = True
                    else:
                        message += "At least two nonlinear materials use the same linear base material. Only one nonlinear material for each linear material allowed. \n"
        # constraints
        if self.analysis_type == "static":
            if not (self.fixed_constraints or self.displacement_constraints):
                message += "Static analysis: Neither constraint fixed nor constraint displacement defined.\n"
        # no check in the regard of loads (constraint force, pressure, self weight) is done because an analysis without loads at all is an valid analysis too
        if self.analysis_type == "thermomech":
            if not self.initialtemperature_constraints:
                if not self.fluid_sections:
                    message += "Thermomechanical analysis: No initial temperature defined.\n"
            if len(self.initialtemperature_constraints) > 1:
                message += "Thermomechanical analysis: Only one initial temperature is allowed.\n"
        # beam sections, fluid sections and shell thicknesses
        if self.beam_sections:
            if self.shell_thicknesses:
                # this needs to be checked only once either here or in shell_thicknesses
                message += "Beam Sections and shell thicknesses in one analysis is not supported at the moment.\n"
            if self.fluid_sections:
                # this needs to be checked only once either here or in shell_thicknesses
                message += "Beam Sections and Fluid Sections in one analysis is not supported at the moment.\n"
            has_no_references = False
            for b in self.beam_sections:
                if len(b['Object'].References) == 0:
                    if has_no_references is True:
                        message += "More than one beam section has an empty references list (Only one empty references list is allowed!).\n"
                    has_no_references = True
            if self.mesh:
                if self.mesh.FemMesh.FaceCount > 0 or self.mesh.FemMesh.VolumeCount > 0:
                    message += "Beam sections defined but FEM mesh has volume or shell elements.\n"
                if self.mesh.FemMesh.EdgeCount == 0:
                    message += "Beam sections defined but FEM mesh has no edge elements.\n"
        if self.shell_thicknesses:
            has_no_references = False
            for s in self.shell_thicknesses:
                if len(s['Object'].References) == 0:
                    if has_no_references is True:
                        message += "More than one shell thickness has an empty references list (Only one empty references list is allowed!).\n"
                    has_no_references = True
            if self.mesh:
                if self.mesh.FemMesh.VolumeCount > 0:
                    message += "Shell thicknesses defined but FEM mesh has volume elements.\n"
                if self.mesh.FemMesh.FaceCount == 0:
                    message += "Shell thicknesses defined but FEM mesh has no shell elements.\n"
        if self.fluid_sections:
            if not self.selfweight_constraints:
                message += "A fluid network analysis requires self weight constraint to be applied"
            if self.analysis_type != "thermomech":
                message += "A fluid network analysis can only be done in a thermomech analysis"
            has_no_references = False
            for f in self.fluid_sections:
                if len(f['Object'].References) == 0:
                    if has_no_references is True:
                        message += "More than one fluid section has an empty references list (Only one empty references list is allowed!).\n"
                    has_no_references = True
            if self.mesh:
                if self.mesh.FemMesh.FaceCount > 0 or self.mesh.FemMesh.VolumeCount > 0:
                    message += "Fluid sections defined but FEM mesh has volume or shell elements.\n"
                if self.mesh.FemMesh.EdgeCount == 0:
                    message += "Fluid sections defined but FEM mesh has no edge elements.\n"
        return message

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
    #  @param analysis_type type of the analysis.
    def set_analysis_type(self, analysis_type=None):
        if analysis_type is not None:
            self.analysis_type = analysis_type
        else:
            try:
                self.analysis_type = self.solver.AnalysisType
            except:
                self.fem_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/General")
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
            self.fem_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/General")
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
    #    Prin1 Principal stress 1
    #    Prin2 Principal stress 2
    #    Prin3 Principal stress 3
    #    MaxSear maximum shear stress
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
                         "MaxPrin": (m.Stats[15], m.Stats[16], m.Stats[17]),
                         "MidPrin": (m.Stats[18], m.Stats[19], m.Stats[20]),
                         "MinPrin": (m.Stats[21], m.Stats[22], m.Stats[23]),
                         "MaxShear": (m.Stats[24], m.Stats[25], m.Stats[26]),
                         "None": (0.0, 0.0, 0.0)}
                stats = match[result_type]
        return stats

##  @}
