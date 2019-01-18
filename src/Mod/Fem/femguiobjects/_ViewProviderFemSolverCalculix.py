# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2015 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD FEM solver calculix ViewProvider for the document object"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package ViewProviderFemSolverCalculix
#  \ingroup FEM
#  \brief FreeCAD FEM _ViewProviderFemSolverCalculix

import FreeCAD
import FreeCADGui
import FemGui

# for the panel
from femtools import ccxtools
from PySide import QtCore
from PySide import QtGui
from PySide.QtCore import Qt
from PySide.QtGui import QApplication
import os
import time
import sys
if sys.version_info.major >= 3:
    def unicode(text, *args):
        return str(text)


class _ViewProviderFemSolverCalculix:
    "A View Provider for the FemSolverCalculix object"

    def __init__(self, vobj):
        vobj.Proxy = self

    def getIcon(self):
        return ":/icons/fem-solver-standard.svg"

    def attach(self, vobj):
        self.ViewObject = vobj
        self.Object = vobj.Object

    def updateData(self, obj, prop):
        return

    def onChanged(self, vobj, prop):
        return

    def setEdit(self, vobj, mode=0):
        taskd = _TaskPanelFemSolverCalculix(self.Object)
        FreeCADGui.Control.showDialog(taskd)
        return True

    def unsetEdit(self, vobj, mode=0):
        FreeCADGui.Control.closeDialog()
        return True

    def doubleClicked(self, vobj):
        doc = FreeCADGui.getDocument(vobj.Object.Document)
        if not doc.getInEdit():
            # may be go the other way around and just activate the analysis the user has doubleClicked on ?!
            if FemGui.getActiveAnalysis() is not None:
                if FemGui.getActiveAnalysis().Document is FreeCAD.ActiveDocument:
                    if self.Object in FemGui.getActiveAnalysis().Group:
                        doc.setEdit(vobj.Object.Name)
                    else:
                        FreeCAD.Console.PrintError('Activate the analysis this solver belongs to!\n')
                else:
                    FreeCAD.Console.PrintError('Active Analysis is not in active Document!\n')
            else:
                FreeCAD.Console.PrintError('No active Analysis found!\n')
        else:
            from PySide.QtGui import QMessageBox
            message = 'Active Task Dialog found! Please close this one before open a new one!'
            QMessageBox.critical(None, "Error in tree view", message)
            FreeCAD.Console.PrintError(message + '\n')
        return True

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None


