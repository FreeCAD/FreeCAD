#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2013 - Juergen Riegel <FreeCAD@juergen-riegel.net>      *  
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
    import FreeCADGui,FemGui
    from FreeCAD import Vector
    from PySide import QtCore, QtGui
    from PyQt4.QtCore import Qt
    from PyQt4.QtGui import QApplication, QCursor
    from pivy import coin
    from FreeCADGui import PySideUic as uic

__title__="Mechanical Analysis managment"
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
        return {'Pixmap'  : 'Fem_Analysis',
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
                FreeCADGui.doCommand("App.activeDocument().ActiveObject.Member = App.activeDocument().ActiveObject.Member + [App.activeDocument()."+sel[0].Name+"]")
            if(sel[0].isDerivedFrom("Part::Feature")):
                FreeCADGui.doCommand("App.activeDocument().addObject('Fem::FemMeshShapeNetgenObject','"+sel[0].Name +"_Mesh')")
                FreeCADGui.doCommand("App.activeDocument().ActiveObject.Shape = App.activeDocument()."+sel[0].Name)                
                FreeCADGui.doCommand("FemGui.getActiveAnalysis().Member = FemGui.getActiveAnalysis().Member + [App.activeDocument().ActiveObject]")
                FreeCADGui.doCommand("Gui.activeDocument().hide('"+sel[0].Name+"')")
                #FreeCADGui.doCommand("App.activeDocument().ActiveObject.touch()")
                #FreeCADGui.doCommand("App.activeDocument().recompute()")
                FreeCADGui.doCommand("Gui.activeDocument().setEdit(App.ActiveDocument.ActiveObject.Name)")

        FreeCAD.ActiveDocument.commitTransaction()
        FreeCADGui.Selection.clearSelection()
       
    def IsActive(self):
        import FemGui
        return FreeCADGui.ActiveDocument != None and FemGui.getActiveAnalysis() == None

class _CommandMechanicalJobControl:
    "the Fem JobControl command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Fem_NewAnalysis',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Fem_JobControl","Start calculation"),
                'Accel': "A",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Fem_JobControl","Dialog to start the calculation of the machanical anlysis")}
        
    def Activated(self):
        import FemGui
        
        taskd = _JobControlTaskPanel(FemGui.getActiveAnalysis())
        #taskd.obj = vobj.Object
        taskd.update()
        FreeCADGui.Control.showDialog(taskd)

       
    def IsActive(self):
        import FemGui
        return FreeCADGui.ActiveDocument != None and FemGui.getActiveAnalysis() != None



class _CommandMechanicalShowResult:
    "the Fem JobControl command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Fem_Result',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Fem_ResultDisplacement","Show result"),
                'Accel': "A",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Fem_ResultDisplacement","Show result imformatation of an analysis")}
        
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
                if i.DataType == 'VanMisesStress':
                    StressObject = i

        if not DisplacementObject and not StressObject:
            QtGui.QMessageBox.critical(None, "Missing prerequisit","No result found in active Analysis")
            return
        
        taskd = _ResultControlTaskPanel(FemGui.getActiveAnalysis())
        #taskd.obj = vobj.Object
        taskd.update()
        FreeCADGui.Control.showDialog(taskd)

       
    def IsActive(self):
        import FemGui
        return FreeCADGui.ActiveDocument != None and FemGui.getActiveAnalysis() != None


        
