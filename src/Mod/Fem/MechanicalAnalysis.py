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

import FreeCAD, Fem, FemLib
import os,sys,string,math,shutil,glob,subprocess,tempfile

if FreeCAD.GuiUp:
    import FreeCADGui,FemGui
    from FreeCAD import Vector
    from PySide import QtCore, QtGui
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
        if FemGui.getActiveAnalysis() == None:
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

        self.obj = object
        self.formUi = form_class()
        self.form = QtGui.QWidget()
        self.formUi.setupUi(self.form)
        #self.params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem")

        #Connect Signals and Slots
        QtCore.QObject.connect(self.formUi.toolButton_chooseOutputDir, QtCore.SIGNAL("clicked()"), self.chooseOutputDir)
        QtCore.QObject.connect(self.formUi.pushButton_generate, QtCore.SIGNAL("clicked()"), self.run)

        self.update()
        


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
        dirName = self.formUi.lineEdit_outputDir.text()
        
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
        
        
        filename = dirName + '/' + MeshObject.Name + '.inp'
        
        MeshObject.FemMesh.writeABAQUS(filename)
        
        # now open again and write the setup:
        inpfile = open(filename,'a')
        inpfile.write('*MATERIAL, Name='+matmap['General_name'] + '\n')
        
        
        #*MATERIAL, Name=steel
        #*ELASTIC
        #28000000, 0.3
        #*SOLID SECTION, Elset=Eall, Material=steel
        #*STEP
        #*STATIC
        #*BOUNDARY
        #Nfix1,3,3,0
        #Nfix2,2,3,0
        #Nfix3,1,3,0
        #*DLOAD
        #*INCLUDE, INPUT=load.dlo
        #*NODE FILE
        #U
        #*EL FILE
        #S, E
        #*END STEP 
        #current_file_name
        
        #young_modulus = float(matmap['FEM_youngsmodulus'])
        #poisson_ratio = float(matmap['FEM_poissonratio'])
        
    
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