class _TaskPanelFemSolverCalculix:
    '''The TaskPanel for CalculiX ccx tools solver object'''

    def __init__(self, solver_object):
        self.form = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/SolverCalculix.ui")
        self.ccx_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/Ccx")
        ccx_binary = self.ccx_prefs.GetString("ccxBinaryPath", "")
        if ccx_binary:
            self.CalculixBinary = ccx_binary
            print("Using CalculiX binary path from FEM preferences: {}".format(ccx_binary))
        else:
            from platform import system
            if system() == 'Linux':
                self.CalculixBinary = 'ccx'
            elif system() == 'Windows':
                self.CalculixBinary = FreeCAD.getHomePath() + 'bin/ccx.exe'
            else:
                self.CalculixBinary = 'ccx'

        self.solver_object = solver_object

        self.Calculix = QtCore.QProcess()
        self.Timer = QtCore.QTimer()
        self.Timer.start(300)

        self.fem_console_message = ''

        # Connect Signals and Slots
        QtCore.QObject.connect(self.form.tb_choose_working_dir, QtCore.SIGNAL("clicked()"), self.choose_working_dir)
        QtCore.QObject.connect(self.form.pb_write_inp, QtCore.SIGNAL("clicked()"), self.write_input_file_handler)
        QtCore.QObject.connect(self.form.pb_edit_inp, QtCore.SIGNAL("clicked()"), self.editCalculixInputFile)
        QtCore.QObject.connect(self.form.pb_run_ccx, QtCore.SIGNAL("clicked()"), self.runCalculix)
        QtCore.QObject.connect(self.form.rb_static_analysis, QtCore.SIGNAL("clicked()"), self.select_static_analysis)
        QtCore.QObject.connect(self.form.rb_frequency_analysis, QtCore.SIGNAL("clicked()"), self.select_frequency_analysis)
        QtCore.QObject.connect(self.form.rb_thermomech_analysis, QtCore.SIGNAL("clicked()"), self.select_thermomech_analysis)

        QtCore.QObject.connect(self.Calculix, QtCore.SIGNAL("started()"), self.calculixStarted)
        QtCore.QObject.connect(self.Calculix, QtCore.SIGNAL("stateChanged(QProcess::ProcessState)"), self.calculixStateChanged)
        QtCore.QObject.connect(self.Calculix, QtCore.SIGNAL("error(QProcess::ProcessError)"), self.calculixError)
        QtCore.QObject.connect(self.Calculix, QtCore.SIGNAL("finished(int)"), self.calculixFinished)

        QtCore.QObject.connect(self.Timer, QtCore.SIGNAL("timeout()"), self.UpdateText)

        self.update()

    def getStandardButtons(self):
        # only show a close button
        # def accept() in no longer needed, since there is no OK button
        return int(QtGui.QDialogButtonBox.Close)

    def reject(self):
        FreeCADGui.ActiveDocument.resetEdit()

    def update(self):
        'fills the widgets'
        self.form.le_working_dir.setText(self.solver_object.WorkingDir)
        if self.solver_object.AnalysisType == 'static':
            self.form.rb_static_analysis.setChecked(True)
        elif self.solver_object.AnalysisType == 'frequency':
            self.form.rb_frequency_analysis.setChecked(True)
        elif self.solver_object.AnalysisType == 'thermomech':
            self.form.rb_thermomech_analysis.setChecked(True)
        return

    def femConsoleMessage(self, message="", color="#000000"):
        if sys.version_info.major < 3:
            message = message.encode("utf-8", "replace")
        self.fem_console_message = self.fem_console_message + '<font color="#0000FF">{0:4.1f}:</font> <font color="{1}">{2}</font><br>'.\
            format(time.time() - self.Start, color, message)
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
                print(rx)
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
        if '*ERROR in e_c3d: nonpositive jacobian' in out:
            error_message = (
                "\n\nCalculiX returned an error due to "
                "nonpositive jacobian determinant in at least one element\n"
                "Use the run button on selected solver to get a better error output.\n"
            )
            FreeCAD.Console.PrintError(error_message)
        if '*ERROR' in out:
            return False
        else:
            return True

    def UpdateText(self):
        if(self.Calculix.state() == QtCore.QProcess.ProcessState.Running):
            self.form.l_time.setText('Time: {0:4.1f}: '.format(time.time() - self.Start))

    def calculixError(self, error=''):
        print("Error() {}".format(error))
        self.femConsoleMessage("CalculiX execute error: {}".format(error), "#FF0000")

    def calculixNoError(self):
        print("CalculiX done without error!")
        self.femConsoleMessage("CalculiX done without error!", "#00AA00")

    def calculixStarted(self):
        print("calculixStarted()")
        print(self.Calculix.state())
        self.form.pb_run_ccx.setText("Break CalculiX")

    def calculixStateChanged(self, newState):
        if (newState == QtCore.QProcess.ProcessState.Starting):
                self.femConsoleMessage("Starting CalculiX...")
        if (newState == QtCore.QProcess.ProcessState.Running):
                self.femConsoleMessage("CalculiX is running...")
        if (newState == QtCore.QProcess.ProcessState.NotRunning):
                self.femConsoleMessage("CalculiX stopped.")

    def calculixFinished(self, exitCode):
        print("calculixFinished() {}".format(exitCode))
        print(self.Calculix.state())

        # Restore previous cwd
        QtCore.QDir.setCurrent(self.cwd)

        self.Timer.stop()

        if self.printCalculiXstdout():
            self.calculixNoError()
        else:
            self.calculixError()

        self.form.pb_run_ccx.setText("Re-run CalculiX")
        self.femConsoleMessage("Loading result sets...")
        self.form.l_time.setText('Time: {0:4.1f}: '.format(time.time() - self.Start))
        fea = ccxtools.FemToolsCcx(None, self.solver_object)
        fea.reset_mesh_purge_results_checked()
        fea.inp_file_name = self.inp_file_name
        QApplication.setOverrideCursor(Qt.WaitCursor)
        try:
            fea.load_results()
        except:
            QApplication.restoreOverrideCursor()
            majorVersion, minorVersion = fea.get_ccx_version()
            if majorVersion == 2 and minorVersion <= 10:
                message = "The used CalculiX version {}.{} creates broken output files.\n" \
                    "Please upgrade to a newer version.".format(majorVersion, minorVersion)
                QtGui.QMessageBox.warning(None, "Upgrade CalculiX", message)
            raise
        else:
            QApplication.restoreOverrideCursor()
            self.form.l_time.setText('Time: {0:4.1f}: '.format(time.time() - self.Start))

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
        self.Start = time.time()
        self.form.l_time.setText('Time: {0:4.1f}: '.format(time.time() - self.Start))
        QApplication.restoreOverrideCursor()
        if self.check_prerequisites_helper():
            QApplication.setOverrideCursor(Qt.WaitCursor)
            self.inp_file_name = ""
            fea = ccxtools.FemToolsCcx(None, self.solver_object)
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
        self.form.l_time.setText('Time: {0:4.1f}: '.format(time.time() - self.Start))

    def check_prerequisites_helper(self):
        self.Start = time.time()
        self.femConsoleMessage("Check dependencies...")
        self.form.l_time.setText('Time: {0:4.1f}: '.format(time.time() - self.Start))

        fea = ccxtools.FemToolsCcx(None, self.solver_object)
        fea.update_objects()
        message = fea.check_prerequisites()
        if message != "":
            QtGui.QMessageBox.critical(None, "Missing prerequisite(s)", message)
            return False
        return True

    def start_ext_editor(self, ext_editor_path, filename):
        if not hasattr(self, "ext_editor_process"):
            self.ext_editor_process = QtCore.QProcess()
        if self.ext_editor_process.state() != QtCore.QProcess.Running:
            self.ext_editor_process.start(ext_editor_path, [filename])

    def editCalculixInputFile(self):
        print('editCalculixInputFile {}'.format(self.inp_file_name))
        if self.ccx_prefs.GetBool("UseInternalEditor", True):
            FemGui.open(self.inp_file_name)
        else:
            ext_editor_path = self.ccx_prefs.GetString("ExternalEditorPath", "")
            if ext_editor_path:
                self.start_ext_editor(ext_editor_path, self.inp_file_name)
            else:
                print("External editor is not defined in FEM preferences. Falling back to internal editor")
                FemGui.open(self.inp_file_name)

    def runCalculix(self):
        print('runCalculix')
        self.Start = time.time()

        self.femConsoleMessage("CalculiX binary: {}".format(self.CalculixBinary))
        self.femConsoleMessage("Run CalculiX...")

        # run Calculix
        print('run CalculiX at: {} with: {}'.format(self.CalculixBinary, os.path.splitext(self.inp_file_name)[0]))
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

    def select_thermomech_analysis(self):
        self.select_analysis_type('thermomech')

    # That function overlaps with ccxtools setup_working_dir and could be removed when the one from ccxtools would be used
    def setup_working_dir(self):
        wd = self.solver_object.WorkingDir
        if not (os.path.isdir(wd)):
            try:
                os.makedirs(wd)
            except:
                print("Dir \'{}\' from FEM preferences doesn't exist and cannot be created.".format(wd))
                import tempfile
                wd = tempfile.gettempdir()
                print("Dir \'{}\' will be used instead.".format(wd))
        return wd
