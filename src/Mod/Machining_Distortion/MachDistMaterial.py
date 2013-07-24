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
__url__ = "http://free-cad.sourceforge.net"

StartMat = {'FEM_youngsmodulus'         :'7000.00',
            'PartDist_poissonratio'     :'0.30',
            'PartDist_platethickness'   :'40.0',
            'PartDist_lc1'              :'0.0',
            'PartDist_lc2'              :'0.0',
            'PartDist_lc3'              :'0.0',
            'PartDist_lc4'              :'0.0',
            'PartDist_lc5'              :'0.0',
            'PartDist_lc6'              :'0.0',
            'PartDist_ltc1'             :'0.0',
            'PartDist_ltc2'             :'0.0',
            'PartDist_ltc3'             :'0.0',
            'PartDist_ltc4'             :'0.0',
            'PartDist_ltc5'             :'0.0',
            'PartDist_ltc6'             :'0.0'
            }
            

def makeMaterial(name):
    '''makeMaterial(name): makes an Material
    name there fore is a material name or an file name for a FCMat file'''
    obj = FreeCAD.ActiveDocument.addObject("App::MaterialObjectPython",name)
    _Material(obj)
    _ViewProviderMaterial(obj.ViewObject)
    #FreeCAD.ActiveDocument.recompute()
    return obj

class _CommandMaterial:
    "the MachDist Material command definition"
    def GetResources(self):
        return {'Pixmap'  : 'MachDist_Material',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("MachDist_Material","Material"),
                'Accel': "A, X",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("MachDist_Material","Creates or edit the material definition.")}
        
    def Activated(self):
        MatObj = None
        for i in FemGui.getActiveAnalysis().Member:
            if i.isDerivedFrom("App::MaterialObject"):
                    MatObj = i

        if (not MatObj):
            FreeCAD.ActiveDocument.openTransaction("Create Material")
            FreeCADGui.addModule("MachDistMaterial")
            FreeCADGui.doCommand("mat = MachDistMaterial.makeMaterial('Material')")
            FreeCADGui.doCommand("App.activeDocument()."+FemGui.getActiveAnalysis().Name+".Member = App.activeDocument()."+FemGui.getActiveAnalysis().Name+".Member + [mat]")
            FreeCADGui.doCommand("Gui.activeDocument().setEdit(mat.Name,0)")
            #FreeCADGui.doCommand("MachDist.makeMaterial()")
        else:
            FreeCADGui.doCommand("Gui.activeDocument().setEdit('"+MatObj.Name+"',0)")
        
    def IsActive(self):
        if FemGui.getActiveAnalysis():
            return True
        else:
            return False

       
class _Material:
    "The Material object"
    def __init__(self,obj):
        self.Type = "MachDistMaterial"
        obj.Proxy = self
        obj.Material = StartMat
        #obj.addProperty("App::PropertyString","MaterialName","Base",
        #                "The name of the distorion material")

        
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
        
class _ViewProviderMaterial:
    "A View Provider for the Material object"

    def __init__(self,vobj):
        #vobj.addProperty("App::PropertyLength","BubbleSize","Base", str(translate("MachDist","The size of the axis bubbles")))
        vobj.Proxy = self
       
    def getIcon(self):
        import machdist_rc
        return ":/icons/MachDist_Material.svg"

    def claimChildren(self):
        return []

    def attach(self, vobj):
        self.ViewObject = vobj
        self.Object = vobj.Object
        self.bubbles = None


    def updateData(self, obj, prop):
        return

    def onChanged(self, vobj, prop):
        return
  
    def setEdit(self,vobj,mode):
        taskd = _MaterialTaskPanel(self.Object)
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


