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

import FreeCAD, Fem, MachDistMoveTools

if FreeCAD.GuiUp:
    import FreeCADGui, FemGui
    from FreeCAD import Vector
    from PyQt4 import QtCore, QtGui
    from pivy import coin
    import PyQt4.uic as uic

__title__="Machine-Distortion Alignment managment"
__author__ = "Juergen Riegel"
__url__ = "http://free-cad.sourceforge.net"



class _CommandAlignment:
    "the MachDist Alignment command definition"
    def GetResources(self):
        return {'Pixmap'  : 'MachDist_Align',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("MachDist_Alignment","Part Alignment"),
                'Accel': "A",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("MachDist_Alignment","Part Alignment")}
        
    def Activated(self):
        FemMeshObject = None
        import FemGui
        # check if a active analysis is present and no Mesh in it
        if FemGui.getActiveAnalysis() != None:
            for i in FemGui.getActiveAnalysis().Member:
                if i.isDerivedFrom("Fem::FemMeshObject"):
                    FemMeshObject = i
                    break
        else:
            return 
        FreeCAD.ActiveDocument.openTransaction("Alignment")
        
        
        taskd = _AlignTaskPanel(FemMeshObject)
        FreeCADGui.Control.showDialog(taskd)
       
    def IsActive(self):
        if FemGui.getActiveAnalysis():
            for i in FemGui.getActiveAnalysis().Member:
                if i.isDerivedFrom("Fem::FemMeshObject"):
                    return True
        else:
            return False

            
class _AlignTaskPanel:
    '''The editmode TaskPanel for Material objects'''
    def __init__(self,object):
        # the panel has a tree widget that contains categories
        # for the subcomponents, such as additions, subtractions.
        # the categories are shown only if they are not empty.
        form_class, base_class = uic.loadUiType(FreeCAD.getHomePath() + "Mod/Machining_Distortion/Aligment.ui")

        self.obj = object
        self.formUi = form_class()
        self.form = QtGui.QWidget()
        self.formUi.setupUi(self.form)

        #Connect Signals and Slots
        QtCore.QObject.connect(self.formUi.pushButton_FlipX, QtCore.SIGNAL("clicked()"), self.flipX)
        QtCore.QObject.connect(self.formUi.pushButton_FlipY, QtCore.SIGNAL("clicked()"), self.flipY)
        QtCore.QObject.connect(self.formUi.pushButton_FlipZ, QtCore.SIGNAL("clicked()"), self.flipZ)
        QtCore.QObject.connect(self.formUi.checkBox_AutoMinimize, QtCore.SIGNAL("stateChanged(int)"), self.autoMinToogle)
        QtCore.QObject.connect(self.formUi.pushButton_Minimize, QtCore.SIGNAL("clicked()"), self.minimize)

        self.formUi.checkBox_AutoMinimize.setCheckState(0)
        self.update()
        
        # switch on Bound Box
        #self.obj.ViewObject.BoundingBox = True
        
        # calculate eigen transformation and transform the mesh
        QtGui.qApp.setOverrideCursor(QtCore.Qt.WaitCursor)
        import Mesh
        # find the eigen axis
        self.obj.Placement = Mesh.calculateEigenTransform(self.obj.FemMesh.Nodes.values())
        
        # make the first alignment persistent
        m = Fem.FemMesh(self.obj.FemMesh)
        m.setTransform(self.obj.Placement)
        self.obj.FemMesh = m
        self.obj.Placement = FreeCAD.Placement()
        
        # move in the first quandrant and minimize bound box
        MachDistMoveTools.moveHome(self.obj)
        MachDistMoveTools.minimizeBoundVolume(self.obj)
        MachDistMoveTools.moveHome(self.obj)


        # make the first alignment persistent
        m = Fem.FemMesh(self.obj.FemMesh)
        m.setTransform(self.obj.Placement)
        self.obj.FemMesh = m
        self.obj.Placement = FreeCAD.Placement()




        self.showData()

       
        
        QtGui.qApp.restoreOverrideCursor()



    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Ok) | int(QtGui.QDialogButtonBox.Cancel)
    
    def update(self):
        'fills the widgets'
        return 
                
    def accept(self):
        FreeCADGui.Control.closeDialog()
        #self.obj.ViewObject.BoundingBox = False
        FreeCAD.ActiveDocument.commitTransaction()
        
                    
    def reject(self):
        FreeCADGui.Control.closeDialog()
        #self.obj.ViewObject.BoundingBox = False
        FreeCAD.ActiveDocument.abortTransaction()
     
    def autoMinToogle(self,state):
        if state == 0: self.formUi.pushButton_Minimize.setEnabled(True)
        if state == 2: self.formUi.pushButton_Minimize.setEnabled(False)
    
    def minimize(self):
        QtGui.qApp.setOverrideCursor(QtCore.Qt.WaitCursor)
        MachDistMoveTools.minimizeBoundVolume(self.obj)
        MachDistMoveTools.moveHome(self.obj)
        self.showData()
        QtGui.qApp.restoreOverrideCursor()
       
        
    def showData(self):
        b = self.obj.FemMesh.BoundBox
        self.formUi.lineEdit_XS.setText("%f"%b.XLength)
        self.formUi.lineEdit_YS.setText("%f"%b.YLength)
        self.formUi.lineEdit_ZS.setText("%f"%b.ZLength)
        self.formUi.lineEdit_VS.setText("%f"% float(b.XLength*b.YLength*b.ZLength))
        
    def afterFlip(self):       
        QtGui.qApp.setOverrideCursor(QtCore.Qt.WaitCursor)
        MachDistMoveTools.minimizeBoundVolume(self.obj)
        MachDistMoveTools.moveHome(self.obj)
        QtGui.qApp.restoreOverrideCursor()
    
    def flipX(self):
        QtGui.qApp.setOverrideCursor(QtCore.Qt.WaitCursor)
        p = self.obj.Placement
        p.Rotation = p.Rotation.multiply(FreeCAD.Rotation(FreeCAD.Vector(1,0,0),90))
        MachDistMoveTools.moveHome(self.obj)
        if(self.formUi.checkBox_AutoMinimize.isChecked()):
            MachDistMoveTools.minimizeBoundVolume(self.obj)
            MachDistMoveTools.moveHome(self.obj)
        self.showData()
        QtGui.qApp.restoreOverrideCursor()
                    
    def flipY(self):
        QtGui.qApp.setOverrideCursor(QtCore.Qt.WaitCursor)
        p = self.obj.Placement
        p.Rotation = p.Rotation.multiply(FreeCAD.Rotation(FreeCAD.Vector(0,1,0),90))
 
        MachDistMoveTools.moveHome(self.obj)
        if(self.formUi.checkBox_AutoMinimize.isChecked()):
            MachDistMoveTools.minimizeBoundVolume(self.obj)
            MachDistMoveTools.moveHome(self.obj)
        self.showData()
        QtGui.qApp.restoreOverrideCursor()
           
    def flipZ(self):
        QtGui.qApp.setOverrideCursor(QtCore.Qt.WaitCursor)
        p = self.obj.Placement
        p.Rotation = p.Rotation.multiply(FreeCAD.Rotation(FreeCAD.Vector(0,0,1),90))
        
        MachDistMoveTools.moveHome(self.obj)
        if(self.formUi.checkBox_AutoMinimize.isChecked()):
            MachDistMoveTools.minimizeBoundVolume(self.obj)
            MachDistMoveTools.moveHome(self.obj)
        self.showData()
        QtGui.qApp.restoreOverrideCursor()
                  

FreeCADGui.addCommand('MachDist_Alignment',_CommandAlignment())
