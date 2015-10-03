from Preimport import *

class JobControlTaskPanel:
    """
    """
    def __init__(self, analysis_object):
        """
        """
        self.form = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Fem/CaeAnalysis.ui") #<FemToCae> changed name
        #self.prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem")
    
        self.analysis_object = analysis_object
        import CaeSolver
        self.solver_object = CaeSolver.makeCaeSolver('Calculix')
        #Femgui.getActiveAnalysis().Proxy.solver  #CaeAnalysis instance has no attribute  error!
        #self.solver_object is solver_process, emit process started and finished signal
        #self.solver_object = analysis_object.solver.setup_solver().  should be called in other place, or when created!
        
        self.Timer = QtCore.QTimer()
        self.Timer.start(300)  #why? time out?

        self.fc_console_message = ''

        #Connect Signals and Slots
        QtCore.QObject.connect(self.form.tb_choose_working_dir, QtCore.SIGNAL("clicked()"), self.choose_working_dir)
        QtCore.QObject.connect(self.form.pushButton_write, QtCore.SIGNAL("clicked()"), self.write_input_file_handler)
        #editSolverInputFile() method has been moved into the concrete solver class
        QtCore.QObject.connect(self.form.pushButton_edit, QtCore.SIGNAL("clicked()"), self.solver_object.editSolverInputFile) # slot name changed
        #rename: self.form.pushButton_generate -> self.form.pushButton_run ?
        QtCore.QObject.connect(self.form.pushButton_generate, QtCore.SIGNAL("clicked()"), self.runSolver)

        
        QtCore.QObject.connect(self.solver_object , QtCore.SIGNAL("started()"), self.solverStarted)
        QtCore.QObject.connect(self.solver_object , QtCore.SIGNAL("finished(int)"), self.solverFinished)
        #These signal is not supported by CaeSolver yet!!!,
        #QtCore.QObject.connect(self.solver_object , QtCore.SIGNAL("stateChanged(QProcess::ProcessState)"), self.solverStateChanged)
        #QtCore.QObject.connect(self.solver_object, QtCore.SIGNAL("hasError(QProcess::ProcessError)"), self.solverError)
        
        QtCore.QObject.connect(self.Timer, QtCore.SIGNAL("timeout()"), self.UpdateText)

        self.update()

    def fcConsoleMessage(self, message="", color="#000000"):
        self.fc_console_message = self.fc_console_message + '<font color="#0000FF">{0:4.1f}:</font> <font color="{1}">{2}</font><br>'.\
            format(time.time() - self.Start, color, message.encode('utf-8', 'replace'))
        self.form.textEdit_Output.setText(self.fc_console_message)
        self.form.textEdit_Output.moveCursor(QtGui.QTextCursor.End)

    def printSolverStdout(self):
        out = self.solver_object.readAllStandardOutput()
        if out.isEmpty():
            self.fcConsoleMessage("Solver stdout is empty", "#FF0000")
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
                self.fcConsoleMessage(out.replace('\n', '<br>'))
            except UnicodeDecodeError:
                self.fcConsoleMessage("Error converting stdout from Solver", "#FF0000")

    def UpdateText(self): # name should begin with lower case word!
        if(self.solver_object.state() == QtCore.QProcess.ProcessState.Running):
            self.form.label_Time.setText('Time: {0:4.1f}: '.format(time.time() - self.Start))

    def solverError(self, error):
        print "Error()", error
        self.fcConsoleMessage("Solver execute error: {}".format(error), "#FF0000")

    def solverStarted(self):
        print "SolverStarted()"
        print self.solver_object.state()
        self.form.pushButton_generate.setText("Break Solver")
    """
    def solverStateChanged(self, newState):
        if (newState == QtCore.QProcess.ProcessState.Starting):
                self.fcConsoleMessage("Starting Solver...")
        if (newState == QtCore.QProcess.ProcessState.Running):
                self.fcConsoleMessage("Solver is running...")
        if (newState == QtCore.QProcess.ProcessState.NotRunning):
                self.fcConsoleMessage("Solver stopped.")
    """
    def solverFinished(self, exitCode):
        print "SolverFinished()", exitCode
        print self.solver_object.state()

        self.printSolverstdout()
        self.Timer.stop()

        self.fcConsoleMessage("Solver done!", "#00AA00")

        self.form.pushButton_generate.setText("Re-run Solver")
        print "Loading results...."
        self.fcConsoleMessage("Loading result sets...")
        self.form.label_Time.setText('Time: {0:4.1f}: '.format(time.time() - self.Start))
        
        self.solver_object.reset_all()
        if os.path.isfile(self.base_name + '.frd'):
            QApplication.setOverrideCursor(Qt.WaitCursor)
            self.solver_object.importResult( self.activeAnalysis)
            QApplication.restoreOverrideCursor()
            self.fcConsoleMessage("Loading results done!", "#00AA00")
        else:
            self.fcConsoleMessage("Loading results failed! Results file doesn\'t exist", "#FF0000")
        self.form.label_Time.setText('Time: {0:4.1f}: '.format(time.time() - self.Start))

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Close)

    def update(self):
        'fills the widgets'
        self.form.le_working_dir.setText(self.solver_object.working_dir)
        return

    def accept(self):
        FreeCADGui.Control.closeDialog()

    def reject(self):
        FreeCADGui.Control.closeDialog()

    def choose_working_dir(self):
        self.working_dir = QtGui.QFileDialog.getExistingDirectory(None,
                                                                  'Choose Solver working directory',
                                                                  self.solver_object.prefs.GetString("WorkingDir", '/tmp'))
        if self.working_dir:
            self.solver_object.prefs.SetString("WorkingDir", str(self.working_dir))
            self.form.le_working_dir.setText(self.working_dir)

    def write_input_file_handler(self):
        QApplication.restoreOverrideCursor()
        if self.check_prerequisites_helper():
            QApplication.setOverrideCursor(Qt.WaitCursor)
            self.base_name = "" #why? 
            
            self.fcConsoleMessage("type of self.solver_object: "+type(self.solver_object))
            self.solver_object.write_case_file()# #check and update_objects() is called inside this function
            
            if self.solver_object.base_name != "":
                self.base_name = self.solver_object.base_name
                self.fcConsoleMessage("Write completed.")
                self.form.pushButton_edit.setEnabled(True)
                self.form.pushButton_generate.setEnabled(True)
            else:
                self.fcConsoleMessage("Write input case file failed!", "#FF0000")
            QApplication.restoreOverrideCursor()

    def check_prerequisites_helper(self):
        self.Start = time.time()
        self.fcConsoleMessage("Check dependencies...")
        self.form.label_Time.setText('Time: {0:4.1f}: '.format(time.time() - self.Start))

        self.solver_object.update_objects()
        message = self.solver_object.check_prerequisites()
        if message != "":
            QtGui.QMessageBox.critical(None, "Missing prerequisit(s)", message)
            return False
        return True


    def runSolver(self):
        print 'runSolver'
        self.Start = time.time()

        self.fcConsoleMessage("Solver binary: {}".format(self.solver_object.solver_binary ))
        self.fcConsoleMessage("Run Solver...")

        # run Solver
        print 'run Solver at: ', self.solver_object.solver_binary , '  with: ', self.solver_object.base_name

        self.solver_object.run()

        QApplication.restoreOverrideCursor()