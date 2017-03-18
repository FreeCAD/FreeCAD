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

import FreeCAD
import FemTools
from PySide import QtCore
if FreeCAD.GuiUp:
    from PySide import QtGui


class FemToolsCcx(FemTools.FemTools):

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
                self.set_analysis_type()
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

    def write_inp_file(self):
        import FemInputWriterCcx as iw
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
                self.beam_sections, self.shell_thicknesses, self.fluid_sections,
                self.analysis_type, self.working_dir)
            self.inp_file_name = inp_writer.write_calculix_input_file()
        except:
            print("Unexpected error when writing CalculiX input file:", sys.exc_info()[0])
            raise

    ## Sets CalculiX ccx binary path and velidates if the binary can be executed
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
            if ccx_binary_sig in ccx_stdout:
                self.ccx_binary_present = True
            else:
                raise Exception("FEM: wrong ccx binary")  # since we raise an exception the try will fail and the exception later with the error popup will be raised
                # TODO: I'm still able to break it. If user gives not a file but a path without a file or a file which is not a binary no excetion at all is raised.
        except OSError as e:
            FreeCAD.Console.PrintError(e.message)
            if e.errno == 2:
                error_message = "FEM: CalculiX binary ccx \'{}\' not found. Please set the CalculiX binary ccx path in FEM preferences tab CalculiX.\n".format(ccx_binary)
                if FreeCAD.GuiUp:
                    QtGui.QMessageBox.critical(None, error_title, error_message)
                raise Exception(error_message)
        except Exception as e:
            FreeCAD.Console.PrintError(e.message)
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
            self.has_for_nonpositive_jacobians()
            print("--------end of stdout---------")

    def has_for_nonpositive_jacobians(self):
        if '*ERROR in e_c3d: nonpositive jacobian' in self.ccx_stdout:
            print('CalculiX returned an error due to nonpositive jacobian elements.')
            nonpositive_jacobian_elements = []
            nonpositive_jacobian_elenodes = []
            for line in self.ccx_stdout.splitlines():
                if 'determinant in element' in line:
                    # print line
                    # print line.split()
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
            print('nonpositive_jacobian_elements = ' + str(nonpositive_jacobian_elements))
            print(command_for_nonposjacnodes)
            print(command_to_highlight)
            print('Gui.ActiveDocument.Extrude_Mesh.HighlightedNodes = []\n')  # command to reset the Highlighted Nodes
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
        import importCcxFrdResults
        frd_result_file = os.path.splitext(self.inp_file_name)[0] + '.frd'
        if os.path.isfile(frd_result_file):
            result_name_prefix = 'CalculiX_' + self.solver.AnalysisType + '_'
            importCcxFrdResults.importFrd(frd_result_file, self.analysis, result_name_prefix)
            for m in self.analysis.Member:
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
        import importCcxDatResults
        dat_result_file = os.path.splitext(self.inp_file_name)[0] + '.dat'
        if os.path.isfile(dat_result_file):
            mode_frequencies = importCcxDatResults.import_dat(dat_result_file, self.analysis)
        else:
            raise Exception('FEM: No .dat results found at {}!'.format(dat_result_file))
        if mode_frequencies:
            # print(mode_frequencies)
            for m in self.analysis.Member:
                if m.isDerivedFrom("Fem::FemResultObject") and m.Eigenmode > 0:
                    for mf in mode_frequencies:
                        if m.Eigenmode == mf['eigenmode']:
                            m.EigenmodeFrequency = mf['frequency']

##  @}
