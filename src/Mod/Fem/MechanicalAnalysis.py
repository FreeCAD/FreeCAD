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

import FreeCAD, Fem, FemLib, CalculixLib
import os,sys,string,math,shutil,glob,subprocess,tempfile,time

if FreeCAD.GuiUp:
    import FreeCADGui
    import FemGui
    from PySide import QtCore, QtGui
    from PySide.QtCore import Qt
    from PySide.QtGui import QApplication

__title__ = "Mechanical Analysis managment"
__author__ = "Juergen Riegel"
__url__ = "http://www.freecadweb.org"


def makeMechanicalAnalysis(name):
    '''makeFemAnalysis(name): makes a Fem Analysis object'''
    obj = FreeCAD.ActiveDocument.addObject("Fem::FemAnalysisPython",name)
    _FemAnalysis(obj)
    _ViewProviderFemAnalysis(obj.ViewObject)
    #FreeCAD.ActiveDocument.recompute()
    return obj


class _CommandNewMechanicalAnalysis:
    "the Fem Analysis command definition"
    def GetResources(self):
        return {'Pixmap': 'Fem_Analysis',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Fem_Analysis","New mechanical analysis"),
                'Accel': "A",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Fem_Analysis","Create a new mechanical analysis")}

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create Analysis")
        FreeCADGui.addModule("FemGui")
        FreeCADGui.addModule("MechanicalAnalysis")
        #FreeCADGui.doCommand("FreeCADGui.ActiveDocument.ActiveView.setAxisCross(True)")
        FreeCADGui.doCommand("MechanicalAnalysis.makeMechanicalAnalysis('MechanicalAnalysis')")
        FreeCADGui.doCommand("FemGui.setActiveAnalysis(App.activeDocument().ActiveObject)")
        sel = FreeCADGui.Selection.getSelection()
        if (len(sel) == 1):
            if(sel[0].isDerivedFrom("Fem::FemMeshObject")):
                FreeCADGui.doCommand("App.activeDocument().ActiveObject.Member = App.activeDocument().ActiveObject.Member + [App.activeDocument()." + sel[0].Name + "]")
            if(sel[0].isDerivedFrom("Part::Feature")):
                FreeCADGui.doCommand("App.activeDocument().addObject('Fem::FemMeshShapeNetgenObject','" + sel[0].Name + "_Mesh')")
                FreeCADGui.doCommand("App.activeDocument().ActiveObject.Shape = App.activeDocument()." + sel[0].Name)
                FreeCADGui.doCommand("FemGui.getActiveAnalysis().Member = FemGui.getActiveAnalysis().Member + [App.activeDocument().ActiveObject]")
                #FreeCADGui.doCommand("Gui.activeDocument().hide('" + sel[0].Name + "')")
                #FreeCADGui.doCommand("App.activeDocument().ActiveObject.touch()")
                #FreeCADGui.doCommand("App.activeDocument().recompute()")
                FreeCADGui.doCommand("Gui.activeDocument().setEdit(App.ActiveDocument.ActiveObject.Name)")

        #FreeCAD.ActiveDocument.commitTransaction()
        FreeCADGui.Selection.clearSelection()

    def IsActive(self):
        import FemGui
        return FreeCADGui.ActiveDocument is not None and FemGui.getActiveAnalysis() is None


class _CommandMechanicalJobControl:
    "the Fem JobControl command definition"
    def GetResources(self):
        return {'Pixmap': 'Fem_NewAnalysis',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Fem_JobControl","Start calculation"),
                'Accel': "A",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Fem_JobControl","Dialog to start the calculation of the mechanical anlysis")}

    def Activated(self):
        import FemGui

        taskd = _JobControlTaskPanel(FemGui.getActiveAnalysis())
        #taskd.obj = vobj.Object
        taskd.update()
        FreeCADGui.Control.showDialog(taskd)

    def IsActive(self):
        import FemGui
        return FreeCADGui.ActiveDocument is not None and FemGui.getActiveAnalysis() is not None


class _CommandMechanicalShowResult:
    "the Fem JobControl command definition"
    def GetResources(self):
        return {'Pixmap': 'Fem_Result',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Fem_ResultDisplacement","Show result"),
                'Accel': "A",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Fem_ResultDisplacement","Show result information of an analysis")}

    def Activated(self):
        import FemGui
        DisplacementObject = None
        for i in FemGui.getActiveAnalysis().Member:
            if i.isDerivedFrom("Fem::FemResultVector"):
                if i.DataType == 'Displacement':
                    DisplacementObject = i
        StressObject = None
        for i in FemGui.getActiveAnalysis().Member:
            if i.isDerivedFrom("Fem::FemResultValue"):
                if i.DataType == 'VonMisesStress':
                    StressObject = i

        if not DisplacementObject and not StressObject:
            QtGui.QMessageBox.critical(None, "Missing prerequisite","No result found in active Analysis")
            return

        taskd = _ResultControlTaskPanel(FemGui.getActiveAnalysis())
        FreeCADGui.Control.showDialog(taskd)

    def IsActive(self):
        import FemGui
        return FreeCADGui.ActiveDocument is not None and FemGui.getActiveAnalysis() is not None


class _FemAnalysis:
    "The Material object"
    def __init__(self,obj):
        self.Type = "FemAnalysis"
        obj.Proxy = self
        #obj.Material = StartMat
        obj.addProperty("App::PropertyString","OutputDir","Base","Directory where the jobs get generated")
        obj.addProperty("App::PropertyFloat","PlateThickness","Base","Thickness of the plate")

    def execute(self,obj):
        return

    def onChanged(self,obj,prop):
        if prop in ["MaterialName"]:
            return

    def __getstate__(self):
        return self.Type

    def __setstate__(self,state):
        if state:
            self.Type = state


class _ViewProviderFemAnalysis:
    "A View Provider for the Material object"

    def __init__(self,vobj):
        #vobj.addProperty("App::PropertyLength","BubbleSize","Base", str(translate("Fem","The size of the axis bubbles")))
        vobj.Proxy = self

    def getIcon(self):
        return ":/icons/Fem_Analysis.svg"

    def attach(self, vobj):
        self.ViewObject = vobj
        self.Object = vobj.Object
        self.bubbles = None

    def updateData(self, obj, prop):
        return

    def onChanged(self, vobj, prop):
        return

    def doubleClicked(self,vobj):
        import FemGui
        if not FemGui.getActiveAnalysis() == self.Object:
            if FreeCADGui.activeWorkbench().name() != 'FemWorkbench':
                FreeCADGui.activateWorkbench("FemWorkbench")
            FemGui.setActiveAnalysis(self.Object)
            return True
        else:
            taskd = _JobControlTaskPanel(self.Object)
            FreeCADGui.Control.showDialog(taskd)
        return True

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None


class _JobControlTaskPanel:
    '''The editmode TaskPanel for Material objects'''
    def __init__(self,object):
        # the panel has a tree widget that contains categories
        # for the subcomponents, such as additions, subtractions.
        # the categories are shown only if they are not empty.
        self.form=FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Fem/MechanicalAnalysis.ui")
        self.fem_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem")
        ccx_binary = self.fem_prefs.GetString("ccxBinaryPath","")
        if ccx_binary:
            self.CalculixBinary = ccx_binary
            print "Using ccx binary path from FEM preferences: {}".format(ccx_binary)
        else:
            from platform import system
            if system() == 'Linux':
                self.CalculixBinary = 'ccx'
            elif system() == 'Windows':
                self.CalculixBinary = FreeCAD.getHomePath() + 'bin/ccx.exe'
            else:
                self.CalculixBinary = 'ccx'
        self.TempDir = FreeCAD.ActiveDocument.TransientDir.replace('\\','/') + '/FemAnl_'+ object.Uid[-4:]
        if not os.path.isdir(self.TempDir):
            os.mkdir(self.TempDir)

        self.obj = object
        #self.params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem")
        self.Calculix = QtCore.QProcess()
        self.Timer = QtCore.QTimer()
        self.Timer.start(300)
        
        self.fem_console_message = ''
        
        #Connect Signals and Slots
        QtCore.QObject.connect(self.form.toolButton_chooseOutputDir, QtCore.SIGNAL("clicked()"), self.chooseOutputDir)
        QtCore.QObject.connect(self.form.pushButton_write, QtCore.SIGNAL("clicked()"), self.write_input_file_handler)
        QtCore.QObject.connect(self.form.pushButton_edit, QtCore.SIGNAL("clicked()"), self.editCalculixInputFile)
        QtCore.QObject.connect(self.form.pushButton_generate, QtCore.SIGNAL("clicked()"), self.runCalculix)

        QtCore.QObject.connect(self.Calculix, QtCore.SIGNAL("started()"), self.calculixStarted)
        QtCore.QObject.connect(self.Calculix, QtCore.SIGNAL("stateChanged(QProcess::ProcessState)"), self.calculixStateChanged)
        QtCore.QObject.connect(self.Calculix, QtCore.SIGNAL("error(QProcess::ProcessError)"), self.calculixError)
        QtCore.QObject.connect(self.Calculix, QtCore.SIGNAL("finished(int)"), self.calculixFinished)

        QtCore.QObject.connect(self.Timer, QtCore.SIGNAL("timeout()"), self.UpdateText)

        self.update()
        
    def femConsoleMessage(self, message="", color="#000000"):
        self.fem_console_message = self.fem_console_message + '<font color="#0000FF">{0:4.1f}:</font> <font color="{1}">{2}</font><br>'.\
                                    format(time.time() - self.Start, color, message.encode('utf-8'))
        self.form.textEdit_Output.setText(self.fem_console_message)

    def printCalculiXstdout(self):
        #There is probably no need to show user output from CalculiX. It should be
        #written to a file in the calcs directory and shown to user upon request [BUTTON]
        out = self.Calculix.readAllStandardOutput()
        if out.isEmpty():
            self.femConsoleMessage("CalculiX stdout is empty", "#FF0000")
        else:
            try:
                out = unicode(out, 'utf-8')
                rx = QtCore.QRegExp("\\*ERROR.*\\n\\n")
                rx.setMinimal(True)
                pos = rx.indexIn(out)
                while not pos < 0:
                    match = rx.cap(0)
                    FreeCAD.Console.PrintError(match.strip().replace('\n',' ') + '\n')
                    pos = rx.indexIn(out, pos + 1)
                self.femConsoleMessage(out.replace('\n','<br>'))
            except UnicodeDecodeError:
                self.femConsoleMessage("Error converting stdout from CalculiX", "#FF0000")

    def UpdateText(self):
        if(self.Calculix.state() == QtCore.QProcess.ProcessState.Running):
            self.printCalculiXstdout()
            self.form.label_Time.setText('Time: {0:4.1f}: '.format(time.time() - self.Start))

    def calculixError(self, error):
        print "Error()", error
        self.femConsoleMessage("CalculiX execute error: {}".format(error), "#FF0000")

    def calculixStarted(self):
        print "calculixStarted()"
        print self.Calculix.state()
        self.form.pushButton_generate.setText("Break Calculix")

    def calculixStateChanged(self, newState):
        if (newState == QtCore.QProcess.ProcessState.Starting):
                self.femConsoleMessage("Staring CalculiX...")
        if (newState == QtCore.QProcess.ProcessState.Running):
                self.femConsoleMessage("CalculiX is running...")
        if (newState == QtCore.QProcess.ProcessState.NotRunning):
                self.femConsoleMessage("CalculiX stopped.")

    def calculixFinished(self,exitCode):
        print "calculixFinished()",exitCode
        print self.Calculix.state()

        # Restore previous cwd
        QtCore.QDir.setCurrent(self.cwd)

        self.printCalculiXstdout()
        self.Timer.stop()

        self.femConsoleMessage("Calculix done!", "#00AA00")

        self.form.pushButton_generate.setText("Re-run Calculix")
        print "Loading results...."
        self.femConsoleMessage("Loading result sets...")
        self.form.label_Time.setText('Time: {0:4.1f}: '.format(time.time() - self.Start))

        if os.path.isfile(self.Basename + '.frd'):
            QApplication.setOverrideCursor(Qt.WaitCursor)
            CalculixLib.importFrd(self.Basename + '.frd',FemGui.getActiveAnalysis())
            QApplication.restoreOverrideCursor()
            self.femConsoleMessage("Loading results done!", "#00AA00")
        else:
            self.femConsoleMessage("Loading results failed! Results file doesn\'t exist", "#FF0000")
        self.form.label_Time.setText('Time: {0:4.1f}: '.format(time.time() - self.Start))

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Close)

    def update(self):
        'fills the widgets'
        self.form.lineEdit_outputDir.setText(tempfile.gettempdir())
        return

    def accept(self):
        FreeCADGui.Control.closeDialog()

    def reject(self):
        FreeCADGui.Control.closeDialog()

    def chooseOutputDir(self):
        print "chooseOutputDir"
        dirname = QtGui.QFileDialog.getExistingDirectory(None, 'Choose material directory',self.params.GetString("JobDir",'/'))
        if(dirname):
            self.params.SetString("JobDir",str(dirname))
            self.form.lineEdit_outputDir.setText(dirname)

    def write_input_file_handler(self):
        QApplication.restoreOverrideCursor()
        if self.check_prerequisites():
            QApplication.setOverrideCursor(Qt.WaitCursor)
            try:
                self.write_calculix_input_file()
            except:
                print "Unexpected error when writing CalculiX input file:", sys.exc_info()[0]
                raise
            finally:
                QApplication.restoreOverrideCursor()
            self.form.pushButton_edit.setEnabled(True)
            self.form.pushButton_generate.setEnabled(True)

    def check_prerequisites(self):
        self.Start = time.time()
        self.femConsoleMessage("Check dependencies...")
        self.form.label_Time.setText('Time: {0:4.1f}: '.format(time.time() - self.Start))
        self.MeshObject = None
        if FemGui.getActiveAnalysis():
            for i in FemGui.getActiveAnalysis().Member:
                if i.isDerivedFrom("Fem::FemMeshObject"):
                    self.MeshObject = i
        else:
            QtGui.QMessageBox.critical(None, "Missing prerequisite","No active Analysis")
            return False

        if not self.MeshObject:
            QtGui.QMessageBox.critical(None, "Missing prerequisite","No mesh object in the Analysis")
            return False

        self.MaterialObjects = [] # [{'Object':MaterialObject}, {}, ...]
        for i in FemGui.getActiveAnalysis().Member:
            MaterialObjectDict = {}
            if i.isDerivedFrom("App::MaterialObjectPython"):
                MaterialObjectDict['Object'] = i
                self.MaterialObjects.append(MaterialObjectDict)
        if not self.MaterialObjects:
            QtGui.QMessageBox.critical(None, "Missing prerequisite","No material object in the Analysis")
            return False

        self.FixedObjects = []    # [{'Object':FixedObject, 'NodeSupports':bool}, {}, ...]
        for i in FemGui.getActiveAnalysis().Member:
            FixedObjectDict = {}
            if i.isDerivedFrom("Fem::ConstraintFixed"):
                FixedObjectDict['Object'] = i
                self.FixedObjects.append(FixedObjectDict)
        if not self.FixedObjects:
            QtGui.QMessageBox.critical(None, "Missing prerequisite","No fixed-constraint nodes defined in the Analysis")
            return False

        self.ForceObjects = []    # [{'Object':ForceObject, 'NodeLoad':value}, {}, ...]
        for i in FemGui.getActiveAnalysis().Member:
            ForceObjectDict = {}
            if i.isDerivedFrom("Fem::ConstraintForce"):
                ForceObjectDict['Object'] = i
                self.ForceObjects.append(ForceObjectDict)
        if not self.ForceObjects:
            QtGui.QMessageBox.critical(None, "Missing prerequisite","No force-constraint nodes defined in the Analysis")
            return False
        return True

    def write_calculix_input_file(self):
        print 'writeCalculixInputFile'

        #dirName = self.form.lineEdit_outputDir.text()
        dirName = self.TempDir
        print 'CalculiX run directory: ',dirName

        self.Basename = self.TempDir + '/' + self.MeshObject.Name
        filename = self.Basename + '.inp'

        self.femConsoleMessage(self.Basename)
        self.femConsoleMessage("Write mesh...")

        # write mesh
        self.MeshObject.FemMesh.writeABAQUS(filename)

        # reopen file with "append" and add the analysis definition
        inpfile = open(filename,'a')
        inpfile.write('\n\n')

        self.femConsoleMessage("Write loads & Co...")

        # write material element sets
        inpfile.write('\n\n***********************************************************\n')
        inpfile.write('** element sets for materials\n')
        for MaterialObject in self.MaterialObjects:
            print MaterialObject['Object'].Name, ':  ', MaterialObject['Object'].Material['General_name']
            inpfile.write('*ELSET,ELSET=' + MaterialObject['Object'].Name + '\n')   
            if len(self.MaterialObjects) == 1:
                inpfile.write('Eall\n')
            else:
                if MaterialObject['Object'].Name == 'MechanicalMaterial':
                    inpfile.write('Eall\n')
            inpfile.write('\n\n')

        # write fixed node sets
        inpfile.write('\n\n***********************************************************\n')
        inpfile.write('** node set for fixed constraint\n')
        for FixedObject in self.FixedObjects:
            print FixedObject['Object'].Name
            inpfile.write('*NSET,NSET=' + FixedObject['Object'].Name + '\n')
            for o,f in FixedObject['Object'].References:
                fo = o.Shape.getElement(f)
                n = []
                if fo.ShapeType == 'Face':
                    print '  Face Support (fixed face) on: ', f
                    n = self.MeshObject.FemMesh.getNodesByFace(fo)
                elif fo.ShapeType == 'Edge':
                    print '  Line Support (fixed edge) on: ', f
                    n = self.MeshObject.FemMesh.getNodesByEdge(fo)
                elif fo.ShapeType == 'Vertex':
                    print '  Point Support (fixed vertex) on: ', f
                    n = self.MeshObject.FemMesh.getNodesByVertex(fo)
                for i in n:
                    inpfile.write(str(i) + ',\n')
            inpfile.write('\n\n')

        # write load node sets and calculate node loads
        inpfile.write('\n\n***********************************************************\n')
        inpfile.write('** node sets for loads\n')
        for ForceObject in self.ForceObjects:
            print ForceObject['Object'].Name
            inpfile.write('*NSET,NSET=' + ForceObject['Object'].Name + '\n')
            NbrForceNodes = 0
            for o,f in ForceObject['Object'].References:
                fo = o.Shape.getElement(f)
                n = []
                if fo.ShapeType == 'Face':
                    print '  AreaLoad (face load) on: ', f
                    n = self.MeshObject.FemMesh.getNodesByFace(fo)
                elif fo.ShapeType == 'Edge':
                    print '  Line Load (edge load) on: ', f
                    n = self.MeshObject.FemMesh.getNodesByEdge(fo)
                elif fo.ShapeType == 'Vertex':
                    print '  Point Load (vertex load) on: ', f
                    n = self.MeshObject.FemMesh.getNodesByVertex(fo)
                for i in n:
                    inpfile.write(str(i) + ',\n')
                    NbrForceNodes = NbrForceNodes + 1   # NodeSum of mesh-nodes of ALL reference shapes from ForceObject
            # calculate node load
            if NbrForceNodes == 0:
                print '  Warning --> no FEM-Mesh-node to apply the load to was found?'
            else:
                ForceObject['NodeLoad'] = (ForceObject['Object'].Force) / NbrForceNodes
                inpfile.write('** concentrated load [N] distributed on all mesh nodes of the given shapes\n')
                inpfile.write('** ' + str(ForceObject['Object'].Force) + ' N / ' + str(NbrForceNodes) + ' Nodes = ' + str(ForceObject['NodeLoad']) + ' N on each node\n')
            if ForceObject['Object'].Force == 0:
                print '  Warning --> Force = 0'
            inpfile.write('\n\n')

        # write materials
        inpfile.write('\n\n***********************************************************\n')
        inpfile.write('** materials\n')
        inpfile.write('** youngs modulus unit is MPa = N/mm2\n')
        for MaterialObject in self.MaterialObjects:
            # get material properties
            YM = FreeCAD.Units.Quantity(MaterialObject['Object'].Material['Mechanical_youngsmodulus'])
            if YM.Unit.Type == '':
                print 'Material "Mechanical_youngsmodulus" has no Unit, asuming kPa!'
                YM = FreeCAD.Units.Quantity(YM.Value, FreeCAD.Units.Unit('Pa'))
            else:
                print 'YM unit: ', YM.Unit.Type
            print 'YM = ', YM
            PR = float(MaterialObject['Object'].Material['FEM_poissonratio'])
            print 'PR = ', PR
            # write material properties
            inpfile.write('*MATERIAL, NAME=' + MaterialObject['Object'].Material['General_name'] + '\n')
            inpfile.write('*ELASTIC \n')
            inpfile.write('{0:.3f}, '.format(YM.Value * 1E-3))
            inpfile.write('{0:.3f}\n'.format(PR))
            # write element properties
            if len(self.MaterialObjects) == 1:
                inpfile.write('*SOLID SECTION, ELSET=' + MaterialObject['Object'].Name + ', MATERIAL=' + MaterialObject['Object'].Material['General_name'] + '\n\n')
            else:
                if MaterialObject['Object'].Name == 'MechanicalMaterial':
                    inpfile.write('*SOLID SECTION, ELSET=' + MaterialObject['Object'].Name + ', MATERIAL=' + MaterialObject['Object'].Material['General_name'] + '\n\n')

        # write step beginn
        inpfile.write('\n\n\n\n***********************************************************\n')
        inpfile.write('** one step is needed to calculate the mechanical analysis of FreeCAD\n')
        inpfile.write('** loads are applied quasi-static, means without involving the time dimension\n')
        inpfile.write('*STEP\n')
        inpfile.write('*STATIC\n\n')

        # write constaints
        inpfile.write('\n** constaints\n')
        for FixedObject in self.FixedObjects:
            inpfile.write('*BOUNDARY\n')
            inpfile.write(FixedObject['Object'].Name + ',1\n')
            inpfile.write(FixedObject['Object'].Name + ',2\n')
            inpfile.write(FixedObject['Object'].Name + ',3\n\n')

        # write loads
        #inpfile.write('*DLOAD\n')
        #inpfile.write('Eall,NEWTON\n')
        inpfile.write('\n** loads\n')
        inpfile.write('** node loads, see load node sets for how the value is calculated!\n')
        for ForceObject in self.ForceObjects:
            if 'NodeLoad' in ForceObject:
                vec = ForceObject['Object'].DirectionVector
                inpfile.write('*CLOAD\n')
                inpfile.write('** force: ' + str(ForceObject['NodeLoad']) + ' N,  direction: ' + str(vec) + '\n')
                inpfile.write(ForceObject['Object'].Name + ',1,' + repr(vec.x * ForceObject['NodeLoad']) + '\n')
                inpfile.write(ForceObject['Object'].Name + ',2,' + repr(vec.y * ForceObject['NodeLoad']) + '\n')
                inpfile.write(ForceObject['Object'].Name + ',3,' + repr(vec.z * ForceObject['NodeLoad']) + '\n\n')

        # write outputs, both are needed by FreeCAD
        inpfile.write('\n** outputs --> frd file\n')
        inpfile.write('*NODE FILE\n')
        inpfile.write('U\n')
        inpfile.write('*EL FILE\n')
        inpfile.write('S, E\n')
        inpfile.write('** outputs --> dat file\n')
        inpfile.write('*NODE PRINT , NSET=Nall \n')
        inpfile.write('U \n')
        inpfile.write('*EL PRINT , ELSET=Eall \n')
        inpfile.write('S \n')
        inpfile.write('\n\n')

        # write step end
        inpfile.write('*END STEP \n')

        # write some informations
        FcVersionInfo = FreeCAD.Version()
        inpfile.write('\n\n\n\n***********************************************************\n')
        inpfile.write('**\n')
        inpfile.write('**   CalculiX Inputfile\n')
        inpfile.write('**\n')
        inpfile.write('**   written by    --> FreeCAD ' + FcVersionInfo[0] + '.' + FcVersionInfo[1] + '.' + FcVersionInfo[2] + '\n')
        inpfile.write('**   written on    --> ' + time.ctime() + '\n')
        inpfile.write('**   file name     --> ' + os.path.basename(FreeCAD.ActiveDocument.FileName) + '\n')
        inpfile.write('**   analysis name --> ' + FemGui.getActiveAnalysis().Name + '\n')
        inpfile.write('**\n')
        inpfile.write('**\n')
        inpfile.write('**   Units\n')
        inpfile.write('**\n')
        inpfile.write('**   Geometry (mesh data)        --> mm\n')
        inpfile.write("**   Materials (young's modulus) --> N/mm2 = MPa\n")
        inpfile.write('**   Loads (nodal loads)         --> N\n')
        inpfile.write('**\n')
        inpfile.write('**\n')

        inpfile.close()
        self.femConsoleMessage("Write completed.")

    def start_ext_editor(self, ext_editor_path, filename):
        if not hasattr(self, "ext_editor_process"):
            self.ext_editor_process = QtCore.QProcess()
        if  self.ext_editor_process.state() != QtCore.QProcess.Running:
            self.ext_editor_process.start(ext_editor_path, [filename])

    def editCalculixInputFile(self):
        filename = self.Basename + '.inp'
        print 'editCalculixInputFile {}'.format(filename)
        if self.fem_prefs.GetBool("UseInternalEditor",True):
            FemGui.open(filename)
        else:
            ext_editor_path = self.fem_prefs.GetString("ExternalEditorPath","")
            if ext_editor_path:
                self.start_ext_editor(ext_editor_path, filename)
            else:
                print "External editor is not defined in FEM preferences. Falling back to internal editor"
                FemGui.open(filename)

    def runCalculix(self):
        print 'runCalculix'
        self.Start = time.time()

        self.femConsoleMessage("CalculiX binary: {}".format(self.CalculixBinary))
        self.femConsoleMessage("Run Calculix...")

        # run Calculix
        print 'run Calculix at: ', self.CalculixBinary , '  with: ', self.Basename
        # change cwd because ccx may crash if directory has no write permission
        # there is also a limit of the length of file names so jump to the document directory
        self.cwd = QtCore.QDir.currentPath()
        fi = QtCore.QFileInfo(self.Basename)
        QtCore.QDir.setCurrent(fi.path())
        self.Calculix.start(self.CalculixBinary, ['-i',fi.baseName()])

        QApplication.restoreOverrideCursor()


class _ResultControlTaskPanel:
    '''The control for the displacement post-processing'''
    def __init__(self,object):
        # the panel has a tree widget that contains categories
        # for the subcomponents, such as additions, subtractions.
        # the categories are shown only if they are not empty.
        self.form = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Fem/ShowDisplacement.ui")

        self.obj = object

        #Connect Signals and Slots
        QtCore.QObject.connect(self.form.comboBox_Type, QtCore.SIGNAL("currentIndexChanged(QString)"), self.typeChanged)

        QtCore.QObject.connect(self.form.checkBox_ShowDisplacement, QtCore.SIGNAL("clicked(bool)"), self.showDisplacementClicked)
        QtCore.QObject.connect(self.form.horizontalScrollBar_Factor, QtCore.SIGNAL("valueChanged(int)"), self.sliderValue)
        QtCore.QObject.connect(self.form.spinBox_SliderFactor, QtCore.SIGNAL("valueChanged(int)"), self.sliderMaxValue)
        QtCore.QObject.connect(self.form.spinBox_DisplacementFactor, QtCore.SIGNAL("valueChanged(int)"), self.displacementFactorValue)

        self.DisplacementObject = None
        self.StressObject = None

        self.update()

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Close)

    def typeChanged(self,typeName):
        if typeName == "None":
            self.MeshObject.ViewObject.NodeColor = {}
            self.MeshObject.ViewObject.ElementColor = {}
            return

        QApplication.setOverrideCursor(Qt.WaitCursor)

        if typeName[:2] == "Ua" and self.DisplacementObject:
            (min,max,avg) = self.MeshObject.ViewObject.setNodeColorByResult(self.DisplacementObject)
        if typeName[:2] == "U1" and self.DisplacementObject:
            (min,max,avg) = self.MeshObject.ViewObject.setNodeColorByResult(self.DisplacementObject,1)
        if typeName[:2] == "U2" and self.DisplacementObject:
            (min,max,avg) = self.MeshObject.ViewObject.setNodeColorByResult(self.DisplacementObject,2)
        if typeName[:2] == "U3" and self.DisplacementObject:
            (min,max,avg) = self.MeshObject.ViewObject.setNodeColorByResult(self.DisplacementObject,3)
        if typeName[:2] == "Sa" and self.StressObject:
            (min,max,avg) = self.MeshObject.ViewObject.setNodeColorByResult(self.StressObject)

        self.form.lineEdit_Max.setText(str(max))
        self.form.lineEdit_Min.setText(str(min))
        self.form.lineEdit_Avg.setText(str(avg))

        print typeName

        QtGui.qApp.restoreOverrideCursor()

    def showDisplacementClicked(self,bool):
        QApplication.setOverrideCursor(Qt.WaitCursor)
        self.setDisplacement()
        QtGui.qApp.restoreOverrideCursor()

    def sliderValue(self,value):
        if(self.form.checkBox_ShowDisplacement.isChecked()):
            self.MeshObject.ViewObject.animate(value)

        self.form.spinBox_DisplacementFactor.setValue(value)

    def sliderMaxValue(self,value):
        #print 'sliderMaxValue()'
        self.form.horizontalScrollBar_Factor.setMaximum(value)

    def displacementFactorValue(self,value):
        #print 'displacementFactorValue()'
        self.form.horizontalScrollBar_Factor.setValue(value)

    def setDisplacement(self):
        if self.DisplacementObject:
            self.MeshObject.ViewObject.setNodeDisplacementByResult(self.DisplacementObject)

    def setColorStress(self):
        if self.StressObject:
            values = self.StressObject.Values
            maxVal = max(values)
            self.form.doubleSpinBox_MinValueColor.setValue(maxVal)

            self.MeshObject.ViewObject.setNodeColorByResult(self.StressObject)

    def update(self):
        'fills the widgets'
        #print "Update-------------------------------"
        self.MeshObject = None
        if FemGui.getActiveAnalysis():
            for i in FemGui.getActiveAnalysis().Member:
                if i.isDerivedFrom("Fem::FemMeshObject"):
                    self.MeshObject = i

        for i in FemGui.getActiveAnalysis().Member:
            if i.isDerivedFrom("Fem::FemResultVector"):
                if i.DataType == 'Displacement':
                    self.DisplacementObject = i
                    self.form.comboBox_Type.addItem("U1   (Disp. X)")
                    self.form.comboBox_Type.addItem("U2   (Disp. Y)")
                    self.form.comboBox_Type.addItem("U3   (Disp. z)")
                    self.form.comboBox_Type.addItem("Uabs (Disp. abs)")
        for i in FemGui.getActiveAnalysis().Member:
            if i.isDerivedFrom("Fem::FemResultValue"):
                if i.DataType == 'VonMisesStress':
                    self.StressObject = i
                    self.form.comboBox_Type.addItem("Sabs (Von Mises Stress)")

    def accept(self):
        FreeCADGui.Control.closeDialog()

    def reject(self):
        FreeCADGui.Control.closeDialog()

FreeCADGui.addCommand('Fem_NewMechanicalAnalysis',_CommandNewMechanicalAnalysis())
FreeCADGui.addCommand('Fem_MechanicalJobControl',_CommandMechanicalJobControl())
FreeCADGui.addCommand('Fem_ShowResult',_CommandMechanicalShowResult())
