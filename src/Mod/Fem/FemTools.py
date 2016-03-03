# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2015 - Przemo Firszt <przemo@firszt.eu>                 *
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


import FreeCAD
from PySide import QtCore


class FemTools(QtCore.QRunnable, QtCore.QObject):

    finished = QtCore.Signal(int)

    known_analysis_types = ["static", "frequency"]

    ## The constructor
    #  @param analysis - analysis object to be used as the core object.
    #  @param test_mode - True indicates that no real calculations will take place, so ccx bianry is not required. Used by test module.
    #  "__init__" tries to use current active analysis in analysis is left empty.
    #  Rises exception if analysis is not set and there is no active analysis
    def __init__(self, analysis=None, test_mode=False):
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
        if self.analysis:
            self.update_objects()
            ## @var base_name
            #  base name of .inp/.frd file (without extension). It is used to construct .inp file path that is passed to CalculiX ccx
            self.base_name = ""
            ## @var results_present
            #  boolean variable indicating if there are calculation results ready for use
            self.results_present = False
            if self.solver:
                self.set_analysis_type()
                self.set_eigenmode_parameters()
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

        ## @var solver
        #  solver of the analysis. Used to store solver and analysis parameters
        self.solver = None
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

        for m in self.analysis.Member:
            if m.isDerivedFrom("Fem::FemSolverObjectPython"):
                if not self.solver:
                    self.solver = m
                else:
                    raise Exception('FEM: Multiple solver in analysis not yet supported!')
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
            message += "No mesh object in the Analysis\n"
        if not self.materials:
            message += "No material object in the Analysis\n"
        has_no_references = False
        for m in self.materials:
            if len(m['Object'].References) == 0:
                if has_no_references is True:
                    message += "More than one Material has empty References list (Only one empty References list is allowed!).\n"
                has_no_references = True
        if not (self.fixed_constraints):
            message += "No fixed-constraint nodes defined in the Analysis\n"
        if self.analysis_type == "static":
            if not (self.force_constraints or self.pressure_constraints):
                message += "No force-constraint or pressure-constraint defined in the Analysis\n"
        if self.beam_sections:
            has_no_references = False
            for b in self.beam_sections:
                if len(b['Object'].References) == 0:
                    if has_no_references is True:
                        message += "More than one BeamSection has empty References list (Only one empty References list is allowed!).\n"
                    has_no_references = True
        if self.shell_thicknesses:
            has_no_references = False
            for s in self.shell_thicknesses:
                if len(s['Object'].References) == 0:
                    if has_no_references is True:
                        message += "More than one ShellThickness has empty References list (Only one empty References list is allowed!).\n"
                    has_no_references = True
        return message

    def write_inp_file(self):
        import ccxInpWriter as iw
        import sys
        self.inp_file_name = ""
        try:
            inp_writer = iw.inp_writer(self.analysis, self.mesh, self.materials,
                                       self.fixed_constraints,
                                       self.force_constraints, self.pressure_constraints,
                                       self.displacement_constraints,  # OvG: Stick to naming convention
                                       self.beam_sections, self.shell_thicknesses,
                                       self.analysis_type, self.eigenmode_parameters,
                                       self.working_dir)
            self.inp_file_name = inp_writer.write_calculix_input_file()
        except:
            print("Unexpected error when writing CalculiX input file:", sys.exc_info()[0])
            raise

    def start_ccx(self):
        import multiprocessing
        import os
        import subprocess
        self.ccx_stdout = ""
        self.ccx_stderr = ""
        if self.inp_file_name != "" and self.ccx_binary_present:
            ont_backup = os.environ.get('OMP_NUM_THREADS')
            if not ont_backup:
                ont_backup = ""
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
            try:
                self.working_dir = self.solver.WorkingDir
            except:
                FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem").GetString("WorkingDir")
                self.working_dir = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem").GetString("WorkingDir")

        if not (os.path.isdir(self.working_dir)):
            try:
                os.makedirs(self.working_dir)
            except:
                print("Dir \'{}\' doesn't exist and cannot be created.".format(self.working_dir))
                import tempfile
                self.working_dir = tempfile.gettempdir()
                print("Dir \'{}\' will be used instead.".format(self.working_dir))
        # Update inp file name
        self.set_inp_file_name()

    ## Sets CalculiX ccx binary path and velidates if the binary can be executed
    #  @param self The python object self
    #  @ccx_binary path to ccx binary, default is guessed: "bin/ccx" windows, "ccx" for other systems
    #  @ccx_binary_sig expected output form ccx when run empty. Default value is "CalculiX.exe -i jobname"
    def setup_ccx(self, ccx_binary=None, ccx_binary_sig="CalculiX"):
        from platform import system
        if not ccx_binary:
            self.fem_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem")
            ccx_binary = self.fem_prefs.GetString("ccxBinaryPath", "")
        if not ccx_binary:
            if system() == "Linux":
                ccx_binary = "ccx"
            elif system() == "Windows":
                ccx_binary = FreeCAD.getHomePath() + "bin/ccx.exe"
            else:
                ccx_binary = "ccx"
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
            if ccx_binary_sig in ccx_stdout:
                self.ccx_binary_present = True
        except OSError as e:
            FreeCAD.Console.PrintError(e.message)
            if e.errno == 2:
                raise Exception("FEM: CalculiX binary ccx \'{}\' not found. Please set it in FEM preferences.".format(ccx_binary))
        except Exception as e:
            FreeCAD.Console.PrintError(e.message)
            raise Exception("FEM: CalculiX ccx \'{}\' output \'{}\' doesn't contain expected phrase \'{}\'. Please use ccx 2.6 or newer".
                            format(ccx_binary, ccx_stdout, ccx_binary_sig))

    ## Load results of ccx calculations from .frd file.
    #  @param self The python object self
    def load_results(self):
        import ccxFrdReader
        import os
        self.results_present = False
        frd_result_file = os.path.splitext(self.inp_file_name)[0] + '.frd'
        if os.path.isfile(frd_result_file):
            ccxFrdReader.importFrd(frd_result_file, self.analysis)
            for m in self.analysis.Member:
                if m.isDerivedFrom("Fem::FemResultObject"):
                    self.result_object = m
            if self.result_object:
                self.results_present = True
        else:
            raise Exception('FEM: No results found at {}!'.format(frd_result_file))

        import ccxDatReader
        dat_result_file = os.path.splitext(self.inp_file_name)[0] + '.dat'
        if os.path.isfile(dat_result_file):
            mode_frequencies = ccxDatReader.import_dat(dat_result_file, self.analysis)
        else:
            raise Exception('FEM: No .dat results found at {}!'.format(dat_result_file))
        for m in self.analysis.Member:
            if m.isDerivedFrom("Fem::FemResultObject") and m.Eigenmode > 0:
                    m.EigenmodeFrequency = mode_frequencies[m.Eigenmode - 1]['frequency']

    def use_results(self, results_name=None):
        for m in self.analysis.Member:
            if m.isDerivedFrom("Fem::FemResultObject") and m.Name == results_name:
                self.result_object = m
                break
        if not self.result_object:
            raise Exception("{} doesn't exist".format(results_name))

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
        else:
            print("Running analysis failed! {}".format(message))
        if ret_code or self.ccx_stderr:
            print("Analysis failed with exit code {}".format(ret_code))
            print("--------start of stderr-------")
            print(self.ccx_stderr)
            print("--------end of stderr---------")
            print("--------start of stdout-------")
            print(self.ccx_stdout)
            print("--------end of stdout---------")

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
