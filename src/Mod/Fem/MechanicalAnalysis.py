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

__title__ = "Mechanical Analysis managment"
__author__ = "Juergen Riegel"
__url__ = "http://www.freecadweb.org"

#makeCaeSolver is equal to Femtools
from CaeSolver import makeCaeSolver #could be loaded in FreeCADGui.addModule(), to suport macro

#following code should equal to put all these modules into one file as "MechanicaAnalysis"
#from CaeAnalysis import     CaeAnalysis  #loaded in FreeCADGui.addModule()
#from FemResultControlTaskPanel import  ResultControlTaskPanel
#from JobControlTaskPanel import  _JobControlTaskPanel

import os
import time

from PreImport import *



class _CommandNewMechanicalAnalysis:
    "the Fem Analysis command definition"
    def GetResources(self):
        return {'Pixmap': 'fem-analysis',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Fem_Analysis", "New mechanical analysis"),
                'Accel': "N, A",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Fem_Analysis", "Create a new mechanical analysis")}

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create Analysis")
        FreeCADGui.addModule("FemGui")
        
        #FreeCADGui.doCommand("FreeCADGui.ActiveDocument.ActiveView.setAxisCross(True)") 
        #FreeCADGui.addModule("CaeAnalysis") #<FemToCae> rename done
        #FreeCADGui.doCommand("FemGui.setActiveAnalysis(CaeAnalysis.makeCaeAnalysis('MechanicalAnalysis'))") #<FemToCae> rename done
        FreeCADGui.addModule("MechanicalAnalysis") 
        FreeCADGui.doCommand("FemGui.setActiveAnalysis(MechanicalAnalysis.makeCaeAnalysis('MechanicalAnalysis'))")
        
        sel = FreeCADGui.Selection.getSelection()
        if (len(sel) == 1):
            if(sel[0].isDerivedFrom("Fem::FemMeshObject")):
                FreeCADGui.doCommand("App.activeDocument().ActiveObject.Member = App.activeDocument().ActiveObject.Member + [App.activeDocument()." + sel[0].Name + "]")
            if(sel[0].isDerivedFrom("Part::Feature")):
                FreeCADGui.doCommand("App.activeDocument().addObject('Fem::FemMeshShapeNetgenObject', '" + sel[0].Name + "_Mesh')")
                FreeCADGui.doCommand("App.activeDocument().ActiveObject.Shape = App.activeDocument()." + sel[0].Name)
                FreeCADGui.doCommand("FemGui.getActiveAnalysis().Member = FemGui.getActiveAnalysis().Member + [App.activeDocument().ActiveObject]")
                #FreeCADGui.doCommand("Gui.activeDocument().hide('" + sel[0].Name + "')")
                #FreeCADGui.doCommand("App.activeDocument().ActiveObject.touch()")
                #FreeCADGui.doCommand("App.activeDocument().recompute()")
                FreeCADGui.doCommand("Gui.activeDocument().setEdit(App.ActiveDocument.ActiveObject.Name)")

        #FreeCAD.ActiveDocument.commitTransaction()
        FreeCADGui.Selection.clearSelection()

    def IsActive(self):
        return FreeCADGui.ActiveDocument is not None and FemGui.getActiveAnalysis() is None


class _CommandFemFromShape:
    def GetResources(self):
        return {'Pixmap': 'fem-fem-mesh-from-shape',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Fem_CreateFromShape", "Create FEM mesh"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Fem_CreateFromShape", "Create FEM mesh from shape")}

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create FEM mesh")
        FreeCADGui.addModule("FemGui")
        #FreeCADGui.addModule("CaeAnalysis")  #<FemToCae> Name changed
        sel = FreeCADGui.Selection.getSelection()
        if (len(sel) == 1):
            if(sel[0].isDerivedFrom("Part::Feature")):
                FreeCADGui.doCommand("App.activeDocument().addObject('Fem::FemMeshShapeNetgenObject', '" + sel[0].Name + "_Mesh')")
                FreeCADGui.doCommand("App.activeDocument().ActiveObject.Shape = App.activeDocument()." + sel[0].Name)
                FreeCADGui.doCommand("Gui.activeDocument().setEdit(App.ActiveDocument.ActiveObject.Name)")

        FreeCADGui.Selection.clearSelection()

    def IsActive(self):
        sel = FreeCADGui.Selection.getSelection()
        if len(sel) == 1:
            return sel[0].isDerivedFrom("Part::Feature")
        return False