class _FemAnalysis:
    "The Material object"
    def __init__(self,obj):
        self.Type = "FemAnalysis"
        obj.Proxy = self
        #obj.Material = StartMat
        obj.addProperty("App::PropertyString","OutputDir","Base","Directory where the jobs get generated")
        obj.addProperty("App::PropertyFloat","PlateThikness","Base","Thikness of the plate")

        
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
            taskd.obj = vobj.Object
            taskd.update()
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
        form_class, base_class = uic.loadUiType(FreeCAD.getHomePath() + "Mod/Fem/MechanicalAnalysis.ui")
        
        self.CalculixBinary = FreeCAD.getHomePath() +'bin/ccx.exe'
        self.TempDir = FreeCAD.ActiveDocument.TransientDir.replace('\\','/') + '/FemAnl_'+ object.Uid[-4:]
        if not os.path.isdir(self.TempDir):
            os.mkdir(self.TempDir)

        self.obj = object
        self.formUi = form_class()
        self.form = QtGui.QWidget()
        self.formUi.setupUi(self.form)
        #self.params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem")
        self.Calculix = QtCore.QProcess()
        self.Timer = QtCore.QTimer()
        self.Timer.start(300)
        
        self.OutStr = ''
        
        #Connect Signals and Slots
        QtCore.QObject.connect(self.formUi.toolButton_chooseOutputDir, QtCore.SIGNAL("clicked()"), self.chooseOutputDir)
        QtCore.QObject.connect(self.formUi.pushButton_generate, QtCore.SIGNAL("clicked()"), self.run)

        QtCore.QObject.connect(self.Calculix, QtCore.SIGNAL("started()"), self.calculixStarted)
        QtCore.QObject.connect(self.Calculix, QtCore.SIGNAL("finished(int)"), self.calculixFinished)

        QtCore.QObject.connect(self.Timer, QtCore.SIGNAL("timeout()"), self.UpdateText)
        
        self.update()
        

    def UpdateText(self):
        if(self.Calculix.state() == QtCore.QProcess.ProcessState.Running):
            out = self.Calculix.readAllStandardOutput()
            #print out
            if out:
                self.OutStr = self.OutStr + unicode(out).replace('\n','<br>')
                self.formUi.textEdit_Output.setText(self.OutStr)
            self.formUi.label_Time.setText('Time: {0:4.1f}: '.format(time.time() - self.Start) )

    def calculixError(self,error):
        print "Error()",error
        
    def calculixStarted(self):
        print "calculixStarted()"
        print self.Calculix.state()
        self.formUi.pushButton_generate.setText("Break Calculix")
        
        
    def calculixFinished(self,exitCode):
        print "calculixFinished()",exitCode
        print self.Calculix.state()
        out = self.Calculix.readAllStandardOutput()
        print out
        if out:
            self.OutStr = self.OutStr + unicode(out).replace('\n','<br>')
            self.formUi.textEdit_Output.setText(self.OutStr)

        self.Timer.stop()
        
        self.OutStr = self.OutStr + '<font color="#0000FF">{0:4.1f}:</font> '.format(time.time() - self.Start) + '<font color="#00FF00">Calculix done!</font><br>'
        self.formUi.textEdit_Output.setText(self.OutStr)

        self.formUi.pushButton_generate.setText("Re-run Calculix")
        print "Loading results...."
        self.OutStr = self.OutStr + '<font color="#0000FF">{0:4.1f}:</font> '.format(time.time() - self.Start) + 'Loading result sets...<br>'
        self.formUi.textEdit_Output.setText(self.OutStr)
        self.formUi.label_Time.setText('Time: {0:4.1f}: '.format(time.time() - self.Start) )

        QApplication.setOverrideCursor(QCursor(Qt.WaitCursor))
        CalculixLib.importFrd(self.Basename + '.frd',FemGui.getActiveAnalysis() )
        QApplication.restoreOverrideCursor()
        self.OutStr = self.OutStr + '<font color="#0000FF">{0:4.1f}:</font> '.format(time.time() - self.Start) + '<font color="#00FF00">Loading results done!</font><br>'
        self.formUi.textEdit_Output.setText(self.OutStr)
        self.formUi.label_Time.setText('Time: {0:4.1f}: '.format(time.time() - self.Start) )
        
    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Close)
    
    def update(self):
        'fills the widgets'
        self.formUi.lineEdit_outputDir.setText(tempfile.gettempdir())
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
            self.formUi.lineEdit_outputDir.setText(dirname)
        
    def run(self):
        self.Start = time.time()
        
        #dirName = self.formUi.lineEdit_outputDir.text()
        dirName = self.TempDir
        print 'run() dir:',dirName
        self.OutStr = self.OutStr + '<font color="#0000FF">{0:4.1f}:</font> '.format(time.time() - self.Start) + 'Check dependencies...<br>'
        self.formUi.textEdit_Output.setText(self.OutStr)
        self.formUi.label_Time.setText('Time: {0:4.1f}: '.format(time.time() - self.Start) )
        MeshObject = None
        if FemGui.getActiveAnalysis():
            for i in FemGui.getActiveAnalysis().Member:
                if i.isDerivedFrom("Fem::FemMeshObject"):
                    MeshObject = i
        else:
            QtGui.QMessageBox.critical(None, "Missing prerequisit","No active Analysis")
            return
            
        if not MeshObject:
            QtGui.QMessageBox.critical(None, "Missing prerequisit","No mesh object in the Analysis")
            return
        
        MathObject = None
        for i in FemGui.getActiveAnalysis().Member:
            if i.isDerivedFrom("App::MaterialObjectPython"):
                MathObject = i
        if not MathObject:
            QtGui.QMessageBox.critical(None, "Missing prerequisit","No material object in the Analysis")
            return
        matmap = MathObject.Material
            
        FixedObject = None
        for i in FemGui.getActiveAnalysis().Member:
            if i.isDerivedFrom("Fem::ConstraintFixed"):
                FixedObject = i
        if not FixedObject:
            QtGui.QMessageBox.critical(None, "Missing prerequisit","No fixed-constraint nodes defined in the Analysis")
            return
            
        ForceObject = None
        for i in FemGui.getActiveAnalysis().Member:
            if i.isDerivedFrom("Fem::ConstraintForce"):
                ForceObject = i
        if not ForceObject:
            QtGui.QMessageBox.critical(None, "Missing prerequisit","No force-constraint nodes defined in the Analysis")
            return
        
        QApplication.setOverrideCursor(QCursor(Qt.WaitCursor))
        
        self.Basename = self.TempDir + '/' + MeshObject.Name
        filename = self.Basename + '.inp'

        self.OutStr = self.OutStr + '<font color="#0000FF">{0:4.1f}:</font> '.format(time.time() - self.Start) + self.Basename + '<br>'
        self.formUi.textEdit_Output.setText(self.OutStr)
        
        self.OutStr = self.OutStr + '<font color="#0000FF">{0:4.1f}:</font> '.format(time.time() - self.Start) + 'Write mesh...<br>'
        self.formUi.textEdit_Output.setText(self.OutStr)

        MeshObject.FemMesh.writeABAQUS(filename)
        # reopen file with "append" and add the analysis definition
        inpfile = open(filename,'a')
        inpfile.write('\n\n')
        
        self.OutStr = self.OutStr + '<font color="#0000FF">{0:4.1f}:</font> '.format(time.time() - self.Start) + 'Write loads & Co...<br>'
        self.formUi.textEdit_Output.setText(self.OutStr)
        
        # write the fixed node set
        NodeSetName = FixedObject.Name 
        inpfile.write('*NSET,NSET=' + NodeSetName + '\n')
        for o,f in FixedObject.References:
            fo = o.Shape.getElement(f)
            n = MeshObject.FemMesh.getNodesByFace(fo)
            for i in n:
                inpfile.write( str(i)+',\n')
        inpfile.write('\n\n')
        
        # write the load node set 
        NodeSetNameForce = ForceObject.Name 
        inpfile.write('*NSET,NSET=' + NodeSetNameForce + '\n')
        NbrForceNods = 0
        for o,f in ForceObject.References:
            fo = o.Shape.getElement(f)
            n = MeshObject.FemMesh.getNodesByFace(fo)
            for i in n:
                inpfile.write( str(i)+',\n')
                NbrForceNods = NbrForceNods + 1
        inpfile.write('\n\n')
        
        # get the material properties
        YM = FreeCAD.Units.Quantity(MathObject.Material['Mechanical_youngsmodulus'])
        if YM.Unit.Type == '':
            print 'Material "Mechanical_youngsmodulus" has no Unit, asuming kPa!'
            YM = FreeCAD.Units.Quantity(YM.Value, FreeCAD.Units.Unit('Pa') )
        
        print YM
        
        PR = float( MathObject.Material['FEM_poissonratio'] )
        print PR
        
        # now open again and write the setup:
        inpfile.write('*MATERIAL, Name='+matmap['General_name'] + '\n')
        inpfile.write('*ELASTIC \n')
        inpfile.write('{0:.3f}, '.format(YM.Value) )
        inpfile.write('{0:.3f}\n'.format(PR) )
        inpfile.write('*SOLID SECTION, Elset=Eall, Material='+matmap['General_name'] + '\n')
        inpfile.write('*INITIAL CONDITIONS, TYPE=STRESS, USER\n')
        inpfile.write('*STEP\n')
        inpfile.write('*STATIC\n')
        inpfile.write('*BOUNDARY\n')
        inpfile.write(NodeSetName + ',1,3,0.0\n')
        #inpfile.write('*DLOAD\n')
        #inpfile.write('Eall,NEWTON\n')
        
        Force = (ForceObject.Force * 1000.0) / NbrForceNods
        vec = ForceObject.NormalDirection
        inpfile.write('*CLOAD\n')
        inpfile.write(NodeSetNameForce + ',1,' + `vec.x * Force` + '\n')
        inpfile.write(NodeSetNameForce + ',2,' + `vec.y * Force` + '\n')
        inpfile.write(NodeSetNameForce + ',3,' + `vec.z * Force` + '\n')

        inpfile.write('*NODE FILE\n')
        inpfile.write('U\n')
        inpfile.write('*EL FILE\n')
        inpfile.write('S, E\n')
        inpfile.write('*NODE PRINT , NSET=Nall \n')
        inpfile.write('U \n')
        inpfile.write('*EL PRINT , ELSET=Eall \n')
        inpfile.write('S \n')
        inpfile.write('*END STEP \n')
        
        self.OutStr = self.OutStr + '<font color="#0000FF">{0:4.1f}:</font> '.format(time.time() - self.Start) + self.CalculixBinary + '<br>'
        self.formUi.textEdit_Output.setText(self.OutStr)
        
        self.OutStr = self.OutStr + '<font color="#0000FF">{0:4.1f}:</font> '.format(time.time() - self.Start) + 'Run Calculix...<br>'
        self.formUi.textEdit_Output.setText(self.OutStr)

        # run Claculix
        print 'run Calclulix at:', self.CalculixBinary , '  with: ', self.Basename
        self.Calculix.start(self.CalculixBinary, ['-i',self.Basename])
        
        
        QApplication.restoreOverrideCursor()
    
