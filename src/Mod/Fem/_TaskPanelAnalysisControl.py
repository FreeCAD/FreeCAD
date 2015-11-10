#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2013-2015 - Juergen Riegel <FreeCAD@juergen-riegel.net> *
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

__title__ = "Job Control Task Panel"
__author__ = "Juergen Riegel"
__url__ = "http://www.freecadweb.org"

from FemTools import FemTools
import FreeCAD
import ccxFrdReader
import os
import os.path
import time

if FreeCAD.GuiUp:
    import FreeCADGui
    import FemGui
    from PySide import QtCore
    from PySide import QtGui
    from PySide.QtCore import Qt
    from PySide.QtGui import QApplication


class _TaskPanelAnalysisControl:
    def __init__(self, analysis_object):
        self.form = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Fem/TaskPanelAnalysisControl.ui")
        self.fem_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem")

        self.analysis_object = analysis_object
        import CaeTools
        self.solver_object = CaeTools.getSolver(self.analysis_object)
        # test if Proxy instance is not None and correct type, or create a new python instance
        self.solver_python_object = CaeTools.getSolverPythonFromAnalysis(self.analysis_object)

        self.SolverProcess = QtCore.QProcess()
        self.Timer = QtCore.QTimer()
        self.Timer.start(300)
        
        # update UI text according to solver info, replace sover with specific solver name

        # consider: moved into FemSolverObject.Proxy class
        if self.solver_object.SolverName == "Calculix":
            ccx_binary = self.fem_prefs.GetString("ccxBinaryPath", "")
            if ccx_binary:
                self.CalculixBinary = ccx_binary
                print ("Using ccx binary path from FEM preferences: {}".format(ccx_binary))
            else:
                from platform import system
                if system() == 'Linux':
                    self.CalculixBinary = 'ccx'
                elif system() == 'Windows':
                    self.CalculixBinary = FreeCAD.getHomePath() + 'bin/ccx.exe'
                else:
                    self.CalculixBinary = 'ccx'
            self.fem_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem")  # why twice? refresh?

        self.fem_console_message = ''

        # Connect Signals and Slots
        QtCore.QObject.connect(self.form.tb_choose_working_dir, QtCore.SIGNAL("clicked()"), self.choose_working_dir)
        QtCore.QObject.connect(self.form.pb_write_inp, QtCore.SIGNAL("clicked()"), self.write_input_file_handler)
        QtCore.QObject.connect(self.form.pb_edit_inp, QtCore.SIGNAL("clicked()"), self.editSolverInputFile)
        QtCore.QObject.connect(self.form.pb_run_solver, QtCore.SIGNAL("clicked()"), self.runSolverProcess)
        # combobox is preferred
        QtCore.QObject.connect(self.form.rb_static_analysis, QtCore.SIGNAL("clicked()"), self.select_static_analysis)
        QtCore.QObject.connect(self.form.rb_frequency_analysis, QtCore.SIGNAL("clicked()"), self.select_frequency_analysis)

        QtCore.QObject.connect(self.SolverProcess, QtCore.SIGNAL("started()"), self.solverProcessStarted)
        QtCore.QObject.connect(self.SolverProcess, QtCore.SIGNAL("stateChanged(QProcess::ProcessState)"), self.solverProcessStateChanged)
        QtCore.QObject.connect(self.SolverProcess, QtCore.SIGNAL("error(QProcess::ProcessError)"), self.solverProcessError)
        QtCore.QObject.connect(self.SolverProcess, QtCore.SIGNAL("finished(int)"), self.solverProcessFinished)

        QtCore.QObject.connect(self.Timer, QtCore.SIGNAL("timeout()"), self.updateText)
        self.update()

    def femConsoleMessage(self, message="", color="#000000"):
        self.fem_console_message = self.fem_console_message + '<font color="#0000FF">{0:4.1f}:</font> <font color="{1}">{2}</font><br>'.\
            format(time.time() - self.Start, color, message.encode('utf-8', 'replace'))
        self.form.textEdit_Output.setText(self.fem_console_message)
        self.form.textEdit_Output.moveCursor(QtGui.QTextCursor.End)

    def printSolverProcessStdout(self):
        out = self.SolverProcess.readAllStandardOutput()
        if out.isEmpty():
            self.femConsoleMessage("Solver stdout is empty", "#FF0000")
        else:
            try:
                out = unicode(out, 'utf-8', 'replace')
                rx = QtCore.QRegExp("\\*ERROR.*\\n\\n")  # this is solver specific feature
                rx.setMinimal(True)
                pos = rx.indexIn(out)
                while not pos < 0:
                    match = rx.cap(0)
                    FreeCAD.Console.PrintError(match.strip().replace('\n', ' ') + '\n')
                    pos = rx.indexIn(out, pos + 1)
                out = os.linesep.join([s for s in out.splitlines() if s])
                self.femConsoleMessage(out.replace('\n', '<br>'))
            except UnicodeDecodeError:
                self.femConsoleMessage("Error converting stdout from Solver", "#FF0000")

    def updateText(self):
        if(self.SolverProcess.state() == QtCore.QProcess.ProcessState.Running):
            self.form.l_time.setText('Time: {0:4.1f}: '.format(time.time() - self.Start))

    def solverProcessError(self, error):
        print ("Error() {}".format(error))
        self.femConsoleMessage("Solver execute error: {}".format(error), "#FF0000")

    def solverProcessStarted(self):
        print ("calculixStarted()")
        print (self.SolverProcess.state())
        self.form.pb_run_solver.setText("Break Solver process")

    def solverProcessStateChanged(self, newState):
        if (newState == QtCore.QProcess.ProcessState.Starting):
            self.femConsoleMessage("Starting Solver...")
        if (newState == QtCore.QProcess.ProcessState.Running):
            self.femConsoleMessage("Solver is running...")
        if (newState == QtCore.QProcess.ProcessState.NotRunning):
            self.femConsoleMessage("Solver stopped.")

    def solverProcessFinished(self, exitCode):
        print ("Solver Process Finished() {}".format(exitCode))
        print self.SolverProcess.state()

        # Restore previous cwd
        QtCore.QDir.setCurrent(self.cwd)

        self.printSolverProcessStdout()
        self.Timer.stop()
        self.femConsoleMessage("External solver process is done!", "#00AA00")
        self.form.pb_run_solver.setText("Re-run Solver")

        if self.solver_object.SolverName == "Calculix":
            self.femConsoleMessage("Loading result sets...")
            self.form.l_time.setText('Time: {0:4.1f}: '.format(time.time() - self.Start))
            fea = FemTools()
            fea.reset_all()
            frd_result_file = os.path.splitext(self.inp_file_name)[0] + '.frd'
            if os.path.isfile(frd_result_file):
                QApplication.setOverrideCursor(Qt.WaitCursor)
                ccxFrdReader.importFrd(frd_result_file, FemGui.getActiveAnalysis())
                self.femConsoleMessage("Loading results done!", "#00AA00")
            else:
                self.femConsoleMessage("Loading results failed! Results file doesn\'t exist", "#FF0000")
            self.form.l_time.setText('Time: {0:4.1f}: '.format(time.time() - self.Start))
        else:  # self.solver_object.SolverName == "OpenFOAM":
            self.femConsoleMessage("Please use external view to view the result", "#00AA00")
            self.solver_python_object.view_result_externally()
        QApplication.restoreOverrideCursor()

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Close)

    def update(self):
        'fills the widgets'
        if self.solver_object.SolverName == "Calculix":
            self.form.le_working_dir.setText(self.analysis_object.WorkingDir)  # need unification
        else:
            self.form.le_working_dir.setText(self.solver_object.WorkingDir)  # need unification
        # to-do: should use combobox instead
        if self.analysis_object.AnalysisType == 'static':
            self.form.rb_static_analysis.setChecked(True)
        elif self.analysis_object.AnalysisType == 'frequency':
            self.form.rb_frequency_analysis.setChecked(True)
        return

    def accept(self):
        FreeCADGui.Control.closeDialog()

    def reject(self):
        FreeCADGui.Control.closeDialog()

    def choose_working_dir(self):
        current_wd = self.setup_working_dir()
        wd = QtGui.QFileDialog.getExistingDirectory(None,
                                                    'Choose Solver working directory',
                                                    current_wd)
        if self.solver_object.SolverName == "Calculix":
            info_obj = self.analysis_object
        else:
            info_obj = self.solver_object
        if wd:
            info_obj.WorkingDir = wd
        else:
            info_obj.WorkingDir = current_wd
        self.form.le_working_dir.setText(info_obj.WorkingDir)

    def write_input_file_handler(self):
        QApplication.restoreOverrideCursor()
        try:  # protect code from exception and freezing UI by waitCursor
            if self.check_prerequisites_helper():
                QApplication.setOverrideCursor(Qt.WaitCursor)
                if self.solver_object.SolverName == "Calculix":
                    self.inp_file_name = ""
                    fea = FemTools()
                    fea.set_analysis_type(self.analysis_object.AnalysisType)
                    fea.update_objects()
                    fea.write_inp_file()
                    if fea.inp_file_name != "":
                        self.inp_file_name = fea.inp_file_name
                        self.femConsoleMessage("Write completed.")
                        self.form.pb_edit_inp.setEnabled(True)
                        self.form.pb_run_solver.setEnabled(True)
                    else:
                        self.femConsoleMessage("Write .inp file failed!", "#FF0000")
                else:  # self.solver_object.SolverName == "OpenFOAM":
                    self.femConsoleMessage("{} case writer is called".format(self.solver_object.SolverName))
                    if self.solver_python_object.write_case(self.analysis_object):
                        self.femConsoleMessage("Write {} case is completed.".format(self.solver_object.SolverName))
                        self.form.pb_edit_inp.setEnabled(True)
                        self.form.pb_run_solver.setEnabled(True)
                    else:
                        self.femConsoleMessage("Write .inp file failed!", "#FF0000")
        finally:
            QApplication.restoreOverrideCursor()

    def check_prerequisites_helper(self):
        self.Start = time.time()
        self.femConsoleMessage("Check dependencies...")
        self.form.l_time.setText('Time: {0:4.1f}: '.format(time.time() - self.Start))

        if self.solver_object.SolverName == "Calculix":
            fea = FemTools()
            fea.update_objects()
            message = fea.check_prerequisites()
        else:
            message = self.solver_python_object.check_prerequisites(self.analysis_object)
        if message != "":
            QtGui.QMessageBox.critical(None, "Missing prerequisit(s)", message)
            return False
        return True

    def start_ext_editor(self, ext_editor_path, filename):
        if not hasattr(self, "ext_editor_process"):
            self.ext_editor_process = QtCore.QProcess()
        if self.ext_editor_process.state() != QtCore.QProcess.Running:
            self.ext_editor_process.start(ext_editor_path, [filename])

    def editSolverInputFile(self):
        if self.solver_object.SolverName == "Calculix":
            print ('editCalculixInputFile {}'.format(self.inp_file_name))
            if self.fem_prefs.GetBool("UseInternalEditor", True):
                FemGui.open(self.inp_file_name)
            else:
                ext_editor_path = self.fem_prefs.GetString("ExternalEditorPath", "")
                if ext_editor_path:
                    self.start_ext_editor(ext_editor_path, self.inp_file_name)
                else:
                    print "External editor is not defined in FEM preferences. Falling back to internal editor"
                    FemGui.open(self.inp_file_name)
        else:
            self.femConsoleMessage("Edit case input file in FreeCAD is not implemented!")
            self.solver_python_object.edit_case_externally()

    def runSolverProcess(self):
        print ('run solver process')
        self.Start = time.time()
        self.femConsoleMessage("Run {}...".format(self.solver_object.SolverName))

        if self.solver_object.SolverName == "Calculix":

            self.femConsoleMessage("Solver binary: {}".format(self.CalculixBinary))
            print('run Calculix at: ', self.CalculixBinary, ' with: ', os.path.splitext(self.inp_file_name)[0])
            # change cwd because ccx may crash if directory has no write permission
            # there is also a limit of the length of file names so jump to the document directory
            self.cwd = QtCore.QDir.currentPath()
            fi = QtCore.QFileInfo(self.inp_file_name)
            QtCore.QDir.setCurrent(fi.path())
            self.SolverProcess.start(self.CalculixBinary, ['-i', fi.baseName()])
        else:  # self.solver_object.SolverName == "OpenFOAM":
            self.femConsoleMessage("Run OpenFoam at {}".format(self.solver_object.WorkingDir))
            self.cwd = QtCore.QDir.currentPath()
            QtCore.QDir.setCurrent(self.solver_object.WorkingDir)
            self.SolverProcess.start(self.solver_python_object.generate_cmdline())

        QApplication.restoreOverrideCursor()

    def select_analysis_type(self, analysis_type):
        if self.analysis_object.AnalysisType != analysis_type:
            self.analysis_object.AnalysisType = analysis_type
            self.form.pb_edit_inp.setEnabled(False)
            self.form.pb_run_solver.setEnabled(False)

    def select_static_analysis(self):
        self.select_analysis_type('static')

    def select_frequency_analysis(self):
        self.select_analysis_type('frequency')

    # That function overlaps with FemTools setup_working_dir and needs to be removed when we migrate fully to FemTools
    def setup_working_dir(self):
        if self.solver_object.SolverName == "Calculix":
            wd = self.analysis_object.WorkingDir
        else:
            wd = self.solver_object.WorkingDir
        if not (os.path.isdir(wd)):
            try:
                os.makedirs(wd)
            except:
                print ("Dir \'{}\' from FEM preferences doesn't exist and cannot be created.".format(wd))
                import tempfile
                wd = tempfile.gettempdir()
                print ("Dir \'{}\' will be used instead.".format(wd))
        return wd
