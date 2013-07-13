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
        import FemGui, Mesh
        # check if a active analysis is present and no Mesh in it
        if FemGui.getActiveAnalysis() != None:
            for i in FemGui.getActiveAnalysis().Member:
                if i.isDerivedFrom("Fem::FemMeshObject"):
                    FemMeshObject = i
                    break
        else:
            return 
        FreeCAD.ActiveDocument.openTransaction("Alignment")
        
        # switch on Bound Box
        FemMeshObject.ViewObject.BoundingBox = True
        QtGui.qApp.setOverrideCursor(QtCore.Qt.WaitCursor)
        n = FemMeshObject.FemMesh.Nodes
        p = Mesh.calculateEigenTransform(n)
        #FemMeshObject.Placement = p
        m = Fem.FemMesh(FemMeshObject.FemMesh)
        m.setTransform(p)
        FemMeshObject.FemMesh = m
        FemMeshObject.Placement = FreeCAD.Placement()
        
        QtGui.qApp.restoreOverrideCursor()
        
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

        self.update()


    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Ok) | int(QtGui.QDialogButtonBox.Cancel)
    
    def update(self):
        'fills the widgets'
        return 
                
    def accept(self):
        FreeCADGui.Control.closeDialog()
        self.obj.ViewObject.BoundingBox = False
        FreeCAD.ActiveDocument.commitTransaction()
        
                    
    def reject(self):
        FreeCADGui.Control.closeDialog()
        self.obj.ViewObject.BoundingBox = False
        FreeCAD.ActiveDocument.abortTransaction()
        
    def flipX(self):
        p = self.obj.Placement
        r2 = p.Rotation.multiply(FreeCAD.Rotation(FreeCAD.Vector(1,0,0),180))
        p.Rotation = r2
        return            
    def flipY(self):
        p = self.obj.Placement
        r2 = p.Rotation.multiply(FreeCAD.Rotation(FreeCAD.Vector(0,1,0),180))
        p.Rotation = r2
        return            
    def flipZ(self):
        p = self.obj.Placement
        r2 = p.Rotation.multiply(FreeCAD.Rotation(FreeCAD.Vector(0,0,1),180))
        zl = self.obj.FemMesh.BoundBox.ZLength
        p.Rotation = r2
        return            

FreeCADGui.addCommand('MachDist_Alignment',_CommandAlignment())
