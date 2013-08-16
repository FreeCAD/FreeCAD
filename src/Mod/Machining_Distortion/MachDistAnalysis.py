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

import FreeCAD, Fem, os,sys,string,math,shutil,glob,subprocess,tempfile
from ApplyingBC_IC  import ApplyingBC_IC

if FreeCAD.GuiUp:
    import FreeCADGui,FemGui
    from FreeCAD import Vector
    from PyQt4 import QtCore, QtGui
    from pivy import coin
    import PyQt4.uic as uic

__title__="Machine-Distortion Analysis managment"
__author__ = "Juergen Riegel"
__url__ = "http://free-cad.sourceforge.net"


def makeMachDistAnalysis(name):
    '''makeMachDistAnalysis(name): makes a MachDist Analysis object'''
    obj = FreeCAD.ActiveDocument.addObject("Fem::FemAnalysisPython",name)
    _MachDistAnalysis(obj)
    _ViewProviderMachDistAnalysis(obj.ViewObject)
    #FreeCAD.ActiveDocument.recompute()
    return obj
    
    
class _CommandAnalysis:
    "the MachDist Analysis command definition"
    def GetResources(self):
        return {'Pixmap'  : 'MachDist_NewAnalysis',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("MachDist_Analysis","Machine-Distortion Analysis"),
                'Accel': "A",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("MachDist_Analysis","Add or edit a Machine-Distortion Analysis")}
        
    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create Analysis")
        FreeCADGui.addModule("FemGui")
        FreeCADGui.addModule("MachDistAnalysis")
        FreeCADGui.doCommand("FreeCADGui.ActiveDocument.ActiveView.setAxisCross(True)")
        #FreeCADGui.doCommand("App.activeDocument().addObject('Fem::FemAnalysis','PartDistortion')")
        FreeCADGui.doCommand("MachDistAnalysis.makeMachDistAnalysis('PartDistortion')")
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

class _CommandJobControl:
    "the MachDist JobControl command definition"
    def GetResources(self):
        return {'Pixmap'  : 'MachDist_Upload',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("MachDist_JobControl","Generate Jobs"),
                'Accel': "A",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("MachDist_Analysis","Dialog to generate the jobs")}
        
    def Activated(self):
        taskd = _JobControlTaskPanel()
        #taskd.obj = vobj.Object
        taskd.update()
        FreeCADGui.Control.showDialog(taskd)

       
    def IsActive(self):
        import FemGui
        return True

        
class _MachDistAnalysis:
    "The Material object"
    def __init__(self,obj):
        self.Type = "MachDistAnalysis"
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
        
