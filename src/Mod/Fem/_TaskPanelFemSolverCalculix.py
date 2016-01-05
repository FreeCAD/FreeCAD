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

__title__ = "CalculiX Job Control Task Panel"
__author__ = "Juergen Riegel"
__url__ = "http://www.freecadweb.org"

from FemTools import FemTools
import FreeCAD
import os
import time

if FreeCAD.GuiUp:
    import FreeCADGui
    import FemGui
    from PySide import QtCore, QtGui
    from PySide.QtCore import Qt
    from PySide.QtGui import QApplication


class _TaskPanelFemSolverCalculix:
    def __init__(self, solver_object):
        self.form = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Fem/TaskPanelFemSolverCalculix.ui")
        self.fem_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem")
        ccx_binary = self.fem_prefs.GetString("ccxBinaryPath", "")
        if ccx_binary:
            self.CalculixBinary = ccx_binary
            print ("Using CalculiX binary path from FEM preferences: {}".format(ccx_binary))
        else:
            from platform import system
            if system() == 'Linux':
                self.CalculixBinary = 'ccx'
            elif system() == 'Windows':
                self.CalculixBinary = FreeCAD.getHomePath() + 'bin/ccx.exe'
            else:
                self.CalculixBinary = 'ccx'
        self.fem_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem")

        self.solver_object = solver_object

        self.Calculix = QtCore.QProcess()
        self.Timer = QtCore.QTimer()
        self.Timer.start(300)

        self.fem_console_message = ''

        #Connect Signals and Slots
        QtCore.QObject.connect(self.form.tb_choose_working_dir, QtCore.SIGNAL("clicked()"), self.choose_working_dir)
        QtCore.QObject.connect(self.form.pb_write_inp, QtCore.SIGNAL("clicked()"), self.write_input_file_handler)
        QtCore.QObject.connect(self.form.pb_edit_inp, QtCore.SIGNAL("clicked()"), self.editCalculixInputFile)
        QtCore.QObject.connect(self.form.pb_run_ccx, QtCore.SIGNAL("clicked()"), self.runCalculix)
        QtCore.QObject.connect(self.form.rb_static_analysis, QtCore.SIGNAL("clicked()"), self.select_static_analysis)
        QtCore.QObject.connect(self.form.rb_frequency_analysis, QtCore.SIGNAL("clicked()"), self.select_frequency_analysis)

        QtCore.QObject.connect(self.Calculix, QtCore.SIGNAL("started()"), self.calculixStarted)
        QtCore.QObject.connect(self.Calculix, QtCore.SIGNAL("stateChanged(QProcess::ProcessState)"), self.calculixStateChanged)
        QtCore.QObject.connect(self.Calculix, QtCore.SIGNAL("error(QProcess::ProcessError)"), self.calculixError)
        QtCore.QObject.connect(self.Calculix, QtCore.SIGNAL("finished(int)"), self.calculixFinished)

        QtCore.QObject.connect(self.Timer, QtCore.SIGNAL("timeout()"), self.UpdateText)

        self.update()

    def femConsoleMessage(self, message="", color="#000000"):
        self.fem_console_message = self.fem_console_message + '<font color="#0000FF">{0:4.1f}:</font> <font color="{1}">{2}</font><br>'.\
            format(time.time() - self.Start, color, message.encode('utf-8', 'replace'))
        self.form.textEdit_Output.setText(self.fem_console_message)
        self.form.textEdit_Output.moveCursor(QtGui.QTextCursor.End)

    def printCalculiXstdout(self):
        out = self.Calculix.readAllStandardOutput()
        if out.isEmpty():
            self.femConsoleMessage("CalculiX stdout is empty", "#FF0000")
        else:
            try:
                out = unicode(out, 'utf-8', 'replace')
                rx = QtCore.QRegExp("\\*ERROR.*\\n\\n")
                rx.setMinimal(True)
                pos = rx.indexIn(out)
                while not pos < 0:
                    match = rx.cap(0)
                    FreeCAD.Console.PrintError(match.strip().replace('\n', ' ') + '\n')
                    pos = rx.indexIn(out, pos + 1)
                out = os.linesep.join([s for s in out.splitlines() if s])
                self.femConsoleMessage(out.replace('\n', '<br>'))
            except UnicodeDecodeError:
                self.femConsoleMessage("Error converting stdout from CalculiX", "#FF0000")

    def UpdateText(self):
        if(self.Calculix.state() == QtCore.QProcess.ProcessState.Running):
            self.form.l_time.setText('Time: {0:4.1f}: '.format(time.time() - self.Start))

    def calculixError(self, error):
        print ("Error() {}".format(error))
        self.femConsoleMessage("CalculiX execute error: {}".format(error), "#FF0000")

    def calculixStarted(self):
        print ("calculixStarted()")
        print (self.Calculix.state())
        self.form.pb_run_ccx.setText("Break CalculiX")

    def calculixStateChanged(self, newState):
        if (newState == QtCore.QProcess.ProcessState.Starting):
                self.femConsoleMessage("Starting CalculiX...")
        if (newState == QtCore.QProcess.ProcessState.Running):
                self.femConsoleMessage("CalculiX is running...")
        if (newState == QtCore.QProcess.ProcessState.NotRunning):
                self.femConsoleMessage("CalculiX stopped.")

    def calculixFinished(self, exitCode):
        print ("calculixFinished() {}".format(exitCode))
        print (self.Calculix.state())

        # Restore previous cwd
        QtCore.QDir.setCurrent(self.cwd)

        self.printCalculiXstdout()
        self.Timer.stop()

        self.femConsoleMessage("CalculiX done!", "#00AA00")

        self.form.pb_run_ccx.setText("Re-run CalculiX")
        self.femConsoleMessage("Loading result sets...")
        self.form.l_time.setText('Time: {0:4.1f}: '.format(time.time() - self.Start))
        fea = FemTools()
        fea.reset_all()
        fea.inp_file_name = self.inp_file_name
        QApplication.setOverrideCursor(Qt.WaitCursor)
        fea.load_results()
        QApplication.restoreOverrideCursor()
        self.form.l_time.setText('Time: {0:4.1f}: '.format(time.time() - self.Start))

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Close)

    def update(self):
        'fills the widgets'
        self.form.le_working_dir.setText(self.solver_object.WorkingDir)
        if self.solver_object.AnalysisType == 'static':
            self.form.rb_static_analysis.setChecked(True)
        elif self.solver_object.AnalysisType == 'frequency':
            self.form.rb_frequency_analysis.setChecked(True)
        return

    def accept(self):
        FreeCADGui.ActiveDocument.resetEdit()

    def reject(self):
        FreeCADGui.ActiveDocument.resetEdit()

    def choose_working_dir(self):
        current_wd = self.setup_working_dir()
        wd = QtGui.QFileDialog.getExistingDirectory(None, 'Choose CalculiX working directory',
                                                    current_wd)
        if wd:
            self.solver_object.WorkingDir = wd
        else:
            self.solver_object.WorkingDir = current_wd
        self.form.le_working_dir.setText(self.solver_object.WorkingDir)

    def write_input_file_handler(self):
        QApplication.restoreOverrideCursor()
        if self.check_prerequisites_helper():
            QApplication.setOverrideCursor(Qt.WaitCursor)
            self.inp_file_name = ""
            fea = FemTools()
            fea.set_analysis_type(self.solver_object.AnalysisType)
            fea.update_objects()
            fea.write_inp_file()
            if fea.inp_file_name != "":
                self.inp_file_name = fea.inp_file_name
                self.femConsoleMessage("Write completed.")
                self.form.pb_edit_inp.setEnabled(True)
                self.form.pb_run_ccx.setEnabled(True)
            else:
                self.femConsoleMessage("Write .inp file failed!", "#FF0000")
            QApplication.restoreOverrideCursor()

    def check_prerequisites_helper(self):
        self.Start = time.time()
        self.femConsoleMessage("Check dependencies...")
        self.form.l_time.setText('Time: {0:4.1f}: '.format(time.time() - self.Start))

        fea = FemTools()
        fea.update_objects()
        message = fea.check_prerequisites()
        if message != "":
            QtGui.QMessageBox.critical(None, "Missing prerequisit(s)", message)
            return False
        return True

    def start_ext_editor(self, ext_editor_path, filename):
        if not hasattr(self, "ext_editor_process"):
            self.ext_editor_process = QtCore.QProcess()
        if self.ext_editor_process.state() != QtCore.QProcess.Running:
            self.ext_editor_process.start(ext_editor_path, [filename])

    def editCalculixInputFile(self):
        print ('editCalculixInputFile {}'.format(self.inp_file_name))
        if self.fem_prefs.GetBool("UseInternalEditor", True):
            FemGui.open(self.inp_file_name)
        else:
            ext_editor_path = self.fem_prefs.GetString("ExternalEditorPath", "")
            if ext_editor_path:
                self.start_ext_editor(ext_editor_path, self.inp_file_name)
            else:
                print ("External editor is not defined in FEM preferences. Falling back to internal editor")
                FemGui.open(self.inp_file_name)

    def runCalculix(self):
        print ('runCalculix')
        self.Start = time.time()

        self.femConsoleMessage("CalculiX binary: {}".format(self.CalculixBinary))
        self.femConsoleMessage("Run CalculiX...")

        # run Calculix
        print ('run CalculiX at: {} with: {}'.format(self.CalculixBinary, os.path.splitext(self.inp_file_name)[0]))
        # change cwd because ccx may crash if directory has no write permission
        # there is also a limit of the length of file names so jump to the document directory
        self.cwd = QtCore.QDir.currentPath()
        fi = QtCore.QFileInfo(self.inp_file_name)
        QtCore.QDir.setCurrent(fi.path())
        self.Calculix.start(self.CalculixBinary, ['-i', fi.baseName()])

        QApplication.restoreOverrideCursor()

    def select_analysis_type(self, analysis_type):
        if self.solver_object.AnalysisType != analysis_type:
            self.solver_object.AnalysisType = analysis_type
            self.form.pb_edit_inp.setEnabled(False)
            self.form.pb_run_ccx.setEnabled(False)

    def select_static_analysis(self):
        self.select_analysis_type('static')

    def select_frequency_analysis(self):
        self.select_analysis_type('frequency')

    # That function overlaps with FemTools setup_working_dir and needs to be removed when we migrate fully to FemTools
    def setup_working_dir(self):
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