class _MaterialTaskPanel:
    '''The editmode TaskPanel for Material objects'''
    def __init__(self,obj):
        # the panel has a tree widget that contains categories
        # for the subcomponents, such as additions, subtractions.
        # the categories are shown only if they are not empty.
        form_class, base_class = uic.loadUiType(FreeCAD.getHomePath() + "Mod/Machining_Distortion/Material.ui")

        self.obj = obj
        self.formUi = form_class()
        self.form = QtGui.QWidget()
        self.formUi.setupUi(self.form)
        self.params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Machining_Distortion")


        QtCore.QObject.connect(self.formUi.select_L_file, QtCore.SIGNAL("clicked()"), self.add_L_data)
        QtCore.QObject.connect(self.formUi.select_LT_file, QtCore.SIGNAL("clicked()"), self.add_LT_data)
        QtCore.QObject.connect(self.formUi.pushButton_SaveMat, QtCore.SIGNAL("clicked()"), self.saveMat)
        QtCore.QObject.connect(self.formUi.toolButton_chooseDir, QtCore.SIGNAL("clicked()"), self.chooseDir)
        QtCore.QObject.connect(self.formUi.comboBox_MaterialsInDir, QtCore.SIGNAL("currentIndexChanged(int)"), self.chooseMat)
        
        self.update()
        
    def transferTo(self):
        "Transfer from the dialog to the object" 
        
        matmap = self.obj.Material

        matmap['FEM_youngsmodulus']       = str(self.formUi.spinBox_young_modulus.value())
        matmap['PartDist_poissonratio']   = str(self.formUi.spinBox_poisson_ratio.value())
        matmap['PartDist_platethickness'] = str(self.formUi.spinBox_Plate_Thickness.value())


        matmap['PartDist_lc1'] = str(self.formUi.lc1.value())
        matmap['PartDist_lc2'] = str(self.formUi.lc2.value())
        matmap['PartDist_lc3'] = str(self.formUi.lc3.value())
        matmap['PartDist_lc4'] = str(self.formUi.lc4.value())
        matmap['PartDist_lc5'] = str(self.formUi.lc5.value())
        matmap['PartDist_lc6'] = str(self.formUi.lc6.value())

        matmap['PartDist_ltc1'] = str(self.formUi.ltc1.value())
        matmap['PartDist_ltc2'] = str(self.formUi.ltc2.value())
        matmap['PartDist_ltc3'] = str(self.formUi.ltc3.value())
        matmap['PartDist_ltc4'] = str(self.formUi.ltc4.value())
        matmap['PartDist_ltc5'] = str(self.formUi.ltc5.value())
        matmap['PartDist_ltc6'] = str(self.formUi.ltc6.value())
        self.obj.Material = matmap 

    
    def transferFrom(self):
        "Transfer from the object to the dialog"
        matmap = self.obj.Material

        self.formUi.spinBox_young_modulus.setValue(float(matmap['FEM_youngsmodulus']))
        self.formUi.spinBox_poisson_ratio.setValue(float(matmap['PartDist_poissonratio']))
        self.formUi.spinBox_Plate_Thickness.setValue(float(matmap['PartDist_platethickness']))


        self.formUi.lc1.setValue(float(matmap['PartDist_lc1']))
        self.formUi.lc2.setValue(float(matmap['PartDist_lc2']))
        self.formUi.lc3.setValue(float(matmap['PartDist_lc3']))
        self.formUi.lc4.setValue(float(matmap['PartDist_lc4']))
        self.formUi.lc5.setValue(float(matmap['PartDist_lc5']))
        self.formUi.lc6.setValue(float(matmap['PartDist_lc6']))

        self.formUi.ltc1.setValue(float(matmap['PartDist_ltc1']))
        self.formUi.ltc2.setValue(float(matmap['PartDist_ltc2']))
        self.formUi.ltc3.setValue(float(matmap['PartDist_ltc3']))
        self.formUi.ltc4.setValue(float(matmap['PartDist_ltc4']))
        self.formUi.ltc5.setValue(float(matmap['PartDist_ltc5']))
        self.formUi.ltc6.setValue(float(matmap['PartDist_ltc6']))

    def isAllowedAlterSelection(self):
        return False

    def isAllowedAlterView(self):
        return True

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Ok) | int(QtGui.QDialogButtonBox.Cancel)
    
    def update(self):
        'fills the widgets'
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
            
    def chooseDir(self):
        dirname = QtGui.QFileDialog.getExistingDirectory(None, 'Choose material directory',self.params.GetString("MaterialDir",'/'))
        if(dirname):
            self.params.SetString("MaterialDir",str(dirname))
            self.fillMaterialCombo()
    
    def chooseMat(self,index):
        if index == 0:return 
        import Material
        name = self.pathList[index-1]
        #print 'Import ', str(name)
        
        self.obj.Material = Material.importFCMat(str(name))
        #print self.obj.Material
        
        self.transferFrom()
        
    def fillMaterialCombo(self):
        import glob,os
        dirname = self.params.GetString("MaterialDir",'/')
        self.pathList = glob.glob(dirname + '/*.FCMat')
        self.formUi.comboBox_MaterialsInDir.clear()
        self.formUi.comboBox_MaterialsInDir.addItem('-> choose Material')
        for i in self.pathList:
            self.formUi.comboBox_MaterialsInDir.addItem(os.path.basename(i) )
        
    def add_L_data(self):
        l_filename = QtGui.QFileDialog.getOpenFileName(None, 'Open file','','R-Script File for L Coefficients (*.txt)')
        values = self.parse_R_output(l_filename)
        self.formUi.lc1.setValue(values[0])
        self.formUi.lc2.setValue(values[1])
        self.formUi.lc3.setValue(values[2])
        self.formUi.lc4.setValue(values[3])
        self.formUi.lc5.setValue(values[4])
        self.formUi.lc6.setValue(values[5])
        
        
    def add_LT_data(self):
        lt_filename = QtGui.QFileDialog.getOpenFileName(None, 'Open file','','R-Script File for LT Coefficients (*.txt)')
        values = self.parse_R_output(lt_filename)
        self.formUi.ltc1.setValue(values[0])
        self.formUi.ltc2.setValue(values[1])
        self.formUi.ltc3.setValue(values[2])
        self.formUi.ltc4.setValue(values[3])
        self.formUi.ltc5.setValue(values[4])
        self.formUi.ltc6.setValue(values[5])
        

    def parse_R_output(self,filename):
        file = open(str(filename))
        lines = file.readlines()
        found = False
        coeff = []
        for line in lines:
            if line[0:9] == "c0 to c5:":
                found = True
                coeff.append(float(line[15:]))
                continue
            if found and line[0:4] == "MSE:":
                found = False
            if found:
                coeff.append(float(line[15:]))
        
        file.close()
        return coeff[0],coeff[1],coeff[2],coeff[3],coeff[4],coeff[5]
         
FreeCADGui.addCommand('MachDist_Material',_CommandMaterial())