class _CommandMechanicalJobControl:  #consider name change from   _CommandMechanicalJobControl to _CommandAnalysisJobControl
    "the Fem JobControl command definition"
    def GetResources(self):
        return {'Pixmap': 'fem-new-analysis',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Fem_JobControl", "Start calculation"),
                'Accel': "S, C",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Fem_JobControl", "Dialog to start the calculation of the mechanical anlysis")}

    def Activated(self):
        #FreeCADGui.addModule("JobControlTaskPanel")
        #FreeCADGui.doCommand("from JobControlTaskPanel import  _JobControlTaskPanel")
        taskd = _JobControlTaskPanel(FemGui.getActiveAnalysis())  #avoid same module name and class name?!
        #taskd.obj = vobj.Object
        taskd.update()
        FreeCADGui.Control.showDialog(taskd)

    def IsActive(self):
        return FreeCADGui.ActiveDocument is not None and FemGui.getActiveAnalysis() is not None


class _CommandPurgeFemResults:
    def GetResources(self):
        return {'Pixmap': 'fem-purge-results',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Fem_PurgeResults", "Purge results"),
                'Accel': "S, S",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Fem_PurgeResults", "Purge results from an analysis")}

    def Activated(self):
        makeCaeSolver('Calculix').reset_all()  # <FemToCae>   refactoring work needed here, => getActiveAnalysis().solver

    def IsActive(self):
        return FreeCADGui.ActiveDocument is not None and results_present()


class _CommandQuickAnalysis:
    def GetResources(self):
        return {'Pixmap': 'fem-quick-analysis',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Fem_Quick_Analysis", "Run CalculiX ccx"),
                'Accel': "R, C",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Fem_Quick_Analysis", "Write .inp file and run CalculiX ccx")}

    def Activated(self):
        run_solver('static')

    def show_results_on_mesh(self):
        #FIXME proprer mesh refreshing as per FreeCAD.FEM_dialog settings required
        # or confirmation that it's safe to call restore_result_dialog
        tp = _ResultControlTaskPanel()
        tp.restore_result_dialog()

    def IsActive(self):
        return FreeCADGui.ActiveDocument is not None and FemGui.getActiveAnalysis() is not None


class _CommandFrequencyAnalysis:
    def GetResources(self):
        return {'Pixmap': 'fem-frequency-analysis',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Fem_Frequency_Analysis", "Run frequency analysis with CalculiX ccx"),
                'Accel': "R, F",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Fem_Frequency_Analysis", "Write .inp file and run frequency analysis with CalculiX ccx")}

    def Activated(self):
        run_solver('frequency')

    def IsActive(self):
        return FreeCADGui.ActiveDocument is not None and FemGui.getActiveAnalysis() is not None


class _CommandMechanicalShowResult:
    "the Fem JobControl command definition"
    def GetResources(self):
        return {'Pixmap': 'fem-result',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Fem_Result", "Show result"),
                'Accel': "S, R",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Fem_Result", "Show result information of an analysis")}

    def Activated(self):
        self.result_object = get_results_object(FreeCADGui.Selection.getSelection())

        if not self.result_object:
            QtGui.QMessageBox.critical(None, "Missing prerequisite", "No result found in active Analysis")
            return
            
        taskd = _ResultControlTaskPanel()
        FreeCADGui.Control.showDialog(taskd)

    def IsActive(self):
        return FreeCADGui.ActiveDocument is not None and results_present()


###########################################################

def results_present():
    results = False
    analysis_members = FemGui.getActiveAnalysis().Member
    for o in analysis_members:
        if o.isDerivedFrom('Fem::FemResultObject'):
            results = True
    return results


def get_results_object(sel):
    if (len(sel) == 1):
        if sel[0].isDerivedFrom("Fem::FemResultObject"):
            return sel[0]

    for i in FemGui.getActiveAnalysis().Member:
        if(i.isDerivedFrom("Fem::FemResultObject")):
            return i
    return None

def run_solver(analysis_type):
    def load_results(ret_code):
        if ret_code == 0:
            solver.load_results()
        else:
            FreeCAD.Console.PrintMessage("CalculiX failed ccx finished with error {}".format(ret_code))

    solver = makeCaeSolver('Calculix') #<FemToCae>  done
    #solver.setup_solver()
    solver.reset_all()
    solver.set_analysis_type(analysis_type)
    message = solver.check_prerequisites()
    if message:
        QtGui.QMessageBox.critical(None, "Missing prerequisite", message)
        return

    solver.write_case_file()
    solver.finished.connect(load_results)
    QtCore.QThreadPool.globalInstance().start(solver) #start exteranal program in a new Thread
    

    
    
################## appending other files #########################

def makeCaeAnalysis(name):  #<FemToCae> name changed!
    '''makeCaeAnalysis(name): makes a Fem Analysis object'''
    obj = FreeCAD.ActiveDocument.addObject("Fem::FemAnalysisPython", name)
    CaeAnalysis(obj)  #
    if FreeCAD.GuiUp:
        ViewProviderCaeAnalysis()
    #FreeCAD.ActiveDocument.recompute()
    return obj

class  CaeAnalysis:
    """The CaeAnalysis container object, serve CFD ,FEM, etc
    to-do: Gui dialog is needed to select category and solver
    """
    def __init__(self, obj):
        self.Type = "FemAnalysis"  #<FemToCae>this is related with C++ code! C++ code refactoring needed
        obj.Proxy = self #link between App::DocumentObject to  this object
        obj.addProperty("App::PropertyString", "OutputDir", "Base", "Directory where the jobs get generated")
        #default solver name and category, late show a dialog/TaskView to show and set it
        self.category="FEM"
        self.solverName="Calculix"
        obj.addProperty("App::PropertyString", "category", "Base", "desc")
        obj.addProperty("App::PropertyString", "solverName", "Base", "desc of this property")
        #
    """
    def setSolver(self, category, solverName):
        #should check available and show error!
        self.category=category
        self.solverName=solverName
        #
        self.solver=makeCaeSolver(self)
    """    
    def getMesh(self):
        for i in FemGui.getActiveAnalysis().Member:
            if i.isDerivedFrom("Fem::FemMeshObject"):
                return i
        
    def execute(self, obj):
        return

    def onChanged(self, obj, prop):
        if prop in ["MaterialName"]:
            return

    def __getstate__(self):
        return self.Type

    def __setstate__(self, state):
        if state:
            self.Type = state


class ViewProviderCaeAnalysis:
    """A View Provider for the CaeAnalysis container object
    doubleClicked() is 
    """

    def __init__(self):
        #vobj.addProperty("App::PropertyLength", "BubbleSize", "Base", str(translate("Fem", "The size of the axis bubbles")))
        self.icon=":/icons/fem-analysis.svg"

    def getIcon(self):
        return self.icon
        
    def setIcon(self,icon):
        self.icon=icon

    def attach(self, vobj):
        self.ViewObject = vobj
        self.Object = vobj.Object
        self.bubbles = None

    def updateData(self, obj, prop):
        return

    def onChanged(self, vobj, prop):
        return

    def doubleClicked(self, vobj):
        if not FemGui.getActiveAnalysis() == self.Object:
            if FreeCADGui.activeWorkbench().name() != 'FemWorkbench': #<FemToCae> 
                FreeCADGui.activateWorkbench("FemWorkbench")  #<FemToCae> 
            FemGui.setActiveAnalysis(self.Object)
            return True
        else:
            taskd = _JobControlTaskPanel(self.Object)   #can not been imported/constructed twice?
            FreeCADGui.Control.showDialog(taskd)
        return True

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None


class _JobControlTaskPanel:
    """
    """
    def __init__(self, analysis_object):
        """
        """
        self.form = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Fem/MechanicalAnalysis.ui") #<FemToCae> 
        #self.prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem")
    
        self.analysis_object = analysis_object 
        import CaeSolver
        self.solver_object = CaeSolver.makeCaeSolver('Calculix')
        self.process_object=QtCore.QProcess() # Ideally, process_object is belonged to solver_object!
        #Femgui.getActiveAnalysis().Proxy.solver  # can not get solver_object from, CaeAnalysis instance has no attribute  error!
        
        self.Timer = QtCore.QTimer()
        self.Timer.start(300)  #time out 

        self.fem_console_message = ''

        #Connect Signals and Slots
        QtCore.QObject.connect(self.form.tb_choose_working_dir, QtCore.SIGNAL("clicked()"), self.choose_working_dir)
        QtCore.QObject.connect(self.form.pushButton_write, QtCore.SIGNAL("clicked()"), self.write_input_file_handler)
        #editSolverInputFile() method has been moved into the concrete solver class
        QtCore.QObject.connect(self.form.pushButton_edit, QtCore.SIGNAL("clicked()"), self.solver_object.editSolverInputFile) # slot name changed
        #rename: self.form.pushButton_generate -> self.form.pushButton_run ?
        QtCore.QObject.connect(self.form.pushButton_generate, QtCore.SIGNAL("clicked()"), self.runSolver)

        
        QtCore.QObject.connect(self.process_object, QtCore.SIGNAL("started()"), self.solverStarted)
        QtCore.QObject.connect(self.process_object, QtCore.SIGNAL("finished(int)"), self.solverFinished)
        QtCore.QObject.connect(self.process_object , QtCore.SIGNAL("stateChanged(QProcess::ProcessState)"), self.solverStateChanged)
        QtCore.QObject.connect(self.process_object, QtCore.SIGNAL("hasError(QProcess::ProcessError)"), self.solverError)
        
        QtCore.QObject.connect(self.Timer, QtCore.SIGNAL("timeout()"), self.UpdateText)

        self.update()

    def femConsoleMessage(self, message="", color="#000000"):
        self.fem_console_message = self.fem_console_message + '<font color="#0000FF">{0:4.1f}:</font> <font color="{1}">{2}</font><br>'.\
            format(time.time() - self.Start, color, message.encode('utf-8', 'replace'))
        self.form.textEdit_Output.setText(self.fem_console_message)
        self.form.textEdit_Output.moveCursor(QtGui.QTextCursor.End)

    def printSolverStdout(self):
        out = self.process_object.readAllStandardOutput()
        if out.isEmpty():
            self.femConsoleMessage("Solver stdout is empty", "#FF0000")
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
                self.femConsoleMessage("Error converting stdout from Solver", "#FF0000")

    def UpdateText(self): # name should begin with lower case word!
        if(self.process_object.state() == QtCore.QProcess.ProcessState.Running):
            self.form.label_Time.setText('Time: {0:4.1f}: '.format(time.time() - self.Start))

    def solverError(self, error):
        print "Error()", error
        self.femConsoleMessage("Solver execute error: {}".format(error), "#FF0000")

    def solverStarted(self):
        print "SolverStarted()"
        print self.process_object.state()
        self.form.pushButton_generate.setText("Break Solver")

    def solverStateChanged(self, newState):
        if (newState == QtCore.QProcess.ProcessState.Starting):
                self.femConsoleMessage("Starting Solver...")
        if (newState == QtCore.QProcess.ProcessState.Running):
                self.femConsoleMessage("Solver is running...")
        if (newState == QtCore.QProcess.ProcessState.NotRunning):
                self.femConsoleMessage("Solver stopped.")

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
        self.solver_object.working_dir = QtGui.QFileDialog.getExistingDirectory(None,
                                                                  'Choose Solver working directory',
                                                                  self.solver_object.prefs.GetString("WorkingDir", '/tmp'))
        if self.solver_object.working_dir:
            self.solver_object.prefs.SetString("WorkingDir", str(self.solver_object.working_dir))
            self.form.le_working_dir.setText(self.working_dir)


    def check_prerequisites_helper(self):
        self.femConsoleMessage("Check dependencies...")
        self.Start = time.time()
        self.form.label_Time.setText('Time: {0:4.1f}: '.format(time.time() - self.Start))
        
        #self.solver_object.update_objects()  is called inside check_prerequisites() 
        message = self.solver_object.check_prerequisites() 
        FreeCAD.Console.PrintMessage('check_prerequisites'+message)
        if message != "":
            QtGui.QMessageBox.critical(None, "Missing prerequisit(s)", message)
            return False
        return True

    def write_input_file_handler(self):
        #self.femConsoleMessage('write_input_file_handler is fired') # this line does not update ui 
        print 'Debuginfo: write_input_file_handler is fired\n'
        QApplication.restoreOverrideCursor()
        if self.check_prerequisites_helper():
            QApplication.setOverrideCursor(Qt.WaitCursor)
            self.case_file_name = "" #

            self.solver_object.write_case_file()# #check and update_objects() is called inside this function
            FreeCAD.Console.PrintMessage('input case file has been written\n') #debug info to remove soon later
            if self.solver_object.case_file_name  != "":
                self.case_file_name  = self.solver_object.case_file_name 
                self.femConsoleMessage("Write completed "+self.solver_object.case_file_name )
                self.form.pushButton_edit.setEnabled(True)
                self.form.pushButton_generate.setEnabled(True)
            else:
                self.femConsoleMessage("Write input case file failed!", "#FF0000")
            QApplication.restoreOverrideCursor()

    def runSolver(self):
        # run Calculix
        self.Start = time.time()

        self.femConsoleMessage("Solver binary: {} with case file: {}".format(self.solver_object.solver_binary,os.path.splitext(self.case_file_name)[0] ))
        self.femConsoleMessage("Run Solver...")

        # change cwd because ccx may crash if directory has no write permission
        # there is also a limit of the length of file names so jump to the document directory
        self.cwd = QtCore.QDir.currentPath()
        fi = QtCore.QFileInfo(self.solver_object.case_file_name)
        QtCore.QDir.setCurrent(fi.path())

        # run Solver in QProcess
        self.process_object.start(self.solver_object.solver_binary, [' -i ', fi.baseName()])
        QApplication.restoreOverrideCursor()
        #in the future, should call, self.solver_object.run()
        #self.solver_object.run() # currently, using Popen for the moment

        QApplication.restoreOverrideCursor()
        
    def solverFinished(self, exitCode):
        print "SolverFinished()", exitCode
        print self.process_object.state()

        self.printSolverstdout()
        self.Timer.stop()

        self.femConsoleMessage("Solver done!", "#00AA00")

        self.form.pushButton_generate.setText("Re-run Solver")
        #print "Loading results...."
        self.femConsoleMessage("Loading result sets...")
        self.form.label_Time.setText('Time: {0:4.1f}: '.format(time.time() - self.Start))
        
        self.solver_object.reset_all()
        self.solver_object.load_results()
        
        self.form.label_Time.setText('Time: {0:4.1f}: '.format(time.time() - self.Start))

##This file should be moved out into FemResultControlTaskPanel, split and shared with CFD code
class _ResultControlTaskPanel:
    '''The control for the displacement post-processing'''
    def __init__(self):
        self.form = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Fem/ShowDisplacement.ui")

        #Connect Signals and Slots
        QtCore.QObject.connect(self.form.rb_none, QtCore.SIGNAL("toggled(bool)"), self.none_selected)
        QtCore.QObject.connect(self.form.rb_x_displacement, QtCore.SIGNAL("toggled(bool)"), self.x_displacement_selected)
        QtCore.QObject.connect(self.form.rb_y_displacement, QtCore.SIGNAL("toggled(bool)"), self.y_displacement_selected)
        QtCore.QObject.connect(self.form.rb_z_displacement, QtCore.SIGNAL("toggled(bool)"), self.z_displacement_selected)
        QtCore.QObject.connect(self.form.rb_abs_displacement, QtCore.SIGNAL("toggled(bool)"), self.abs_displacement_selected)
        QtCore.QObject.connect(self.form.rb_vm_stress, QtCore.SIGNAL("toggled(bool)"), self.vm_stress_selected)

        QtCore.QObject.connect(self.form.cb_show_displacement, QtCore.SIGNAL("clicked(bool)"), self.show_displacement)
        QtCore.QObject.connect(self.form.hsb_displacement_factor, QtCore.SIGNAL("valueChanged(int)"), self.hsb_disp_factor_changed)
        QtCore.QObject.connect(self.form.sb_displacement_factor, QtCore.SIGNAL("valueChanged(int)"), self.sb_disp_factor_changed)
        QtCore.QObject.connect(self.form.sb_displacement_factor_max, QtCore.SIGNAL("valueChanged(int)"), self.sb_disp_factor_max_changed)

        self.update()
        self.restore_result_dialog()

    def restore_result_dialog(self):
        try:
            rt = FreeCAD.FEM_dialog["results_type"]
            if rt == "None":
                self.form.rb_none.setChecked(True)
                self.none_selected(True)
            elif rt == "Uabs":
                self.form.rb_abs_displacement.setChecked(True)
                self.abs_displacement_selected(True)
            elif rt == "U1":
                self.form.rb_x_displacement.setChecked(True)
                self.x_displacement_selected(True)
            elif rt == "U2":
                self.form.rb_y_displacement.setChecked(True)
                self.y_displacement_selected(True)
            elif rt == "U3":
                self.form.rb_z_displacement.setChecked(True)
                self.z_displacement_selected(True)
            elif rt == "Sabs":
                self.form.rb_vm_stress.setChecked(True)
                self.vm_stress_selected(True)

            sd = FreeCAD.FEM_dialog["show_disp"]
            self.form.cb_show_displacement.setChecked(sd)
            self.show_displacement(sd)

            df = FreeCAD.FEM_dialog["disp_factor"]
            dfm = FreeCAD.FEM_dialog["disp_factor_max"]
            self.form.hsb_displacement_factor.setMaximum(dfm)
            self.form.hsb_displacement_factor.setValue(df)
            self.form.sb_displacement_factor_max.setValue(dfm)
            self.form.sb_displacement_factor.setValue(df)
        except:
            FreeCAD.FEM_dialog = {"results_type": "None", "show_disp": False,
                                  "disp_factor": 0, "disp_factor_max": 100}

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Close)

    def get_result_stats(self, type_name, analysis=None):
        if analysis is None:
            analysis = FemGui.getActiveAnalysis()
        for i in analysis.Member:
            if (i.isDerivedFrom("Fem::FemResultObject")) and ("Stats" in i.PropertiesList):
                match_table = {"U1": (i.Stats[0], i.Stats[1], i.Stats[2]),
                               "U2": (i.Stats[3], i.Stats[4], i.Stats[5]),
                               "U3": (i.Stats[6], i.Stats[7], i.Stats[8]),
                               "Uabs": (i.Stats[9], i.Stats[10], i.Stats[11]),
                               "Sabs": (i.Stats[12], i.Stats[13], i.Stats[14]),
                               "None": (0.0, 0.0, 0.0)}
                return match_table[type_name]
        return (0.0, 0.0, 0.0)

    def none_selected(self, state):
        FreeCAD.FEM_dialog["results_type"] = "None"
        self.set_result_stats("mm", 0.0, 0.0, 0.0)
        fea = FemTools()
        fea.reset_mesh_color()

    def abs_displacement_selected(self, state):
        FreeCAD.FEM_dialog["results_type"] = "Uabs"
        self.select_displacement_type("Uabs")

    def x_displacement_selected(self, state):
        FreeCAD.FEM_dialog["results_type"] = "U1"
        self.select_displacement_type("U1")

    def y_displacement_selected(self, state):
        FreeCAD.FEM_dialog["results_type"] = "U2"
        self.select_displacement_type("U2")

    def z_displacement_selected(self, state):
        FreeCAD.FEM_dialog["results_type"] = "U3"
        self.select_displacement_type("U3")

    def vm_stress_selected(self, state):
        FreeCAD.FEM_dialog["results_type"] = "Sabs"
        QApplication.setOverrideCursor(Qt.WaitCursor)
        self.MeshObject.ViewObject.setNodeColorByScalars(self.result_object.ElementNumbers, self.result_object.StressValues)
        (minm, avg, maxm) = self.get_result_stats("Sabs")
        self.set_result_stats("MPa", minm, avg, maxm)
        QtGui.qApp.restoreOverrideCursor()

    def select_displacement_type(self, disp_type):
        QApplication.setOverrideCursor(Qt.WaitCursor)
        if disp_type == "Uabs":
            self.MeshObject.ViewObject.setNodeColorByScalars(self.result_object.ElementNumbers, self.result_object.DisplacementLengths)
        else:
            match = {"U1": 0, "U2": 1, "U3": 2}
            d = zip(*self.result_object.DisplacementVectors)
            displacements = list(d[match[disp_type]])
            self.MeshObject.ViewObject.setNodeColorByScalars(self.result_object.ElementNumbers, displacements)
        (minm, avg, maxm) = self.get_result_stats(disp_type)
        self.set_result_stats("mm", minm, avg, maxm)
        QtGui.qApp.restoreOverrideCursor()

    def set_result_stats(self, unit, minm, avg, maxm):
        self.form.le_min.setProperty("unit", unit)
        self.form.le_min.setText("{:.6} {}".format(minm, unit))
        self.form.le_avg.setProperty("unit", unit)
        self.form.le_avg.setText("{:.6} {}".format(avg, unit))
        self.form.le_max.setProperty("unit", unit)
        self.form.le_max.setText("{:.6} {}".format(maxm, unit))

    def update_displacement(self, factor=None):
        if factor is None:
            if FreeCAD.FEM_dialog["show_disp"]:
                factor = self.form.hsb_displacement_factor.value()
            else:
                factor = 0.0
        self.MeshObject.ViewObject.applyDisplacement(factor)

    def show_displacement(self, checked):
        QApplication.setOverrideCursor(Qt.WaitCursor)
        FreeCAD.FEM_dialog["show_disp"] = checked
        if "result_object" in FreeCAD.FEM_dialog:
            if FreeCAD.FEM_dialog["result_object"] != self.result_object:
                self.update_displacement()
        FreeCAD.FEM_dialog["result_object"] = self.result_object
        self.MeshObject.ViewObject.setNodeDisplacementByVectors(self.result_object.ElementNumbers, self.result_object.DisplacementVectors)
        self.update_displacement()
        QtGui.qApp.restoreOverrideCursor()
        
########################################################
#should moved to InitGui.py, load module there
if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Fem_NewMechanicalAnalysis', _CommandNewMechanicalAnalysis())
    FreeCADGui.addCommand('Fem_CreateFromShape', _CommandFemFromShape())
    FreeCADGui.addCommand('Fem_MechanicalJobControl', _CommandMechanicalJobControl())
    FreeCADGui.addCommand('Fem_Quick_Analysis', _CommandQuickAnalysis())
    FreeCADGui.addCommand('Fem_Frequency_Analysis', _CommandFrequencyAnalysis())
    FreeCADGui.addCommand('Fem_PurgeResults', _CommandPurgeFemResults())
    FreeCADGui.addCommand('Fem_ShowResult', _CommandMechanicalShowResult())

    def hsb_disp_factor_changed(self, value):
        self.form.sb_displacement_factor.setValue(value)
        self.update_displacement()

    def sb_disp_factor_max_changed(self, value):
        FreeCAD.FEM_dialog["disp_factor_max"] = value
        self.form.hsb_displacement_factor.setMaximum(value)

    def sb_disp_factor_changed(self, value):
        FreeCAD.FEM_dialog["disp_factor"] = value
        self.form.hsb_displacement_factor.setValue(value)

    def update(self):
        self.MeshObject = None
        self.result_object = get_results_object(FreeCADGui.Selection.getSelection())

        for i in FemGui.getActiveAnalysis().Member:
            if i.isDerivedFrom("Fem::FemMeshObject"):
                self.MeshObject = i
                break

    def accept(self):
        FreeCADGui.Control.closeDialog()

    def reject(self):
        FreeCADGui.Control.closeDialog()

# Helpers