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
        FreeCAD.ActiveDocument.openTransaction("Create Material")
        FreeCADGui.addModule("MachDistMaterial")
        FreeCADGui.doCommand("mat = MachDistMaterial.makeMaterial('Material')")
        FreeCADGui.doCommand("App.activeDocument()."+FemGui.getActiveAnalysis().Name+".Member = App.activeDocument()."+FemGui.getActiveAnalysis().Name+".Member + [mat]")
        FreeCADGui.doCommand("Gui.activeDocument().setEdit(mat.Name,0)")
        #FreeCADGui.doCommand("MachDist.makeMaterial()")
        FreeCAD.ActiveDocument.commitTransaction()
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
        taskd = _MaterialTaskPanel()
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
    def __init__(self):
        # the panel has a tree widget that contains categories
        # for the subcomponents, such as additions, subtractions.
        # the categories are shown only if they are not empty.
        form_class, base_class = uic.loadUiType(FreeCAD.getHomePath() + "Mod/Machining_Distortion/Material.ui")

        self.obj = None
        self.formUi = form_class()
        self.form = QtGui.QWidget()
        self.formUi.setupUi(self.form)
        #self.form.setObjectName("TaskPanel")
        #self.grid = QtGui.QGridLayout(self.form)
        #self.grid.setObjectName("grid")
        #self.title = QtGui.QLabel(self.form)
        #self.grid.addWidget(self.title, 0, 0, 1, 2)

        # tree
        #self.tree = QtGui.QTreeWidget(self.form)
        #self.grid.addWidget(self.tree, 1, 0, 1, 2)
        #self.tree.setColumnCount(3)
        #self.tree.header().resizeSection(0,50)
        #self.tree.header().resizeSection(1,80)
        #self.tree.header().resizeSection(2,60)
        
        # buttons       
        #self.addButton = QtGui.QPushButton(self.form)
        #self.addButton.setObjectName("addButton")
        #self.addButton.setIcon(QtGui.QIcon(":/icons/MachDist_Add.svg"))
        #self.grid.addWidget(self.addButton, 3, 0, 1, 1)
        #self.addButton.setEnabled(True)

        #self.delButton = QtGui.QPushButton(self.form)
        #self.delButton.setObjectName("delButton")
        #self.delButton.setIcon(QtGui.QIcon(":/icons/MachDist_Remove.svg"))
        #self.grid.addWidget(self.delButton, 3, 1, 1, 1)
        #self.delButton.setEnabled(True)

        #QtCore.QObject.connect(self.addButton, QtCore.SIGNAL("clicked()"), self.addElement)
        #QtCore.QObject.connect(self.delButton, QtCore.SIGNAL("clicked()"), self.removeElement)
        self.update()

    def isAllowedAlterSelection(self):
        return False

    def isAllowedAlterView(self):
        return True

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Ok) | int(QtGui.QDialogButtonBox.Cancel)
    
    def update(self):
        'fills the widgets'
        return 
                
    def accept(self):
        FreeCADGui.ActiveDocument.resetEdit()
                    
    def reject(self):
        FreeCADGui.ActiveDocument.resetEdit()
                    
          
FreeCADGui.addCommand('MachDist_Material',_CommandMaterial())
