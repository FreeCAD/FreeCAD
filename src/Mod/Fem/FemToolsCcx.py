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


import FreeCAD
import FemTools
from PySide import QtCore


class FemToolsCcx(FemTools.FemTools):

    known_analysis_types = ["static", "frequency"]
    finished = QtCore.Signal(int)

    ## The constructor
    #  @param analysis - analysis object to be used as the core object.
    #  @param test_mode - True indicates that no real calculations will take place, so ccx bianry is not required. Used by test module.
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

    def write_inp_file(self):
        import FemInputWriterCcx as iw
        import sys
        self.inp_file_name = ""
        try:
            inp_writer = iw.FemInputWriterCcx(self.analysis, self.solver,
                                              self.mesh, self.materials,
                                              self.fixed_constraints,
                                              self.selfweight_constraints, self.force_constraints, self.pressure_constraints,
                                              self.displacement_constraints,
                                              self.beam_sections, self.shell_thicknesses,
                                              self.analysis_type, self.eigenmode_parameters,
                                              self.working_dir)
            self.inp_file_name = inp_writer.write_calculix_input_file()
        except:
            print("Unexpected error when writing CalculiX input file:", sys.exc_info()[0])
            raise

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

    def load_results(self):
        self.results_present = False
        self.load_results_ccxfrd()
        self.load_results_ccxdat()

    ## Load results of ccx calculations from .frd file.
    #  @param self The python object self
    def load_results_ccxfrd(self):
        import os
        import ccxFrdReader
        frd_result_file = os.path.splitext(self.inp_file_name)[0] + '.frd'
        if os.path.isfile(frd_result_file):
            result_name_prefix = 'CalculiX_' + self.solver.AnalysisType + '_'
            ccxFrdReader.importFrd(frd_result_file, self.analysis, result_name_prefix)
            for m in self.analysis.Member:
                if m.isDerivedFrom("Fem::FemResultObject"):
                    self.results_present = True
        else:
            raise Exception('FEM: No results found at {}!'.format(frd_result_file))

    ## Load results of ccx calculations from .dat file.
    #  @param self The python object self
    def load_results_ccxdat(self):
        import os
        import ccxDatReader
        dat_result_file = os.path.splitext(self.inp_file_name)[0] + '.dat'
        if os.path.isfile(dat_result_file):
            mode_frequencies = ccxDatReader.import_dat(dat_result_file, self.analysis)
        else:
            raise Exception('FEM: No .dat results found at {}!'.format(dat_result_file))
        if mode_frequencies:
            print(mode_frequencies)
            for m in self.analysis.Member:
                if m.isDerivedFrom("Fem::FemResultObject") and m.Eigenmode > 0:
                    for mf in mode_frequencies:
                        if m.Eigenmode == mf['eigenmode']:
                            m.EigenmodeFrequency = mf['frequency']