class _ViewProviderMachDistAnalysis:
    "A View Provider for the Material object"

    def __init__(self,vobj):
        #vobj.addProperty("App::PropertyLength","BubbleSize","Base", str(translate("MachDist","The size of the axis bubbles")))
        vobj.Proxy = self
       
    def getIcon(self):
        import machdist_rc
        return ":/icons/MachDist_NewAnalysis.svg"


    def attach(self, vobj):
        self.ViewObject = vobj
        self.Object = vobj.Object
        self.bubbles = None


    def updateData(self, obj, prop):
        return

    def onChanged(self, vobj, prop):
        return
  
    def doubleClicked(self,vobj):
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
    def __init__(self):
        # the panel has a tree widget that contains categories
        # for the subcomponents, such as additions, subtractions.
        # the categories are shown only if they are not empty.
        form_class, base_class = uic.loadUiType(FreeCAD.getHomePath() + "Mod/Machining_Distortion/JobControl.ui")

        #self.obj = object
        self.formUi = form_class()
        self.form = QtGui.QWidget()
        self.formUi.setupUi(self.form)
        self.params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Machining_Distortion")

        #Connect Signals and Slots
        QtCore.QObject.connect(self.formUi.toolButton_chooseOutputDir, QtCore.SIGNAL("clicked()"), self.chooseOutputDir)
        QtCore.QObject.connect(self.formUi.pushButton_generate, QtCore.SIGNAL("clicked()"), self.generate)

        self.update()
        


    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Close)
    
    def update(self):
        'fills the widgets'
        self.formUi.lineEdit_outputDir.setText(self.params.GetString("JobDir",'/'))
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
        
    def generate(self):
        print "pushButton_generate"
        print self.formUi.lineEdit_outputDir.text()
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
            
        IsoNodeObject = None
        for i in FemGui.getActiveAnalysis().Member:
            if i.isDerivedFrom("App::FeaturePython"):
                if i.Proxy.Type == 'MachDist_IsostaticNodes':
                    IsoNodeObject = i
        if not IsoNodeObject:
            QtGui.QMessageBox.critical(None, "Missing prerequisit","No Isostatic nodes defined in the Analysis")
            return
        IsoNodes = IsoNodeObject.IsostaticNodes
        
        filename_without_suffix = MeshObject.Name
        #current_file_name
        
        z_offset_from = self.formUi.spinBox_z_level_from.value()
        z_offset_to = self.formUi.spinBox_z_level_to.value()
        z_offset_intervall = self.formUi.spinBox_z_level_intervall.value()
        x_rot_from = self.formUi.spinBox_misalignment_x_from.value()
        x_rot_to = self.formUi.spinBox_misalignment_x_to.value()
        x_rot_intervall = self.formUi.spinBox_misalignment_x_intervall.value()
        y_rot_from = self.formUi.spinBox_misalignment_y_from.value()
        y_rot_to = self.formUi.spinBox_misalignment_y_to.value()
        y_rot_intervall = self.formUi.spinBox_misalignment_y_intervall.value()
        z_rot_from = self.formUi.spinBox_misalignment_z_from.value()
        z_rot_to = self.formUi.spinBox_misalignment_z_to.value()
        z_rot_intervall = self.formUi.spinBox_misalignment_z_intervall.value()

        #current_file_name = self.JobTable.item(job,0).text()
        
        lc1 = float(matmap['PartDist_lc1'])
        lc2 = float(matmap['PartDist_lc2'])
        lc3 = float(matmap['PartDist_lc3'])
        lc4 = float(matmap['PartDist_lc4'])
        lc5 = float(matmap['PartDist_lc5'])
        lc6 = float(matmap['PartDist_lc6'])        
        ltc1 =float(matmap['PartDist_ltc1'])
        ltc2 =float(matmap['PartDist_ltc2'])
        ltc3 =float(matmap['PartDist_ltc3'])
        ltc4 =float(matmap['PartDist_ltc4'])
        ltc5 =float(matmap['PartDist_ltc5'])
        ltc6 =float(matmap['PartDist_ltc6'])
        young_modulus = float(matmap['FEM_youngsmodulus'])
        poisson_ratio = float(matmap['PartDist_poissonratio'])
        plate_thickness = float(matmap['PartDist_platethickness'])
        
        batch = open(str(dirName + "/" + "lcmt_CALCULIX_Calculation_batch.bat"),'wb')
        batch.write("#!/bin/bash\n")        
        batch.write("export CCX_NPROC=4\n")

        OutStr = "Generate:\n"
        print z_rot_intervall,y_rot_intervall,x_rot_intervall,z_offset_intervall
        print z_offset_from,z_offset_intervall,z_offset_to
        i = z_offset_from
        while i <= z_offset_to:
            j = x_rot_from
            while j <= x_rot_to:
                k = y_rot_from
                while k <= y_rot_to:
                    l = z_rot_from
                    while l <= z_rot_to:
                        OutStr = OutStr + str(j) + "," + str(k) + "," + str(l)
                        self.formUi.textEdit_Output.setText(OutStr)
                        
                        rotation_around_x = FreeCAD.Base.Placement(FreeCAD.Base.Vector(0,0,0),FreeCAD.Base.Vector(1,0,0),j)
                        rotation_around_y = FreeCAD.Base.Placement(FreeCAD.Base.Vector(0,0,0),FreeCAD.Base.Vector(0,1,0),k)
                        rotation_around_z = FreeCAD.Base.Placement(FreeCAD.Base.Vector(0,0,0),FreeCAD.Base.Vector(0,0,1),l)
                        translate = FreeCAD.Base.Placement(FreeCAD.Base.Vector(0,0,i),FreeCAD.Base.Vector(0,0,0),0.0)
                        translation = rotation_around_x.multiply(rotation_around_y).multiply(rotation_around_z).multiply(translate)
                        MeshObject.Placement = translation
                        BndBox = MeshObject.FemMesh.BoundBox
                        if(BndBox.ZLength > plate_thickness):
                            print " Too heavy rotations"
                            print str(plate_thickness)
                            l= l + z_rot_intervall
                            OutStr = OutStr + " Too heavy rotations"
                            self.formUi.textEdit_Output.setText(OutStr)

                            continue

                        Case_Dir = str(dirName) + "/" + filename_without_suffix +\
                        "_"+"x_rot"+ str(int(j))+ \
                        "_"+"y_rot"+ str(int(k))+ \
                        "_"+"z_rot"+ str(int(l))+ \
                        "_"+"z_l"+ str(int(i))
                        if ( os.path.exists(str(Case_Dir)) ):
                            os.chdir(str(dirName))
                            shutil.rmtree(str(Case_Dir))
                        OutStr = OutStr + "\n"
                        self.formUi.textEdit_Output.setText(OutStr)
                        
                        FreeCADGui.updateGui()
                        os.mkdir(str(Case_Dir))

                        #Lets generate a sigini Input Deck for the calculix user subroutine
                        sigini_input = open (str(Case_Dir + "/" + "sigini_input.txt"),'wb')
                        
                        #Write plate thickness to the sigini_file
                        sigini_input.write(str(plate_thickness) + "\n")
                        #Now write the Interpolation coefficients, first the L and then the LC ones
                        sigini_input.write(\
                        str(lc1) + "," + \
                        str(lc2) + "," + \
                        str(lc3) + "," + \
                        str(lc4) + "," + \
                        str(lc5) + "," + \
                        str(lc6) + "\n")
                        sigini_input.write(\
                        str(ltc1) + "," + \
                        str(ltc2) + "," + \
                        str(ltc3) + "," + \
                        str(ltc4) + "," + \
                        str(ltc5) + "," + \
                        str(ltc6) + "\n")
                        sigini_input.close()
                        #Check if the 
                        MeshObject.FemMesh.writeABAQUS(str(Case_Dir + "/" + "geometry_fe_input.inp"))
                        IsoNodes = list(IsoNodes)
                        ApplyingBC_IC(Case_Dir, young_modulus,poisson_ratio,IsoNodes[0],IsoNodes[1],IsoNodes[2],MeshObject)
                        batch.write("cd \"" + str(Case_Dir) + "\"\n")
                        batch.write("ccx -i geometry_fe_input\n")
        
                        l= l + z_rot_intervall
                    k = k + y_rot_intervall
                j = j + x_rot_intervall
            i = i+ z_offset_intervall
        # set the neutral placement from the beginning
        MeshObject.Placement = FreeCAD.Base.Placement()
    
    
    
    
    
FreeCADGui.addCommand('MachDist_Analysis',_CommandAnalysis())
FreeCADGui.addCommand('MachDist_JobControl',_CommandJobControl())