class _ResultControlTaskPanel:
    '''The control for the displacement post-processing'''
    def __init__(self,object):
        # the panel has a tree widget that contains categories
        # for the subcomponents, such as additions, subtractions.
        # the categories are shown only if they are not empty.
        form_class, base_class = uic.loadUiType(FreeCAD.getHomePath() + "Mod/Fem/ShowDisplacement.ui")

        self.obj = object
        self.formUi = form_class()
        self.form = QtGui.QWidget()
        self.formUi.setupUi(self.form)

        #Connect Signals and Slots
        QtCore.QObject.connect(self.formUi.radioButton_Displacement, QtCore.SIGNAL("clicked(bool)"), self.displacementClicked)
        QtCore.QObject.connect(self.formUi.radioButton_Stress, QtCore.SIGNAL("clicked(bool)"), self.stressClicked)
        QtCore.QObject.connect(self.formUi.radioButton_NoColor, QtCore.SIGNAL("clicked(bool)"), self.noColorClicked)
        QtCore.QObject.connect(self.formUi.checkBox_ShowDisplacement, QtCore.SIGNAL("clicked(bool)"), self.showDisplacementClicked)

        QtCore.QObject.connect(self.formUi.verticalScrollBar_Factor, QtCore.SIGNAL("valueChanged(int)"), self.sliderValue)

        QtCore.QObject.connect(self.formUi.spinBox_SliderFactor, QtCore.SIGNAL("valueChanged(double)"), self.sliderMaxValue)
        QtCore.QObject.connect(self.formUi.spinBox_DisplacementFactor, QtCore.SIGNAL("valueChanged(double)"), self.displacementFactorValue)

        self.DisplacementObject = None
        self.StressObject = None

        self.update()
        


    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Close)
        
    def displacementClicked(self,bool):
        QtGui.qApp.setOverrideCursor(QtCore.Qt.WaitCursor)
        self.setColorDisplacement()
        QtGui.qApp.restoreOverrideCursor()
        
    def stressClicked(self,bool):
        print 'stressClicked()'
        QtGui.qApp.setOverrideCursor(QtCore.Qt.WaitCursor)
        self.setColorStress()
        QtGui.qApp.restoreOverrideCursor()
        
    def noColorClicked(self,bool):
        self.MeshObject.ViewObject.NodeColor = {}
        self.MeshObject.ViewObject.ElementColor = {}
        
    def showDisplacementClicked(self,bool):
        QtGui.qApp.setOverrideCursor(QtCore.Qt.WaitCursor)
        self.setDisplacement()
        QtGui.qApp.restoreOverrideCursor()
    
    def sliderValue(self,value):
        if(self.formUi.checkBox_ShowDisplacement.isChecked()):
            self.MeshObject.ViewObject.animate(value)
        
        self.formUi.spinBox_DisplacementFactor.setValue(value)

    def sliderMaxValue(self,value):
        print 'sliderMaxValue()'
        self.formUi.verticalScrollBar_Factor.setMaximum(value)
        
    def displacementFactorValue(self,value):
        print 'displacementFactorValue()'
        self.formUi.verticalScrollBar_Factor.setValue(value)
        
    def setColorDisplacement(self):
        if self.DisplacementObject:
            values = self.DisplacementObject.Values
            maxL = 0.0
            for i in values:
                if i.Length > maxL:
                    maxL = i.Length
            
            self.formUi.lineEdit_Max.setText(str(maxL))
            self.formUi.doubleSpinBox_MinValueColor.setValue(maxL)
            
            self.MeshObject.ViewObject.setNodeColorByResult(self.DisplacementObject)
            
    def setDisplacement(self):
        if self.DisplacementObject:
            self.MeshObject.ViewObject.setNodeDisplacementByResult(self.DisplacementObject)   
    
    def setColorStress(self):
        if self.StressObject:
            values = self.StressObject.Values
            maxVal = max(values)
            self.formUi.doubleSpinBox_MinValueColor.setValue(maxVal)
            
            self.MeshObject.ViewObject.setNodeColorByResult(self.StressObject)

    def update(self):
        'fills the widgets'

        self.MeshObject = None
        if FemGui.getActiveAnalysis():
            for i in FemGui.getActiveAnalysis().Member:
                if i.isDerivedFrom("Fem::FemMeshObject"):
                    self.MeshObject = i

        for i in FemGui.getActiveAnalysis().Member:
            if i.isDerivedFrom("Fem::FemResultVector"):
                if i.DataType == 'Displacement':
                    self.DisplacementObject = i
        for i in FemGui.getActiveAnalysis().Member:
            if i.isDerivedFrom("Fem::FemResultValue"):
                if i.DataType == 'VanMisesStress':
                    self.StressObject = i
                
    def accept(self):
        FreeCADGui.Control.closeDialog()
        
                    
    def reject(self):
        FreeCADGui.Control.closeDialog()

        
        
    
    
    
FreeCADGui.addCommand('Fem_NewMechanicalAnalysis',_CommandNewMechanicalAnalysis())
FreeCADGui.addCommand('Fem_MechanicalJobControl',_CommandMechanicalJobControl())
FreeCADGui.addCommand('Fem_ShowResult',_CommandMechanicalShowResult())
