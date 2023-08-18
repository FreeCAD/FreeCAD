# ***************************************************************************
# *   Copyright (c) 2015 Przemo Firszt <przemo@firszt.eu>                   *
# *   Copyright (c) 2016 Bernd Hahnebach <bernd@bimstatik.org>              *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
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
__url__ = "https://www.freecad.org"

## \addtogroup FEM
#  @{

import os
import sys
import subprocess

import FreeCAD

from femtools import femutils
from femtools import membertools

from PySide import QtCore  # there might be a special reason this is not guarded ?!?
if FreeCAD.GuiUp:
    from PySide import QtGui
    import FemGui


class FemToolsCcx(QtCore.QRunnable, QtCore.QObject):
    """

    Attributes
    ----------
    analysis : Fem::FemAnalysis
        FEM group analysis object
        has to be present, will be set in __init__
    solver : Fem::FemSolverObjectPython
        FEM solver object
        has to be present, will be set in __init__
    base_name : str
        name of .inp/.frd file (without extension)
        It is used to construct .inp file path that is passed to CalculiX ccx
    ccx_binary : str
    working_dir : str
    results_present : bool
        indicating if there are calculation results ready for us
    members : class femtools/membertools/AnalysisMember
        contains references to all analysis member except solvers and mesh
        Updated with update_objects
    """

    finished = QtCore.Signal(int)

    def __init__(self, analysis=None, solver=None, test_mode=False):
        """The constructor

        Parameters
        ----------
        analysis : Fem::FemAnalysis, optional
            analysis group as a container for all  objects needed for the analysis
        solver : Fem::FemSolverObjectPython, optional
            solver object to be used for this solve
        test_mode : bool, optional
            mainly used in unit tests
        """

        QtCore.QRunnable.__init__(self)
        QtCore.QObject.__init__(self)

        self.ccx_binary_present = False
        self.analysis = None
        self.solver = None

        # TODO if something will go wrong in __init__ do not continue,
        # but do not raise a exception, break in a smarter way

        if analysis:
            self.analysis = analysis
            if solver:
                # analysis and solver given
                self.solver = solver
            else:
                # analysis given, search for the solver
                self.find_solver()
                if not self.solver:
                    raise Exception("FEM: No solver found!")
        else:
            if solver:
                # solver given, search for the analysis
                self.solver = solver
                self.find_solver_analysis()
                if not self.analysis:
                    raise Exception(
                        "FEM: The solver was given as parameter, "
                        "but no analysis for this solver was found!"
                    )
            else:
                # neither analysis nor solver given, search both
                self.find_analysis()
                if not self.analysis:
                    raise Exception(
                        "FEM: No solver was given and either no active analysis "
                        "or no analysis at all or more than one analysis found!"
                    )
                self.find_solver()
                if not self.solver:
                    raise Exception("FEM: No solver found!")

        if self.analysis.Document is not self.solver.Document:
            raise Exception(
                "FEM: The analysis and solver are not in the same document!"
            )
        if self.solver not in self.analysis.Group:
            raise Exception(
                "FEM: The solver is not part of the analysis Group!"
            )

        # print(self.solver)
        # print(self.analysis)
        if self.analysis and self.solver:
            self.working_dir = ""
            self.ccx_binary = ""
            self.base_name = ""
            self.results_present = False
            if test_mode:
                self.test_mode = True
                self.ccx_binary_present = True
            else:
                self.test_mode = False
                self.ccx_binary_present = False
            self.result_object = None
        else:
            raise Exception(
                "FEM: Something went wrong, "
                "the exception should have been raised earlier!"
            )

    def purge_results(self):
        """Remove all result objects and result meshes from an analysis group
        """
        from femresult.resulttools import purge_results as pr
        pr(self.analysis)

    def reset_mesh_purge_results_checked(self):
        """Reset mesh color, deformation and removes all result objects
        if preferences to keep them is not set.
        """
        self.fem_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/General")
        keep_results_on_rerun = self.fem_prefs.GetBool("KeepResultsOnReRun", False)
        if not keep_results_on_rerun:
            self.purge_results()

    def reset_all(self):
        """Reset mesh color, deformation and removes all result objects
        """
        self.purge_results()

    def _get_several_member(self, obj_type):
        return membertools.get_several_member(self.analysis, obj_type)

    def find_analysis(self):
        if FreeCAD.GuiUp:
            self.analysis = FemGui.getActiveAnalysis()
        if self.analysis:
            return
        found_analysis = False
        # search in the active document
        for m in FreeCAD.activeDocument().Objects:
            if femutils.is_of_type(m, "Fem::FemAnalysis"):
                if not found_analysis:
                    self.analysis = m
                    found_analysis = True
                else:
                    self.analysis = None  # more than one analysis
        if self.analysis:
            if FreeCAD.GuiUp:
                FemGui.setActiveAnalysis(self.analysis)

    def find_solver_analysis(self):
        """ get the analysis group the solver belongs to
        """
        if self.solver.getParentGroup():
            obj = self.solver.getParentGroup()
            if femutils.is_of_type(obj, "Fem::FemAnalysis"):
                self.analysis = obj
                if FreeCAD.GuiUp:
                    FemGui.setActiveAnalysis(self.analysis)

    def find_solver(self):
        found_solver_for_use = False
        for m in self.analysis.Group:
            if femutils.is_of_type(m, "Fem::SolverCcxTools"):
                # we are going to explicitly check for the ccx tools solver type only,
                # thus it is possible to have lots of framework solvers inside the analysis anyway
                # for some methods no solver is needed (purge_results) --> solver could be none
                # analysis has one solver and no solver was set --> use the one solver
                # analysis has more than one solver and no solver was set --> use solver none
                # analysis has no solver --> use solver none
                if not found_solver_for_use:
                    # no solver was found before
                    self.solver = m
                    found_solver_for_use = True
                else:
                    # another solver was found --> We have more than one solver
                    # we do not know which one to use, so we use none !
                    self.solver = None
                    FreeCAD.Console.PrintLog(
                        "FEM: More than one solver in the analysis "
                        "and no solver given to analyze. "
                        "No solver is set!\n"
                    )

    def update_objects(self):
        ## @var mesh
        #  mesh for the analysis
        self.mesh = None
        mesh, message = membertools.get_mesh_to_solve(self.analysis)
        if mesh is not None:
            self.mesh = mesh
        else:
            # the prerequisites will run anyway and they will print a message box anyway
            # thus do not print one here, but print a console warning
            FreeCAD.Console.PrintWarning(
                "{} The prerequisite check will fail.\n"
                .format(message)
            )

        ## @var members
        # members of the analysis. All except the solver and the mesh
        self.member = membertools.AnalysisMember(self.analysis)

    def check_prerequisites(self):
        FreeCAD.Console.PrintMessage("\n")  # because of time print in separate line
        FreeCAD.Console.PrintMessage("Check prerequisites...\n")
        message = ""
        # analysis
        if not self.analysis:
            message += "No active Analysis\n"
        # solver
        if not self.solver:
            message += "No solver object defined in the analysis\n"
        if not self.working_dir:
            message += "Working directory not set\n"
        if not os.path.isdir(self.working_dir):
            message += (
                "Working directory \'{}\' doesn't exist."
                .format(self.working_dir)
            )
        from femtools.checksanalysis import check_member_for_solver_calculix
        message += check_member_for_solver_calculix(
            self.analysis,
            self.solver,
            self.mesh,
            self.member
        )
        return message

    def set_base_name(self, base_name=None):
        """
        Set base_name

        Parameters
        ----------
        base_name : str, optional
            base_name base name of .inp/.frd file (without extension).
            It is used to construct .inp file path that is passed to CalculiX ccx
        """
        if base_name is None:
            self.base_name = ""
        else:
            self.base_name = base_name
        # Update inp file name
        self.set_inp_file_name()

    def set_inp_file_name(self, inp_file_name=None):
        """
        Set inp file name. Normally inp file name is set by write_inp_file.
        That name is also used to determine location and name of frd result file.

        Parameters
        ----------
        inp_file_name : str, optional
            input file name path
        """
        if inp_file_name is not None:
            self.inp_file_name = inp_file_name
        else:
            self.inp_file_name = os.path.join(self.working_dir, (self.base_name + ".inp"))

    def setup_working_dir(self, param_working_dir=None, create=False):
        """Set working dir for solver execution.

        Parameters
        ----------
        param_working_dir :  str, optional
            directory to be used for writing
        create : bool, optional
            Should the working directory be created if it does not exist
        """
        self.working_dir = ""
        # try to use given working dir or overwrite with solver working dir
        fem_general_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/General")
        if param_working_dir is not None:
            self.working_dir = param_working_dir
            if femutils.check_working_dir(self.working_dir) is not True:
                if create is True:
                    FreeCAD.Console.PrintMessage(
                        "Dir given as parameter \'{}\' doesn't exist.\n".format(self.working_dir)
                    )
                else:
                    FreeCAD.Console.PrintError(
                        "Dir given as parameter \'{}\' doesn't exist "
                        "and create parameter is set to False.\n"
                        .format(self.working_dir)
                    )
                    self.working_dir = femutils.get_pref_working_dir(self.solver)
                    FreeCAD.Console.PrintMessage(
                        "Dir \'{}\' will be used instead.\n"
                        .format(self.working_dir)
                    )
        elif fem_general_prefs.GetBool("OverwriteSolverWorkingDirectory", True) is False:
            self.working_dir = self.solver.WorkingDir
            if femutils.check_working_dir(self.working_dir) is not True:
                if self.working_dir == '':
                    FreeCAD.Console.PrintError(
                        "Working Dir is set to be used from solver object "
                        "but Dir from solver object \'{}\' is empty.\n"
                        .format(self.working_dir)
                    )
                else:
                    FreeCAD.Console.PrintError(
                        "Dir from solver object \'{}\' doesn't exist.\n"
                        .format(self.working_dir)
                    )
                self.working_dir = femutils.get_pref_working_dir(self.solver)
                FreeCAD.Console.PrintMessage(
                    "Dir \'{}\' will be used instead.\n"
                    .format(self.working_dir)
                )
        else:
            self.working_dir = femutils.get_pref_working_dir(self.solver)

        # check working_dir exist, if not use a tmp dir and inform the user
        if femutils.check_working_dir(self.working_dir) is not True:
            FreeCAD.Console.PrintError(
                "Dir \'{}\' doesn't exist or cannot be created.\n"
                .format(self.working_dir)
            )
            self.working_dir = femutils.get_temp_dir(self.solver)
            FreeCAD.Console.PrintMessage(
                "Dir \'{}\' will be used instead.\n"
                .format(self.working_dir)
            )

        # Update inp file name
        self.set_inp_file_name()

    def write_inp_file(self):

        # get mesh set data
        # TODO use separate method for getting the mesh set data
        from femmesh import meshsetsgetter
        meshdatagetter = meshsetsgetter.MeshSetsGetter(
            self.analysis,
            self.solver,
            self.mesh,
            membertools.AnalysisMember(self.analysis),
        )
        # save the sets into the member objects of the instanz meshdatagetter
        meshdatagetter.get_mesh_sets()

        # write input file
        import femsolver.calculix.writer as iw
        self.inp_file_name = ""
        try:
            inp_writer = iw.FemInputWriterCcx(
                self.analysis,
                self.solver,
                self.mesh,
                meshdatagetter.member,
                self.working_dir,
                meshdatagetter.mat_geo_sets
            )
            self.inp_file_name = inp_writer.write_solver_input()
        except Exception:
            FreeCAD.Console.PrintError(
                "Unexpected error when writing CalculiX input file: {}\n"
                .format(sys.exc_info()[1])
            )
            raise

    def setup_ccx(self, ccx_binary=None, ccx_binary_sig="CalculiX"):
        """Set Calculix binary path and validate its execution.

        Parameters
        ----------
        ccx_binary : str, optional
            It defaults to `None`. The path to the `ccx` binary. If it is `None`,
            the path is guessed.
        ccx_binary_sig : str, optional
            Defaults to 'CalculiX'. Expected output from `ccx` when run empty.

        """
        error_title = "No or wrong CalculiX binary ccx"
        error_message = ""
        from platform import system
        ccx_std_location = FreeCAD.ParamGet(
            "User parameter:BaseApp/Preferences/Mod/Fem/Ccx"
        ).GetBool("UseStandardCcxLocation", True)
        if ccx_std_location:
            if system() == "Windows":
                ccx_path = FreeCAD.getHomePath() + "bin/ccx.exe"
                FreeCAD.ParamGet(
                    "User parameter:BaseApp/Preferences/Mod/Fem/Ccx"
                ).SetString("ccxBinaryPath", ccx_path)
                self.ccx_binary = ccx_path
            elif system() in ("Linux", "Darwin"):
                p1 = subprocess.Popen(["which", "ccx"], stdout=subprocess.PIPE)
                if p1.wait() == 0:
                    ccx_path = p1.stdout.read().decode("utf8").split("\n")[0]
                elif p1.wait() == 1:
                    error_message = (
                        "FEM: CalculiX binary ccx not found in "
                        "standard system binary path. "
                        "Please install ccx or set path to binary "
                        "in FEM preferences tab CalculiX.\n"
                    )
                    if FreeCAD.GuiUp:
                        QtGui.QMessageBox.critical(None, error_title, error_message)
                    raise Exception(error_message)
                self.ccx_binary = ccx_path
        else:
            if not ccx_binary:
                self.ccx_prefs = FreeCAD.ParamGet(
                    "User parameter:BaseApp/Preferences/Mod/Fem/Ccx"
                )
                ccx_binary = self.ccx_prefs.GetString("ccxBinaryPath", "")
                if not ccx_binary:
                    FreeCAD.ParamGet(
                        "User parameter:BaseApp/Preferences/Mod/Fem/Ccx"
                    ).SetBool("UseStandardCcxLocation", True)
                    error_message = (
                        "FEM: CalculiX binary ccx path not set at all. "
                        "The use of standard path was activated in "
                        "FEM preferences tab CalculiX. Please try again!\n"
                    )
                    if FreeCAD.GuiUp:
                        QtGui.QMessageBox.critical(None, error_title, error_message)
                    FreeCAD.Console.PrintError(error_message)
            self.ccx_binary = ccx_binary

        startup_info = None
        if system() == "Windows":
            # Windows workaround to avoid blinking terminal window
            startup_info = subprocess.STARTUPINFO()
            startup_info.dwFlags = subprocess.STARTF_USESHOWWINDOW
        ccx_stdout = None
        ccx_stderr = None
        try:
            p = subprocess.Popen(
                [self.ccx_binary],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                shell=False,
                startupinfo=startup_info
            )
            ccx_stdout, ccx_stderr = p.communicate()
            if ccx_binary_sig in str(ccx_stdout):
                self.ccx_binary_present = True
            else:
                error_message = "FEM: wrong ccx binary\n"
                if FreeCAD.GuiUp:
                    QtGui.QMessageBox.critical(None, error_title, error_message)
                FreeCAD.Console.PrintError(error_message)
                # TODO: I'm still able to break it.
                # If user doesn't give a file but a path without a file or
                # a file which is not a binary no exception at all is raised.
        except OSError as e:
            FreeCAD.Console.PrintError("{}\n".format(e))
            if e.errno == 2:
                error_message = (
                    "FEM: CalculiX binary ccx \'{}\' not found. "
                    "Please set the CalculiX binary ccx path in "
                    "FEM preferences tab CalculiX.\n"
                    .format(ccx_binary)
                )
                if FreeCAD.GuiUp:
                    QtGui.QMessageBox.critical(None, error_title, error_message)
                FreeCAD.Console.PrintError(error_message)

        except Exception as e:
            FreeCAD.Console.PrintError("{}\n".format(e))
            error_message = (
                "FEM: CalculiX ccx \'{}\' output \'{}\' doesn't "
                "contain expected phrase \'{}\'. "
                "There are some problems when running the ccx binary. "
                "Check if ccx runs standalone without FreeCAD.\n"
                .format(ccx_binary, ccx_stdout, ccx_binary_sig)
            )
            if FreeCAD.GuiUp:
                QtGui.QMessageBox.critical(None, error_title, error_message)
            FreeCAD.Console.PrintError(error_message)

    def start_ccx(self):
        import multiprocessing
        self.ccx_stdout = ""
        self.ccx_stderr = ""
        ont_backup = os.environ.get("OMP_NUM_THREADS")
        self.ccx_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/Ccx")
        # If number of CPU's specified
        num_cpu_pref = self.ccx_prefs.GetInt("AnalysisNumCPUs", 1)
        if not ont_backup:
            ont_backup = str(num_cpu_pref)
        if num_cpu_pref > 1:
            # If user picked a number use that instead
            _env = os.putenv("OMP_NUM_THREADS", str(num_cpu_pref))
        else:
            _env = os.putenv("OMP_NUM_THREADS", str(multiprocessing.cpu_count()))
        # change cwd because ccx may crash if directory has no write permission
        # there is also a limit of the length of file names so jump to the document directory
        cwd = QtCore.QDir.currentPath()
        f = QtCore.QFileInfo(self.inp_file_name)
        QtCore.QDir.setCurrent(f.path())
        p = subprocess.Popen(
            [self.ccx_binary, "-i ", f.baseName()],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            shell=False,
            env=_env
        )
        self.ccx_stdout, self.ccx_stderr = p.communicate()
        self.ccx_stdout = self.ccx_stdout.decode()
        self.ccx_stderr = self.ccx_stderr.decode()
        os.putenv("OMP_NUM_THREADS", ont_backup)
        QtCore.QDir.setCurrent(cwd)
        return p.returncode

    def get_ccx_version(self):
        self.setup_ccx()
        import re
        from platform import system
        startup_info = None
        if system() == "Windows":
            # Windows workaround to avoid blinking terminal window
            startup_info = subprocess.STARTUPINFO()
            startup_info.dwFlags = subprocess.STARTF_USESHOWWINDOW
        ccx_stdout = None
        ccx_stderr = None
        # Now extract the version number
        p = subprocess.Popen(
            [self.ccx_binary, "-v"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            shell=False,
            startupinfo=startup_info
        )
        ccx_stdout, ccx_stderr = p.communicate()
        ccx_stdout = ccx_stdout.decode()
        m = re.search(r"(\d+).(\d+)", ccx_stdout)
        return (int(m.group(1)), int(m.group(2)))

    def ccx_run(self):
        ret_code = None
        FreeCAD.Console.PrintMessage("\n")  # because of time print in separate line
        FreeCAD.Console.PrintMessage("CalculiX solver run...\n")
        if self.test_mode:
            FreeCAD.Console.PrintError("CalculiX can not be run if test_mode is True.\n")
            return
        self.setup_ccx()
        if self.ccx_binary_present is False:
            error_message = (
                "FEM: CalculiX binary ccx \'{}\' not found. "
                "Please set the CalculiX binary ccx path in FEM preferences tab CalculiX.\n"
                .format(self.ccx_binary)
            )
            if FreeCAD.GuiUp:
                QtGui.QMessageBox.critical(None, "No CalculiX binary ccx", error_message)
            return
        progress_bar = FreeCAD.Base.ProgressIndicator()
        progress_bar.start("Everything seams fine. CalculiX ccx will be executed ...", 0)
        ret_code = self.start_ccx()
        self.finished.emit(ret_code)
        progress_bar.stop()
        if ret_code or self.ccx_stderr:
            if ret_code == 201 and self.solver.AnalysisType == "check":
                FreeCAD.Console.PrintMessage(
                    "It seams we run into NOANALYSIS problem, "
                    "thus workaround for wrong exit code for *NOANALYSIS check "
                    "and set ret_code to 0.\n"
                )
                # https://forum.freecad.org/viewtopic.php?f=18&t=31303&start=10#p260743
                ret_code = 0
            else:
                FreeCAD.Console.PrintError("CalculiX failed with exit code {}\n".format(ret_code))
                FreeCAD.Console.PrintMessage("--------start of stderr-------\n")
                FreeCAD.Console.PrintMessage(self.ccx_stderr)
                FreeCAD.Console.PrintMessage("--------end of stderr---------\n")
                FreeCAD.Console.PrintMessage("--------start of stdout-------\n")
                FreeCAD.Console.PrintMessage(self.ccx_stdout)
                FreeCAD.Console.PrintMessage("\n--------end of stdout---------\n")
                FreeCAD.Console.PrintMessage("--------start problems---------\n")
                self.has_no_material_assigned()
                self.has_nonpositive_jacobians()
                FreeCAD.Console.PrintMessage("\n--------end problems---------\n")
        else:
            FreeCAD.Console.PrintMessage("CalculiX finished without error.\n")
        return ret_code

    def run(self):
        self.update_objects()
        self.setup_working_dir()
        message = self.check_prerequisites()
        if message:
            text = "CalculiX can not be started due to missing prerequisites:\n"
            error_app = "{}{}".format(text, message)
            error_gui = "{}\n{}".format(text, message)
            FreeCAD.Console.PrintError(error_app)
            if FreeCAD.GuiUp:
                QtGui.QMessageBox.critical(
                    None,
                    "Missing prerequisite",
                    error_gui
                )
            return False
        else:
            self.write_inp_file()
            if self.inp_file_name == "":
                error_message = "Error on writing CalculiX input file.\n"
                FreeCAD.Console.PrintError(error_message)
                if FreeCAD.GuiUp:
                    QtGui.QMessageBox.critical(
                        None,
                        "Error",
                        error_message
                    )
                return False
            else:
                FreeCAD.Console.PrintLog(
                    "Writing CalculiX input file completed.\n"
                )
                ret_code = self.ccx_run()
                if ret_code is None:
                    error_message = (
                        "CalculiX has not been run. The CalculiX binary search returned: {}.\n"
                        .format(self.ccx_binary_present)
                    )
                    FreeCAD.Console.PrintError(error_message)
                    if FreeCAD.GuiUp:
                        QtGui.QMessageBox.critical(
                            None,
                            "Error",
                            error_message
                        )
                    return False
                if ret_code != 0:
                    error_message = (
                        "CalculiX finished with error {}.\n"
                        .format(ret_code)
                    )
                    FreeCAD.Console.PrintError(error_message)
                    if FreeCAD.GuiUp:
                        QtGui.QMessageBox.critical(
                            None,
                            "Error",
                            error_message
                        )
                    return False
                else:
                    FreeCAD.Console.PrintLog("Try to read result files\n")
                    self.load_results()
                    # TODO: output an error message if there where problems reading the results
        return True

    def has_no_material_assigned(self):
        if " *ERROR in calinput: no material was assigned" in self.ccx_stdout:
            without_material_elements = []
            without_material_elemnodes = []
            for line in self.ccx_stdout.splitlines():
                if "to element" in line:
                    # print(line)
                    # print(line.split())
                    non_mat_ele = int(line.split()[2])
                    # print(non_mat_ele)
                    if non_mat_ele not in without_material_elements:
                        without_material_elements.append(non_mat_ele)
            for e in without_material_elements:
                for n in self.mesh.FemMesh.getElementNodes(e):
                    without_material_elemnodes.append(n)
            without_material_elements = sorted(without_material_elements)
            without_material_elemnodes = sorted(without_material_elemnodes)
            command_for_withoutmatnodes = (
                "without_material_elemnodes = {}"
                .format(without_material_elemnodes)
            )
            command_to_highlight = (
                "Gui.ActiveDocument.{}.HighlightedNodes = without_material_elemnodes"
                .format(self.mesh.Name)
            )
            # some output for the user
            FreeCAD.Console.PrintError(
                "\n\nCalculiX returned an error due to elements without materials.\n"
            )
            FreeCAD.Console.PrintMessage(
                "without_material_elements = {}\n"
                .format(without_material_elements)
            )
            FreeCAD.Console.PrintMessage(command_for_withoutmatnodes + "\n")
            if FreeCAD.GuiUp:
                import FreeCADGui
                # with this the list without_material_elemnodes
                # will be available for further user interaction
                FreeCADGui.doCommand(command_for_withoutmatnodes)
                FreeCAD.Console.PrintMessage("\n")
                FreeCADGui.doCommand(command_to_highlight)
            FreeCAD.Console.PrintMessage(
                "\nFollowing some commands to copy. "
                "They will highlight the elements without materials "
                "or to reset the highlighted nodes:\n"
            )
            FreeCAD.Console.PrintMessage(command_to_highlight + "\n")
            # command to reset the Highlighted Nodes
            FreeCAD.Console.PrintMessage(
                "Gui.ActiveDocument.{}.HighlightedNodes = []\n\n"
                .format(self.mesh.Name)
            )
            return True
        else:
            return False

    def has_nonpositive_jacobians(self):
        if "*ERROR in e_c3d: nonpositive jacobian" in self.ccx_stdout:
            nonpositive_jacobian_elements = []
            nonpositive_jacobian_elenodes = []
            for line in self.ccx_stdout.splitlines():
                if "determinant in element" in line:
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
            command_for_nonposjacnodes = (
                "nonpositive_jacobian_elenodes = {}"
                .format(nonpositive_jacobian_elenodes)
            )
            command_to_highlight = (
                "Gui.ActiveDocument.{}.HighlightedNodes = nonpositive_jacobian_elenodes"
                .format(self.mesh.Name)
            )
            # some output for the user
            FreeCAD.Console.PrintError(
                "\n\nCalculiX returned an error due to nonpositive jacobian elements.\n"
            )
            FreeCAD.Console.PrintMessage(
                "nonpositive_jacobian_elements = {}\n"
                .format(nonpositive_jacobian_elements)
            )
            FreeCAD.Console.PrintMessage(command_for_nonposjacnodes + "\n")
            if FreeCAD.GuiUp:
                import FreeCADGui
                # with this the list nonpositive_jacobian_elenodes
                # will be available for further user interaction
                FreeCADGui.doCommand(command_for_nonposjacnodes)
                FreeCAD.Console.PrintMessage("\n")
                FreeCADGui.doCommand(command_to_highlight)
            FreeCAD.Console.PrintMessage(
                "\nFollowing some commands to copy. "
                "They highlight the nonpositive jacobians "
                "or to reset the highlighted nodes:\n"
            )
            FreeCAD.Console.PrintMessage(command_to_highlight + "\n")
            # command to reset the Highlighted Nodes
            FreeCAD.Console.PrintMessage(
                "Gui.ActiveDocument.{}.HighlightedNodes = []\n\n"
                .format(self.mesh.Name)
            )
            return True
        else:
            return False

    def load_results(self):
        FreeCAD.Console.PrintMessage("\n")  # because of time print in separate line
        FreeCAD.Console.PrintMessage("CalculiX read results...\n")
        self.results_present = False
        self.load_results_ccxfrd()
        self.load_results_ccxdat()
        self.analysis.Document.recompute()

    def load_results_ccxfrd(self):
        """Load results of ccx calculations from .frd file.
        """
        import feminout.importCcxFrdResults as importCcxFrdResults
        frd_result_file = os.path.splitext(self.inp_file_name)[0] + ".frd"
        if os.path.isfile(frd_result_file):
            importCcxFrdResults.importFrd(
                frd_result_file,
                self.analysis,
                "CCX_",
                self.solver.AnalysisType
            )
            for m in self.analysis.Group:
                if m.isDerivedFrom("Fem::FemResultObject"):
                    self.results_present = True
                    break
            else:
                if self.solver.AnalysisType == "check":
                    for m in self.analysis.Group:
                        if m.isDerivedFrom("Fem::FemMeshObjectPython"):
                            # we have no result object but a mesh object
                            # this happens in NOANALYSIS mode
                            break
                else:
                    FreeCAD.Console.PrintError("FEM: No result object in active Analysis.\n")
        else:
            FreeCAD.Console.PrintError(
                "FEM: No frd result file found at {}\n"
                .format(frd_result_file)
            )

    def load_results_ccxdat(self):
        """Load results of ccx calculations from .dat file.
        """
        import feminout.importCcxDatResults as importCcxDatResults
        dat_result_file = os.path.splitext(self.inp_file_name)[0] + ".dat"
        mode_frequencies = None
        dat_content = None

        if os.path.isfile(dat_result_file):
            mode_frequencies = importCcxDatResults.import_dat(dat_result_file, self.analysis)

            dat_file = open(dat_result_file, "r")
            dat_content = dat_file.read()
            dat_file.close()
        else:
            FreeCAD.Console.PrintError(
                "FEM: No dat result file found at {}\n"
                .format(dat_result_file)
            )

        if mode_frequencies:
            # print(mode_frequencies)
            for m in self.analysis.Group:
                if m.isDerivedFrom("Fem::FemResultObject") and m.Eigenmode > 0:
                    for mf in mode_frequencies:
                        if m.Eigenmode == mf["eigenmode"]:
                            m.EigenmodeFrequency = mf["frequency"]

        if dat_content:
            # print(dat_content)
            dat_text_obj = self.analysis.Document.addObject("App::TextDocument", "ccx_dat_file")
            dat_text_obj.Text = dat_content
            dat_text_obj.setPropertyStatus("Text", "ReadOnly")  # set property editor readonly
            if FreeCAD.GuiUp:
                dat_text_obj.ViewObject.ReadOnly = True  # set editor view readonly
            self.analysis.addObject(dat_text_obj)


class CcxTools(FemToolsCcx):

    def __init__(self, solver=None):
        FemToolsCcx.__init__(self, None, solver)

##  @}
