# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2015 - Przemo Firszt <przemo@firszt.eu>                 *
# *   Copyright (c) 2016 - Bernd Hahnebach <bernd@bimstatik.org>            *
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

__title__ = "FemToolsCcx"
__author__ = "Przemo Firszt, Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## \addtogroup FEM
#  @{

import sys
import FreeCAD
from PySide import QtCore
if FreeCAD.GuiUp:
    from PySide import QtGui


class FemToolsCcx(QtCore.QRunnable, QtCore.QObject):

    known_analysis_types = ["static", "frequency", "thermomech"]
    finished = QtCore.Signal(int)

    ## The constructor
    #  @param analysis - analysis object to be used as the core object.
    #  @param test_mode - True indicates that no real calculations will take place, so ccx binary is not required. Used by test module.
    #  "__init__" tries to use current active analysis in analysis is left empty.
    #  Rises exception if analysis is not set and there is no active analysis
    def __init__(self, analysis=None, solver=None, test_mode=False):
        QtCore.QRunnable.__init__(self)
        QtCore.QObject.__init__(self)
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
            ## @var base_name
            #  base name of .inp/.frd file (without extension). It is used to construct .inp file path that is passed to CalculiX ccx
            self.base_name = ""
            ## @var results_present
            #  boolean variable indicating if there are calculation results ready for use
            self.results_present = False
            if self.solver:
                self.setup_working_dir()
            else:
                raise Exception('FEM: No solver found!')
            if test_mode:
                self.ccx_binary_present = True
            else:
                self.ccx_binary_present = False
                self.setup_ccx()
            self.result_object = None
        else:
            raise Exception('FEM: No active analysis found!')

    ## Removes all result objects
    #  @param self The python object self
    def purge_results(self):
        for m in self.analysis.Group:
            if (m.isDerivedFrom('Fem::FemResultObject')):
                if m.Mesh and hasattr(m.Mesh, "Proxy") and m.Mesh.Proxy.Type == "Fem::FemMeshResult":
                    self.analysis.Document.removeObject(m.Mesh.Name)
                self.analysis.Document.removeObject(m.Name)
        FreeCAD.ActiveDocument.recompute()

    ## Resets mesh color, deformation and removes all result objects if preferences to keep them is not set
    #  @param self The python object self
    def reset_mesh_purge_results_checked(self):
        self.fem_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/General")
        keep_results_on_rerun = self.fem_prefs.GetBool("KeepResultsOnReRun", False)
        if not keep_results_on_rerun:
            self.purge_results()

    ## Resets mesh color, deformation and removes all result objects
    #  @param self The python object self
    def reset_all(self):
        self.purge_results()

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
        # [{'Object':beam_rotations, 'xxxxxxxx':value}, {}, ...]
        # [{'Object':fluid_sections, 'xxxxxxxx':value}, {}, ...]
        # [{'Object':shell_thicknesses, 'xxxxxxxx':value}, {}, ...]
        # [{'Object':contact_constraints, 'xxxxxxxx':value}, {}, ...]

        ## @var mesh
        #  mesh of the analysis. Used to generate .inp file and to show results
        self.mesh = None
        ## @var materials_linear
        #  list of linear materials from the analysis. Updated with update_objects
        self.materials_linear = []
        ## @var materials_nonlinear
        #  list of nonlinear materials from the analysis. Updated with update_objects
        self.materials_nonlinear = []
        ## @var fixed_constraints
        #  list of fixed constraints from the analysis. Updated with update_objects
        self.fixed_constraints = []
        ## @var selfweight_constraints
        #  list of selfweight constraints from the analysis. Updated with update_objects
        self.selfweight_constraints = []
        ## @var force_constraints
        #  list of force constraints from the analysis. Updated with update_objects
        self.force_constraints = []
        ## @var pressure_constraints
        #  list of pressure constraints from the analysis. Updated with update_objects
        self.pressure_constraints = []
        ## @var beam_sections
        # list of beam sections from the analysis. Updated with update_objects
        self.beam_sections = []
        ## @var beam_rotations
        # list of beam rotations from the analysis. Updated with update_objects
        self.beam_rotations = []
        ## @var fluid_sections
        # list of fluid sections from the analysis. Updated with update_objects
        self.fluid_sections = []
        ## @var shell_thicknesses
        # list of shell thicknesses from the analysis. Updated with update_objects
        self.shell_thicknesses = []
        ## @var displacement_constraints
        # list of displacements for the analysis. Updated with update_objects
        self.displacement_constraints = []
        ## @var temperature_constraints
        # list of temperatures for the analysis. Updated with update_objects
        self.temperature_constraints = []
        ## @var heatflux_constraints
        # list of heatflux constraints for the analysis. Updated with update_objects
        self.heatflux_constraints = []
        ## @var initialtemperature_constraints
        # list of initial temperatures for the analysis. Updated with update_objects
        self.initialtemperature_constraints = []
        ## @var planerotation_constraints
        #  list of plane rotation constraints from the analysis. Updated with update_objects
        self.planerotation_constraints = []
        ## @var contact_constraints
        #  list of contact constraints from the analysis. Updated with update_objects
        self.contact_constraints = []
        ## @var transform_constraints
        #  list of transform constraints from the analysis. Updated with update_objects
        self.transform_constraints = []

        found_solver_for_use = False
        for m in self.analysis.Group:
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
                    # FreeCAD.Console.PrintMessage('FEM: More than one solver in the analysis and no solver given to analys. No solver is set!\n')
            elif m.isDerivedFrom("Fem::FemMeshObject"):
                if not self.mesh:
                    self.mesh = m
                else:
                    message = 'FEM: Multiple mesh in analysis not yet supported!'
                    if FreeCAD.GuiUp:
                        QtGui.QMessageBox.critical(None, "Missing prerequisite", message)
                    raise Exception(message + '\n')
            elif m.isDerivedFrom("App::MaterialObjectPython"):
                material_linear_dict = {}
                material_linear_dict['Object'] = m
                self.materials_linear.append(material_linear_dict)
            elif hasattr(m, "Proxy") and m.Proxy.Type == "Fem::MaterialMechanicalNonlinear":
                material_nonlinear_dict = {}
                material_nonlinear_dict['Object'] = m
                self.materials_nonlinear.append(material_nonlinear_dict)
            elif m.isDerivedFrom("Fem::ConstraintFixed"):
                fixed_constraint_dict = {}
                fixed_constraint_dict['Object'] = m
                self.fixed_constraints.append(fixed_constraint_dict)
            elif hasattr(m, "Proxy") and m.Proxy.Type == "Fem::ConstraintSelfWeight":
                selfweight_dict = {}
                selfweight_dict['Object'] = m
                self.selfweight_constraints.append(selfweight_dict)
            elif m.isDerivedFrom("Fem::ConstraintForce"):
                force_constraint_dict = {}
                force_constraint_dict['Object'] = m
                force_constraint_dict['RefShapeType'] = get_refshape_type(m)
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
            elif hasattr(m, "Proxy") and m.Proxy.Type == "Fem::FemElementGeometry1D":
                beam_section_dict = {}
                beam_section_dict['Object'] = m
                self.beam_sections.append(beam_section_dict)
            elif hasattr(m, "Proxy") and m.Proxy.Type == "Fem::FemElementRotation1D":
                beam_rotation_dict = {}
                beam_rotation_dict['Object'] = m
                self.beam_rotations.append(beam_rotation_dict)
            elif hasattr(m, "Proxy") and m.Proxy.Type == "Fem::FemElementFluid1D":
                fluid_section_dict = {}
                fluid_section_dict['Object'] = m
                self.fluid_sections.append(fluid_section_dict)
            elif hasattr(m, "Proxy") and m.Proxy.Type == "Fem::FemElementGeometry2D":
                shell_thickness_dict = {}
                shell_thickness_dict['Object'] = m
                self.shell_thicknesses.append(shell_thickness_dict)

    def check_prerequisites(self):
        from FreeCAD import Units
        message = ""
        # analysis
        if not self.analysis:
            message += "No active Analysis\n"
        if not self.working_dir:
            message += "Working directory not set\n"
        import os
        if not (os.path.isdir(self.working_dir)):
                message += "Working directory \'{}\' doesn't exist.".format(self.working_dir)
        # solver
        if not self.solver:
            message += "No solver object defined in the analysis\n"
        else:
            if self.solver.AnalysisType not in self.known_analysis_types:
                message += "Unknown analysis type: {}\n".format(self.solver.AnalysisType)
            if self.solver.AnalysisType == "frequency":
                if not hasattr(self.solver, "EigenmodeHighLimit"):
                    message += "Frequency analysis: Solver has no EigenmodeHighLimit.\n"
                elif not hasattr(self.solver, "EigenmodeLowLimit"):
                    message += "Frequency analysis: Solver has no EigenmodeLowLimit.\n"
                elif not hasattr(self.solver, "EigenmodesCount"):
                    message += "Frequency analysis: Solver has no EigenmodesCount.\n"
            if hasattr(self.solver, "MaterialNonlinearity") and self.solver.MaterialNonlinearity == "nonlinear":
                if not self.materials_nonlinear:
                    message += "Solver is set to nonlinear materials, but there is no nonlinear material in the analysis.\n"
                if self.solver.Proxy.Type == 'Fem::FemSolverCalculixCcxTools' and self.solver.GeometricalNonlinearity != "nonlinear":
                    # nonlinear geometry --> should be set https://forum.freecadweb.org/viewtopic.php?f=18&t=23101&p=180489#p180489
                    message += "Solver CalculiX triggers nonlinear geometry for nonlinear material, thus it should to be set too.\n"
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
        # material linear and nonlinear
        if not self.materials_linear:
            message += "No material object defined in the analysis\n"
        has_no_references = False
        for m in self.materials_linear:
            if len(m['Object'].References) == 0:
                if has_no_references is True:
                    message += "More than one material has an empty references list (Only one empty references list is allowed!).\n"
                has_no_references = True
        mat_ref_shty = ''
        for m in self.materials_linear:
            ref_shty = get_refshape_type(m['Object'])
            if not mat_ref_shty:
                mat_ref_shty = ref_shty
            if mat_ref_shty and ref_shty and ref_shty != mat_ref_shty:  # mat_ref_shty could be empty in one material, only the not empty ones should have the same shape type
                message += 'Some material objects do not have the same reference shape type (all material objects must have the same reference shape type, at the moment).\n'
        for m in self.materials_linear:
            mat_map = m['Object'].Material
            mat_obj = m['Object']
            if mat_obj.Category == 'Solid':
                if 'YoungsModulus' in mat_map:
                    # print(Units.Quantity(mat_map['YoungsModulus']).Value)
                    if not Units.Quantity(mat_map['YoungsModulus']).Value:
                        message += "Value of YoungsModulus is set to 0.0.\n"
                else:
                    message += "No YoungsModulus defined for at least one material.\n"
                if 'PoissonRatio' not in mat_map:
                    message += "No PoissonRatio defined for at least one material.\n"  # PoissonRatio is allowed to be 0.0 (in ccx), but it should be set anyway.
            if self.solver.AnalysisType == "frequency" or self.selfweight_constraints:
                if 'Density' not in mat_map:
                    message += "No Density defined for at least one material.\n"
            if self.solver.AnalysisType == "thermomech":
                if 'ThermalConductivity' in mat_map:
                    if not Units.Quantity(mat_map['ThermalConductivity']).Value:
                        message += "Value of ThermalConductivity is set to 0.0.\n"
                else:
                    message += "Thermomechanical analysis: No ThermalConductivity defined for at least one material.\n"
                if 'ThermalExpansionCoefficient' not in mat_map and mat_obj.Category == 'Solid':
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
                        message += "At least two nonlinear materials use the same linear base material. Only one nonlinear material for each linear material allowed.\n"
        # which analysis needs which constraints
        # no check in the regard of loads existence (constraint force, pressure, self weight) is done because an analysis without loads at all is an valid analysis too
        if self.solver.AnalysisType == "static":
            if not (self.fixed_constraints or self.displacement_constraints):
                message += "Static analysis: Neither constraint fixed nor constraint displacement defined.\n"
        if self.solver.AnalysisType == "thermomech":
            if not self.initialtemperature_constraints:
                if not self.fluid_sections:
                    message += "Thermomechanical analysis: No initial temperature defined.\n"
            if len(self.initialtemperature_constraints) > 1:
                message += "Thermomechanical analysis: Only one initial temperature is allowed.\n"
        # constraints
        # fixed
        if self.fixed_constraints:
            for c in self.fixed_constraints:
                if len(c['Object'].References) == 0:
                    message += "At least one constraint fixed has an empty reference.\n"
        # displacement
        if self.displacement_constraints:
            for di in self.displacement_constraints:
                if len(di['Object'].References) == 0:
                    message += "At least one constraint displacement has an empty reference.\n"
        # plane rotation
        if self.planerotation_constraints:
            for c in self.planerotation_constraints:
                if len(c['Object'].References) == 0:
                    message += "At least one constraint plane rotation has an empty reference.\n"
        # contact
        if self.contact_constraints:
            for c in self.contact_constraints:
                if len(c['Object'].References) == 0:
                    message += "At least one constraint contact has an empty reference.\n"
        # transform
        if self.transform_constraints:
            for c in self.transform_constraints:
                if len(c['Object'].References) == 0:
                    message += "At least one constraint transform has an empty reference.\n"
        # pressure
        if self.pressure_constraints:
            for c in self.pressure_constraints:
                if len(c['Object'].References) == 0:
                    message += "At least one constraint pressure has an empty reference.\n"
        # force
        if self.force_constraints:
            for c in self.force_constraints:
                if len(c['Object'].References) == 0:
                    message += "At least one constraint force has an empty reference.\n"
        # temperature
        if self.temperature_constraints:
            for c in self.temperature_constraints:
                if len(c['Object'].References) == 0:
                    message += "At least one constraint temperature has an empty reference.\n"
        # heat flux
        if self.heatflux_constraints:
            for c in self.heatflux_constraints:
                if len(c['Object'].References) == 0:
                    message += "At least one constraint heat flux has an empty reference.\n"
        # beam section
        if self.beam_sections:
            if self.shell_thicknesses:
                # this needs to be checked only once either here or in shell_thicknesses
                message += "Beam sections and shell thicknesses in one analysis is not supported at the moment.\n"
            if self.fluid_sections:
                # this needs to be checked only once either here or in shell_thicknesses
                message += "Beam sections and fluid sections in one analysis is not supported at the moment.\n"
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
            if len(self.beam_rotations) > 1:
                message += "Multiple beam rotations in one analysis are not supported at the moment.\n"
        # beam rotations
        if self.beam_rotations and not self.beam_sections:
            message += "Beam rotations in the analysis but no beam sections defined.\n"
        # shell thickness
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
        # fluid section
        if self.fluid_sections:
            if not self.selfweight_constraints:
                message += "A fluid network analysis requires self weight constraint to be applied"
            if self.solver.AnalysisType != "thermomech":
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
    #  @param base_name base name of .inp/.frd file (without extension). It is used to construct .inp file path that is passed to CalculiX ccx
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
            # self.working_dir does have a slash at the end
            self.inp_file_name = self.working_dir + self.base_name + '.inp'

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
                    FreeCAD.Console.PrintError('Could not set working directory to FEM Preferences working directory.\n')
            else:
                FreeCAD.Console.PrintMessage('FEM preferences working dir is not set, the solver working directory is used.\n')
                if self.solver.WorkingDir:
                    try:
                        self.working_dir = self.solver.WorkingDir
                    except:
                        FreeCAD.Console.PrintError('Could not set working directory to solver working directory.\n')

        # check working_dir has a slash at the end, if not add one
        self.working_dir = os.path.join(self.working_dir, '')

        if not (os.path.isdir(self.working_dir)):
            try:
                os.makedirs(self.working_dir)
            except:
                FreeCAD.Console.PrintError("Dir \'{}\' doesn't exist and cannot be created.\n".format(self.working_dir))
                import tempfile
                self.working_dir = tempfile.gettempdir()
                FreeCAD.Console.PrintMessage("Dir \'{}\' will be used instead.\n".format(self.working_dir))
        FreeCAD.Console.PrintMessage('FemToolsCCx.setup_working_dir()  -->  self.working_dir = ' + self.working_dir + '\n')
        # Update inp file name
        self.set_inp_file_name()

    def write_inp_file(self):
        import femsolver.calculix.writer as iw
        import sys
        self.inp_file_name = ""
        try:
            inp_writer = iw.FemInputWriterCcx(
                self.analysis, self.solver,
                self.mesh, self.materials_linear, self.materials_nonlinear,
                self.fixed_constraints, self.displacement_constraints,
                self.contact_constraints, self.planerotation_constraints, self.transform_constraints,
                self.selfweight_constraints, self.force_constraints, self.pressure_constraints,
                self.temperature_constraints, self.heatflux_constraints, self.initialtemperature_constraints,
                self.beam_sections, self.beam_rotations, self.shell_thicknesses, self.fluid_sections,
                self.working_dir)
            self.inp_file_name = inp_writer.write_calculix_input_file()
        except:
            FreeCAD.Console.PrintError("Unexpected error when writing CalculiX input file: {}\n".format(sys.exc_info()[0]))
            raise

    ## Sets CalculiX ccx binary path and validates if the binary can be executed
    #  @param self The python object self
    #  @ccx_binary path to ccx binary, default is guessed: "bin/ccx" windows, "ccx" for other systems
    #  @ccx_binary_sig expected output form ccx when run empty. Default value is "CalculiX.exe -i jobname"
    def setup_ccx(self, ccx_binary=None, ccx_binary_sig="CalculiX"):
        error_title = "No CalculiX binary ccx"
        error_message = ""
        from platform import system
        ccx_std_location = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/Ccx").GetBool("UseStandardCcxLocation", True)
        if ccx_std_location:
            if system() == "Windows":
                ccx_path = FreeCAD.getHomePath() + "bin/ccx.exe"
                FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/Ccx").SetString("ccxBinaryPath", ccx_path)
                self.ccx_binary = ccx_path
            elif system() == "Linux":
                import subprocess
                p1 = subprocess.Popen(['which', 'ccx'], stdout=subprocess.PIPE)
                if p1.wait() == 0:
                    if sys.version_info.major >= 3:
                        ccx_path = str(p1.stdout.read()).split('\n')[0]
                    else:
                        ccx_path = p1.stdout.read().split('\n')[0]
                elif p1.wait() == 1:
                    error_message = "FEM: CalculiX binary ccx not found in standard system binary path. Please install ccx or set path to binary in FEM preferences tab CalculiX.\n"
                    if FreeCAD.GuiUp:
                        QtGui.QMessageBox.critical(None, error_title, error_message)
                    raise Exception(error_message)
                self.ccx_binary = ccx_path
        else:
            if not ccx_binary:
                self.ccx_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/Ccx")
                ccx_binary = self.ccx_prefs.GetString("ccxBinaryPath", "")
                if not ccx_binary:
                    FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/Ccx").SetBool("UseStandardCcxLocation", True)
                    error_message = "FEM: CalculiX binary ccx path not set at all. The use of standard path was activated in FEM preferences tab CalculiX. Please try again!\n"
                    if FreeCAD.GuiUp:
                        QtGui.QMessageBox.critical(None, error_title, error_message)
                    raise Exception(error_message)
            self.ccx_binary = ccx_binary

        import subprocess
        startup_info = None
        if system() == "Windows":
            # Windows workaround to avoid blinking terminal window
            startup_info = subprocess.STARTUPINFO()
            startup_info.dwFlags = subprocess.STARTF_USESHOWWINDOW
        ccx_stdout = None
        ccx_stderr = None
        try:
            p = subprocess.Popen([self.ccx_binary], stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE, shell=False,
                                 startupinfo=startup_info)
            ccx_stdout, ccx_stderr = p.communicate()
            if ccx_binary_sig in str(ccx_stdout):
                self.ccx_binary_present = True
            else:
                raise Exception("FEM: wrong ccx binary")  # since we raise an exception the try will fail and the exception later with the error popup will be raised
                # TODO: I'm still able to break it. If user doesn't give a file but a path without a file or a file which is not a binary no exception at all is raised.
        except OSError as e:
            FreeCAD.Console.PrintError(str(e))
            if e.errno == 2:
                error_message = "FEM: CalculiX binary ccx \'{}\' not found. Please set the CalculiX binary ccx path in FEM preferences tab CalculiX.\n".format(ccx_binary)
                if FreeCAD.GuiUp:
                    QtGui.QMessageBox.critical(None, error_title, error_message)
                raise Exception(error_message)
        except Exception as e:
            FreeCAD.Console.PrintError(str(e))
            error_message = "FEM: CalculiX ccx \'{}\' output \'{}\' doesn't contain expected phrase \'{}\'. Please use ccx 2.6 or newer\n".format(ccx_binary, ccx_stdout, ccx_binary_sig)
            if FreeCAD.GuiUp:
                QtGui.QMessageBox.critical(None, error_title, error_message)
            raise Exception(error_message)

    def start_ccx(self):
        import multiprocessing
        import os
        import subprocess
        self.ccx_stdout = ""
        self.ccx_stderr = ""
        if self.inp_file_name != "" and self.ccx_binary_present:
            ont_backup = os.environ.get('OMP_NUM_THREADS')
            self.ccx_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/Ccx")
            num_cpu_pref = self.ccx_prefs.GetInt("AnalysisNumCPUs", 1)  # If number of CPU's specified
            if not ont_backup:
                ont_backup = str(num_cpu_pref)
            if num_cpu_pref > 1:
                _env = os.putenv('OMP_NUM_THREADS', str(num_cpu_pref))  # if user picked a number use that instead
            else:
                _env = os.putenv('OMP_NUM_THREADS', str(multiprocessing.cpu_count()))
            # change cwd because ccx may crash if directory has no write permission
            # there is also a limit of the length of file names so jump to the document directory
            cwd = QtCore.QDir.currentPath()
            f = QtCore.QFileInfo(self.inp_file_name)
            QtCore.QDir.setCurrent(f.path())
            p = subprocess.Popen([self.ccx_binary, "-i ", f.baseName()],
                                 stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                 shell=False, env=_env)
            self.ccx_stdout, self.ccx_stderr = p.communicate()
            os.putenv('OMP_NUM_THREADS', ont_backup)
            QtCore.QDir.setCurrent(cwd)
            return p.returncode
        return -1

    def get_ccx_version(self):
        import re
        import subprocess
        from platform import system
        startup_info = None
        if system() == "Windows":
            # Windows workaround to avoid blinking terminal window
            startup_info = subprocess.STARTUPINFO()
            startup_info.dwFlags = subprocess.STARTF_USESHOWWINDOW
        ccx_stdout = None
        ccx_stderr = None
        # Now extract the version number
        p = subprocess.Popen([self.ccx_binary, '-v'], stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE, shell=False,
                             startupinfo=startup_info)
        ccx_stdout, ccx_stderr = p.communicate()
        m = re.search(r"(\d+).(\d+)", ccx_stdout)
        return (int(m.group(1)), int(m.group(2)))

    def run(self):
        ret_code = 0
        message = self.check_prerequisites()
        if not message:
            self.write_inp_file()
            from FreeCAD import Base
            progress_bar = Base.ProgressIndicator()
            progress_bar.start("Running CalculiX ccx...", 0)
            ret_code = self.start_ccx()
            self.finished.emit(ret_code)
            progress_bar.stop()
            if ret_code or self.ccx_stderr:
                FreeCAD.Console.PrintError("CalculiX failed with exit code {}\n".format(ret_code))
                FreeCAD.Console.PrintMessage("--------start of stderr-------\n")
                FreeCAD.Console.PrintMessage(self.ccx_stderr)
                FreeCAD.Console.PrintMessage("--------end of stderr---------\n")
                FreeCAD.Console.PrintMessage("--------start of stdout-------\n")
                FreeCAD.Console.PrintMessage(self.ccx_stdout)
                self.has_for_nonpositive_jacobians()
                FreeCAD.Console.PrintMessage("--------end of stdout---------\n")
            else:
                FreeCAD.Console.PrintMessage("CalculiX finished without error\n")
        else:
            FreeCAD.Console.PrintError("CalculiX was not started due to missing prerequisites:\n{}\n".format(message))
            # ATM it is not possible to start CalculiX if prerequisites are not fulfilled

    def has_for_nonpositive_jacobians(self):
        if '*ERROR in e_c3d: nonpositive jacobian' in self.ccx_stdout:
            FreeCAD.Console.PrintError('CalculiX returned an error due to nonpositive jacobian elements.\n')
            nonpositive_jacobian_elements = []
            nonpositive_jacobian_elenodes = []
            for line in self.ccx_stdout.splitlines():
                if 'determinant in element' in line:
                    # print(line)
                    # print(line.split())
                    non_posjac_ele = int(line.split()[3])
                    # print(non_posjac_ele)
                    if non_posjac_ele not in nonpositive_jacobian_elements:
                        nonpositive_jacobian_elements.append(non_posjac_ele)
            for e in nonpositive_jacobian_elements:
                for n in self.mesh.FemMesh.getElementNodes(e):
                    nonpositive_jacobian_elenodes.append(n)
            nonpositive_jacobian_elements = sorted(nonpositive_jacobian_elements)
            nonpositive_jacobian_elenodes = sorted(nonpositive_jacobian_elenodes)
            command_for_nonposjacnodes = 'nonpositive_jacobian_elenodes = ' + str(nonpositive_jacobian_elenodes)
            command_to_highlight = "Gui.ActiveDocument." + self.mesh.Name + ".HighlightedNodes = nonpositive_jacobian_elenodes"
            FreeCAD.Console.PrintMessage('nonpositive_jacobian_elements = {}\n'.format(nonpositive_jacobian_elements))
            FreeCAD.Console.PrintMessage(command_for_nonposjacnodes + '\n')
            FreeCAD.Console.PrintMessage(command_to_highlight + '\n')
            FreeCAD.Console.PrintMessage('Gui.ActiveDocument.Extrude_Mesh.HighlightedNodes = []\n\n')  # command to reset the Highlighted Nodes
            if FreeCAD.GuiUp:
                import FreeCADGui
                FreeCADGui.doCommand(command_for_nonposjacnodes)
                FreeCADGui.doCommand(command_to_highlight)
            return True
        else:
            return False

    def load_results(self):
        self.results_present = False
        self.load_results_ccxfrd()
        self.load_results_ccxdat()

    ## Load results of ccx calculations from .frd file.
    #  @param self The python object self
    def load_results_ccxfrd(self):
        import os
        import feminout.importCcxFrdResults as importCcxFrdResults
        frd_result_file = os.path.splitext(self.inp_file_name)[0] + '.frd'
        if os.path.isfile(frd_result_file):
            result_name_prefix = 'CalculiX_' + self.solver.AnalysisType + '_'
            importCcxFrdResults.importFrd(frd_result_file, self.analysis, result_name_prefix)
            for m in self.analysis.Group:
                if m.isDerivedFrom("Fem::FemResultObject"):
                    self.results_present = True
                    break
            else:
                FreeCAD.Console.PrintError('FEM: No result object in active Analysis.\n')
        else:
            raise Exception('FEM: No results found at {}!'.format(frd_result_file))

    ## Load results of ccx calculations from .dat file.
    #  @param self The python object self
    def load_results_ccxdat(self):
        import os
        import feminout.importCcxDatResults as importCcxDatResults
        dat_result_file = os.path.splitext(self.inp_file_name)[0] + '.dat'
        if os.path.isfile(dat_result_file):
            mode_frequencies = importCcxDatResults.import_dat(dat_result_file, self.analysis)
        else:
            raise Exception('FEM: No .dat results found at {}!'.format(dat_result_file))
        if mode_frequencies:
            # print(mode_frequencies)
            for m in self.analysis.Group:
                if m.isDerivedFrom("Fem::FemResultObject") and m.Eigenmode > 0:
                    for mf in mode_frequencies:
                        if m.Eigenmode == mf['eigenmode']:
                            m.EigenmodeFrequency = mf['frequency']


# helper
def get_refshape_type(fem_doc_object):
    # returns the reference shape type
    # for force object:
    # in GUI defined frc_obj all frc_obj have at least one ref_shape and ref_shape have all the same shape type
    # for material object:
    # in GUI defined material_obj could have no RefShape and RefShapes could be different type
    # we're going to need the RefShapes to be the same type inside one fem_doc_object
    # TODO: check if all RefShapes inside the object really have the same type
    import femmesh.meshtools as FemMeshTools
    if hasattr(fem_doc_object, 'References') and fem_doc_object.References:
        first_ref_obj = fem_doc_object.References[0]
        first_ref_shape = FemMeshTools.get_element(first_ref_obj[0], first_ref_obj[1][0])
        st = first_ref_shape.ShapeType
        FreeCAD.Console.PrintMessage(fem_doc_object.Name + ' has ' + st + ' reference shapes.\n')
        return st
    else:
        FreeCAD.Console.PrintMessage(fem_doc_object.Name + ' has empty References.\n')
        return ''

##  @}
