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

import FreeCAD, Fem

if FreeCAD.GuiUp:
    import FreeCADGui,FemGui
    from FreeCAD import Vector
    from PyQt4 import QtCore, QtGui
    from pivy import coin
    import PyQt4.uic as uic

__title__="Machine-Distortion FemSetGeometryObject managment"
__author__ = "Juergen Riegel"
__url__ = "http://www.freecadweb.org"

          

def makeMechanicalMaterial(name):
    '''makeMaterial(name): makes an Material
    name there fore is a material name or an file name for a FCMat file'''
    obj = FreeCAD.ActiveDocument.addObject("App::MaterialObjectPython",name)
    _MechanicalMaterial(obj)
    _ViewProviderMechanicalMaterial(obj.ViewObject)
    #FreeCAD.ActiveDocument.recompute()
    return obj

class _CommandMechanicalMaterial:
    "the Fem Material command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Fem_Material',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Fem_Material","Mechanical material..."),
                'Accel': "A, X",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Fem_Material","Creates or edit the mechanical material definition.")}
        
    def Activated(self):
        MatObj = None
        for i in FemGui.getActiveAnalysis().Member:
            if i.isDerivedFrom("App::MaterialObject"):
                    MatObj = i

        if (not MatObj):
            FreeCAD.ActiveDocument.openTransaction("Create Material")
            FreeCADGui.addModule("MechanicalMaterial")
            FreeCADGui.doCommand("MechanicalMaterial.makeMechanicalMaterial('MechanicalMaterial')")
            FreeCADGui.doCommand("App.activeDocument()."+FemGui.getActiveAnalysis().Name+".Member = App.activeDocument()."+FemGui.getActiveAnalysis().Name+".Member + [App.ActiveDocument.ActiveObject]")
            FreeCADGui.doCommand("Gui.activeDocument().setEdit(App.ActiveDocument.ActiveObject.Name,0)")
            #FreeCADGui.doCommand("Fem.makeMaterial()")
        else:
            FreeCADGui.doCommand("Gui.activeDocument().setEdit('"+MatObj.Name+"',0)")
        
    def IsActive(self):
        if FemGui.getActiveAnalysis():
            return True
        else:
            return False

       
class _MechanicalMaterial:
    "The Material object"
    def __init__(self,obj):
        self.Type = "MechanicaltMaterial"
        obj.Proxy = self
        #obj.Material = StartMat

        
    def execute(self,obj):
        return
        
        
class _ViewProviderMechanicalMaterial:
    "A View Provider for the MechanicalMaterial object"

    def __init__(self,vobj):
        vobj.Proxy = self
       
    def getIcon(self):
        return ":/icons/Fem_Material.svg"

    def attach(self, vobj):
        self.ViewObject = vobj
        self.Object = vobj.Object


    def updateData(self, obj, prop):
        return

    def onChanged(self, vobj, prop):
        return
  
    def setEdit(self,vobj,mode):
        taskd = _MechanicalMaterialTaskPanel(self.Object)
        taskd.obj = vobj.Object
        taskd.update()
        FreeCADGui.Control.showDialog(taskd)
        return True
    
    def unsetEdit(self,vobj,mode):
        FreeCADGui.Control.closeDialog()
        return

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None


class _MechanicalMaterialTaskPanel:
    '''The editmode TaskPanel for MechanicalMaterial objects'''
    def __init__(self,obj):
        # the panel has a tree widget that contains categories
        # for the subcomponents, such as additions, subtractions.
        # the categories are shown only if they are not empty.
        form_class, base_class = uic.loadUiType(FreeCAD.getHomePath() + "Mod/Fem/MechanicalMaterial.ui")

        self.obj = obj
        self.formUi = form_class()
        self.form = QtGui.QWidget()
        self.formUi.setupUi(self.form)
        self.params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem")


        QtCore.QObject.connect(self.formUi.pushButton_MatWeb, QtCore.SIGNAL("clicked()"), self.goMatWeb)
        QtCore.QObject.connect(self.formUi.comboBox_MaterialsInDir, QtCore.SIGNAL("currentIndexChanged(int)"), self.chooseMat)
        
        self.update()
        
    def transferTo(self):
        "Transfer from the dialog to the object" 
        
        matmap = self.obj.Material

        matmap['Mechanical_youngsmodulus']       = str(self.formUi.spinBox_young_modulus.value() * 1e+6)
        matmap['FEM_poissonratio']   = str(self.formUi.spinBox_poisson_ratio.value())

        self.obj.Material = matmap 

    
    def transferFrom(self):
        "Transfer from the object to the dialog"
        matmap = self.obj.Material

        if matmap.has_key('Mechanical_youngsmodulus'):
            print float(matmap['Mechanical_youngsmodulus'])
            self.formUi.spinBox_young_modulus.setValue(float(matmap['Mechanical_youngsmodulus'])/1e+6)
        if matmap.has_key('FEM_poissonratio'):
            print float(matmap['FEM_poissonratio'])
            self.formUi.spinBox_poisson_ratio.setValue(float(matmap['FEM_poissonratio']))

    def isAllowedAlterSelection(self):
        return False

    def isAllowedAlterView(self):
        return True

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Ok) | int(QtGui.QDialogButtonBox.Cancel)
    
    def update(self):
        'fills the widgets'
        self.formUi.spinBox_young_modulus.setValue(0.0)
        self.formUi.spinBox_poisson_ratio.setValue(0.0)
        self.transferFrom()
        self.fillMaterialCombo()


        return 
                
    def accept(self):
        self.transferTo()
        FreeCADGui.ActiveDocument.resetEdit()
                    
    def reject(self):
        FreeCADGui.ActiveDocument.resetEdit()

    def saveMat(self):
        self.transferTo()
        filename = QtGui.QFileDialog.getSaveFileName(None, 'Save Material file file',self.params.GetString("MaterialDir",'/'),'FreeCAD material file (*.FCMat)')
        if(filename):
            import Material
            Material.exportFCMat(filename,self.obj.Material)
            
    def goMatWeb(self):
        import webbrowser
        webbrowser.open("http://matweb.com")
    
    def chooseMat(self,index):
        if index == 0:return 
        import Material
        print index
        name = self.pathList[index-1]
        print 'Import ', str(name)
        
        self.obj.Material = Material.importFCMat(str(name))
        print self.obj.Material
        
        self.transferFrom()
        
    def fillMaterialCombo(self):
        import glob,os
        matmap = self.obj.Material
        dirname =  FreeCAD.ConfigGet("AppHomePath")+"data/Mod/Material/StandardMaterial" 
        self.pathList = glob.glob(dirname + '/*.FCMat')
        self.formUi.comboBox_MaterialsInDir.clear()
        if(matmap.has_key('General_name')):
            self.formUi.comboBox_MaterialsInDir.addItem(matmap['General_name'])
        else:
            self.formUi.comboBox_MaterialsInDir.addItem('-> choose Material')
        for i in self.pathList:
            self.formUi.comboBox_MaterialsInDir.addItem(os.path.basename(i) )
        
        

         
FreeCADGui.addCommand('Fem_MechanicalMaterial',_CommandMechanicalMaterial())
